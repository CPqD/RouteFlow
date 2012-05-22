/*
 * libnetlink.hh
 * =============
 * Sources adapted for C++ linkage from libnetlink.h, available through iproute-20100519.
 * CODE AS-IS. Any licensing applied to iproute-20100519 also applies here.
 *
 *  Created on: Feb 14, 2011
 *      Author: bill <carlos.nilton@gmail.com>
 */

#ifndef LIBNETLINK_HH_
#define LIBNETLINK_HH_

#include <asm/types.h>
#include <linux/if_link.h>
#include <linux/if_addr.h>
#include <linux/neighbour.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>

struct rtnl_handle {
	int fd;
	struct sockaddr_nl local;
	struct sockaddr_nl peer;
	__u32 seq;
	__u32 dump;
};

extern "C" int rcvbuf;

extern "C" int rtnl_open(struct rtnl_handle *rth, unsigned subscriptions);
extern "C" int rtnl_open_byproto(struct rtnl_handle *rth,
		unsigned subscriptions, int protocol);
extern "C" void rtnl_close(struct rtnl_handle *rth);
extern "C" int
		rtnl_wilddump_request(struct rtnl_handle *rth, int fam, int type);
extern "C" int rtnl_dump_request(struct rtnl_handle *rth, int type, void *req,
		int len);

typedef int (*rtnl_filter_t)(const struct sockaddr_nl *, struct nlmsghdr *n,
		void *);

struct rtnl_dump_filter_arg {
	rtnl_filter_t filter;
	void *arg1;
	rtnl_filter_t junk;
	void *arg2;
};

extern "C" int rtnl_dump_filter_l(struct rtnl_handle *rth,
		const struct rtnl_dump_filter_arg *arg);
extern "C" int rtnl_dump_filter(struct rtnl_handle *rth, rtnl_filter_t filter,
		void *arg1, rtnl_filter_t junk, void *arg2);

extern "C" int rtnl_talk(struct rtnl_handle *rtnl, struct nlmsghdr *n,
		pid_t peer, unsigned groups, struct nlmsghdr *answer,
		rtnl_filter_t junk, void *jarg);
extern "C" int rtnl_send(struct rtnl_handle *rth, const char *buf, int);
extern "C" int rtnl_send_check(struct rtnl_handle *rth, const char *buf, int);

extern "C" int addattr32(struct nlmsghdr *n, int maxlen, int type, __u32 data);
extern "C" int addattr_l(struct nlmsghdr *n, int maxlen, int type,
		const void *data, int alen);
extern "C" int addraw_l(struct nlmsghdr *n, int maxlen, const void *data,
		int len);
extern "C" struct rtattr
		*addattr_nest(struct nlmsghdr *n, int maxlen, int type);
extern "C" int addattr_nest_end(struct nlmsghdr *n, struct rtattr *nest);
extern "C" struct rtattr *addattr_nest_compat(struct nlmsghdr *n, int maxlen,
		int type, const void *data, int len);
extern "C" int addattr_nest_compat_end(struct nlmsghdr *n, struct rtattr *nest);
extern "C" int rta_addattr32(struct rtattr *rta, int maxlen, int type,
		__u32 data);
extern "C" int rta_addattr_l(struct rtattr *rta, int maxlen, int type,
		const void *data, int alen);

extern "C" int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta,
		int len);
extern "C" int parse_rtattr_byindex(struct rtattr *tb[], int max,
		struct rtattr *rta, int len);
extern "C" int __parse_rtattr_nested_compat(struct rtattr *tb[], int max,
		struct rtattr *rta, int len);

#define parse_rtattr_nested(tb, max, rta) \
	(parse_rtattr((tb), (max), RTA_DATA(rta), RTA_PAYLOAD(rta)))

#define parse_rtattr_nested_compat(tb, max, rta, data, len) \
({	data = RTA_PAYLOAD(rta) >= len ? RTA_DATA(rta) : NULL; \
	__parse_rtattr_nested_compat(tb, max, rta, len); })

extern "C" int rtnl_listen(struct rtnl_handle *, rtnl_filter_t handler,
		void *jarg);
extern "C" int rtnl_from_file(FILE *, rtnl_filter_t handler, void *jarg);

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#ifndef IFA_RTA
#define IFA_RTA(r) \
	((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifaddrmsg))))
#endif
#ifndef IFA_PAYLOAD
#define IFA_PAYLOAD(n)	NLMSG_PAYLOAD(n,sizeof(struct ifaddrmsg))
#endif

#ifndef IFLA_RTA
#define IFLA_RTA(r) \
	((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ifinfomsg))))
#endif
#ifndef IFLA_PAYLOAD
#define IFLA_PAYLOAD(n)	NLMSG_PAYLOAD(n,sizeof(struct ifinfomsg))
#endif

#ifndef NDA_RTA
#define NDA_RTA(r) \
	((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif
#ifndef NDA_PAYLOAD
#define NDA_PAYLOAD(n)	NLMSG_PAYLOAD(n,sizeof(struct ndmsg))
#endif

#ifndef NDTA_RTA
#define NDTA_RTA(r) \
	((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndtmsg))))
#endif
#ifndef NDTA_PAYLOAD
#define NDTA_PAYLOAD(n) NLMSG_PAYLOAD(n,sizeof(struct ndtmsg))
#endif

#endif /* LIBNETLINK_HH_ */
