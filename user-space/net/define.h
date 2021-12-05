#ifndef HACKERNEL_NET_DEFINE_H
#define HACKERNEL_NET_DEFINE_H

enum {
    NET_A_UNSPEC,
    NET_A_SESSION,

    NET_A_STATUS_CODE,
    NET_A_OP_TYPE,
    NET_A_ID,
    NET_A_PRIORITY,
    NET_A_ADDR_SRC_BEGIN,
    NET_A_ADDR_SRC_END,
    NET_A_ADDR_DST_BEGIN,
    NET_A_ADDR_DST_END,
    NET_A_PORT_SRC_BEGIN,
    NET_A_PORT_SRC_END,
    NET_A_PORT_DST_BEGIN,
    NET_A_PORT_DST_END,
    NET_A_PROTOCOL_BEGIN,
    NET_A_PROTOCOL_END,
    NET_A_RESPONSE,
    NET_A_FLAGS,

    __NET_A_MAX,
};
#define NET_A_MAX (__NET_A_MAX - 1)

#endif