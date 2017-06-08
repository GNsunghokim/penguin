#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__

#include <linux/types.h>

#include <drivers/dispatcher/dispatcher.h>

int dispatcher_init(void);
int dispatcher_exit(void);

int dispatcher_create_nic(void* nic);
int dispatcher_destroy_nic(void* nic);

int dispatcher_create_vnic(void* vnic);
int dispatcher_destroy_vnic(void* vnic);
int dispatcher_update_vnic(void* vnic);
int dispatcher_get_vnic(void* vnic);

#endif /* __DISPATCHER_H__ */
