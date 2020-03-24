//
// Created by roni on 3/24/20.
//

#include "evworkers.h"
#include "workers_pool.h"

workers_pool* worker_pool;

void
evworkers_pool_init(void) {
  int workers_num = 10; // TODO Load from config
  worker_pool = create_pool(workers_num);
}

void
evworkers_destroy_pool(void) {
  free_pool(worker_pool);
}

void
evworker_add_work(struct kthread_work *work) {
  add_work(worker_pool, work);
}