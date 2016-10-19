// Standard C
#include <stdio.h>
#include <string.h>
#include <errno.h>

// Core
#include <util/event.h>

// Kernel
#include "asm.h"
#include "timer.h"
#include "malloc.h"
#include "gmalloc.h"
#include "page.h"
#include "stdio.h"
#include "port.h"
#include "mp.h"
#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "acpi.h"
#include "pci.h"
#include "apic.h"
#include "ioapic.h"
#include "task.h"
#include "icc.h"
#include "symbols.h"
#include "file.h"
#include "module.h"
#include "device.h"
#include "vnic.h"
#include "manager.h"
#include "shell.h"
#include "loader.h"
#include "vfio.h"
#include "shared.h"
#include "pnkc.h"
#include "mmap.h"

// Drivers
#include "driver/pata.h"
#include "driver/usb/usb.h"
#include "driver/ramdisk.h"
#include "driver/nic.h"
#include "driver/virtio_blk.h"
#include "driver/fs.h"
#include "driver/bfs.h"
#include "driver/console.h"

static void ap_timer_init() {
	extern const uint64_t TIMER_FREQUENCY_PER_SEC;
	extern uint64_t __timer_ms;
	extern uint64_t __timer_us;
	extern uint64_t __timer_ns;

	*(uint64_t*)&TIMER_FREQUENCY_PER_SEC = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&TIMER_FREQUENCY_PER_SEC);
	__timer_ms = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&__timer_ms);
	__timer_us = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&__timer_us);
	__timer_ns = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&__timer_ns);
}

static void init_nics(int count) {
	int index = 0;
	extern uint64_t manager_mac;
	for(int i = 0; i < count; i++) {
		nic_devices[i] = device_get(DEVICE_TYPE_NIC, i);
		if(!nic_devices[i])
			continue;

		NICPriv* nic_priv = gmalloc(sizeof(NICPriv));
		if(!nic_priv)
			continue;

		nic_priv->nics = map_create(8, NULL, NULL, NULL);

		nic_devices[i]->priv = nic_priv;

		NICInfo info;
		((NICDriver*)nic_devices[i]->driver)->get_info(nic_devices[i]->id, &info);

		nic_priv->port_count = info.port_count;
		for(int j = 0; j < info.port_count; j++) {
			nic_priv->mac[j] = info.mac[j];

			char name_buf[64];
			sprintf(name_buf, "eth%d", index);
			uint16_t port = j << 12;

			Map* vnics = map_create(16, NULL, NULL, NULL);
			map_put(nic_priv->nics, (void*)(uint64_t)port, vnics);

			printf("NICs in private data : %p\n", nic_priv->nics);

			printf("\t%s : [%02x:%02x:%02x:%02x:%02x:%02x] [%c]\n", name_buf,
				(info.mac[j] >> 40) & 0xff,
				(info.mac[j] >> 32) & 0xff,
				(info.mac[j] >> 24) & 0xff,
				(info.mac[j] >> 16) & 0xff,
				(info.mac[j] >> 8) & 0xff,
				(info.mac[j] >> 0) & 0xff,
				manager_mac == 0 ? '*' : ' ');

			if(!manager_mac)
				manager_mac = info.mac[j];

			index++;
		}
	}
}

#define VGA_BUFFER_PAGES	12
//static uint64_t idle_time;
extern Device* nic_devices[];

static bool idle0_event(void* data) {
	/*
	static uint64_t tick;
	uint64_t time = cpu_tsc();
	
	if(time > tick) {
		tick = time + cpu_frequency;
		nic_statistics(time);
		printf("\033[0;68HLoad: %3d%%", (cpu_frequency - idle_time) * 100 / cpu_frequency);
		idle_time = 0;
		return;
	}
	*/
	
	// Poll NICs
	extern Device* nic_current;
	int poll_count = 0;
	for(int i = 0; i < NIC_MAX_DEVICE_COUNT; i++) {
		Device* dev = nic_devices[i];
		if(dev == NULL)
			break;
		
		nic_current = dev;
		NICDriver* nic = nic_current->driver;
		
		poll_count += nic->poll(nic_current->id);
	}

#ifdef VFIO_ENABLED
	// Poll FIO
#define MAX_VM_COUNT	128
	uint32_t vmids[MAX_VM_COUNT];
	int vm_count = vm_list(vmids, MAX_VM_COUNT);
	for(int i = 0; i < vm_count; i++) {
		VM* vm = vm_get(vmids[i]);
		VFIO* fio = vm->fio;
		if(!fio)
			continue;

		if(!fio->input_addr)
			continue;

		// Check if user changed request_id on purpose, and fix it
		if(fio->user_fio->request_id != fio->request_id + fifo_size(fio->input_addr))
			fio->user_fio->request_id = fio->request_id + fifo_size(fio->input_addr);

		// Check if there's something in the input fifo
		if(fifo_size(fio->input_addr) > 0)
			vfio_poll(vm);
	}
#endif

	// idle
/*	
	for(int i = 0; i < 1000; i++)
		asm volatile("nop");
*/	
	
	//idle_time += cpu_tsc() - time;
	return true;
}

