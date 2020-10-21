#ifndef KLEARNER_DEVICE_INCLUDE
#define KLEARNER_DEVICE_INCLUDE

#include <asm/atomic.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/time.h>
#include "kernel_device.h"

extern paxos_kernel_device* create_kleaner_device(void);
extern void                 kset_message(char* msg, size_t size);

#endif
