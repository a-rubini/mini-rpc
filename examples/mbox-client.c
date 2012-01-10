/*
 * Example mbox client
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 *
 * This process requests "stat" calls to the mbox-bridge, which in turn
 * asks it to the "remote" process that lives behing an mbox (shmem).
 * The program reads filenames from stdin, in order to allow several clients
 * connected to the same bridge.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "minipc.h"

#define CLIENT_TIMEOUT 100 /* ms */

/* The description here is the same as in the server */
struct minipc_pd rpc_stat = {
	.name = "stat",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, struct stat),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_END,
	},
};

int main(int argc, char **argv)
{
	struct minipc_ch *client;
	struct stat stbuf;
	int ret;
	char s[80];

	client = minipc_client_create("mbox", 0);
	if (!client) {
		fprintf(stderr, "%s: client_create(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	minipc_set_logfile(client, stderr);

	while (fgets(s, sizeof(s), stdin)) {
		/* strip trailing newline */
		s[strlen(s)-1] = '\0';
		ret = minipc_call(client, CLIENT_TIMEOUT, &rpc_stat,
				  &stbuf, s);
		if (ret < 0) {
			fprintf(stderr, "stat(%s): %s\n", s, strerror(errno));
			continue;
		}
		printf("%s:\n", s);
		printf("mode %o, size %li, owner %i, atime %li\n",
		       stbuf.st_mode, (long)stbuf.st_size, stbuf.st_uid,
		       stbuf.st_atime);
	}
	return 0;
}
