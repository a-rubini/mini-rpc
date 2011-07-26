/*
 * Example mini-ipc server
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "minipc.h"
#include "pty-server.h"

#define CLIENT_TIMEOUT 200 /* ms */

/* Retrieve the counters of the pty file descriptors */
static int do_count(struct minipc_ch *client, char **argv)
{
	int ret;
	struct pty_counts c;

	ret = minipc_call(client, CLIENT_TIMEOUT, &rpc_count, &c);
	if (ret < 0)
		return ret;
	printf("counts: %i %i\n", c.in, c.out);
	return 0;
}

/* getenv and setenv in the remote process */
static int do_getenv(struct minipc_ch *client, char **argv)
{
	int ret;
	char buf[256]; /* FIXME: size limits? */
	ret = minipc_call(client, CLIENT_TIMEOUT, &rpc_getenv, buf, argv[2]);
	if (ret < 0)
		return ret;
	printf("getenv(%s) = %s\n", argv[2], buf);
	return 0;
}

static int do_setenv(struct minipc_ch *client, char **argv)
{
	int ret, ret2;
	ret = minipc_call(client, CLIENT_TIMEOUT, &rpc_setenv, &ret2, argv[2],
			  argv[3]);
	if (ret < 0)
		return ret;
	printf("setenv %s = %s : success\n", argv[2], argv[3]);
	return 0;
}

/*
 * This is a parsing table for argv[1]
 */
struct {
	char *name;
	int (*f)(struct minipc_ch *client, char **argv);
	int argc;
} *cp, calls[] = {
	{ "count", do_count, 2},
	{ "getenv", do_getenv, 3},
	{ "setenv", do_setenv, 4},
	{NULL, },
};

/*
 * And this is the trivial main function -- more error checking than code
 */
int main(int argc, char **argv)
{
	struct minipc_ch *client;

	if (argc < 2) {
		fprintf(stderr, "%s: use \"%s <cmd> [<arg> ...]\"\n",
			argv[0], argv[0]);
		exit(1);
	}

	client = minipc_client_create(PTY_RPC_NAME, 0);
	if (!client) {
		fprintf(stderr, "%s: client_create(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}

	/* scan the table */
	for (cp = calls; cp->name; cp++)
		if (!strcmp(argv[1], cp->name))
			break;
	if (!cp->name) {
		fprintf(stderr, "%s: no such command \"%s\"\n", argv[0],
			argv[1]);
		exit(1);
	}
	if (argc != cp->argc) {
		fprintf(stderr, "%s: %s: wrong number of arguments\n",
			argv[0], argv[1]);
		exit(1);
	}

	/* run the RPC we found */
	if (cp->f(client, argv) < 0) {
		fprintf(stderr, "%s: remote \"%s\": %s\n",
			argv[0], argv[1], strerror(errno));
		exit(1);
	}
	exit(0);
}
