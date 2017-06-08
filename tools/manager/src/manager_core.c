#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <control/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <_malloc.h>
#include <fcntl.h>

#include "rpc.h"

err_t *accept(RPC* crpc);

typedef struct {
	int	fd;
	struct	sockaddr_in caddr;
} RPCData;

static int sock_read(RPC* rpc, void* buf, int size) {
	RPCData* data = (RPCData*)rpc->data;
	int len = recv(data->fd, buf, size, MSG_DONTWAIT);
	#if DEBUG
	if(len > 0) {
		printf("Read: ");
		for(int i = 0; i < len; i++) {
			printf("%02x ", ((uint8_t*)buf)[i]);
		}
		printf("\n");
	}
	#endif /* DEBUG */

	if(len < 0) {
		if(errno == EAGAIN || errno == 0) {
			int error;
			unsigned int len = sizeof(error);
			getsockopt(data->fd, SOL_SOCKET, SO_ERROR, &error, &len);
			if(error) {
				return -1;
			}

			return 0;
		} else
			return -1;
	} else {
		return len;
	}
}

static int sock_write(RPC* rpc, void* buf, int size) {
	RPCData* data = (RPCData*)rpc->data;
	int len = send(data->fd, buf, size, MSG_DONTWAIT);
	#if DEBUG
	if(len > 0) {
		printf("Write: ");
		for(int i = 0; i < len; i++) {
			printf("%02x ", ((uint8_t*)buf)[i]);
		}
		printf("\n");
	}
	#endif /* DEBUG */

	if(len < 0) {
		if(errno == EAGAIN || errno == 0) {
			int error;
			unsigned int len = sizeof(error);
			getsockopt(data->fd, SOL_SOCKET, SO_ERROR, &error, &len);
			if(error) {
				return -1;
			}

			return 0;
		} else
			return -1;
	} else if(len == 0) {
		return -1;
	} else {
		return len;
	}
}

static void sock_close(RPC* rpc) {
	RPCData* data = (RPCData*)rpc->data;
	close(data->fd);
	data->fd = -1;
#if DEBUG
	printf("Connection closed : %s\n", inet_ntoa(data->caddr.sin_addr));
#endif
}

void handler(int signo) {
	// Do nothing just interrupt
}

extern void* __malloc_pool;
RPC* rpc_open(const char* host, int port, int timeout) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	if(fd < 0) {
		return NULL;
	}

	struct sigaction sigact, old_sigact;
	sigact.sa_handler = handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_INTERRUPT;

	if(sigaction(SIGALRM, &sigact, &old_sigact) < 0) {
		close(fd);
		return NULL;
	}

	struct sockaddr_in addr;
	memset(&addr, 0x0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(port);

	alarm(timeout);

	if(connect(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
		alarm(0);
		sigaction(SIGALRM, &old_sigact, NULL);
		close(fd);
		return NULL;
	}

	alarm(0);

	if(sigaction(SIGALRM, &old_sigact, NULL) < 0) {
		close(fd);
		return NULL;
	}

	RPC* rpc = malloc(sizeof(RPC) + sizeof(RPCData));
	memset(rpc, 0, sizeof(RPC) + sizeof(RPCData));
	rpc->read = sock_read;
	rpc->write = sock_write;
	rpc->close = sock_close;

	RPCData* data = (RPCData*)rpc->data;
	data->fd = fd;

	return rpc;
}

bool rpc_is_closed(RPC* rpc) {
	RPCData* data = (RPCData*)rpc->data;
	return data->fd < 0;

}

void rpc_close(RPC* rpc) {
	if(rpc->close)
		rpc->close(rpc);
}

void rpc_disconnect(RPC* rpc) {
	if(!rpc_is_closed(rpc))
		rpc_close(rpc);	
}

bool rpc_connected(RPC* rpc) {
	return !rpc_is_closed(rpc);
}

typedef struct {
	bool keep_session;
	uint16_t count;
	uint64_t current;
	RPC* rpc;
} HelloContext;

static bool callback_hello(void* context) {
	HelloContext* context_hello = context;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint64_t current = tv.tv_sec * 1000 * 1000 + tv.tv_usec;

	printf("Ping PacketNgin time = %0.3f ms\n", (float)(current - context_hello->current) / (float)1000);
	context_hello->current = current;
	context_hello->count--;
	RPC* rpc = context_hello->rpc;

	if(context_hello->count == 0) {
		if(!context_hello->keep_session) {
			rpc_disconnect(rpc);
		}
		free(context_hello);

		return false;
	}

	rpc_hello(rpc, callback_hello, context_hello);

	return true;
}

RPC* rpc_connect(char* host, int port, int timeout, bool keep_session) {
	RPC* rpc = rpc_open(host, port, timeout);
	if(rpc == NULL)
		return NULL;

	struct timeval tv;
	gettimeofday(&tv, NULL);

	HelloContext* context_hello = malloc(sizeof(HelloContext));
	if(context_hello == NULL) {
		rpc_close(rpc);
		return NULL;
	}
	memset(context_hello, 0, sizeof(HelloContext));

	context_hello->current = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
	context_hello->count = 1;
	context_hello->rpc = rpc;
	context_hello->keep_session = keep_session;

	rpc_hello(rpc, callback_hello, context_hello);

	return rpc;
}

RPCSession* rpc_session() {
	RPCSession* session = malloc(sizeof(RPCSession));
	if(!session)
		return NULL;
	memset(session, 0, sizeof(RPCSession));

	session->host = getenv("MANAGER_IP");
	if(!session->host)
		return NULL;

	char* port = getenv("MANAGER_PORT");
	if(!port)
		return NULL;
	session->port = atoi(port);

	return session;
}

static char buf[0x200000];
void rpc_init() {
	__malloc_init(buf, 0x200000);
}

RPC* rpc_listen(int port) {
       int fd = socket(AF_INET, SOCK_STREAM, 0);
       if(fd < 0) {
               return NULL;
       }

       int reuse = 1;
       if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
               perror("Failed to set socket option - SO_REUSEADDR\n");

       struct sockaddr_in addr;
       memset(&addr, 0x0, sizeof(struct sockaddr_in));
       addr.sin_family = AF_INET;
       addr.sin_addr.s_addr = htonl(INADDR_ANY);
       addr.sin_port = htons(port);

       if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
               return NULL;
       }

       RPC* rpc = malloc(sizeof(RPC) + sizeof(RPCData));
       memset(rpc, 0x0, sizeof(RPC));
       RPCData* data = (RPCData*)rpc->data;
       data->fd = fd;

       return rpc;
}

