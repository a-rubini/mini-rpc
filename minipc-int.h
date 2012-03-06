/*
 * Private definition for mini-ipc
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 * Based on ideas by Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 */
#ifndef __MINIPC_INT_H__
#define __MINIPC_INT_H__

#include <sys/types.h>
#if __STDC_HOSTED__ /* freestanding servers have less material */
#include <sys/un.h>
#include <sys/select.h>
#endif

#include "minipc.h"

/*  be safe, in case some other header had them slightly differntly */
#undef container_of
#undef offsetof
#undef ARRAY_SIZE

/* We are strongly based on container_of internally */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/*
 * While public symbols are minipc_* internal ones are mpc_* to be shorter.
 * The connection includes an fd, which is the only thing returned back
 */

/* The list of functions attached to a service */
struct mpc_flist {
	const struct minipc_pd *pd;
	struct mpc_flist *next;
};

/*
 * The main server or client structure. Server links have client sockets
 * hooking on it.
 */
struct mpc_link {
	struct minipc_ch ch;
	int magic;
	int pid;
	int flags;
	struct mpc_link *nextl;
	struct mpc_flist *flist;
	void *memaddr;
	int memsize;
#if __STDC_HOSTED__ /* these fields are not used in freestanding uC */
	FILE *logf;
	struct sockaddr_un addr;
	int fd[MINIPC_MAX_CLIENTS];
	fd_set fdset;
#endif
	char name[MINIPC_MAX_NAME];
};
#define MPC_MAGIC		0xc0ffee99

#define MPC_FLAG_SERVER		0x00010000
#define MPC_FLAG_CLIENT		0x00020000
#define MPC_FLAG_SHMEM		0x00040000
#define MPC_FLAG_DEVMEM		0x00080000
#define MPC_USER_FLAGS(x)	((x) & 0xffff)

/* The request packet being transferred */
struct mpc_req_packet {
	char name[MINIPC_MAX_NAME];
	uint32_t args[MINIPC_MAX_ARGUMENTS];
};
/* The reply packet being transferred */
struct mpc_rep_packet {
	uint32_t type;
	uint8_t val[MINIPC_MAX_REPLY];
};

/* A structure for shared memory (takes more than 2kB) */
struct mpc_shmem {
	uint32_t	nrequest;	/* incremented at each request */
	uint32_t	nreply;		/* incremented at each reply */
	struct mpc_req_packet	request;
	struct mpc_rep_packet	reply;
};

#define MPC_TIMEOUT		1000 /* msec, hardwired */

static inline struct mpc_link *mpc_get_link(struct minipc_ch *ch)
{
	return container_of(ch, struct mpc_link, ch);
}

#define CHECK_LINK(link) /* Horrible shortcut, don't tell around... */	\
	if ((link)->magic != MPC_MAGIC) {				\
		errno = EINVAL;						\
		return -1;						\
	}

extern struct mpc_link *__mpc_base;
extern void mpc_free_flist(struct mpc_link *link, struct mpc_flist *flist);

extern struct minipc_ch *__minipc_link_create(const char *name, int flags);

/* Used for lists and structures -- sizeof(uint32_t) is 4, is it? */
#define MINIPC_GET_ANUM(len) (((len) + 3) >> 2)


#endif /* __MINIPC_INT_H__ */
