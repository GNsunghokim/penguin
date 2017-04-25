#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include <util/event.h>
#include <timer.h>

#include "apic.h"
#include "icc.h"
#include "mapping.h"
#include "shared.h"
#include "gmalloc.h"
#include "vm.h"
#include "manager.h"
#include "shell.h"
#include "malloc.h"
#include "vnic.h"
#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "smap.h"
#include "stdio.h"
#include "elf.h"
#include "popcorn.h"
#include "netlink.h"
#include "dispatcher.h"
#include "driver/nicdev.h"

static bool idle0_event() {
	//nic_poll();
	return true;
}

extern int cpu_start;
extern int cpu_end;
static int parse_present_mask(char* present_mask) {
	char* next = present_mask;
	char* end = NULL;
	char* start = strtok_r(next, "-", &end);

	if(start) {
		cpu_start = strtol(start, NULL, 10);
		cpu_end = cpu_start;
	} else
		return -1;

	if(*end) {
		cpu_end = strtol(end, NULL, 10);
		if(cpu_start > cpu_end)
			return -2;
	}

	return 0;
}

static char _boot_command_line[1024] = {0,};
static int parse_args(char* args) {
	printf("%s\n", args);
	int fd = open(args, O_RDONLY);
	if(fd < 0) {
		perror("Failed to open ");
		return -1;
	}

	ssize_t len = read(fd, _boot_command_line, 1023);
	if(!len)
		goto error;

	close(fd);

	char* next = strtok(_boot_command_line, "\n");
	char* str;
	bool smap_fixed = false;
	while((str = strtok_r(next, " ", &next))) {
		char* value = NULL;
		char* key = strtok_r(str, "=", &value);

		if(!strcmp(key, "present_mask")) {
			int ret = parse_present_mask(value);
			if(ret)
				goto error;
		} else if(!strcmp(key, "memmap")) {
			int ret = smap_update_memmap(value);
			if(ret)
				goto error;
			smap_fixed = true;
		} else if(!strcmp(key, "mem")){
			int ret = smap_update_mem(value);
			if(ret)
				goto error;
			smap_fixed = true;
		}
	}

	if(smap_fixed) {
		printf("\tSystem Memory Map Updated\n");
		smap_dump();
	}

	printf("\tCPU Usage: %d - %d\n", cpu_start, cpu_end);

	return 0;

error:
	printf("\tFailed to parse arguments\n");
	close(fd);
	return -2;
}

unsigned long kernel_start_address;	// Kernel Start address
unsigned long rd_start_address; 	// Ramdisk Start address
uint8_t	start_core;
char kernel_elf[256];
char kernel_args[256];

static int parse_params(char* params) {
	int fd = open(params, O_RDONLY);
	if(fd < 0)
		return -1;
	char buf[128] = {0,};
	ssize_t len = read(fd, buf, 127);
	if(!len)
		return -2;
	close(fd);

	char* next = buf;
	char* param = strtok_r(next, " ", &next);
	strcpy(kernel_elf, param);

	param = strtok_r(next, " ", &next);
	strcpy(kernel_args, param);

	// TODO: No need start core
	param = strtok_r(next, " ", &next);
	start_core = strtol(param, NULL, 10);

	// TODO: check alignment 2Mbyte
	param = strtok_r(next, " ", &next);
	kernel_start_address = strtol(param, NULL, 16);

	// TODO: NO Need ramdisk
	param = strtok_r(next, " ", &next);
	rd_start_address = strtol(param, NULL, 16);

	printf("\tPenguinNgin ELF File:\t%s\n", kernel_elf);
	printf("\tArgument File:\t\t%s\n", kernel_args);
	printf("\tStart Core:\t\t%d\n", start_core);
	printf("\tKernel Start address:\t%p\n", kernel_start_address);
	printf("\tRamDisk Start address:\t%p\n", rd_start_address);

	return 0;
}

