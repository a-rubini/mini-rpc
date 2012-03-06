/*
 * Public definition for mini-ipc
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
#ifndef __MINIPC_H__
#define __MINIPC_H__

#include <stdint.h>
#if __STDC_HOSTED__ /* freestanding servers have less material */
#include <stdio.h>
#include <sys/select.h>
#endif

/* Hard limits */
#define MINIPC_MAX_NAME		20 /* includes trailing 0 */
#define MINIPC_MAX_CLIENTS	64
#define MINIPC_MAX_ARGUMENTS	256 /* Also, max size of packet words -- 1k */
#define MINIPC_MAX_REPLY	1024 /* bytes */
#if !__STDC_HOSTED__
#define MINIPC_MAX_EXPORT	12 /* freestanding: static allocation */
#endif

/* The base pathname, mkdir is performed as needed */
#define MINIPC_BASE_PATH "/tmp/.minipc"

/* Default polling interval for memory-based channels */
#define MINIPC_DEFAULT_POLL  (10*1000)

/* Argument type (and retval type). The size is encoded in the same word */
enum minipc_at {
	MINIPC_ATYPE_ERROR = 0xffff,
	MINIPC_ATYPE_NONE = 0,	/* used as terminator */
	MINIPC_ATYPE_INT = 1,
	MINIPC_ATYPE_INT64,
	MINIPC_ATYPE_DOUBLE,	/* float is promoted to double */
	MINIPC_ATYPE_STRING,	/* size of strings is strlen() each time */
	MINIPC_ATYPE_STRUCT
};
/* Encoding of argument type and size in one word */
#define __MINIPC_ARG_ENCODE(atype, asize) (((atype) << 16) | (asize))
#define MINIPC_ARG_ENCODE(atype, type) __MINIPC_ARG_ENCODE(atype, sizeof(type))
#define MINIPC_GET_ATYPE(word) ((word) >> 16)
#define MINIPC_GET_ASIZE(word) ((word) & 0xffff)
#define MINIPC_ARG_END __MINIPC_ARG_ENCODE(MINIPC_ATYPE_NONE, 0) /* zero */

/* The exported procedure looks like this */
struct minipc_pd;
typedef int (minipc_f)(const struct minipc_pd *, uint32_t *args, void *retval);

/* This is the "procedure definition" */
struct minipc_pd {
	minipc_f *f;			/* pointer to the function */
	char name[MINIPC_MAX_NAME];	/* name of the function */
	uint32_t flags;
	uint32_t retval;		/* type of return value */
	uint32_t args[];		/* zero-terminated */
};
/* Flags: verbosity is about argument and retval marshall/unmarshall */
#define MINIPC_FLAG_VERBOSE		1

/* This is the channel definition */
struct minipc_ch {
	int fd;
};
static inline int minipc_fileno(struct minipc_ch *ch) {return ch->fd;}

/* These return NULL with errno on error, name is the socket pathname */
struct minipc_ch *minipc_server_create(const char *name, int flags);
struct minipc_ch *minipc_client_create(const char *name, int flags);
int minipc_close(struct minipc_ch *ch);

/* Generic: set the default polling interval for mem-based channels */
int minipc_set_poll(int usec);

/* Server: register exported functions */
int minipc_export(struct minipc_ch *ch, const struct minipc_pd *pd);
int minipc_unexport(struct minipc_ch *ch, const struct minipc_pd *pd);

/* Server: helpers to unmarshall a string or struct from a request */
uint32_t *minipc_get_next_arg(uint32_t arg[], uint32_t atype);

/* Handle a request if pending, otherwise -1 and EAGAIN */
int minipc_server_action(struct minipc_ch *ch, int timeoutms);

#if __STDC_HOSTED__
/* Generic: attach diagnostics to a log file */
int minipc_set_logfile(struct minipc_ch *ch, FILE *logf);

/* Return an fdset for the user to select() on the service */
int minipc_server_get_fdset(struct minipc_ch *ch, fd_set *setptr);

/* Client: make requests */
int minipc_call(struct minipc_ch *ch, int millisec_timeout,
		const struct minipc_pd *pd, void *ret, ...);
#endif /* __STDC_HOSTED__ */

#endif /* __MINIPC_H__ */
