//
// Created by roni on 3/24/20.
//

#include "evworkers.h"
#include "workers_pool.h"

workers_pool* worker_pool;

void
evworkers_pool_init(void) {
  int workers_num = paxos_config.num_threads_in_pool;
  worker_pool = create_pool(workers_num);
}

void
evworkers_destroy_pool(void) {
  workers_pool* aux = worker_pool;
  worker_pool = NULL;
  free_pool(aux);
}

void
evworker_add_work(struct kthread_work *work) {
  add_work(worker_pool, work);
}