static bool idle_monitor_event(void* data) {
	static uint8_t trigger;

	monitor(&trigger);
	mwait(1, 0x21);

	return true;
}

static bool idle_hlt_event(void* data) {
	hlt();

	return true;
}

static void context_switch() {
	// Set exception handlers
	APIC_Handler old_exception_handlers[32];
	
	void exception_handler(uint64_t vector, uint64_t err) {
		if(apic_user_rip() == 0 && apic_user_rsp() == task_get_stack(1)) {
			// Do nothing
		} else {
			printf("* User VM exception handler");
			apic_dump(vector, err);
			errno = err;
		}
		
		apic_eoi();
		
		task_destroy(1);
	}
	
	for(int i = 0; i < 32; i++) {
		if(i != 7) {
			old_exception_handlers[i] = apic_register(i, exception_handler);
		}
	}
	
	// Context switching
	// TODO: Move exception handlers to task resources
	task_switch(1);
	
	// Restore exception handlers
	for(int i = 0; i < 32; i++) {
		if(i != 7) {
			apic_register(i, old_exception_handlers[i]);
		}
	}
	
	// Send callback message
	bool is_paused = errno == 0 && task_is_active(1);
	if(is_paused) {
		// ICC_TYPE_PAUSE is not a ICC message but a interrupt in fact, So forcely commit the message
	}
	
	ICC_Message* msg3 = icc_alloc(is_paused ? ICC_TYPE_PAUSED : ICC_TYPE_STOPPED);
	msg3->result = errno;
	if(!is_paused) {
		msg3->data.stopped.return_code = apic_user_return_code();
	}
	errno = 0;
	icc_send(msg3, 0);
	
	printf("VM %s...\n", is_paused ? "paused" : "stopped");
}

static void icc_start(ICC_Message* msg) {
	VM* vm = msg->data.start.vm;
	printf("Loading VM...(%p) \n", vm);

	// TODO: Change blocks[0] to blocks
	uint32_t id = loader_load(vm);

	printf("Loading done %d\n", id);
	if(errno != 0) {
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_STARTED);

		msg2->result = errno;	// errno from loader_load
		icc_send(msg2, msg->apic_id);
		icc_free(msg);
		printf("Execution FAILED: %x\n", errno);
		return;
	}

	*(uint32_t*)task_addr(id, SYM_NIS_COUNT) = vm->nic_count;
	NIC** nics = (NIC**)task_addr(id, SYM_NIS);
	for(int i = 0; i < vm->nic_count; i++) {
		task_resource(id, RESOURCE_NI, vm->nics[i]);
		nics[i] = vm->nics[i]->nic;

		/*
		 *printf("----------------- VNIC %p\n", vm->nics[i]);
		 *printf("----------------- NICS %p\n", nics[i]);
		 *uint8_t* ptr = (uint8_t*)nics[i];
		 *printf("Max buffer size : %d\n", vm->nics[i]->max_buffer_size);
		 *printf("NIC : %p\n", vm->nics[i]->nic);
		 *printf("Pool : %p\n", vm->nics[i]->pool);
		 *for(int j = 0; j < 32; j++) {
		 *        printf("%02x ", ptr[j]);
		 *}
		 *printf("\n");
		 */
	}
		
	printf("Starting VM...\n");
	ICC_Message* msg2 = icc_alloc(ICC_TYPE_STARTED);
	
	msg2->data.started.stdin = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDIN));
	msg2->data.started.stdin_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_HEAD));
	msg2->data.started.stdin_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_TAIL));
	msg2->data.started.stdin_size = *(int*)task_addr(id, SYM_STDIN_SIZE);
	/*
	 *printf("STDOUT ADDRESS : %p\n", (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDOUT)));
	 *printf("HEAD ADDRESS : %p\n",  (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_HEAD)));
	 *printf("TAIL ADDRESS : %p\n",  (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_TAIL)));
	 */
	msg2->data.started.stdout = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDOUT));
	msg2->data.started.stdout_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_HEAD));
	msg2->data.started.stdout_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_TAIL));
	msg2->data.started.stdout_size = *(int*)task_addr(id, SYM_STDOUT_SIZE);
	msg2->data.started.stderr = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDERR));
	msg2->data.started.stderr_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_HEAD));
	msg2->data.started.stderr_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_TAIL));
	msg2->data.started.stderr_size = *(int*)task_addr(id, SYM_STDERR_SIZE);
	
	icc_send(msg2, msg->apic_id);

	icc_free(msg);

	context_switch();
}

