/*
 * Example mini-ipc server
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */

/*
 * This is the pty-server. It only does the pty-master work, all RPC stuff
 * has been confined to prt-rpc_server.c, which is called by our main loop
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pty.h>
#include <fcntl.h>
#include <utmp.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include "minipc.h"
#include "pty-server.h"

int main(int argc, char **argv)
{
	int fdm, fds, pid, exitval = 0;
	struct minipc_ch *ch;
	struct pty_counts counters = {0,};

	/* First, open the pty */
	if (openpty(&fdm, &fds, NULL, NULL, NULL) < 0) {
		fprintf(stderr, "%s: openpty(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}

	/* Run a shell and let it go by itself before we open the rpc */
	fprintf(stderr, "%s: Running a sub-shell in a new pty\n", argv[0]);
	if ((pid = fork()) < 0) {
		fprintf(stderr, "%s: fork(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}
	if (!pid) {
		/* Child: become a shell and disappear... */
		close(fdm);
		login_tty(fds);
		execl("/bin/sh", "sh", NULL);
		fprintf(stderr, "%s: exec(/bin/sh): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}

	/* Open the RPC server channel */
	ch = minipc_server_create(PTY_RPC_NAME, 0);
	if (!ch) {
		fprintf(stderr, "%s: rpc_open(): %s\n", argv[0],
			strerror(errno));
		exit(1);
	}

	/* Log file for diagnostics */
	{
		char name[] = "/tmp/pty-server.XXXXXX";
		int logfd;
		FILE *logf;

		logfd = mkstemp(name);
		if (logfd >= 0) {
			logf = fdopen(logfd, "w");
			if (logf)
				minipc_set_logfile(ch, logf);
		}
	}

	/* Register your functions: all our RPC is split to another source */
	if (pty_export_functions(ch, fdm, &counters)) {
		fprintf(stderr, "%s: exporting RPC functions: %s\n", argv[0],
			strerror(errno));
		exit(1);
	}

	/*
	 * Now, we must mirror stdin/stdout to the pty master, with RPC too.
	 * The first step is horribly changing the termios of our tty
	 */
	close(fds);
	if (system("stty raw -echo") < 0) {
		fprintf(stderr, "%s: can't run \"stty\"\n", argv[0]);
		exit(1);
	}

	while (waitpid(pid, NULL, WNOHANG) != pid) {
		fd_set set;
		int nfd, i, j;
		char buf[256];

		/* ask the RPC engine its current fdset and augment it */
		minipc_server_get_fdset(ch, &set);
		FD_SET(STDIN_FILENO, &set);
		FD_SET(fdm, &set);

		/* wait for any of the FD to be active */
		nfd = select(64 /* Hmmm... */, &set, NULL, NULL, NULL);
		if (nfd < 0 && errno == EINTR)
			continue;
		if (nfd < 0) {
			fprintf(stderr, "%s: select(): %s\n", argv[0],
			strerror(errno));
			exitval = 1;
			break;
		}

		/* Handle fdm and fds by just mirroring stuff and counting */
		if (FD_ISSET(STDIN_FILENO, &set)) {
			i = read(0, buf, sizeof(buf));
			if (i > 0) {
				counters.in += i;
				do {
					j = write(fdm, buf, i);
					i -= j;
				} while (i && j >= 0);
			}
			nfd--;
		}
		if (FD_ISSET(fdm, &set)) {
			i = read(fdm, buf, sizeof(buf));
			if (i > 0) {
				counters.out += i;
				do {
					j = write(1, buf, i);
					i -= j;
				} while (i && j >= 0);
			}
			nfd--;
		}

		/* If there are no more active fd, loop over */
		if (!nfd)
			continue;

		/*
		 * If we are there, there has been an RPC call.
		 * We tell the library to use a 0 timeout, since we know
		 * for sure that at least one of its descriptors is pending.
		 */
		minipc_server_action(ch, 0);
	}

	/* The child shell exited, reset the tty and exit. Let RPC die out */
	if (system("stty sane") < 0)
		fprintf(stderr, "%s: can't restore tty settings\n", argv[0]);
	exit(exitval);
}
