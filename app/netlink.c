#include "netlink.h"
#include "handler.h"
#include "syscall.h"
#include "util.h"
#include <linux/genetlink.h>
#include <netlink/attr.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/mngt.h>
#include <netlink/msg.h>
#include <stdio.h>

struct nl_sock *nlsock = NULL;
int famid = 0;

static int status = 0;

static struct nla_policy hackernel_genl_policy[HACKERNEL_A_MAX + 1] = {
    [HACKERNEL_A_STATUS_CODE] = {.type = NLA_S32},    [HACKERNEL_A_OP_TYPE] = {.type = NLA_U8},  [HACKERNEL_A_SYS_CALL_TABLE_HEADER] = {.type = NLA_U64},
    [HACKERNEL_A_NAME] = {.type = NLA_STRING}, [HACKERNEL_A_PERM] = {.type = NLA_S32}, [HACKERNEL_A_EXECVE_ID] = {.type = NLA_S32},
};

// 在这里扩展 HACKERNEL_C_* 对应的 handler
static struct genl_cmd hackernel_genl_cmds[] = {
    {
        .c_id = HACKERNEL_C_HANDSHAKE,
        .c_name = "HACKERNEL_C_HANDSHAKE",
        .c_maxattr = HACKERNEL_A_MAX,
        .c_attr_policy = hackernel_genl_policy,
        .c_msg_parser = &handshake_handler,
    },
    {
        .c_id = HACKERNEL_C_PROCESS_PROTECT,
        .c_name = "HACKERNEL_C_PROCESS_PROTECT",
        .c_maxattr = HACKERNEL_A_MAX,
        .c_attr_policy = hackernel_genl_policy,
        .c_msg_parser = &process_protect_handler,
    },
    {
        .c_id = HACKERNEL_C_FILE_PROTECT,
        .c_name = "HACKERNEL_C_FILE_PROTECT",
        .c_maxattr = HACKERNEL_A_MAX,
        .c_attr_policy = hackernel_genl_policy,
        .c_msg_parser = &file_protect_handler,
    },
};

static struct genl_ops hackernel_genl_ops = {
    .o_name = HACKERNEL_FAMLY_NAME,
    .o_cmds = hackernel_genl_cmds,
    .o_ncmds = ARRAY_SIZE(hackernel_genl_cmds),
};

int netlink_server_init() {
    int error;

    if (nlsock) {
        LOG("Generic Netlink has be init");
        return 0;
    }

    nlsock = nl_socket_alloc();
    if (!nlsock) {
        LOG("Netlink Socket memory alloc failed");
        goto errout;
    }

    error = genl_connect(nlsock);
    if (error) {
        LOG("Generic Netlink connect failed");
        goto errout;
    }

    // 缓冲区大小设置为4MB
    const int buff_size = 4 * 1024 * 1024;
    error = nl_socket_set_buffer_size(nlsock, buff_size, buff_size);
    if (error) {
        LOG("nl_socket_set_buffer_size failed");
        goto errout;
    }

    error = genl_ops_resolve(nlsock, &hackernel_genl_ops);
    if (error) {
        LOG("Resolve a single Generic Netlink family failed");
        goto errout;
    }

    error = genl_register_family(&hackernel_genl_ops);
    if (error) {
        LOG("Generic Netlink Register failed");
        goto errout;
    }
    famid = hackernel_genl_ops.o_id;

    error = nl_socket_modify_cb(nlsock, NL_CB_VALID, NL_CB_CUSTOM, genl_handle_msg, NULL);
    if (error) {
        LOG("Generic Netlink modify callback failed");
        goto errout;
    }

    error = nl_socket_set_nonblocking(nlsock);
    if (error) {
        LOG("Generic Netlink set noblocking failed");
        goto errout;
    }

    // 应用层收到消息会检查当前期待收到的seq与上次发送的seq是否一致
    nl_socket_disable_seq_check(nlsock);

    // 内核收到消息会自动回复确认
    nl_socket_disable_auto_ack(nlsock);
    return 0;

errout:
    LOG("Generic Netlink init failed");
    return -1;
}

int netlink_server_start(void) {
    int error;

    struct pollfd fds = {
        .fd = nl_socket_get_fd(nlsock),
        .events = POLLIN,
    };

    status = 1;
    while (status) {
        const int nfds = 1;
        const int timeout = 100;

        error = poll(&fds, nfds, timeout);
        if (error == 0) {
            continue;
        }

        if (error < 0) {
            LOG("poll failed");
            break;
        }

        error = nl_recvmsgs_default(nlsock);
        if (error) {
            LOG("nl_recvmsgs_default failed error=[%d]", error);
        }
    }

    nl_close(nlsock);
    nl_socket_free(nlsock);

    return 0;
}

int netlink_server_stop(void) {
    status = 0;
    return 0;
}