static void icc_resume(ICC_Message* msg) {
	if(msg->result < 0) {
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);
		msg2->result = msg->result;
		icc_send(msg2, msg->apic_id);

		icc_free(msg);
		return;
	}

	printf("Resuming VM...\n");
	ICC_Message* msg2 = icc_alloc(ICC_TYPE_RESUMED);

	icc_send(msg2, msg->apic_id);
	icc_free(msg);
	context_switch();
}

static void icc_pause(uint64_t vector, uint64_t error_code) {
	printf("Interrupt occured!\n");
	apic_eoi();
	//task_switch(0);
}

static void icc_stop(ICC_Message* msg) {
	if(msg->result < 0) { // Not yet core is started.
		ICC_Message* msg2 = icc_alloc(ICC_TYPE_STOPPED);
		msg2->result = msg->result;
		icc_send(msg2, msg->apic_id);

		icc_free(msg);
		return;
	}

	icc_free(msg);
	task_destroy(1);
}

#define EXEC_NOT_FOUND_FILE	-1
#define EXEC_ERROR		-2
#define EXEC_NOT_ENOUGH_BUFFER	1
#define EXEC_END		0

static int exec(char* name) {
	static char line[CMD_SIZE];
	static size_t head = 0;
	static size_t eod = 0;
	static size_t seek = 0;
	int ret;

	int fd = open(name, "r");
	if(fd < 0)
		return EXEC_NOT_FOUND_FILE;

	while((ret = read(fd, &line[eod], CMD_SIZE - eod)) > 0) {
		eod += ret;
		for(; seek < eod; seek++) {
			if(line[seek] == '\n' || line[seek] == '\0') {
				for(; head < seek; head++) {
					if(line[head] == ' ')
						head++;
					else
						break;
				}

				if(line[head] == '#') {
					head = seek + 1;
					continue;
				}

				if(__stdin_tail >= __stdin_head) {
					if((seek + 1 - head) > (__stdin_size - ( __stdin_tail - __stdin_head))) {
						printf("Wrong2 %d %d\n", seek - head, __stdin_size - ( __stdin_tail - __stdin_head));
						return EXEC_NOT_ENOUGH_BUFFER;
					}
				} else {
					if((seek + 1 - head) > (__stdin_head - __stdin_tail)) {
						printf("Wrong2_2 %d %d\n", seek - head, __stdin_head - __stdin_tail);
						return EXEC_NOT_ENOUGH_BUFFER;
					}
				}

				apic_disable();
				for(;head <= seek; head++) {
					stdio_putchar(line[head]);
				}
				apic_enable();
			}
		}

		if(head == 0 && eod == CMD_SIZE){
			return EXEC_ERROR;
		} else {
			if((eod - head) > 0) {
				memmove(line, &line[head], eod - head);
				eod -= head;
				seek = eod;
				head = 0;
			}
		}
	}

	close(fd);

	return EXEC_END;
}

/*
 *  * Volatile isn't enough to prevent the compiler from reordering the
 *   * read/write functions for the control registers and messing everything up.
 *    * A memory clobber would solve the problem, but would prevent reordering of
 *     * all loads stores around it, which can hurt performance. Solution is to
 *      * use a variable and mimic reads and writes to it to enforce serialization
 *       */
