#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <malloc.h>
#include <util/event.h>
#include <errno.h>
#include <util/map.h>
#include <util/ring.h>
#include <util/cmd.h>
#include <net/md5.h>
#include <timer.h>
#include "file.h"
#include <vnic.h>
#include "icc.h"
#include "vm.h"
#include "apic.h"
#include "icc.h"
#include "gmalloc.h"
#include "stdio.h"
#include "shared.h"
#include "mmap.h"
#include "driver/nicdev.h"
#include "driver/disk.h"
#include "driver/fs.h"

static uint32_t	last_vmid = 1;
// FIXME: change to static
Map*	vms;

// Core status
typedef struct {
	int			status;		// VM_STATUS_XXX
	int			error_code;
	int			return_code;

	VM*			vm;

	char*			stdin;
	volatile size_t*	stdin_head;
	volatile size_t*	stdin_tail;
	size_t			stdin_size;

	char*			stdout;
	volatile size_t*	stdout_head;
	volatile size_t*	stdout_tail;
	size_t			stdout_size;

	char*			stderr;
	volatile size_t*	stderr_head;
	volatile size_t*	stderr_tail;
	size_t			stderr_size;
} Core;

static Core cores[MP_MAX_CORE_COUNT];

static VM_STDIO_CALLBACK stdio_callback;

static int cmd_md5(int argc, char** argv, void(*callback)(char* result, int exit_status));
static int cmd_create(int argc, char** argv, void(*callback)(char* result, int exit_status));
static int cmd_vm_destroy(int argc, char** argv, void(*callback)(char* result, int exit_status));
static int cmd_vm_list(int argc, char** argv, void(*callback)(char* result, int exit_status));
static int cmd_upload(int argc, char** argv, void(*callback)(char* result, int exit_status));
static int cmd_status_set(int argc, char** argv, void(*callback)(char* result, int exit_status));
static int cmd_status_get(int argc, char** argv, void(*callback)(char* result, int exit_status));
static int cmd_stdio(int argc, char** argv, void(*callback)(char* result, int exit_status));
static int cmd_mount(int argc, char** argv, void(*callback)(char* result, int exit_status));
static Command commands[] = {
	{
		.name = "create",
		.desc = "Create VM",
		.args = "[-c core_count:u8] [-m memory_size:u32] [-s storage_size:u32] "
			"[-n [mac:u64],[dev:str],[ibuf:u32],[obuf:u32],[iband:u64],[oband:u64],[hpad:u16],[tpad:u16],[pool:u32] ] "
			"[-a args:str] -> vmid ",
		.func = cmd_create
	},
	{
		.name = "destroy",
		.desc = "Destroy VM",
		.args = "vmid:u32 -> bool",
		.func = cmd_vm_destroy
	},
	{
		.name = "list",
		.desc = "Print a list of VM IDs",
		.args = " -> u64[]",
		.func = cmd_vm_list
	},
	{
		.name = "upload",
		.desc = "Upload file",
		.args = "vmid:u32 path:str -> bool",
		.func = cmd_upload
	},
	{
		.name = "md5",
		.desc = "MD5 storage",
		.args = "vmid:u32 [size:u64] -> str",
		.func = cmd_md5
	},
	{
		.name = "start",
		.desc = "Start VM",
		.args = "vmid:u32 -> bool",
		.func = cmd_status_set
	},
	{
		.name = "pause",
		.desc = "Pause VM",
		.args = "vmid:u32 -> bool",
		.func = cmd_status_set
	},
	{
		.name = "resume",
		.desc = "Resume VM",
		.args = "vmid:u32 -> bool",
		.func = cmd_status_set
	},
	{
		.name = "stop",
		.desc = "Stop VM",
		.args = "vmid:u32 -> bool",
		.func = cmd_status_set
	},
	{
		.name = "status",
		.desc = "Get VM's status",
		.args = "vmid:u32 -> str{start|pause|stop|invalid}",
		.func = cmd_status_get
	},
	{
		.name = "stdin",
		.desc = "Write stdin to vm",
		.args = "vmid:u32 thread_id:u8 msg:str -> bool",
		.func = cmd_stdio
	},
	{
		.name = "mount",
		.desc = "Mount file system",
		.args = "-t fs_type:str{bfs|ext2|fat} device:str path:str -> bool",
		.func = cmd_mount
	},
};

static void icc_started(ICC_Message* msg) {
	Core* core = &cores[msg->apic_id];
	VM* vm = core->vm;

	if(msg->result == 0) {
		core->error_code = 0;

		core->stdin = (char*)((uint64_t)msg->data.started.stdin - PHYSICAL_OFFSET);
		core->stdin_head = (size_t*)((uint64_t)msg->data.started.stdin_head - PHYSICAL_OFFSET);
		core->stdin_tail = (size_t*)((uint64_t)msg->data.started.stdin_tail - PHYSICAL_OFFSET);
		core->stdin_size = msg->data.started.stdin_size;

		core->stdout = (char*)((uint64_t)msg->data.started.stdout - PHYSICAL_OFFSET);
		core->stdout_head = (size_t*)((uint64_t)msg->data.started.stdout_head - PHYSICAL_OFFSET);
		core->stdout_tail = (size_t*)((uint64_t)msg->data.started.stdout_tail - PHYSICAL_OFFSET);
		core->stdout_size = msg->data.started.stdout_size;

		core->stderr = (char*)((uint64_t)msg->data.started.stderr - PHYSICAL_OFFSET);
		core->stderr_head = (size_t*)((uint64_t)msg->data.started.stderr_head - PHYSICAL_OFFSET);
		core->stderr_tail = (size_t*)((uint64_t)msg->data.started.stderr_tail - PHYSICAL_OFFSET);
		core->stderr_size = msg->data.started.stderr_size;

		core->status = VM_STATUS_START;

		printf("Execution succeed on core[%d].\n", mp_apic_id_to_processor_id(msg->apic_id));

	} else {
		core->error_code = msg->result;

		core->status = VM_STATUS_PAUSE;

		printf("Execution FAILED on core[%d]: Error code 0x%x.\n", mp_apic_id_to_processor_id(msg->apic_id), msg->result);

	}

	icc_free(msg);

	int error_code = 0;
	for(int i = 0; i < vm->core_size; i++) {
		core = &cores[vm->cores[i]];

		if(core->status == VM_STATUS_START) {
			continue;
		}

		if(core->error_code != 0) {
			error_code = core->error_code;
			continue;
		}

		return;
	}

	vm->status = error_code == 0 ? VM_STATUS_START : VM_STATUS_STOP;

	event_trigger_fire(EVENT_VM_STARTED, vm, NULL, NULL);

	if(error_code != 0) {
		for(int i = 0; i < vm->core_size; i++) {
			if(cores[vm->cores[i]].status == VM_STATUS_START) {
				ICC_Message* msg2 = icc_alloc(ICC_TYPE_STOP);
				icc_send(msg2, vm->cores[i]);
			}
		}
	}

	if(error_code == 0) {
		printf("VM started on cores[");
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", mp_apic_id_to_processor_id(vm->cores[i]));
			if(i + 1 < vm->core_size) {
				printf(", ");
			}
		}
		printf("]\n");
	} else {
		printf("VM started with error(s) on cores[");
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d", mp_apic_id_to_processor_id(vm->cores[i]));
			error_code = cores[vm->cores[i]].error_code;
			if(error_code != 0)
				printf("(0x%x)", error_code);

			if(i + 1 < vm->core_size) {
				printf(", ");
			}
		}
		printf("]\n");
	}
}

