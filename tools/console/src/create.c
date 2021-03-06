#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <util/types.h>
#include <control/rpc.h>
#include <control/vmspec.h>

#include "rpc.h"

static RPC* rpc;

static void help() {
	printf("Usage: create [Core Option] [Memory Option] [Storage Option] [NIC Option] [Arguments Option] \n");
}

static bool callback_vm_create(uint32_t id, void* context) {
	if(id == 0)
		printf("Fail\n");
	else
		printf("%d\n", id);

	rpc_disconnect(rpc);
	return false;
}

static int vm_create(int argc, char* argv[]) {
	// Default value
	VMSpec vm;
	vm.core_size = 1;
	vm.memory_size = 0x1000000;	// 16MB
	vm.storage_size = 0x1000000;	// 16MB
	vm.nic_count = 0;
	NICSpec nics[VM_MAX_NIC_COUNT];
	memset(nics, 0, sizeof(NICSpec) * VM_MAX_NIC_COUNT);
	vm.nics = nics;
	vm.argc = 0;
	//char* _args[VM_MAX_ARGC];
	vm.argv = NULL; //_args;

	// Main options
	static struct option options[] = {
		{ "core", required_argument, 0, 'c' },
		{ "memory", required_argument, 0, 'm' },
		{ "storage", required_argument, 0, 's' },
		{ "nic", required_argument, 0, 'n' },
		{ "args", required_argument, 0, 'a' },
		{ 0, 0, 0, 0 }
	};

	int opt;
	int index = 0;
	while((opt = getopt_long(argc, argv, "c:m:s:n:a:", 
					options, &index)) != -1) {
		switch(opt) {
			case 'c' : 
				vm.core_size = atoi(optarg);
				break;
			case 'm' : 
				vm.memory_size = strtol(optarg, NULL, 16);
				break;
			case 's' : 
				vm.storage_size = strtol(optarg, NULL, 16);
				break;
			case 'n' : 
				;
				// Suboptions for NIC
				enum {
					MAC, DEV, IBUF, OBUF, IBAND, OBAND, HPAD, TPAD, POOL, SLOWPATH,
				};

				char* const token[] = {
					[MAC]   = "mac",
					[DEV]   = "dev",
					[IBUF]	= "ibuf",
					[OBUF]	= "obuf",
					[IBAND]	= "iband",
					[OBAND]	= "oband",
					[HPAD]	= "hpad",
					[TPAD]	= "tpad",
					[POOL]	= "pool",
				};

				char* subopts = optarg;
				char* value;

				NICSpec* nic = &vm.nics[vm.nic_count];
				nic->mac = 0;
				nic->dev = "eth0";
				nic->input_buffer_size = 1024;
				nic->output_buffer_size = 1024;
				nic->input_bandwidth = 1000000000; /* 1 GB */
				nic->output_bandwidth = 1000000000; /* 1 GB */
				nic->padding_head = 32;
				nic->padding_tail = 32;
				nic->pool_size = 0x400000; /* 4 MB */
				while(*subopts != '\0') {
					switch(getsubopt(&subopts, token, &value)) {
						case MAC:
							nic->mac = strtoll(value, NULL, 16);
							break;
						case DEV:
							nic->dev = value;
							break;
						case IBUF:
							nic->input_buffer_size = atol(value);
							break;
						case OBUF:
							nic->output_buffer_size = atol(value);
							break;
						case IBAND:
							nic->input_bandwidth = atoll(value);
							break;
						case OBAND:
							nic->output_bandwidth = atoll(value);
							break;
						case HPAD:
							nic->padding_head = atoi(value);
							break;
						case TPAD:
							nic->padding_tail = atoi(value);
							break;
						case POOL:
							nic->pool_size = strtol(value, NULL, 16);
							break;
						default:
							printf("No match found for token : /%s/\n", value);
							help();
							exit(EXIT_FAILURE);
							break;
					}
				}

				vm.nic_count++;
				break;
			case 'a' :
				vm.argv = &optarg;
				break;

			default: 
				help(); 
				exit(EXIT_FAILURE);
		}
	}


	if(vm.nic_count == 0) {
		NICSpec* nic = &vm.nics[0];
		nic->mac = 0;
		nic->dev = "eth0";
		nic->input_buffer_size = 1024;
		nic->output_buffer_size = 1024;
		nic->input_bandwidth = 1000000000; /* 1 GB */
		nic->output_bandwidth = 1000000000; /* 1 GB */
		nic->pool_size = 0x400000; /* 4 MB */
		vm.nic_count = 1;
	}
	rpc_vm_create(rpc, &vm, callback_vm_create, NULL);

	return 0;
}

int main(int argc, char *argv[]) {
	rpc_init();
	RPCSession* session = rpc_session();
	if(!session) {
		printf("RPC server not connected\n");
		return ERROR_RPC_DISCONNECTED;
	}

	rpc = rpc_connect(session->host, session->port, 3, true);
	if(rpc == NULL) {
		printf("Failed to connect RPC server\n");
		return ERROR_RPC_DISCONNECTED;
	}
	
	int rc;
	if((rc = vm_create(argc, argv))) {
		printf("Failed to create VM. Error code : %d\n", rc);
		rpc_disconnect(rpc);
		return ERROR_CMD_EXECUTE;
	}

	while(1) {
		if(rpc_connected(rpc)) {
			rpc_loop(rpc);
		} else {
			free(rpc);
			break;
		}
	}


	return 0;
}

