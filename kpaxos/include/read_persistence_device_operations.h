//
// Created by roni on 13/09/19.
//

#ifndef KERNEL_PAXOS_PERSISTENCE_READ_PERSISTENCE_DEVICE_OPERATIONS_H
#define KERNEL_PAXOS_PERSISTENCE_READ_PERSISTENCE_DEVICE_OPERATIONS_H

#include "kernel_device.h"

extern paxos_kernel_device*     createReadPersistenceDevice(void);
extern kernel_device_callback*  read_persistence_add_message(const char* msg, size_t size);

#endif //KERNEL_PAXOS_PERSISTENCE_READ_PERSISTENCE_DEVICE_OPERATIONS_H