static void icc_paused(ICC_Message* msg) {
	VM* vm = cores[msg->apic_id].vm;

	cores[msg->apic_id].status = VM_STATUS_PAUSE;

	printf("Execution paused on core[%d].\n", mp_apic_id_to_processor_id(msg->apic_id));

	icc_free(msg);
	for(int i = 0; i < vm->core_size; i++) {
		if(cores[vm->cores[i]].status != VM_STATUS_PAUSE)
			return;
	}

	vm->status = VM_STATUS_PAUSE;

	event_trigger_fire(EVENT_VM_PAUSED, vm, NULL, NULL);

	printf("VM paused on cores[");
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", mp_apic_id_to_processor_id(vm->cores[i]));
		if(i + 1 < vm->core_size) {
			printf(", ");
		}
	}
	printf("]\n");
}

static void icc_resumed(ICC_Message* msg) {
	if(msg->result == -1000) {	// VM is not strated yet
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUME);
		icc_send(msg2, msg->apic_id);
		icc_free(msg);
		return;
	}

	VM* vm = cores[msg->apic_id].vm;

	cores[msg->apic_id].status = VM_STATUS_START;

	printf("Execution resumed on core[%d].\n", mp_apic_id_to_processor_id(msg->apic_id));

	icc_free(msg);

	for(int i = 0; i < vm->core_size; i++) {
		if(cores[vm->cores[i]].status != VM_STATUS_START)
			return;
	}

	vm->status = VM_STATUS_START;

	event_trigger_fire(EVENT_VM_RESUMED, vm, NULL, NULL);

	printf("VM resumed on cores[");
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", mp_apic_id_to_processor_id(vm->cores[i]));
		if(i + 1 < vm->core_size) {
			printf(", ");
		}
	}
	printf("]\n");
}

static void icc_stopped(ICC_Message* msg) {
	if(msg->result == -1000) {	// VM is not strated yet
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_STOP);
		icc_send(msg2, msg->apic_id);
		icc_free(msg);
		// Resend stop icc
		return;
	}

	VM* vm = cores[msg->apic_id].vm;

	cores[msg->apic_id].status = VM_STATUS_PAUSE;
	cores[msg->apic_id].error_code = msg->result;
	cores[msg->apic_id].return_code = msg->data.stopped.return_code;
	cores[msg->apic_id].stdin = NULL;
	cores[msg->apic_id].stdout = NULL;
	cores[msg->apic_id].stderr = NULL;

	printf("Execution completed on core[%d].\n", mp_apic_id_to_processor_id(msg->apic_id));

	icc_free(msg);

	for(int i = 0; i < vm->core_size; i++) {
		if(cores[vm->cores[i]].status != VM_STATUS_PAUSE) {
			return;
		}
	}

	vm->status = VM_STATUS_STOP;

	event_trigger_fire(EVENT_VM_STOPPED, vm, NULL, NULL);

	printf("VM stopped on cores[");
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d(%d/%d)", mp_apic_id_to_processor_id(vm->cores[i]), cores[vm->cores[i]].error_code, cores[vm->cores[i]].return_code);
		if(i + 1 < vm->core_size) {
			printf(", ");
		}
	}
	printf("]\n");
}

static bool vm_delete(VM* vm, int core) {
	bool is_destroy = true;

	if(core == -1) {
		for(int i = 0; i < vm->core_size; i++) {
			if(vm->cores[i]) {
				cores[vm->cores[i]].status = VM_STATUS_STOP;
			}
		}
	} else {
		for(int i = 0; i < vm->core_size; i++) {
			if(vm->cores[i] == core) {
				cores[vm->cores[i]].status = VM_STATUS_STOP;
			}

			if(!cores[vm->cores[i]].status == VM_STATUS_STOP)
				is_destroy = false;
		}
	}

	if(is_destroy) {
		if(vm->memory.blocks) {
			for(uint32_t i = 0; i < vm->memory.count; i++) {
				if(vm->memory.blocks[i]) {
					bfree(vm->memory.blocks[i]);
				}
			}

			gfree(vm->memory.blocks);
		}

		if(vm->storage.blocks) {
			for(uint32_t i = 0; i < vm->storage.count; i++) {
				if(vm->storage.blocks[i]) {
					bfree(vm->storage.blocks[i]);
				}
			}

			gfree(vm->storage.blocks);
		}

		if(vm->nics) {
			for(uint16_t i = 0; i < vm->nic_count; i++) {
				if(vm->nics[i]) {
					bfree(vm->nics[i]->nic);
					vnic_free_id(vm->nics[i]->id);
					gfree(vm->nics[i]);
				}
			}

			gfree(vm->nics);
		}

		if(vm->argv) {
			gfree(vm->argv);
		}

		gfree(vm);
	}

	return is_destroy;
}

