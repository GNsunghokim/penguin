#ifndef __MANAGER_CORE_H__
#define __MANAGER_CORE_H__

typedef struct _ManagerCore {
	uint16_t port;
	void* data;
} ManagerCore;

/**
 * Manager Core initialize
 *
 * @return success
 */
bool manager_core_init();

/**
 * Manager Core TCP Server open
 *
 * @param port tcp port number
 * @return ManagerCore
 */
ManagerCore* manager_core_server_open(uint16_t port);

/**
 * Manager Core TCP Server close
 *
 * @param manager_core core data
 * @return success
 */
bool manager_core_server_close(ManagerCore* manager_core);
#endif /*__MANAGER_CORE_H__*/
