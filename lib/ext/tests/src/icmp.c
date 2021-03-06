#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <tlsf.h>
#include <malloc.h>
#include <_malloc.h>
#include <lock.h>
#include <string.h>

#include <net/icmp.h>
#include <util/fifo.h>
#include <net/nic.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/ip.h>

#define POOL_SIZE	0x40000
#define DS_SIZE		8			// Random value

extern int __nic_count;
extern NIC* __nics[NIC_SIZE];

/**
 * Sample_reply_packet
 * 192.168.10.111 > 192.168.10.101: ICMP echo request, id 31419, seq 14, length 64
 */
uint8_t icmp_request_packet[] = { 
			0x74, 0xd4, 0x35, 0x8f, 0x66, 0xbb, 0x10, 0xc3,
			0x7b, 0x93, 0x09, 0xd1, 0x08, 0x00, 0x45, 0x00,
			0x00, 0x54, 0x79, 0xbb, 0x40, 0x00, 0x40, 0x01,
			0x2a, 0xc9, 0xc0, 0xa8, 0x0a, 0x6f, 0xc0, 0xa8, 
			0x0a, 0x65, 0x08, 0x00, 0x50, 0x1d, 0x7a, 0xbb, 
			0x00, 0x0e, 0x08, 0x32, 0xc9, 0x57, 0x00, 0x00,
			0x00, 0x00, 0x97, 0xbc, 0x05, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
			0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 
			0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 
			0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 
			0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 
			0x36, 0x37 
};

/**
 * Sample_reply_packet
 * 192.168.10.101 > 192.168.10.111: ICMP echo reply, id 31419, seq 14, length 64
 */
uint8_t icmp_reply_packet[] = {
			0x10, 0xc3, 0x7b, 0x93, 0x09, 0xd1, 0x74, 0xd4,
			0x35, 0x8f, 0x66, 0xbb, 0x08, 0x00, 0x45, 0x00, 
			0x00, 0x54, 0x48, 0xe9, 0x00, 0x00, 0x40, 0x01,
			0x9b, 0x9b, 0xc0, 0xa8, 0x0a, 0x65, 0xc0, 0xa8,
			0x0a, 0x6f, 0x00, 0x00, 0x58, 0x1d, 0x7a, 0xbb,
			0x00, 0x0e, 0x08, 0x32, 0xc9, 0x57, 0x00, 0x00,
			0x00, 0x00, 0x97, 0xbc, 0x05, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
			0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
			0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
			0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
			0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
			0x36, 0x37
};

static void icmp_process_func(void** state) {
	// Nic initilization
	void* malloc_pool = malloc(POOL_SIZE);
	init_memory_pool(POOL_SIZE, malloc_pool, 0);

	__nics[0] = malloc(sizeof(NIC));
	__nic_count++;

	__nics[0]->pool_size = POOL_SIZE;
	__nics[0]->pool = malloc_pool;
	__nics[0]->output_buffer = fifo_create(DS_SIZE, malloc_pool);
	__nics[0]->config = map_create(DS_SIZE, NULL, NULL, __nics[0]->pool);

	Packet* packet = nic_alloc(__nics[0], sizeof(icmp_request_packet));
	memcpy(packet->buffer + packet->start, icmp_request_packet, sizeof(icmp_request_packet));

	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	uint32_t addr = endian32(ip->destination);
	nic_ip_add(__nics[0], addr);

	// For configuring same value of identification(16bit) and flag(3bit).
	ip->id = 0xe948;
	ip->flags_offset = 0;

	assert_true(icmp_process(packet));
	packet = fifo_pop(__nics[0]->output_buffer);
	assert_memory_equal(packet->buffer + packet->start, icmp_reply_packet, sizeof(icmp_reply_packet));

	destroy_memory_pool(malloc_pool);
	free(malloc_pool);
	malloc_pool = NULL;
}

int main(void) {
	const struct CMUnitTest UnitTest[] = {
		cmocka_unit_test(icmp_process_func),
	};
	return cmocka_run_group_tests(UnitTest, NULL, NULL);
}