static void stdio_dump(int coreno, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size) {
	if(*head == *tail)
		return;

	char header[12] = "[ Core 01 ]";

	printf("\n%s ", header);
	int length = *tail - *head;
	for(int i = 0; i < length; i++) {
		if(buffer[i] == '\0') {
			if(i != length - 1)
				printf("%s ", header);
		}

		putchar(buffer[i]);
	}

	*head = *tail;

/*
 *        int header_len = strlen(header);
 *        int body_len = 80 - header_len;
 *
 *        char* strchrn(const char* s, const char* e, int c) {
 *                char* ch = (char*)s;
 *
 *                while(*ch != c && *ch != '\0' && ch < e)
 *                        ch++;
 *
 *                return ch < e && *ch == c ? ch: NULL;
 *        }
 *
 *        char* dump_lines(char* h, char* e) {
 *                char* t = strchrn(h, e, '\n');
 *                while(t) {
 *                        // Skip newline and null charater
 *                        t += 2;
 *                        while(t - h > body_len) {
 *                                write1(header, header_len);
 *                                write1(h, body_len);
 *
 *                                h += body_len;
 *                        }
 *
 *                        write1(header, header_len);
 *                        write1(h, t - h);
 *
 *                        if(t >= e)
 *                                return NULL;
 *
 *                        h = t;
 *                        t = strchrn(h, e, '\n');
 *                }
 *
 *                while(e - h > body_len) {
 *                        write1(header, header_len);
 *                        write1(h, body_len);
 *
 *                        h += body_len;
 *                }
 *
 *                return h < e ? h : NULL;
 *        }
 *
 *        char* h = buffer + *head;
 *        char* e = buffer + *tail;
 *        if(*head > *tail) {
 *                h = dump_lines(h, buffer + size);
 *                if(h) {
 *                        int len1 = buffer + size - h;
 *                        write1(header, header_len);
 *                        write1(h, len1);
 *
 *                        size_t len2 = body_len - len1;
 *                        if(len2 > *tail)
 *                                len2 = *tail;
 *
 *                        char* t = strchrn(h, buffer + len2, '\n');
 *                        if(t) {
 *                                t++;
 *                                write1(h, t - h);
 *                                h = t;
 *                        } else {
 *                                write1(buffer, len2);
 *                                write1("\n", 1);
 *                                h = buffer + len2;
 *                        }
 *                } else {
 *                        h = buffer;
 *                }
 *        }
 *
 *        h = dump_lines(h, e);
 *        if(h) {
 *                //write1(header, header_len);
 *                write1(h, e - h);
 *                //write1("\n", 1);
 *        }
 *
 *        *head = *tail;
 */
}

static bool vm_loop(void* context) {
	// Standard I/O/E processing
	int get_thread_id(VM* vm, int core) {
		for(int i = 0; i < vm->core_size; i++) {
			if(vm->cores[i] == core)
				return i;
		}

		return -1;
	}

	for(int i = 1; i < MP_MAX_CORE_COUNT; i++) {
		Core* core = &cores[i];

		if(core->status != VM_STATUS_PAUSE && core->status != VM_STATUS_START) {
			continue;
		}
		int thread_id = -1;

		if(core->stdout != NULL && *core->stdout_head != *core->stdout_tail) {
			thread_id = get_thread_id(core->vm, i);

			stdio_callback(core->vm->id, thread_id, 1, core->stdout, core->stdout_head, core->stdout_tail, core->stdout_size);
		}

		if(core->stderr != NULL && *core->stderr_head != *core->stderr_tail) {
			if(thread_id == -1)
				thread_id = get_thread_id(core->vm, i);

			stdio_callback(core->vm->id, thread_id, 2, core->stderr, core->stderr_head, core->stderr_tail, core->stderr_size);
		}
	}

	void stdio_dump(int coreno, int fd, char* buffer, volatile size_t* head, volatile size_t* tail, size_t size);

	for(int i = 1; i < MP_MAX_CORE_COUNT; i++) {
		if(cores[i].status == VM_STATUS_INVALID)
			continue;

		char* buffer = (char*)MP_CORE(*__stdout, i);
		volatile size_t* head = (size_t*)MP_CORE(__stdout_head, i);
		volatile size_t* tail = (size_t*)MP_CORE(__stdout_tail, i);
		size_t size = *(size_t*)MP_CORE(__stdout_size, i);

		while(*head != *tail) {
			stdio_dump(mp_apic_id_to_processor_id(i), 1, buffer, head, tail, size);
		}

/*
 *                buffer = (char*)MP_CORE(__stderr, i);
 *                head = (size_t*)MP_CORE(&__stderr_head, i);
 *                tail = (size_t*)MP_CORE(&__stderr_tail, i);
 *                size = *(size_t*)MP_CORE(&__stderr_size, i);
 *
 *                while(*head != *tail) {
 *                        //stdio_dump(mp_apic_id_to_processor_id(i), 2, buffer, head, tail, size);
 *                }
 */
	}

	return true;
}

void vm_init() {
	vms = map_create(4, map_uint64_hash, map_uint64_equals, NULL);

	icc_register(ICC_TYPE_STARTED, icc_started);
	icc_register(ICC_TYPE_PAUSED, icc_paused);
	icc_register(ICC_TYPE_RESUMED, icc_resumed);
	icc_register(ICC_TYPE_STOPPED, icc_stopped);

	// Core 0 is occupied by RPC manager
	cores[0].status = VM_STATUS_START;

	uint8_t* core_map = mp_processor_map();
	for(int i = 1; i < MP_MAX_CORE_COUNT; i++) {
		if(core_map[i] == MP_CORE_INVALID)
			cores[i].status = VM_STATUS_INVALID;	// Disable the core
	}

	event_idle_add(vm_loop, NULL);

	cmd_register(commands, sizeof(commands) / sizeof(commands[0]));
}

