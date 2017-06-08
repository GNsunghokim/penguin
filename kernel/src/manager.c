#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <net/ether.h>
#undef IP_TTL
#include <net/ip.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <timer.h>
#include <util/event.h>
#include <util/cmd.h>
#include <util/map.h>
#undef BYTE_ORDER
#include <netif/etharp.h>
#include <arch/driver.h>
#include <control/rpc.h>
#include <net/interface.h>
#include <lwip/init.h>
#include <lwip/netif.h>
#include "gmalloc.h"
#include "malloc.h"
#include "mp.h"
#include "shell.h"
#include "vm.h"
#include "stdio.h"
#include "driver/nicdev.h"

#include "manager.h"

static int cmd_manager(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	//TODO error handle
	for(int i = 1; i < argc; i++) {
		if(!strcmp("open", argv[i])) {
			NEXT_ARGUMENTS();

			uint16_t vmid;
			uint16_t vnic_index;
			uint16_t interface_index;
			if(!parse_vnic_interface(argv[i], &vmid, &vnic_index, &interface_index))
				return -i;

			if(vmid)
				return -i;

			VNIC* vnic = manager.vnics[vnic_index];
			if(!vnic)
				return -i;

			IPv4InterfaceTable* table = interface_table_get(vnic->nic);
			if(!table)
				return -i;

			uint16_t offset = 1;
			for(int i = 0; i < interface_index; i++)
				offset <<= 1;

			if(!(table->bitmap & offset))
				return -i;

			IPv4Interface* interface = &table->interfaces[interface_index];
			struct netif* netif = manager_create_netif(vnic->nic, interface->address, interface->netmask, interface->gateway, false);
			printf("open: %x %x %x\n", interface->address, interface->netmask, interface->gateway);
			//manager_netif_server_open(netif, 1111);
			manager_core = manager_core_server_open(port);
			return 0;
		} else if(!strcmp("close", argv[i])) {
			return manager_core_server_close(manager_core);
		}
	}

	return 0;
}

static Command commands[] = {
	{
		.name = "manager",
		.desc = "Set ip, port, netmask, gateway, nic of manager",
		.func = cmd_manager
	}
}
// RPC Handlers
static void vm_create_handler(RPC* rpc, VMSpec* vm_spec, void* context, void(*callback)(RPC* rpc, uint32_t id)) {
	uint32_t id = vm_create(vm_spec);
	callback(rpc, id);
}

static void vm_get_handler(RPC* rpc, uint32_t vmid, void* context, void(*callback)(RPC* rpc, VMSpec* vm)) {
	// TODO: Implement it
	callback(rpc, NULL);
}

static void vm_set_handler(RPC* rpc, VMSpec* vm, void* context, void(*callback)(RPC* rpc, bool result)) {
	// TODO: Implement it
	callback(rpc, false);
}

static void vm_destroy_handler(RPC* rpc, uint32_t vmid, void* context, void(*callback)(RPC* rpc, bool result)) {
	bool result = vm_destroy(vmid);
	callback(rpc, result);
}

static void vm_list_handler(RPC* rpc, int size, void* context, void(*callback)(RPC* rpc, uint32_t* ids, int size)) {
	uint32_t ids[16];
	size = vm_list(ids, size <= 16 ? size : 16);
	callback(rpc, ids, size);
}

static void status_get_handler(RPC* rpc, uint32_t vmid, void* context, void(*callback)(RPC* rpc, VMStatus status)) {
	VMStatus status = vm_status_get(vmid);
	callback(rpc, status);
}

static void status_setted(bool result, void* context) {
	RPC* rpc = context;

	if(list_index_of(manager.clients, rpc, NULL) >= 0) {
		data->callback(rpc, result);
	}
	free(data);
}

static void status_set_handler(RPC* rpc, uint32_t vmid, VMStatus status, void* context, void(*callback)(RPC* rpc, bool result)) {
	vm_status_set(vmid, status, status_setted, rpc);
}