RPC* rpc_accept(RPC* srpc) {
       RPCData* data = (RPCData*)srpc->data;

       if(listen(data->fd, 5) < 0) {
               return NULL;
       }
       
       // TODO: would rather change to nonblock socket
       int rc = fcntl(data->fd, F_SETFL, fcntl(data->fd, F_GETFL, 0) | O_NONBLOCK);
       if(rc < 0)
               perror("Failed to modifiy socket to nonblock\n");
                   
       socklen_t len = sizeof(struct sockaddr_in);
       struct sockaddr_in addr;
       int fd = accept(data->fd, (struct sockaddr*)&addr, &len);
       if(fd < 0)
               return NULL;

       RPC* rpc = malloc(sizeof(RPC) + sizeof(RPCData));
       memcpy(rpc, srpc, sizeof(RPC));
       rpc->ver = 0;
       rpc->rbuf_read = 0;
       rpc->rbuf_index = 0;
       rpc->wbuf_index = 0;
       rpc->read = sock_read;
       rpc->write = sock_write;
       rpc->close = sock_close;

       data = (RPCData*)rpc->data;
       memcpy(&data->caddr, &addr, sizeof(struct sockaddr_in));
       data->fd = fd;

       return rpc;
}

typedef struct {
	int     fd;
	struct  sockaddr_in caddr;
} RPCData;

static List* actives;	/* rpc */


static bool manager_accept_loop(void* rpc) {
	// RPC socket is nonblocking
	RPC* crpc = rpc_accept((RPC*)rpc);
	if(!crpc)
		return true;

	accept(crpc);

	if(list_index_of(actives, crpc, NULL) < 0)
		list_add(actives, crpc);

	RPCData* data = (RPCData*)crpc->data;
	//printf("Connection opened : %s\n", inet_ntoa(data->caddr.sin_addr));

	return true;
}

bool manager_core_server_close(ManagerCore* manager_core) {
}

ManagerCore* manager_core_server_open(uint16_t port) {
	ManagerCore* manager_core = malloc(sizeof(ManagerCore));

	RPC* rpc = rpc_listen(port);
	if(!rpc) {
		perror("\tFailed to listen RPC server socket");
		return NULL;
	}

	if(actives == NULL)
		actives = list_create(NULL);

	event_idle_add(manager_accept_loop, (void*)rpc);

	return manager_core;
}

static bool manager_loop(void* context) {
	if(!list_is_empty(actives)) {
		ListIterator iter;
		list_iterator_init(&iter, actives);
		while(list_iterator_has_next(&iter)) {
			RPC* rpc = list_iterator_next(&iter);
			if(!rpc_is_closed(rpc))
				rpc_loop(rpc);
			else
				list_remove_data(actives, rpc);
		}
	}

	return true;
}

bool manager_core_init() {
	if(!manager_server_open()) {
		printf("\tFailed to open manager server\n");
		return false;
	}
	printf("\tManager RPC server opened\n");

	event_idle_add(manager_loop, NULL);

	vm_stdio_handler(stdio_callback);

	return true;
}