static int symbols_init() {
	void* mmap_symbols[][2] = {
		{ &DESC_TABLE_AREA_START, "DESC_TABLE_AREA_START"},
		{ &DESC_TABLE_AREA_END, "DESC_TABLE_AREA_END"},

		{ &GDTR_ADDR, "GDTR_ADDR" },
		{ &GDT_ADDR, "GDT_ADDR" },
		{ &GDT_END_ADDR, "GDT_END_ADDR" },
		{ &TSS_ADDR, "TSS_ADDR" },

		{ &IDTR_ADDR, "IDTR_ADDR" },
		{ &IDT_ADDR, "IDT_ADDR" },
		{ &IDT_END_ADDR, "IDT_END_ADDR" },

		{ &KERNEL_TEXT_AREA_START, "KERNEL_TEXT_AREA_START" },
		{ &KERNEL_TEXT_AREA_END, "KERNEL_TEXT_AREA_END" },

		{ &KERNEL_DATA_AREA_START, "KERNEL_DATA_AREA_START" },
		{ &KERNEL_DATA_AREA_END, "KERNEL_DATA_AREA_END" },

		{ &KERNEL_DATA_START, "KERNEL_DATA_START" },
		{ &KERNEL_DATA_END, "KERNEL_DATA_END" },

		{ &VGA_BUFFER_START, "VGA_BUFFER_START" },
		{ &VGA_BUFFER_END, "VGA_BUFFER_END" },

		{ &USER_INTR_STACK_START, "USER_INTR_STACK_START" },
		{ &USER_INTR_STACK_END, "USER_INTR_STACK_END" },

		{ &KERNEL_INTR_STACK_START, "KERNEL_INTR_STACK_START" },
		{ &KERNEL_INTR_STACK_END, "KERNEL_INTR_STACK_END" },

		{ &KERNEL_STACK_START, "KERNEL_STACK_START" },
		{ &KERNEL_STACK_END, "KERNEL_STACK_END" },

		{ &PAGE_TABLE_START, "PAGE_TABLE_START" },
		{ &PAGE_TABLE_END, "PAGE_TABLE_END" },

		{ &LOCAL_MALLOC_START, "LOCAL_MALLOC_START" },
		{ &LOCAL_MALLOC_END, "LOCAL_MALLOC_END" },
	};

	int count = sizeof(mmap_symbols) / (sizeof(char*) * 2);
	for(int i = 0; i < count; i++) {
		uint64_t symbol_addr = elf_get_symbol(mmap_symbols[i][1]);
		if(!symbol_addr) {
			printf("Can't Get Symbol Address: \"%s\"", mmap_symbols[i][1]);
			return -1;
		}
		*(char**)mmap_symbols[i][0] = (char*)symbol_addr;
		//printf("\tSymbol \"%s\" : %p\n", mmap_symbols[i][1], symbol_addr);
	}

	return 0;
}

static int dummy_entry() {
	int fd = open_mem(O_RDWR);
	struct boot_params* _boot_params = map_boot_param(fd);
	if(!_boot_params) {
		printf("Can't Open Boot Parameter\n");
		return -1;
	}

	uint64_t symbol_addr = elf_get_symbol("boot_params");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"boot_params\"\n");
		return -2;
	}
	struct boot_params* boot_params = (struct boot_params*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	printf("\tboot_params: %p\n", boot_params);
	memcpy(boot_params, _boot_params, sizeof(struct boot_params));
	unmap_boot_param(_boot_params);
	close_mem(fd);

	symbol_addr = elf_get_symbol("boot_command_line");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"boot_command_line\"");
	}
	char* boot_command_line = (char*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	printf("\tboot_command_line: %p\n", boot_command_line);
	strcpy(boot_command_line, _boot_command_line);

	/*
	 *symbol_addr = elf_get_symbol("kernel_start_address");
	 *if(!symbol_addr) {
	 *        printf("Can't Get Symbol Address: \"kernel_start_address\"\n");
	 *        return -2;
	 *}
	 *uint64_t* _kernel_start_address = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	 *printf("kernel_start_address: %p\n", _kernel_start_address);
	 **_kernel_start_address = (uint64_t)kernel_start_address;
	 */

	return 0;
}

// TODO: Fix Timer init libary
// How to get frequency per second in linux?
static void _timer_init(char* cpu_brand) {
	uint64_t _frequency;
	if(strstr(cpu_brand, "Intel") != NULL && strstr(cpu_brand, "@ ") != NULL) {
		int number = 0;
		int is_dot_found = 0;
		int dot = 0;

		const char* ch = strstr(cpu_brand, "@ ");
		ch += 2;
		while((*ch >= '0' && *ch <= '9') || *ch == '.') {
			if(*ch == '.') {
				is_dot_found = 1;
			} else {
				number *= 10;
				number += *ch - '0';

				dot += is_dot_found;
			}

			ch++;
		}

		uint64_t frequency = 0;
		if(strncmp(ch, "THz", 3) == 0) {
		} else if(strncmp(ch, "GHz", 3) == 0) {
			frequency = 1000000000L;
		} else if(strncmp(ch, "MHz", 3) == 0) {
			frequency = 1000000L;
		} else if(strncmp(ch, "KHz", 3) == 0) {
			frequency = 1000L;
		} else if(strncmp(ch, "Hz", 3) == 0) {
			frequency = 1L;
		}

		while(dot > 0) {
			frequency /= 10;
			dot--;
		}

		_frequency = frequency * number;
	} else {
		uint64_t time_tsc1 = timer_frequency();
		sleep(1);
		uint64_t time_tsc0 = timer_frequency();

		_frequency = time_tsc0 - time_tsc1;
	}

	printf("\tFreqeuncy : %lx\n", _frequency);
	unsigned long symbol_addr = elf_get_symbol("TIMER_FREQUENCY_PER_SEC");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"TIMER_FREQUENCY_PER_SEC\"");
	}
	uint64_t* _TIMER_FREQUENCY_PER_SEC = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	*_TIMER_FREQUENCY_PER_SEC = _frequency;

	symbol_addr = elf_get_symbol("__timer_ms");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"__timer_ms\"");
	}
	uint64_t* ___timer_ms = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	printf("\ttimer_ms symbol address: %p - %p\n", symbol_addr, ___timer_ms);
	*___timer_ms = _frequency / 1000;
	printf("\ttimer_ms: %x \n", *___timer_ms);

	symbol_addr = elf_get_symbol("__timer_us");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"__timer_us\"");
	}
	uint64_t* ___timer_us = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	*___timer_us = *___timer_ms / 1000;

	symbol_addr = elf_get_symbol("__timer_ns");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"__timer_ns\"");
	}
	uint64_t* ___timer_ns = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	*___timer_ns = *___timer_us / 1000;
}

