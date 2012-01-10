/*
 * Example process connected to the world through a mailbox
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */

/*
 * This example uses a shared memory area to talk with the mbox-bridge.
 * In my real-world application this will be an os-less
 * soft-core within an FPGA.
 *
 * The process is both a server and a client. As a server iIt replies
 * to "stat" calls coming from the bridge process; as a client it asks
 * the time of day once a second or so.
 *
 * It doesn't need minipc.h because it is not directly connected to the ipc
 * mechanism
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "minipc-shmem.h" /* The shared data structures */

struct minipc_mbox_info *info; /* unfortunately global... */

/* No arguments */
int main(int argc, char **argv)
{
	void *shmem;
	int ret, i;

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

	/* Loop forever */
	for (i = 0; 1; i++) {
		if (info->bridge_req) {
			/* We got a request: serve it */
			printf("Serving stat(%s)\n", info->fname);
			memset(&info->stbuf, 0, sizeof(info->stbuf));
			stat(info->fname, &info->stbuf);
			info->bridge_req = 0;
		}
		if (i * MBOX_POLL_US >= 1000 * 1000) {
			/* 1s passed, ask for the date */
			info->proc_req = 1;
			while (info->proc_req)
				;
			printf("time: %li.%06li\n", (long)info->tv.tv_sec,
			       (long)info->tv.tv_usec);
			i = 0;
		}
		usleep(MBOX_POLL_US);
	}
}
