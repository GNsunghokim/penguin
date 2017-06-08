#ifndef __RPC_H__
#define __RPC_H__

#include <control/rpc.h>

#define ERROR_RPC_DISCONNECTED	-10000
#define ERROR_CMD_EXECUTE	-10001
#define ERROR_MALLOC_NULL	-10002

typedef struct _RPCSession {
	char* host;
	int port;
} RPCSession;

RPC* rpc_connect(char* host, int port, int timeout, bool keep_session);
void rpc_disconnect(RPC* rpc);
bool rpc_connected(RPC* rpc);
RPCSession* rpc_session();
void rpc_init();

#endif /* __RPC_H__ */