static unsigned long __force_order;

static inline unsigned long native_read_cr3(void)
{
	unsigned long val;
	asm volatile("mov %%cr3,%0\n\t" : "=r" (val), "=m" (__force_order));
	return val;
}

static inline void native_write_cr3(unsigned long val)
{
	asm volatile("mov %0,%%rdi": : "r" (val), "m" (__force_order));
}

static inline void __native_flush_tlb_single(unsigned long addr)
{
	asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static inline void clflush(volatile void *__p) {
	asm volatile("clflush %0" : "+m" (*(volatile char *)__p));
}

static void clflush_cache_range(void *vaddr, unsigned int size) {
	void *vend = vaddr + size - 1;

	#define mb()	asm volatile("mfence":::"memory")

	mb();

	for (; vaddr < vend; vaddr += 64)
		clflush(vaddr);
	/*
	 ** Flush any possible final partial cacheline:
	 **/
	clflush(vend);

	mb();
}

static void fixup_page_table(uint8_t apic_id, uint64_t offset) {
	uint64_t base = PAGE_TABLE_START + apic_id * 0x200000 + offset;
	PageTable* l4u = (PageTable*)(base + PAGE_TABLE_SIZE * PAGE_L4U_INDEX);

	for(int i = 0; i < PAGE_L4U_SIZE * PAGE_ENTRY_COUNT; i++) {
		if(i == 0)
			continue;

		l4u[i].base = i + (offset >> 21);
		l4u[i].p = 1;
		l4u[i].us = 0;
		l4u[i].rw = 1;
		l4u[i].ps = 1;
		// Write through
		//l4u[i].pwt = 1;
	}

	// Local APIC address (0xfee00000: 0x7F7(PFN))
	l4u[0x7f7].base = 0x7f7;

	uint64_t pml4 = PAGE_TABLE_START + apic_id * 0x200000 + offset;
	asm volatile("movq %0, %%cr3" : : "r"(pml4));
}

bool stdio_enabled = false;
void main() {
	mp_init();

	uint64_t apic_id = mp_apic_id() - BSP_APIC_ID_OFFSET;

	extern uint64_t PHYSICAL_OFFSET;
	fixup_page_table(apic_id, PHYSICAL_OFFSET);
	console_init();

	uint64_t vga_buffer = PHYSICAL_TO_VIRTUAL(VGA_BUFFER_START);
	stdio_init(apic_id, (void*)vga_buffer, 64 * 1024);
	malloc_init(vga_buffer);

	stdio_enabled = true;
	printf("\nPacketNgin ver 2.0.\n");
	extern uint64_t _apic_address;
	//mp_sync();	// Barrier #1
	if(apic_id == 0) {
		// Parse kernel arguments
		uint64_t initrd_start = pnkc.initrd_start;
		uint64_t initrd_end = pnkc.initrd_end;

/*
 *                printf("\x1b""32mOK""\x1b""0m\n");
 *
 *                printf("Copy RAM disk image from 0x%x to 0x%x (%d)\n",
 *                                initrd_start,
 *                                PHYSICAL_TO_VIRTUAL(RAMDISK_START),
 *                                initrd_end - initrd_start);
 *
 *                memcpy((void*)PHYSICAL_TO_VIRTUAL(RAMDISK_START),
 *                                (void*)(uintptr_t)PHYSICAL_TO_VIRTUAL(initrd_start),
 *                                initrd_end - initrd_start);
 */

		printf("Analyze CPU information...\n");
		cpu_init();
		gmalloc_init(RAMDISK_START, initrd_end - initrd_start);
		shared_init();
		timer_init(cpu_brand);

		gdt_init();
		tss_init();
		idt_init();

		//mp_sync();	// Barrier #2

		printf("Loading GDT...\n");
		gdt_load();

		printf("Loading TSS...\n");
		tss_load();

		printf("Loading IDT...\n");
		idt_load();

		printf("Initializing APICs...\n");
		apic_activate();

		printf("Initializing PCI...\n");
		pci_init();

		printf("Initailizing local APIC...\n");
		apic_init();
		
		//apic_register(48, icc_pause);

		printf("Initializing I/O APIC...\n");
		//ioapic_init();
		apic_enable();

		printf("Initializing Multi-tasking...\n");
		task_init();

		printf("Initializing events...\n");
		event_init();

		printf("Initializing inter-core communications...\n");
		icc_init();

		/*
		 *apic_write64(APIC_REG_ICR, ((uint64_t)1 << 56) |
		 *                        APIC_DSH_NONE | 
		 *                        APIC_DSH_SELF | 
		 *                        APIC_TM_EDGE | 
		 *                        APIC_LV_DEASSERT | 
		 *                        APIC_DM_PHYSICAL | 
		 *                        APIC_DMODE_FIXED |
		 *                        48);
		 */

/*
 *
 *                printf("Initializing USB controller driver...\n");
 *                usb_initialize();
 *
 *                printf("Initializing disk drivers...\n");
 *                disk_init();
 *                if(!disk_register(&pata_driver, NULL)) {
 *                        printf("\tPATA driver registration FAILED!\n");
 *                        while(1) asm("hlt");
 *                }
 *
 *                if(!disk_register(&usb_msc_driver, NULL)) {
 *                        printf("\tUSB MSC driver registration FAILED!\n");
 *                        while(1) asm("hlt");
 *                }
 *
 *                if(!disk_register(&virtio_blk_driver, NULL)) {
 *                        printf("\tVIRTIO BLOCK driver registration FAILED!\n");
 *                        while(1) asm("hlt");
 *                }
 *
 *                printf("Initializing RAM disk...\n");
 *                char cmdline[32];
 *                sprintf(cmdline, "-addr 0x%x -size 0x%x", RAMDISK_START, initrd_end - initrd_start);
 *                if(!disk_register(&ramdisk_driver, cmdline)) {
 *                        printf("\tRAM disk driver registration FAILED!\n");
 *                        while(1) asm("hlt");
 *                }
 *
 *                printf("Initializing file system...\n");
 *                fs_init();
 *                fs_register(&bfs_driver);
 *                fs_mount(DISK_TYPE_RAMDISK << 16 | 0x00, 0,  FS_TYPE_BFS, "/boot");
 *
 *                printf("Initializing kernel symbols...\n");
 *                symbols_init();
 *
 *                printf("Initializing modules...\n");
 *                module_init();
 *
 *                printf("Initializing device drivers...\n");
 *                device_module_init();
 *
 *                uint16_t nic_count = device_count(DEVICE_TYPE_NIC);
 *                printf("Initializing NICs: %d\n", nic_count);
 *                init_nics(nic_count);
 */

/*
 *                printf("Initializing VM manager...\n");
 *                vm_init();
 *
 *                printf("Initializing RPC manager...\n");
 *                manager_init();
 *
 *                printf("Initializing shell...\n");
 *                shell_init();
 */

//		event_busy_add(idle0_event, NULL);
		icc_register(ICC_TYPE_START, icc_start);
		icc_register(ICC_TYPE_RESUME, icc_resume);
		icc_register(ICC_TYPE_STOP, icc_stop);
		apic_register(49, icc_pause);

		if(cpu_has_feature(CPU_FEATURE_MONITOR_MWAIT) && cpu_has_feature(CPU_FEATURE_MWAIT_INTERRUPT))
			event_idle_add(idle_monitor_event, NULL);
		else {
			printf("IDLE event add\n");
			event_idle_add(idle_hlt_event, NULL);
		}

	} else {
		mp_sync();	// Barrier #2
		ap_timer_init();

		gdt_load();
		tss_load();
		idt_load();

		apic_init();
		apic_enable();

		task_init();
		event_init();
		icc_init();
		icc_register(ICC_TYPE_START, icc_start);
		icc_register(ICC_TYPE_RESUME, icc_resume);
		icc_register(ICC_TYPE_STOP, icc_stop);
		apic_register(49, icc_pause);

		if(cpu_has_feature(CPU_FEATURE_MONITOR_MWAIT) && cpu_has_feature(CPU_FEATURE_MWAIT_INTERRUPT))
			event_idle_add(idle_monitor_event, NULL);
		else
			event_idle_add(idle_hlt_event, NULL);
	}

/*
 *        mp_sync();
 *
 *        if(apic_id == 0) {
 *                while(exec("/boot/init.psh") > 0)
 *                        event_loop();
 *        }
 */

	while(1) {
		event_loop();
	}
}