uint32_t vm_create(VMSpec* vm_spec) {
	VM* vm = gmalloc(sizeof(VM));
	if(!vm)
		return 0;
	memset(vm, 0, sizeof(VM));

	// Allocate args
	int argv_len = sizeof(char*) * vm_spec->argc;
	for(int i = 0; i < vm_spec->argc; i++) {
		argv_len += strlen(vm_spec->argv[i]) + 1;
	}
	vm->argv = gmalloc(argv_len);
	if(!vm->argv) {
		gfree(vm);
		return 0;
	}

	vm->argc = vm_spec->argc;
	if(vm->argc) {
		char* args = (void*)vm->argv + sizeof(char*) * vm_spec->argc;
		for(int i = 0; i < vm_spec->argc; i++) {
			vm->argv[i] = args;
			int len = strlen(vm_spec->argv[i]) + 1;
			memcpy(args, vm_spec->argv[i], len);
			args += len;
		}
	}

	// Allocate core
	vm->core_size = vm_spec->core_size;

	int j = 0;
	for(int i = 1; i < MP_MAX_CORE_COUNT; i++) {
		if(cores[i].status == VM_STATUS_STOP) {
			vm->cores[j++] = i;
			cores[i].status = VM_STATUS_PAUSE;
			cores[i].vm = vm;

			if(j >= vm->core_size)
				break;
		}
	}

	if(j < vm->core_size) {
		printf("Manager: Not enough core to allocate.\n");
		goto fail;
	}

	// Allocate memory
	uint32_t memory_size = vm_spec->memory_size;
	memory_size = (memory_size + (VM_MEMORY_SIZE_ALIGN - 1)) & ~(VM_MEMORY_SIZE_ALIGN - 1);
	if(memory_size > VM_MAX_MEMORY_SIZE) {
		printf("Manager: VM Memory is too large\n");
		goto fail;
	}

	vm->memory.count = memory_size / VM_MEMORY_SIZE_ALIGN;
	vm->memory.blocks = gmalloc(vm->memory.count * sizeof(void*));
	memset(vm->memory.blocks, 0x0, vm->memory.count * sizeof(void*));
	for(uint32_t i = 0; i < vm->memory.count; i++) {
		vm->memory.blocks[i] = bmalloc(1);
		if(!vm->memory.blocks[i]) {
			printf("Manager: Not enough memory to allocate.\n");
			goto fail;
		}
	}

	// Allocate storage
	uint32_t storage_size = vm_spec->storage_size;
	storage_size = (storage_size + (VM_STORAGE_SIZE_ALIGN - 1)) & ~(VM_STORAGE_SIZE_ALIGN - 1);
	if(storage_size > VM_MAX_STORAGE_SIZE) {
		printf("Manager: VM STORAGE is too large\n");
		goto fail;
	}

	vm->storage.count = storage_size / VM_STORAGE_SIZE_ALIGN;
	vm->storage.blocks = gmalloc(vm->storage.count * sizeof(void*));
	memset(vm->storage.blocks, 0x0, vm->storage.count * sizeof(void*));
	for(uint32_t i = 0; i < vm->storage.count; i++) {
		vm->storage.blocks[i] = bmalloc(1);
		if(!vm->storage.blocks[i]) {
			printf("Manager: Not enough storage to allocate.\n");
			goto fail;
		}
	}

	// Allocate VNICs
	NICSpec* nics = vm_spec->nics;
	vm->nic_count = vm_spec->nic_count;
	if(vm->nic_count > VM_MAX_NIC_COUNT) {
		printf("Manager: VNIC is too many\n");
		goto fail;
	}

	// VNIC Create
	if(vm->nic_count) {
		vm->nics = gmalloc(sizeof(VNIC) * vm->nic_count);
		if(!vm->nics) {
			printf("Manager: Can't allocate memory\n");
			goto fail;
		}

		memset(vm->nics, 0, sizeof(VNIC) * vm->nic_count);
		for(int i = 0; i < vm->nic_count; i++) {
			NICDevice* nic_dev = nicdev_get(nics[i].dev ? : NIC_DEFAULT_NICDEV);
			if(!nic_dev) {
				printf("Manager: Invalid NIC device: %s.\n", nics[i].dev);
				goto fail;
			}

			uint64_t mac = nics[i].mac;
			if(mac == 0) {
				do {
					mac = timer_frequency() & 0x0fffffffffffL;
					mac |= 0x02L << 40;	// Locally administrered
				} while(nicdev_get_vnic_mac(nic_dev, mac) != NULL);
			} else if(nicdev_get_vnic_mac(nic_dev, mac) != NULL) {
				printf("Manager: The mac address already in use: %012lx.\n", mac);
				goto fail;
			}

			uint64_t attrs[] = {
				VNIC_MAC, mac,
				VNIC_DEV, (uint64_t)nic_dev->name,
				VNIC_BUDGET, nics[i].budget ? : NIC_DEFAULT_BUDGET_SIZE,
				VNIC_POOL_SIZE, nics[i].pool_size ? : NIC_DEFAULT_POOL_SIZE,
				VNIC_RX_BANDWIDTH, nics[i].input_bandwidth ? : NIC_DEFAULT_BANDWIDTH,
				VNIC_TX_BANDWIDTH, nics[i].output_bandwidth ? : NIC_DEFAULT_BANDWIDTH,
				VNIC_PADDING_HEAD, nics[i].padding_head ? : NIC_DEFAULT_PADDING_SIZE,
				VNIC_PADDING_TAIL, nics[i].padding_tail ? : NIC_DEFAULT_PADDING_SIZE,
				VNIC_RX_QUEUE_SIZE, nics[i].input_buffer_size ? : NIC_DEFAULT_BUFFER_SIZE,
				VNIC_TX_QUEUE_SIZE, nics[i].output_buffer_size ? : NIC_DEFAULT_BUFFER_SIZE,
				VNIC_SLOW_RX_QUEUE_SIZE, nics[i].slow_input_buffer_size ? : NIC_DEFAULT_BUFFER_SIZE,
				VNIC_SLOW_TX_QUEUE_SIZE, nics[i].slow_output_buffer_size ? : NIC_DEFAULT_BUFFER_SIZE,
				VNIC_NONE
			};

			VNIC* vnic = gmalloc(sizeof(VNIC));
			if(!vnic) {
				printf("Manager: Failed to allocate VNIC\n");
				goto fail;
			}

			vnic->id = vnic_alloc_id();
			vnic->nic_size = nics[i].pool_size ? : NIC_DEFAULT_POOL_SIZE;
			vnic->nic = bmalloc(((nics[i].pool_size ? : NIC_DEFAULT_POOL_SIZE) + 0x200000 - 1) / 0x200000);
			if(!vnic->nic) {
				printf("Manager: Failed to allocate NIC in VNIC\n");
				goto fail;
			}

			if(!vnic_init(vnic, attrs)) {
				printf("Manager: Not enough VNIC to allocate: errno=%d.\n", errno);
				goto fail;
			}

			extern int dispatcher_create_vnic(void* vnic);
			#ifdef PACKETNGIN_SINGLE
			int dispatcher_create_vnic(void* vnic) { return 0; }
			#endif
			if(dispatcher_create_vnic(vnic) < 0) {
				printf("Manager: Failed to create VNIC in dispatcher module: errno=%d.\n", errno);
				goto fail;
			}

			vm->nics[i] = vnic;

			nicdev_register_vnic(nic_dev, vnic);
		}
	}

	// Allocate vmid
	// TODO fix
	uint32_t vmid;
	while(true) {
		vmid = last_vmid++;

		if(vmid != 0 && !map_contains(vms, (void*)(uint64_t)vmid)) {
			vm->id = vmid;
			if(!map_put(vms, (void*)(uint64_t)vmid, vm)) {
				goto fail;
			}
			break;
		}
	}

	return vmid;

fail:
	vm_delete(vm, -1);
	return 0;
}

