/*
 * Structures for example shmem mini-ipc backends
 *
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */

/*
 * This file defines the data structures used to describe the RPC calls
 * between server and client. Note that the function pointers are only
 * instantiated in the server
 */
#include "minipc.h"
#include "shmem-structs.h"
#include <sys/stat.h>


/* run getenv in the server */
struct minipc_pd rpc_getenv = {
	.name = "getenv",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_END,
	},
};

/* run setenv in the server */
struct minipc_pd rpc_setenv = {
	.name = "setenv",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_END,
	},
};

/* addition of two integers */
struct minipc_pd rpc_add = {
	.name = "add",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		MINIPC_ARG_END,
	},
};

/* horribly run strlen and strcat */
struct minipc_pd rpc_strlen = {
	.name = "strlen",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_END,
	},
};
struct minipc_pd rpc_strcat = {
	.name = "strcat",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_END,
	},
};

/* run "stat" on a file name */
struct minipc_pd rpc_stat = {
	.name = "stat",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, struct stat),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_END,
	},
};

