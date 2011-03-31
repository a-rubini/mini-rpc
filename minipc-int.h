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
#include <sys/un.h>
#include "minipc.h"

/*  be safe, in case some other header had them slightly differntly */
#undef container_of
#undef offsetof
#undef ARRAY_SIZE

/* We are strongly based on container_of internally */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/*
 * While public symbols are minipc_* internal ones are mpc_* to be shorter.
 * The connection includes an fd, which is the only thing returned back
 */
struct mpc_flist {
	struct minipc_pd *pd;
	char *name;
	struct mpc_flist *next;
};

struct mpc_link {
	struct minipc_ch ch;
	int flags;
	struct mpc_flist *list;
	FILE *logfile;
	struct sockaddr_un addr;
	char name[16];
};
#define MINIPC_FLAG_SERVER		0x10000
#define MINIPC_FLAG_CLIENT		0x20000

extern struct mpc_link *__mpc_base;

#endif /* __MINIPC_INT_H__ */