bool vm_destroy(uint32_t vmid) {
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm) {
		printf("Manager: Vm not found\n");
		return false;
	}
	for(int i = 0; i < vm->core_size; i++) {
		if(cores[vm->cores[i]].status == VM_STATUS_START) {
			return false;
		}
	}

	map_remove(vms, (void*)(uint64_t)vmid);

	printf("Manager: Delete vm[%d] on cores [", vmid);
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", mp_apic_id_to_processor_id(vm->cores[i]));
		if(i + 1 < vm->core_size)
			printf(", ");
	}
	printf("]\n");

	vm_delete(vm, -1);

	return true;
}

bool vm_contains(uint32_t vmid) {
	return map_contains(vms, (void*)(uint64_t)vmid);
}

int vm_count() {
	return map_size(vms);
}

int vm_list(uint32_t* vmids, int size) {
	int i = 0;

	MapIterator iter;
	map_iterator_init(&iter, vms);
	while(i < size && map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		vmids[i++] = (uint32_t)(uint64_t)entry->key;
	}

	return i;
}

typedef struct {
	VM_STATUS_CALLBACK	callback;
	void*			context;
	int			status;
} CallbackInfo;

static bool status_changed(uint64_t event_type, void* event, void* context) {
	VM* vm = event;
	CallbackInfo* info = context;

	bool result = vm->status == info->status || (vm->status == VM_STATUS_START && info->status == VM_STATUS_RESUME);
	info->callback(result, info->context);

	char* op = "";
	switch(info->status) {
		case VM_STATUS_START: op = "start"; break;
		case VM_STATUS_PAUSE: op = "pause"; break;
		case VM_STATUS_RESUME: op = "resume"; break;
		case VM_STATUS_STOP: op = "stop"; break;
	}

	printf("Manager: VM[%d] %s %s on cores [", vm->id, op, result ? "succeed" : "failed");
	for(int i = 0; i < vm->core_size; i++) {
		printf("%d", mp_apic_id_to_processor_id(vm->cores[i]));
		if(i + 1 < vm->core_size)
			printf(", ");
	}
	printf("]\n");

	free(info);

	return false;
}

void vm_status_set(uint32_t vmid, int status, VM_STATUS_CALLBACK callback, void* context) {
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm) {
		callback(false, context);
		return;
	}

	int icc_type = 0;
	uint64_t event_type = 0;
	switch(status) {
		case VM_STATUS_START:
			printf("VM Status start\n");
			if(vm->status != VM_STATUS_STOP) {
				callback(false, context);
				return;
			}
			event_type = EVENT_VM_STARTED;
			icc_type = ICC_TYPE_START;
			break;
		case VM_STATUS_PAUSE:
			if(vm->status != VM_STATUS_START) {
				callback(false, context);
				return;
			}
			event_type = EVENT_VM_PAUSED;
			icc_type = ICC_TYPE_PAUSE;
			break;
		case VM_STATUS_RESUME:
			if(vm->status != VM_STATUS_PAUSE) {
				callback(false, context);
				return;
			}
			event_type = EVENT_VM_RESUMED;
			icc_type = ICC_TYPE_RESUME;
			break;
		case VM_STATUS_STOP:
			printf("VM Status stop\n");
			if(vm->status != VM_STATUS_PAUSE && vm->status != VM_STATUS_START) {
				callback(false, context);
				return;
			}
			event_type = EVENT_VM_STOPPED;
			icc_type = ICC_TYPE_STOP;
			break;
	}

	// Lazy clean up
	if(status == VM_STATUS_START) {
		for(size_t i = 0; i < vm->memory.count; i++) {
			memset(vm->memory.blocks[i], 0x0, 0x200000);
		}
	}

	CallbackInfo* info = malloc(sizeof(CallbackInfo));
	if(!info)
		return;
	info->callback = callback;
	info->context = context;
	info->status = status;

	event_trigger_add(event_type, status_changed, info);

	for(int i = 0; i < vm->core_size; i++) {
		if(status == VM_STATUS_PAUSE) {
			apic_write64(APIC_REG_ICR, ((uint64_t)vm->cores[i] << 56) |
						APIC_DSH_NONE |
						APIC_TM_EDGE |
						APIC_LV_DEASSERT |
						APIC_DM_PHYSICAL |
						APIC_DMODE_FIXED |
						49);
		} else {
			cores[vm->cores[i]].error_code = 0;
			ICC_Message* msg = icc_alloc(icc_type);
			if(status == VM_STATUS_START) {
				msg->data.start.vm = vm;
			}
			icc_send(msg, vm->cores[i]);
		}
	}
}

