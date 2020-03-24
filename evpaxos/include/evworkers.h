//
// Created by roni on 3/24/20.
//

#ifndef KERNEL_EVWORKERS_H
#define KERNEL_EVWORKERS_H

#include <linux/kthread.h>

void evworkers_pool_init(void);
void evworkers_destroy_pool(void);
void evworker_add_work(struct kthread_work *work);

#endif // KERNEL_EVWORKERS_H
