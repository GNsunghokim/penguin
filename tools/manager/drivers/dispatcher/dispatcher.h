#ifndef __DISPATCHER_IMPL_H__
#define __DISPATCHER_IMPL_H__

#include <linux/types.h>

#define DISPATCHER			0xAC

#define DISPATCHER_CREATE_NIC		_IOW(DISPATCHER, 0x10, void *)
#define DISPATCHER_DESTROY_NIC		_IOW(DISPATCHER, 0x11, void *)

#define DISPATCHER_CREATE_VNIC		_IOW(DISPATCHER, 0x20, void *)
#define DISPATCHER_DESTROY_VNIC		_IOW(DISPATCHER, 0x21, void *)
#define DISPATCHER_UPDATE_VNIC		_IOWR(DISPATCHER, 0x22, void *)
#define DISPATCHER_GET_VNIC		_IOR(DISPATCHER, 0x23, void *)

#endif /* __DISPATCHER_IMPL_H__ */
