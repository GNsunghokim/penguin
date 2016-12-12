#include <stdio.h>
#include <string.h>
#include <util/list.h>
#include <tlsf.h>
#include <malloc.h>
#include <_malloc.h>
#include "malloc.h"
#include "idt.h"
#include "pnkc.h"
#include "mp.h"
#include "shared.h"
#include "multiboot2.h"
#include "gmalloc.h"
#include "entry.h"
#include "mmap.h"

void* gmalloc_pool;

uint32_t bmalloc_count;
uint64_t* bmalloc_pool;

// Reserved memory blocks
typedef struct {
	uintptr_t start;
	uintptr_t end;
} Block;

/* Creates physical memory blocks to be reserved */
static int reserved_blocks(Block* reserved, uintptr_t ramdisk_addr, uint32_t ramdisk_size) {
	int reserved_count = 0;
	reserved[reserved_count].start = VIRTUAL_TO_PHYSICAL(BIOS_AREA_START); //IVT_AREA_START;
	reserved[reserved_count].end = VIRTUAL_TO_PHYSICAL(BIOS_AREA_END); //DESC_TABLE_AREA_START; //BDA_AREA_END;
	reserved_count++;

	reserved[reserved_count].start = VIRTUAL_TO_PHYSICAL(DESC_TABLE_AREA_START);
	reserved[reserved_count].end = VIRTUAL_TO_PHYSICAL(DESC_TABLE_AREA_END);
	reserved_count++;

	reserved[reserved_count].start = VIRTUAL_TO_PHYSICAL(KERNEL_TEXT_AREA_START);
	reserved[reserved_count].end = VIRTUAL_TO_PHYSICAL(KERNEL_TEXT_AREA_END);
	reserved_count++;

	uint8_t* core_map = mp_core_map();
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		if(core_map[i] == MP_CORE_INVALID)
			continue;

		reserved[reserved_count].start = 
			VIRTUAL_TO_PHYSICAL(KERNEL_DATA_AREA_START + KERNEL_DATA_AREA_SIZE * i);
		reserved[reserved_count].end = 
			VIRTUAL_TO_PHYSICAL(KERNEL_DATA_AREA_START + KERNEL_DATA_AREA_SIZE * (i + 1));
		reserved_count++;
	}

	reserved[reserved_count].start = VIRTUAL_TO_PHYSICAL(ramdisk_addr);	// RAM disk
	reserved[reserved_count].end = VIRTUAL_TO_PHYSICAL(ramdisk_addr + ramdisk_size);
	reserved_count++;

#ifdef __PENGUIN__
	void relocate(Block* blocks, int count) {
		// TODO: need to set physical end
		for(int i = 0; i < count; i++) {
			Block* r = &reserved[i];
			r->start += PHYSICAL_OFFSET;
			r->end += PHYSICAL_OFFSET;
		}
	}

	// Relocate blocks adding physcial offset. This should be done to find actual
	// physical memory map using e820.
	relocate(reserved, reserved_count);

	reserved[reserved_count].start = 0x0;
	reserved[reserved_count].end = PHYSICAL_OFFSET;
	reserved_count++;

#endif /* __PENGUIN__ */

	return reserved_count;
}

static void extend_gmalloc_pool(List* blocks, void* gmalloc_pool) {
	ListIterator iter;
	list_iterator_init(&iter, blocks);
	while(list_iterator_has_next(&iter)) {
		Block* b = list_iterator_next(&iter);

#ifdef __PENGUIN__
		// Rearrange memory offset since we access physical memory space
		// by MMU having physcial offset
		b->start -= PHYSICAL_OFFSET;
		b->end -= PHYSICAL_OFFSET;
#endif /* __PENGUIN__ */

		uintptr_t start = (b->start + 0x200000 - 1) & ~((uintptr_t)0x200000 - 1);
		uintptr_t end = b->end & ~((uintptr_t)0x200000 - 1);

		if(start >= end) {
			add_new_area((void*)b->start, b->end - b->start, gmalloc_pool);

			b->start = b->end = 0;
		} else {
			if(start > b->start) {
				add_new_area((void*)b->start, start - b->start, gmalloc_pool);

				b->start = start;
			}

			if(end < b->end) {
				add_new_area((void*)end, b->end - end, gmalloc_pool);

				b->end = end;
			}
		}
		if(b->start < b->end) {
			bmalloc_count += (end - start) / 0x200000;
		} else {
			free(b);
			list_iterator_remove(&iter);
		}
	}
}

static void extend_bmalloc_pool(List* blocks, uint64_t* bmalloc_pool) {
	ListIterator iter;
	list_iterator_init(&iter, blocks);
	Block* pop() {
		uintptr_t last_start = UINTPTR_MAX;
		int last_index = -1;

		int i = 0;
		list_iterator_init(&iter, blocks);
		while(list_iterator_has_next(&iter)) {
			Block* b = list_iterator_next(&iter);
			if(b->start < last_start) {
				last_start = b->start;
				last_index = i;
			}

			i++;
		}

		if(last_index >= 0)
			return list_remove(blocks, last_index);
		else
			return NULL;
	}

	uint32_t bmalloc_index = 0;
	Block* block = pop();
	while(block) {
		uintptr_t start = block->start;
		uintptr_t end = block->end;

		while(start < end) {
			bmalloc_pool[bmalloc_index++] = start;
			start += 0x200000;
		}

		free(block);

		block = pop();
	}
	
	list_destroy(blocks);
}

