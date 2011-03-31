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
#include <stdio.h>

/* Hard limit */
#define MINIPC_MAX_NAME		16 /* includes trailing 0 */
#define MINIPC_MAX_CLIENTS	8
#define MINIPC_MAX_ARGUMENTS	32

/* Argument type (and retval type). The size is encoded in the same word */
enum minipc_at {
	MINIPC_AT_ERROR = 0xffff,
	MINIPC_AT_INT = 1,
	MINIPC_AT_INT64,
	MINIPC_AT_FLOAT,
	MINIPC_AT_DOUBLE,
	MINIPC_AT_STRING,
	MINIPC_AT_STRUCT
};
/* Encoding of argument type and size in one word */
#define __MINIPC_ARG_ENCODE(at, asize) (((at) << 16) | (asize))
#define MINIPC_ARG_ENCODE(at, type) __MINIPC_ARG_ENCODE(at, sizeof(type))
#define MINIPC_GET_AT(word) ((word) >> 16)
#define MINIPC_GET_ASIZE(word) ((word) & 0xffff)

/* The exported procedure looks like this */
struct minipc_pd;
typedef int (minipc_f)(const struct minipc_pd *,
		       uint32_t *args, uint32_t *ret);

/* This is the "procedure definition" */
struct minipc_pd {
	minipc_f *f;		/* pointer to the function */
	uint32_t  id;		/* function id, usually a 4-byte ascii */
	uint32_t flags;
	uint32_t retval;	/* type of return value */
	uint32_t args[];	/* the list of arguments, null-terminated */
};
/* Flags: verbosity is about argument and retval marshall/unmarshall */
#define MINIPC_FLAG_VERBOSE		1

/* This is the channel definition */
struct minipc_ch {
	int fd;
};
static inline int minipc_fileno(struct minipc_ch *ch) {return ch->fd;}

/* These return NULL with errno on erro */
struct minipc_ch *minipc_server_create(const char *name, int flags);
struct minipc_ch *minipc_client_create(const char *name, int flags);
int minipc_close(struct minipc_ch *ch);

/* Generic: attach diagnostics to a log file */
int minipc_set_logfile(struct minipc_ch *ch, FILE *logf);

/* Server: register exported functions */
int minipc_export(struct minipc_ch *ch, const char *name,
		  const struct minipc_pd *pd);
int minipc_unexport(struct minipc_ch *ch, const char *name,
		    const struct minipc_pd *pd);

/* Handle a request if pending, otherwise -1 and EAGAIN */
int minipc_server_action(struct minipc_ch *ch, int timeoutms);

/* Return an fdset for the user to select() on the service */
int minipc_server_get_fdset(struct minipc_ch *ch, fd_set *setptr);

/* Client: make requests */
int minipc_call(struct minipc_ch *ch, const struct minipc_pd *pd,
		uint32_t *args, uint32_t *ret);

#endif /* __MINIPC_H__ */