VMStatus vm_status_get(uint32_t vmid) {
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm)
		return -1;

	return vm->status;
}

VM* vm_get(uint32_t vmid) {
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm)
		return NULL;

	return vm;
}

ssize_t vm_storage_read(uint32_t vmid, void** buf, size_t offset, size_t size) {
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm)
		return -1;

	if(offset > vm->storage.count * VM_STORAGE_SIZE_ALIGN) {
		*buf = NULL;
		return 0;
	}

	int index = offset / VM_STORAGE_SIZE_ALIGN;
	offset %= VM_STORAGE_SIZE_ALIGN;
	*buf = vm->storage.blocks[index] + offset;

	if(offset + size > VM_STORAGE_SIZE_ALIGN)
		return VM_STORAGE_SIZE_ALIGN - offset;
	else
		return size;
}

ssize_t vm_storage_write(uint32_t vmid, void* buf, size_t offset, size_t size) {
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm)
		return -1;

	if(!size)
		return 0;

	if((uint64_t)offset + size > (uint64_t)vm->storage.count * VM_STORAGE_SIZE_ALIGN)
		return -1;

	uint32_t index = offset / VM_STORAGE_SIZE_ALIGN;
	if(index >= vm->storage.count)
		return -1;

	size_t _size = size;
	offset %= VM_STORAGE_SIZE_ALIGN;
	for(; index < vm->storage.count; index++) {
		if(offset + _size > VM_STORAGE_SIZE_ALIGN) {
			size_t write_size = VM_STORAGE_SIZE_ALIGN - offset;
			memcpy(vm->storage.blocks[index] + offset, buf, write_size);
			_size -= write_size;
			buf += write_size;
		} else {
			memcpy(vm->storage.blocks[index] + offset, buf, _size);
			_size = 0;
		}

		if(_size == 0) {
			vm->used_size = size;
			break;
		}
		offset = 0;
	}


	if(_size != 0)
		return -1;

	return size;
}

ssize_t vm_storage_clear(uint32_t vmid) {
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm)
		return -1;

	ssize_t size = 0;
	for(uint32_t i = 0; i < vm->storage.count; i++) {
		memset(vm->storage.blocks[i], 0x0, VM_STORAGE_SIZE_ALIGN);
		size += VM_STORAGE_SIZE_ALIGN;
	}

	return size;
}

bool vm_storage_md5(uint32_t vmid, uint32_t size, uint32_t digest[4]) {
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm)
		return false;

	uint32_t block_count = size / VM_MEMORY_SIZE_ALIGN;
	if(vm->storage.count < block_count)
		return false;

	md5_blocks(vm->storage.blocks, vm->storage.count, VM_STORAGE_SIZE_ALIGN, size, digest);

	return true;
}

ssize_t vm_stdio(uint32_t vmid, int thread_id, int fd, const char* str, size_t size) {
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm)
		return -1;

	if(thread_id < 0 || thread_id >= vm->core_size)
		return -1;

	Core* core = &cores[vm->cores[thread_id]];
	if(core->status != VM_STATUS_PAUSE && core->status != VM_STATUS_START)
		return -1;

	switch(fd) {
		case 0:
			if(core->stdin)
				return ring_write(core->stdin, *core->stdin_head, core->stdin_tail, core->stdin_size, str, size);
			else
				return -1;
		case 1:
			if(core->stdout)
				return ring_write(core->stdout, *core->stdout_head, core->stdout_tail, core->stdout_size, str, size);
			else
				return -1;
		case 2:
			if(core->stderr)
				return ring_write(core->stderr, *core->stderr_head, core->stderr_tail, core->stderr_size, str, size);
			return -1;
		default:
			return -1;
	}
}

void vm_stdio_handler(VM_STDIO_CALLBACK callback) {
	stdio_callback = callback;
}


// makerj
static int cmd_md5(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2)
		return CMD_STATUS_WRONG_NUMBER;
	if(!is_uint32(argv[1]))
		return -1;
	if(argc == 3 && !is_uint64(argv[2]))
		return -2;

	uint32_t vmid = parse_uint32(argv[1]);
	uint64_t size = argc == 3 ? parse_uint64(argv[2]) : vm_get(vmid)->used_size;
	uint32_t md5sum[4];

	bool ret = vm_storage_md5(vmid, size, md5sum);
	if(!ret) {
		sprintf(cmd_result, "(nil)");
		printf("Cannot md5 checksum\n");
	} else {
		char* p = (char*)cmd_result;
		for(int i = 0; i < 16; i++, p += 2) {
			sprintf(p, "%02x", ((uint8_t*)md5sum)[i]);
		}
		*p = '\0';
	}
	printf("%s\n", cmd_result);

	if(ret)
		callback(cmd_result, 0);

	return 0;
}

char* strtok(char* argv, const char* delim) {
	char* ptr = strstr(argv, delim);
	if(!ptr)
		return NULL;
	memset(ptr, 0, strlen(delim));

	return argv;
}

char* strtok_r(char* argv, const char* delim, char** next) {
	char* res = strtok(argv, delim);
	if(!res) {
		*next = NULL;
		return argv;
	}

	*next = res + strlen(res) + strlen(delim);
	return res;
}

