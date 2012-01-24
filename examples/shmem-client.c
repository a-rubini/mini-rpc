/*
 * Example mini-ipc client for shmem
 * Mostly copied and adapted from pty-client.c
 *
 * Copyright (C) 2011,2012 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "minipc.h"
#include "shmem-structs.h"

#define CLIENT_TIMEOUT 200 /* ms */

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

static int do_add(struct minipc_ch *client, char **argv)
{
	int ret;
	int a = atoi(argv[2]);
	int b = atoi(argv[3]);
	int sum;

	ret = minipc_call(client, CLIENT_TIMEOUT, &rpc_add, &sum, a, b);
	if (ret < 0)
		return ret;
	printf("add %i + %i = %i : success\n", a, b, sum);
	return 0;
}

/* calculate a remote strlen */
static int do_strlen(struct minipc_ch *client, char **argv)
{
	int ret, len;

	ret = minipc_call(client, CLIENT_TIMEOUT, &rpc_strlen, &len, argv[2]);
	if (ret < 0)
		return ret;
	printf("strlen(%s) = %i\n", argv[2], len);
	return 0;
}

/* calculate a remote strcat */
static int do_strcat(struct minipc_ch *client, char **argv)
{
	int ret;
	char buf[256]; /* FIXME: size limits? */
	ret = minipc_call(client, CLIENT_TIMEOUT, &rpc_strcat, buf,
			  argv[2], argv[3]);
	if (ret < 0)
		return ret;
	printf("strcat(%s,%s) = %s\n", argv[2], argv[3], buf);
	return 0;
}

/* ask for a remote stat on a passed filename */
static int do_stat(struct minipc_ch *client, char **argv)
{
	int ret;
	struct stat stbuf;

	ret = minipc_call(client, CLIENT_TIMEOUT, &rpc_stat, &stbuf,
			  argv[2]);
	if (ret < 0)
		return ret;
	printf("stat(\"%s\"):\n", argv[2]);
	printf("dev %04x, inode %i, mode %o (rdev %04x), size %i\n",
	       (int)stbuf.st_dev, (int)stbuf.st_ino, stbuf.st_mode,
	       (int)stbuf.st_rdev, (int)stbuf.st_size);

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
	{ "getenv", do_getenv, 3},
	{ "setenv", do_setenv, 4},
	{ "add", do_add, 4},
	{ "strlen", do_strlen, 3},
	{ "strcat", do_strcat, 4},
	{ "stat", do_stat, 3},
	{NULL, },
};

/*
 * And this is the trivial main function -- more error checking than code
 */
int main(int argc, char **argv)
{
	struct minipc_ch *client;

	if (argc <= 2) {
		fprintf(stderr, "%s: use \"%s <shmid> <cmd> [<arg> ...]\n",
			argv[0], argv[0]);
		exit(1);
	}

	client = minipc_client_create(argv[1], 0);
	if (!client) {
		fprintf(stderr, "%s: rpc_open(%s): %s\n", argv[0], argv[1],
			strerror(errno));
		exit(1);
	}
	minipc_set_logfile(client, stderr);

	argv[1] = argv[0]; argv++; argc--;

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
