/*
 * Example mini-ipc server
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */

/*
 * This file defines the data structures used to describe the RPC calls
 * between server and client. Note that the function pointers are only
 * instantiated in the server
 */
#include <sys/stat.h> /* for the structure */
#include "minipc.h"
#include "pty-server.h"


/* retrieve in and out counts from the pty-server */
struct minipc_pd rpc_count = {
	.name = "count",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, struct pty_counts),
	.args = {
		MINIPC_ARG_END,
	},
};

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

/* feed a string to the underlying shell */
struct minipc_pd rpc_feed = {
	.name = "feed",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
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