static int cmd_create(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	// Default value
	VMSpec vm;
	memset(&vm, 0, sizeof(VMSpec));
	vm.core_size = 1;
	vm.memory_size = 0x1000000;		/* 16MB */
	vm.storage_size = 0x1000000;		/* 16MB */

	NICSpec nics[VM_MAX_NIC_COUNT];
	memset(nics, 0, sizeof(NICSpec) * VM_MAX_NIC_COUNT);
	vm.nics = nics;

	vm.argc = 0;
	vm.argv = malloc(sizeof(char*) * CMD_MAX_ARGC);
	memset(vm.argv, 0, sizeof(char*) * CMD_MAX_ARGC);

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-c") == 0) {
			NEXT_ARGUMENTS();

			if(!is_uint8(argv[i])) {
				printf("Core must be uint8\n");
				return -1;
			}

			vm.core_size = parse_uint8(argv[i]);
		} else if(strcmp(argv[i], "-m") == 0) {
			NEXT_ARGUMENTS();

			if(!is_uint32(argv[i])) {
				printf("Memory must be uint32\n");
				return -1;
			}

			vm.memory_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "-s") == 0) {
			NEXT_ARGUMENTS();

			if(!is_uint32(argv[i])) {
				printf("Storage must be uint32\n");
				return -1;
			}

			vm.storage_size = parse_uint32(argv[i]);
		} else if(strcmp(argv[i], "-n") == 0) {
			NICSpec* nic = &(vm.nics[vm.nic_count++]);

			NEXT_ARGUMENTS();
			char* next;
			char* token = strtok_r(argv[i], ",", &next);
			while(token) {
				char* value;
				token = strtok_r(token, "=", &value);
				if(strcmp(token, "mac") == 0) {
					if(!is_uint64(value)) {
						printf("Mac must be uint64\n");
						return -1;
					}
					nic->mac = parse_uint64(token);
				} else if(strcmp(token, "dev") == 0) {
					nic->dev = malloc(strlen(token + 1));
					strcpy(nic->dev, value);
				} else if(strcmp(token, "ibuf") == 0) {
					if(!is_uint32(value)) {
						printf("Ibuf must be uint32\n");
						return -1;
					}
					nic->input_buffer_size = parse_uint32(value);
				} else if(strcmp(token, "obuf") == 0) {
					if(!is_uint32(value)) {
						printf("Obuf must be uint32\n");
						return -1;
					}
					nic->output_buffer_size = parse_uint32(value);
				} else if(strcmp(token, "iband") == 0) {
					if(!is_uint64(value)) {
						printf("Iband must be uint64\n");
						return -1;
					}
					nic->input_bandwidth = parse_uint64(value);
				} else if(strcmp(token, "oband") == 0) {
					if(!is_uint64(value)) {
						printf("Oband must be uint64\n");
						return -1;
					}
					nic->output_bandwidth = parse_uint64(value);
				} else if(strcmp(token, "hpad") == 0) {
					if(!is_uint16(value)) {
						printf("Iband must be uint16\n");
						return -1;
					}
					nic->padding_head = parse_uint16(value);
				} else if(strcmp(token, "tpad") == 0) {
					if(!is_uint16(value)) {
						printf("Oband must be uint16\n");
						return -1;
					}
					nic->padding_tail = parse_uint16(value);
				} else if(strcmp(token, "pool") == 0) {
					if(!is_uint32(value)) {
						printf("Pool must be uint32\n");
						return -1;
					}
					nic->pool_size = parse_uint32(value);
				} else {
					i--;
					break;
				}

				token = strtok_r(next, ",", &next);
			}
		} else if(strcmp(argv[i], "-a") == 0) {
			NEXT_ARGUMENTS();

			for( ; i < argc; i++) {
				vm.argv[vm.argc++] = argv[i];
			}
		}
	}

	uint32_t vmid = vm_create(&vm);
	if(vmid == 0) {
		callback(NULL, -1);
	} else {
		VM* vm = vm_get(vmid);

		printf("Manager: VM[%d] created(cores [\n", vm->id);
		for(int i = 0; i < vm->core_size; i++) {
			printf("%d, ", mp_apic_id_to_processor_id(vm->cores[i]));
			if(i + 1 < vm->core_size)
				printf(", ");
		}
		printf("], %dMBs memory, %dMBs storage, VNICs: %d\n",
				vm->memory.count * VM_MEMORY_SIZE_ALIGN / 0x100000,
				vm->storage.count * VM_STORAGE_SIZE_ALIGN / 0x100000, vm->nic_count);

		//Ok
		for(int i = 0; i < vm->nic_count; i++) {
			VNIC* vnic = vm->nics[i];
			printf("\t");
			for(int j = 5; j >= 0; j--) {
				printf("%02lx", (vnic->mac >> (j * 8)) & 0xff);
				if(j - 1 >= 0)
					printf(":");
				else
					printf(" ");
			}
			printf("%ldMbps/%ld, %ldMbps/%ld, %ldMBs\n", vnic->rx_bandwidth / 1000000, vnic->rx.size, vnic->tx_bandwidth / 1000000, vnic->tx.size, vnic->nic_size / (1024 * 1024));
		}

		printf("\targs(%d): ", vm->argc);
		for(int i = 0; i < vm->argc; i++) {
			char* ch = strchr(vm->argv[i], ' ');
			if(ch == NULL)
				printf("%s ", vm->argv[i]);
			else
				printf("\"%s\" ", vm->argv[i]);
		}
		printf("\n");

		sprintf(cmd_result, "%d", vmid);
		printf("%s\n", cmd_result);
		callback(cmd_result, 0);
	}

	for(int i = 0; i < vm.nic_count; i++) {
		if(vm.nics[i].dev)
			free(vm.nics[i].dev);
	}

	free(vm.argv);
	return 0;
}

static int cmd_vm_destroy(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 1) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!is_uint32(argv[1])) {
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	bool ret = vm_destroy(vmid);

	if(ret) {
		printf("Vm destroy success\n");
		callback("true", 0);
	} else {
		printf("Vm destroy success\n");
		callback("false", -1);
	}
	return 0;
}

