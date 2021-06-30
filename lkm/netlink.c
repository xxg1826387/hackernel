#include "netlink.h"
#include "file.h"
#include "perm.h"
#include "process.h"
#include "syscall.h"
#include "util.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <net/genetlink.h>
#include <net/netlink.h>

u32 portid = 0;

static struct nla_policy nla_policy[HACKERNEL_A_MAX + 1] = {
	[HACKERNEL_A_STATUS_CODE] = { .type = NLA_S32 },
	[HACKERNEL_A_OP_TYPE] = { .type = NLA_U8 },
	[HACKERNEL_A_SYS_CALL_TABLE_HEADER] = { .type = NLA_U64 },
	[HACKERNEL_A_NAME] = { .type = NLA_STRING },
	[HACKERNEL_A_PERM] = { .type = NLA_S32 },
	[HACKERNEL_A_EXECVE_ID] = { .type = NLA_S32 },
};

static int handshake_handler(struct sk_buff *skb, struct genl_info *info)
{
	int error = 0;
	unsigned long long syscall_table = 0;
	struct sk_buff *reply = NULL;
	void *head = NULL;
	int code;

	if (!netlink_capable(skb, CAP_SYS_ADMIN)) {
		LOG("netlink_capable failed");
		return -EPERM;
	}

	if (!info->attrs[HACKERNEL_A_SYS_CALL_TABLE_HEADER]) {
		code = -EINVAL;
		LOG("HACKERNEL_A_SYS_CALL_TABLE_HEADER failed");
		goto response;
	}

	syscall_table = nla_get_u64(info->attrs[HACKERNEL_A_SYS_CALL_TABLE_HEADER]);
	code = init_sys_call_table(syscall_table);
	portid = info->snd_portid;

response:
	reply = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (unlikely(!reply)) {
		LOG("genlmsg_new failed");
		goto errout;
	}

	head = genlmsg_put_reply(reply, info, &genl_family, 0,
				 HACKERNEL_C_HANDSHAKE);
	if (unlikely(!head)) {
		LOG("genlmsg_put_reply failed");
		goto errout;
	}

	error = nla_put_s32(reply, HACKERNEL_A_STATUS_CODE, code);
	if (unlikely(error)) {
		LOG("nla_put_s32 failed");
		goto errout;
	}

	genlmsg_end(reply, head);

	// reply指向的内存由 genlmsg_reply 释放
	// 此处调用 nlmsg_free(reply) 会引起内核crash
	error = genlmsg_reply(reply, info);
	if (unlikely(error)) {
		LOG("genlmsg_reply failed");
	}
	return 0;
errout:
	nlmsg_free(reply);
	return 0;
}

static struct genl_small_ops genl_small_ops[] = {
	{
		.cmd = HACKERNEL_C_HANDSHAKE,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.doit = handshake_handler,
	},
	{
		.cmd = HACKERNEL_C_PROCESS_PROTECT,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.doit = process_protect_handler,
	},
	{
		.cmd = HACKERNEL_C_FILE_PROTECT,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.doit = file_protect_handler,
	},
};

struct genl_family genl_family = {
	.hdrsize = 0,
	.name = HACKERNEL_FAMLY_NAME,
	.version = HACKERNEL_FAMLY_VERSION,
	.module = THIS_MODULE,
	.small_ops = genl_small_ops,
	.n_small_ops = ARRAY_SIZE(genl_small_ops),
	.maxattr = HACKERNEL_A_MAX,
	.policy = nla_policy,
};

void netlink_kernel_start(void)
{
	int error = 0;

	error = genl_register_family(&genl_family);
	if (error) {
		LOG("genl_register_family failed");
	}
}

void netlink_kernel_stop(void)
{
	int error = 0;

	error = genl_unregister_family(&genl_family);
	if (error) {
		LOG("genl_unregister_family failed");
	}
}