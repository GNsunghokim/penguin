#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdint.h>
#include <util/list.h>
//#include <lwip/tcp.h>
#include <vnic.h>

#define MANAGER_DEFAULT_NICDEV	"eth0"
#define MANAGER_DEFAULT_IP	0xc0a864fe	// 192.168.100.254
#define MANAGER_DEFAULT_GW	0xc0a86401	// 192.168.100.1
#define MANAGER_DEFAULT_NETMASK	0xffffff00	// 255.255.255.0
#define MANAGER_DEFAULT_PORT	1111

typedef struct _Manager {
	int		vnic_count;
	VNIC*		vnics[NIC_MAX_COUNT];	// gmalloc, ni_create
	List* 		netifs;
	List*		servers;
	List* 		clients;
	List* 		actives;
} Manager;

extern Manager manager;

int manager_init();

/**
 * Create a manager vnic
 *
 * @param NIC Device Name
 * @return VNIC
 */
VNIC* manager_create_vnic(char* name);
/**
 * Destroy a manager vnic
 *
 * @param VNIC
 * @return boolean
 */
bool manager_destroy_vnic(VNIC* vnic);

struct netif* manager_create_netif(NIC* nic, uint32_t ip, uint32_t netmask, uint32_t gateway, bool is_default);
bool manager_destroy_netif(struct netif* netif);

uint32_t manager_get_ip();
void manager_set_ip(struct netif* netif, uint32_t ip);
void manager_set_port(uint16_t port);
uint32_t manager_get_gateway();
void manager_set_gateway(uint32_t gw);
uint32_t manager_get_netmask();
void manager_set_netmask(uint32_t nm);
void manager_set_interface();
struct tcp_pcb* manager_netif_server_open(struct netif* netif, uint16_t port);
bool manager_netif_server_close(struct tcp_pcb* tcp_pcb);

/**
 * Send Arping Request
 *
 * @param nic NIC
 * @param addr Destination Address
 * @param count Seding count
 * @result boolean
 */
bool manager_arping(NIC* nic, uint32_t addr, uint32_t count);

#endif /* __MANAGER_H__ */
