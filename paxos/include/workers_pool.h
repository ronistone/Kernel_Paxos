//
// Created by roni on 12/02/2020.
//

#ifndef KERNEL_PAXOS_PERSISTENCE_WORKERS_POOL_H
#define KERNEL_PAXOS_PERSISTENCE_WORKERS_POOL_H

#include <linux/kthread.h>
#include <linux/vmalloc.h>
//#include "kernel_device.h"
#include "paxos.h"

typedef struct workers_pool {

  struct kthread_worker** worker;
  struct task_struct **workerThread;
  struct task_struct* mainThread;
  int current_worker;
  int num_workers;

} workers_pool;

typedef struct persistence_work {
  struct kthread_work work;
  kernel_device_callback* param;
  uint32_t iid;
} persistence_work;

extern workers_pool* create_pool(int num_workers);
extern void add_work(workers_pool* pool, persistence_work* work);
extern void free_pool(workers_pool *pool);
extern persistence_work* createPersistenceWork(void* func_work);
#endif //KERNEL_PAXOS_PERSISTENCE_WORKERS_POOL_H