int main(int argc, char** argv) {
	int ret;
	if(geteuid() != 0) {
		printf("Permission denied. \n");
		printf("\tUsage: $sudo ./manager [BOOT PARAM FILE]\n");
		return -1;
	}

	if(argc != 2) {
		printf("\tUsage: $sudo ./manager [BOOT PARAM FILE]\n");
		return -1;
	}

	printf("\nPacketNgin 2.0 Manager\n");
	ret = smap_init();
	if(ret)
		goto error;

	printf("\nInitializing PacketNgin kernel module...\n");
	if(dispatcher_init() < 0)
		goto error;

	printf("\nParsing parameter...\n");
	ret = parse_params(argv[1]);
	if(ret) {
		printf("\tFailed to parse parameter\n");
		return ret;
	}

	printf("\nParsing kernel arguments...\n");
	ret = parse_args(kernel_args);
	if(ret) {
		printf("\tFailed to parse kernel arguments : %d\n", ret);
		return ret;
	}

	printf("\nLoading %s...\n", kernel_elf);
	ret = elf_load(kernel_elf);
	if(ret)
		return ret;

	printf("\nInitiliazing mmap symbols...\n");
	ret = symbols_init(kernel_elf);
	if(ret)
		return ret;

	printf("\nInitializing memory mapping... \n");
	PHYSICAL_OFFSET = kernel_start_address - 0x400000;
	ret = mapping_memory();
	if(ret)
		return ret;

	printf("\nCopying kernel image...\n");
	ret = elf_copy(kernel_elf, kernel_start_address);
	if(ret) {
		printf("\tFailed to copy kernel image by kexec\n");
		return ret;
	}

	printf("\nInitializing dummy entry\n");
	ret = dummy_entry();
	if(ret)
		return ret;

	printf("\ninitializing multicore processor...\n");
	mp_init0();

	printf("\nInitializing shared memory area...\n");
	if(shared_init() < 0)
		goto error;

	printf("\nInitializing malloc area...\n");
	malloc_init();

	printf("\nInitializing timer...\n");
	_timer_init(cpu_brand);

	printf("\nInitializing mulitiprocessing...\n");
	mp_init(kernel_start_address);

	printf("\nInitializing cpu...\n");
	cpu_init();

	// TODO Fix Ramdisk Size
	printf("\nInitializing gmalloc area...\n");
	gmalloc_init();

	mp_sync(0); // Barrier #1

	printf("\nInitilizing GDT...\n");
	gdt_init();
	printf("\nInitilizing TSS...\n");
	tss_init();
	printf("\nInitilizing IDT...\n");
	idt_init();

	mp_sync(1); // Barrier #2

	printf("\nInitailizing local APIC...\n");
	if(mapping_apic() < 0)
		goto error;

	printf("\nInitializing events...\n");
	event_init();

	printf("\nInitializing inter-core communications...\n");
	icc_init();

	printf("\nInitializing VM manager...\n");
	vm_init();

	printf("\nInitializing standard IO...\n");
	stdio_init();

	printf("\nInitializing linux netlink devices...\n");
	netlink_init();

	printf("\nInitializing NICs...\n");
	nicdev_init();

	printf("\nInitializing RPC manager...\n");
	manager_init();

	printf("\nInitializing shell...\n");
	shell_init();

	event_idle_add(idle0_event, NULL);

	mp_sync(2);

	while(1)
		event_loop();

	return 0;
error:
	printf("Manager initialization error occured. Terminated...\n");
	return -1;
}
