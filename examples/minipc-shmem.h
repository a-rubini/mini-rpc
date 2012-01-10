#ifndef __MINIPC_SHMEM_H__
#define __MINIPC_SHMEM_H__

#include <sys/stat.h>
#include <sys/time.h>

#define MINIPC_SHM 0x3465 /* Random */

struct minipc_mbox_info {
	volatile int bridge_req;	/* A request is asked by the bridge */
	volatile int proc_req;		/* A request is asked by the process */

	/* The bridge can ask "stat" to the bare process */
	char fname[64];
	struct stat stbuf;

	/* The bare process can ask timeofday to the bridge */
	struct timeval tv;
};

#define MBOX_POLL_US (10*1000) /* 10ms */

#endif /* __MINIPC_SHMEM_H__ */
