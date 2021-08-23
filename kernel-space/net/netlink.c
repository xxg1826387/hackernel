#include "netlink.h"
#include "net.h"
#include "util.h"

extern struct genl_family genl_family;

static int net_protect_info_to_policy(const struct genl_info *info,
				      struct net_policy_t *policy)
{
	if (!info->attrs[NET_A_ID])
		return -EINVAL;
	if (!info->attrs[NET_A_PRIORITY])
		return -EINVAL;

	if (!info->attrs[NET_A_ADDR_SRC_BEGIN])
		return -EINVAL;
	if (!info->attrs[NET_A_ADDR_SRC_END])
		return -EINVAL;
	if (!info->attrs[NET_A_ADDR_DST_BEGIN])
		return -EINVAL;
	if (!info->attrs[NET_A_ADDR_DST_END])
		return -EINVAL;

	if (!info->attrs[NET_A_PORT_SRC_BEGIN])
		return -EINVAL;
	if (!info->attrs[NET_A_PORT_SRC_END])
		return -EINVAL;
	if (!info->attrs[NET_A_PORT_DST_BEGIN])
		return -EINVAL;
	if (!info->attrs[NET_A_PORT_DST_END])
		return -EINVAL;

	if (!info->attrs[NET_A_PROTOCOL_BEGIN])
		return -EINVAL;
	if (!info->attrs[NET_A_PROTOCOL_END])
		return -EINVAL;

	if (!info->attrs[NET_A_RESPONSE])
		return -EINVAL;

	if (!info->attrs[NET_A_FLAGS])
		return -EINVAL;

	policy->id = nla_get_s32(info->attrs[NET_A_ID]);
	policy->priority = nla_get_s8(info->attrs[NET_A_PRIORITY]);

	policy->addr.src.begin = nla_get_u32(info->attrs[NET_A_ADDR_SRC_BEGIN]);
	policy->addr.src.end = nla_get_u32(info->attrs[NET_A_ADDR_SRC_END]);
	policy->addr.dst.begin = nla_get_u32(info->attrs[NET_A_ADDR_DST_BEGIN]);
	policy->addr.dst.end = nla_get_u32(info->attrs[NET_A_ADDR_DST_END]);

	policy->port.src.begin = nla_get_u16(info->attrs[NET_A_PORT_SRC_BEGIN]);
	policy->port.src.end = nla_get_u16(info->attrs[NET_A_PORT_SRC_END]);
	policy->port.dst.begin = nla_get_u16(info->attrs[NET_A_PORT_DST_BEGIN]);
	policy->port.dst.end = nla_get_u16(info->attrs[NET_A_PORT_DST_END]);

	policy->protocol.begin = nla_get_u8(info->attrs[NET_A_PROTOCOL_BEGIN]);
	policy->protocol.end = nla_get_u8(info->attrs[NET_A_PROTOCOL_END]);

	policy->response = nla_get_u32(info->attrs[NET_A_RESPONSE]);
	policy->flags = nla_get_s32(info->attrs[NET_A_FLAGS]);

	return 0;
}

int net_protect_handler(struct sk_buff *skb, struct genl_info *info)
{
	struct net_policy_t policy;
	int error = 0;
	int code = 0;
	struct sk_buff *reply = NULL;
	void *head = NULL;
	u8 type;

	if (portid != info->snd_portid)
		return -EPERM;

	if (!info->attrs[NET_A_OP_TYPE]) {
		code = -EINVAL;
		goto response;
	}

	type = nla_get_u8(info->attrs[NET_A_OP_TYPE]);
	switch (type) {
	case NET_PROTECT_ENABLE:
		code = enable_net_protect();
		goto response;

	case NET_PROTECT_DISABLE:
		code = disable_net_protect();
		goto response;
	case NET_PROTECT_INSERT:
		code = net_protect_info_to_policy(info, &policy);
		if (code)
			goto response;

		code = net_policy_insert(&policy);
		goto response;
	case NET_PROTECT_DELETE:
		if (!info->attrs[NET_A_ID]) {
			code = -EINVAL;
			goto response;
		}
		code = net_policy_delete(nla_get_s32(info->attrs[NET_A_ID]));
		goto response;
	default: {
		LOG("Unknown process protect command");
	}
	}

response:
	reply = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (unlikely(!reply)) {
		LOG("genlmsg_new failed");
		goto errout;
	}

	head = genlmsg_put_reply(reply, info, &genl_family, 0,
				 HACKERNEL_C_NET_PROTECT);
	if (unlikely(!head)) {
		LOG("genlmsg_put_reply failed");
		goto errout;
	}

	error = nla_put_u32(reply, NET_A_OP_TYPE, type);
	if (unlikely(error)) {
		LOG("nla_put_s32 failed");
		goto errout;
	}

	error = nla_put_s32(reply, NET_A_STATUS_CODE, code);
	if (unlikely(error)) {
		LOG("nla_put_s32 failed");
		goto errout;
	}

	genlmsg_end(reply, head);

	error = genlmsg_reply(reply, info);
	if (unlikely(error))
		LOG("genlmsg_reply failed");

	return 0;
errout:
	nlmsg_free(reply);
	return 0;
}