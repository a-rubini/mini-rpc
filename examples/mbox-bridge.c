/*
 * Example bridge beteen mini-ipc and a mailbox memory area
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */

/*
 * This example uses a shared memory area to talk with mbox-process.
 * The same technique can be used to communicate with an os-less
 * soft-core within an FPGA, and actually this is the reason why I'm
 * writing this demo.
 *
 * The bridge can export functions in both ways: as a mini-ipc server
 * it exports "stat", and as a client it calls gettimeofday in the
 * trivial-server.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/select.h>

#include "minipc.h"
#include "minipc-shmem.h" /* The shared data structures */

struct minipc_mbox_info *info; /* unfortunately global... */

/* This function implements the "stat" mini-ipc server, by asking mbox  */
static int mb_stat_server(const struct minipc_pd *pd,
			  uint32_t *args, void *ret)
{
	char *fname = (void *)args;

	/* pass this to the shmem */
	memset(info->fname, 0, sizeof(info->fname));
	strncpy(info->fname, fname, sizeof(info->fname) -1 );

	/* ask and wait */
	info->bridge_req = 1;
	while (info->bridge_req)
		usleep(1000);

	memcpy(ret, &info->stbuf, sizeof(info->stbuf));
	return 0;
}

/* The description here is the same as in the server */
const struct minipc_pd mb_tod_struct = {
	.name = "gettimeofday",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, struct timeval),
	.args = {
		MINIPC_ARG_END,
	},
};

/* And here it's the same as in the client */
struct minipc_pd mb_stat_struct = {
	.f = mb_stat_server,
	.name = "stat",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, struct stat),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		MINIPC_ARG_END,
	},
};

/* No arguments */
int main(int argc, char **argv)
{
	struct minipc_ch *server;
	struct minipc_ch *client;
	void *shmem;
	int ret;

	if (argc > 1) {
		fprintf(stderr, "%s: no arguments please\n", argv[0]);
		exit(1);
	}

	/* Create your shared memory and/or attach to it */
	ret = shmget(MINIPC_SHM, sizeof(struct minipc_mbox_info),
		     IPC_CREAT | 0666);
	if (ret < 0) {
		fprintf(stderr, "%s: shmget(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	shmem = shmat(ret, NULL, SHM_RND);
	if (shmem == (void *)-1) {
		fprintf(stderr, "%s: shmat(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	info = shmem;

	/* Create a server socket for mini-ipc */
	server = minipc_server_create("mbox", 0);
	if (!server) {
		fprintf(stderr, "%s: server_create(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	minipc_set_logfile(server, stderr);
	minipc_export(server, &mb_stat_struct);

	/* Connect as a client to the trivial-server */
	client = minipc_client_create("trivial", 0);
	if (!client) {
		fprintf(stderr, "%s: client_create(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}

	/* Loop serving both mini-ipc and the mailbox */
	while (1) {
		fd_set set;
		struct timeval to = {
			MBOX_POLL_US / (1000*1000),
			MBOX_POLL_US % (1000*1000)
		};

		minipc_server_get_fdset(server, &set);

		/* Wait for any server, with the defined timeout */
		ret = select(16 /* hack */, &set, NULL, NULL, &to);

		if (ret > 0) {
			if (minipc_server_action(server, 0) < 0) {
				fprintf(stderr, "%s: server_action(): %s\n",
					argv[0], strerror(errno));
				exit(1);
			}
		}

		/* No IPC request: if needed act as IPC client */
		if (info->proc_req) {
			memset(&info->tv, 0, sizeof(info->tv));
			minipc_call(client, 100 /* ms */,
				    &mb_tod_struct, &info->tv, NULL);
			info->proc_req = 0;
		}
	}
}
