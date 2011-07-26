#include "minipc.h"

struct pty_counts {
	int in, out;
};

#define PTY_RPC_NAME "pty-server"

/* structures are in pty-rpc_structs.c */
extern struct minipc_pd
rpc_count, rpc_getenv, rpc_setenv, rpc_feed, rpc_strlen, rpc_strcat, rpc_stat;

/* The server needs to call out to a different file */
extern int pty_export_functions(struct minipc_ch *ch,
				int fdm, struct pty_counts *pc);