static void storage_download_handler(RPC* rpc, uint32_t vmid, uint64_t download_size, uint32_t offset, int32_t size, void* context, void(*callback)(RPC* rpc, void* buf, int32_t size)) {
	if(size < 0) {
		callback(rpc, NULL, size);
	} else {
		void* buf;
		if(download_size)
			size = vm_storage_read(vmid, &buf, offset, (offset + size > download_size) ? (download_size - offset) : (uint32_t)size);
		else
			size = vm_storage_read(vmid, &buf, offset, (uint32_t)size);

		callback(rpc, buf, size);
	}
}

static void storage_upload_handler(RPC* rpc, uint32_t vmid, uint32_t offset, void* buf, int32_t size, void* context, void(*callback)(RPC* rpc, int32_t size)) {
	static int total_size = 0;
	if(size < 0) {
		callback(rpc, size);
	} else {
		if(offset == 0) {
			ssize_t len;
			if((len = vm_storage_clear(vmid)) < 0) {
				callback(rpc, len);
				return;
			}
		}

		if(size < 0) {
			printf(". Aborted: %d\n", size);
			callback(rpc, size);
		} else {
			size = vm_storage_write(vmid, buf, offset, size);
			callback(rpc, size);

			if(size > 0) {
				printf(".");
				total_size += size;
			} else if(size == 0) {
				printf(". Done. Total size: %d\n", total_size);
				total_size = 0;
			} else if(size < 0) {
				printf(". Error: %d\n", size);
				total_size = 0;
			}
		}
	}
}

static void stdio_handler(RPC* rpc, uint32_t id, uint8_t thread_id, int fd, char* str, uint16_t size, void* context, void(*callback)(RPC* rpc, uint16_t size)) {
	ssize_t len = vm_stdio(id, thread_id, fd, str, size);
	callback(rpc, len >= 0 ? len : 0);
}

static void storage_md5_handler(RPC* rpc, uint32_t id, uint64_t size, void* context, void(*callback)(RPC* rpc, bool result, uint32_t md5[])) {
	uint32_t md5sum[4];
	bool ret = vm_storage_md5(id, size, md5sum);

	callback(rpc, ret, md5sum);
}

static err_t manager_accept(RPC* rpc) {
	rpc_vm_create_handler(rpc, vm_create_handler, NULL);
	rpc_vm_get_handler(rpc, vm_get_handler, NULL);
	rpc_vm_set_handler(rpc, vm_set_handler, NULL);
	rpc_vm_destroy_handler(rpc, vm_destroy_handler, NULL);
	rpc_vm_list_handler(rpc, vm_list_handler, NULL);
	rpc_status_get_handler(rpc, status_get_handler, NULL);
	rpc_status_set_handler(rpc, status_set_handler, NULL);
	rpc_storage_download_handler(rpc, storage_download_handler, NULL);
	rpc_storage_upload_handler(rpc, storage_upload_handler, NULL);
	rpc_stdio_handler(rpc, stdio_handler, NULL);
	rpc_storage_md5_handler(rpc, storage_md5_handler, NULL);

	return ERR_OK;
}

// RPC Manager
static void stdio_callback(uint32_t vmid, int thread_id, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size) {
	ListIterator iter;
	list_iterator_init(&iter, manager.clients);
	size_t len0, len1, len2;
	bool wrapped;

	if(*head <= *tail) {
		wrapped = false;

		len0 = *tail - *head;
	} else {
		wrapped = true;

		len1 = size - *head;
		len2 = *tail;
	}

	while(list_iterator_has_next(&iter)) {
		RPC* rpc = list_iterator_next(&iter);

		if(wrapped) {
			rpc_stdio(rpc, vmid, thread_id, fd, buffer + *head, len1, NULL, NULL);
			rpc_stdio(rpc, vmid, thread_id, fd, buffer, len2, NULL, NULL);

		} else {
			rpc_stdio(rpc, vmid, thread_id, fd, buffer + *head, len0, NULL, NULL);
		}

		rpc_loop(rpc);
	}

	if(wrapped) {
		*head = (*head + len1 + len2) % size;
	} else {
		*head += len0;
	}
}

int manager_init() {
	manager_core_init();

	vm_stdio_handler(stdio_callback);

	cmd_register(commands, sizeof(commands) / sizeof(commands[0]));

	return 0;
}