static int cmd_vm_list(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	uint32_t vmids[VM_MAX_VM_COUNT];
	int len = vm_list(vmids, VM_MAX_VM_COUNT);

	char* p = cmd_result;

	if(len <= 0) {
		printf("VM not found\n");
		callback("false", 0);
		return 0;
	}

	for(int i = 0; i < len; i++) {
		p += sprintf(p, "%lu", vmids[i]) - 1;
		if(i + 1 < len) {
			*p++ = ' ';
		} else {
			*p++ = '\0';
		}
	}

	printf("%s\n", cmd_result);
	callback(cmd_result, 0);
	return 0;
}

static int cmd_upload(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 3) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!is_uint32(argv[1])) {
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);

	// Attach root directory for full path
	char file_name[FILE_MAX_NAME_LEN];
	file_name[0] = '/';
	strcpy(&file_name[1], argv[2]);

	int fd = open(file_name, "r");
	if(fd < 0) {
		printf("Cannot open file: %s\n", file_name);
		return 1;
	}

	int offset = 0;
	int len;
	char buf[4096];
	while((len = read(fd, buf, 4096)) > 0) {
		if(vm_storage_write(vmid, buf, offset, len) != len) {
			printf("Upload fail : %s\n", file_name);
			callback("false", -1);
			return -1;
		}

		offset += len;
	}

	printf("Upload success : %s\n", vmid);
	callback("true", 0);
	return 0;
}

static void status_setted(bool result, void* context) {
	void (*callback)(char* result, int exit_status) = context;
	callback(result ? "true" : "false", 0);
}

static int cmd_status_set(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}
	if(!is_uint32(argv[1])) {
		callback("invalid", -1);
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	int status = 0;
	if(strcmp(argv[0], "start") == 0) {
		status = VM_STATUS_START;
	} else if(strcmp(argv[0], "pause") == 0) {
		status = VM_STATUS_PAUSE;
	} else if(strcmp(argv[0], "resume") == 0) {
		status = VM_STATUS_RESUME;
	} else if(strcmp(argv[0], "stop") == 0) {
		status = VM_STATUS_STOP;
	} else {
		printf("%s\n: command not found\n", argv[0]);
		callback("invalid", -1);
		return -1;
	}

	shell_sync();

	vm_status_set(vmid, status, status_setted, callback);

	return 0;
}

static int cmd_status_get(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 2) {
		return CMD_STATUS_WRONG_NUMBER;
	}

	if(!is_uint32(argv[1])) {
		return -1;
	}

	uint32_t vmid = parse_uint32(argv[1]);
	extern Map* vms;
	VM* vm = map_get(vms, (void*)(uint64_t)vmid);
	if(!vm) {
		printf("VM not found\n");
		return -1;
	}

	void print_vm_status(int status) {
		switch(status) {
			case VM_STATUS_START:
				printf("start");
				callback("start", 0);
				break;
			case VM_STATUS_PAUSE:
				printf("pause");
				callback("pause", 0);
				break;
			case VM_STATUS_STOP:
				printf("stop");
				callback("stop", 0);
				break;
			default:
				printf("invalid");
				callback("invalid", -1);
				break;
		}
	}

	printf("VM ID: %d\n", vmid);
	printf("Status: ");
	print_vm_status(vm->status);
	printf("\n");
	printf("Core size: %d\n", vm->core_size);
	printf("Core: ");
	for(int i = 0; i < vm->core_size; i++) {
		printf("[%d] ", vm->cores[i]);
	}
	printf("\n");

	return 0;
}


static int cmd_stdio(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 3) {
		printf("Argument is not enough\n");
		return -1;
	}
	if(!is_uint32(argv[1])) {
		printf("VM ID is wrong\n");
		return -1;
	}
	if(!is_uint8(argv[2])) {
		printf("Thread ID is wrong\n");
		return -2;
	}

	uint32_t vmid =  parse_uint32(argv[1]);
	uint8_t thread_id = parse_uint8(argv[2]);
	for(int i = 3; i < argc; i++) {
		printf("%s\n", argv[i]);
		ssize_t len = vm_stdio(vmid, thread_id, 0, argv[i], strlen(argv[i]) + 1);
		if(!len) {
			printf("stdio fail\n");
			return -i;
		}
	}

	return 0;
}

static int cmd_mount(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc < 5) {
		printf("Argument is not enough\n");
		return -1;
	}

	uint8_t type;
	uint8_t disk;
	uint8_t number;
	uint8_t partition;

	if(strcmp(argv[1], "-t") != 0) {
		printf("Argument is wrong\n");
		return -2;
	}

	if(strcmp(argv[2], "bfs") == 0 || strcmp(argv[2], "BFS") == 0) {
		type = FS_TYPE_BFS;
	} else if(strcmp(argv[2], "ext2") == 0 || strcmp(argv[2], "EXT2") == 0) {
		type = FS_TYPE_EXT2;
	} else if(strcmp(argv[2], "fat") == 0 || strcmp(argv[2], "FAT") == 0) {
		type = FS_TYPE_FAT;
	} else {
		printf("%s type is not supported\n", argv[2]);
		return -3;
	}

	if(argv[3][0] == 'v') {
		disk = DISK_TYPE_VIRTIO_BLK;
	} else if(argv[3][0] == 's') {
		disk = DISK_TYPE_USB;
	} else if(argv[3][0] == 'h') {
		disk = DISK_TYPE_PATA;
	} else {
		printf("%c type is not supported\n", argv[3][0]);
		return -3;
	}

	number = argv[3][2] - 'a';
	if(number > 8) {
		printf("Disk number cannot exceed %d\n", DISK_AVAIL_DEVICES);
		return -4;
	}

	partition = argv[3][3] - '1';
	if(partition > 3) {
		printf("Partition number cannot exceed 4\n");
		return -5;
	}

	fs_mount(disk << 16 | number, partition, type, argv[4]);

	return -2;
}