void gmalloc_init() {
	uint64_t start = VIRTUAL_TO_PHYSICAL(IDT_END_ADDR);
	uint64_t end = VIRTUAL_TO_PHYSICAL((uint64_t)shared);

	init_memory_pool(end - start, (void*)start, 0);
	gmalloc_pool = (void*)start;

	Block reserved[4 + MP_MAX_CORE_COUNT + 1 /* PHYSICAL_OFFSET */];

	uintptr_t ramdisk_addr = (uintptr_t)RAMDISK_START;
	uint64_t initrd_start = pnkc.initrd_start;
	uint64_t initrd_end = pnkc.initrd_end;
	uint32_t ramdisk_size = initrd_end - initrd_start;

	int reserved_count = reserved_blocks(reserved, ramdisk_addr, ramdisk_size);
	int count = pnkc.smap_count;

	// Print MMAP information and find available blocks
	List* blocks = list_create(NULL);
	printf("System memory map\n");
	for(int i = 0; i < count; i++) {
		SMAP* entry = &pnkc.smap[i];
		//struct multiboot_mmap_entry* entry = &mmap->entries[i];
		char* type;
		switch(entry->type) {
			case SMAP_TYPE_MEMORY:
				type = "Memory";
				Block* block = malloc(sizeof(Block));
				block->start = entry->base;
				block->end = entry->base + entry->length;
				list_add(blocks, block);
				break;
			case SMAP_TYPE_RESERVED:
				type = "Reserved";
				break;
			case SMAP_TYPE_ACPI:
				type = "ACPI";
				break;
			case SMAP_TYPE_NVS:
				type = "NVS";
				break;
			case SMAP_TYPE_UNUSABLE:
				type = "Disabled";
				break;
			default:
				type = "Unknown";
		}
		printf("\t0x%016lx - 0x%016lx: %s(%d)\n", entry->base, entry->base + entry->length, type, entry->type);
	}

	// Remove reserved blocks
	for(size_t i = 0; i < list_size(blocks); i++) {
		Block* b = list_get(blocks, i);

		for(int j = 0; j < reserved_count; j++) {
			Block* r = &reserved[j];

			if(r->end <= b->start || r->start >= b->end) {		// Out of bounds
				continue;
			} else if(r->start <= b->start && r->end >= b->end) {	// Including exception
				list_remove(blocks, i);
				i--;
				break;
			} else if(r->start <= b->start && r->end < b->end) {	// Head cut
				b->start = r->end;
			} else if(r->start < b->end && r->end >= b->end) {	// Tail cut
				b->end = r->start;
			} else {						// Body cut
				Block* b2 = malloc(sizeof(Block));
				b2->start = r->end;
				b2->end = b->end;
				list_add(blocks, b2);

				b->end = r->start;
			}
		}
	}

	// Extend gmalloc pool
	extend_gmalloc_pool(blocks, gmalloc_pool);

	// Extend bmalloc pool
	bmalloc_pool = malloc(sizeof(uint64_t) * bmalloc_count);
	extend_bmalloc_pool(blocks, bmalloc_pool);

	void print_pool(char* message, size_t total) {
		size_t mb = total / 1024 / 1024;
		size_t kb = total / 1024 - mb * 1024;
		printf("%s: %d.%dMB ", message, mb, kb);
	}
	
	printf("Memory pool: ");
	print_pool("local", malloc_total());
	print_pool("global", gmalloc_total());
	print_pool("block", bmalloc_total());
	printf("\n");
}

inline size_t gmalloc_total() {
	return get_total_size(gmalloc_pool);
}

inline size_t gmalloc_used() {
	return get_used_size(gmalloc_pool);
}

inline void* gmalloc(size_t size) {
	do {
		void* ptr = malloc_ex(size, gmalloc_pool);
		if(ptr)
			return ptr;

		// TODO: print to stderr
		printf("WARN: Not enough global memory!!!\n");
		
		void* block = bmalloc();
		if(!block) {
			// TODO: print to stderr
			printf("ERROR: Not enough block memory!!!\n");
			return NULL;
		}
		
		add_new_area(block, 0x200000, gmalloc_pool);
	} while(1);
}

inline void gfree(void* ptr) {
	free_ex(ptr, gmalloc_pool);
}

inline void* grealloc(void* ptr, size_t size) {
	return realloc_ex(ptr, size, gmalloc_pool);
}

inline void* gcalloc(uint32_t nmemb, size_t size) {
	return calloc_ex(nmemb, size, gmalloc_pool);
}

void* bmalloc() {
	for(size_t i = 0; i < bmalloc_count; i++) {
		if(!(bmalloc_pool[i] & 0x01)) {
			void* ptr = (void*)bmalloc_pool[i];
			bmalloc_pool[i] |= 0x01;
			return ptr;
		}
	}
	
	// TODO: print to stderr
	printf("Not enough block memory!!!");
	
	return NULL;
}

void bfree(void* ptr) {
	uint64_t addr = (uint64_t)ptr;
	addr |= 0x01;
	
	for(size_t i = 0; i < bmalloc_count; i++) {
		if(bmalloc_pool[i] == addr) {
			bmalloc_pool[i] ^= 0x01;
			break;
		}
	}
}

size_t bmalloc_total() {
	return bmalloc_count * 0x200000;
}

size_t bmalloc_used() {
	size_t size = 0;
	for(size_t i = 0; i < bmalloc_count; i++) {
		if(bmalloc_pool[i] & 0x01) {
			size += 0x200000;
		}
	}
	
	return size;
}
