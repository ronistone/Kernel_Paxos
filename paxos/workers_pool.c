//
// Created by roni on 12/02/2020.
//

#include "workers_pool.h"
#include <linux/kthread.h>
#include <linux/err.h>

static int init_workers(workers_pool *pool);

static void free_workers(workers_pool *pool);

static void free_workers_threads(workers_pool *pool);

void free_pool(workers_pool *pool);

static int init_workers_threads(workers_pool *pool);

static workers_pool *___pool_error_exit(workers_pool *pool, const char* message);

workers_pool* create_pool(int num_workers){

  int error = 0;
  workers_pool* pool = vmalloc(sizeof(workers_pool));


  if(pool == NULL) {
    return ___pool_error_exit(pool, "ERROR: Fail to alloc workers_pool!\n");
  }

  pool -> current_worker = 0;
  pool -> num_workers = num_workers;

  error = init_workers(pool);

  if(error != 0) {
    return ___pool_error_exit(pool, "ERROR: Fail to init Workers!\n");
  }

  error = init_workers_threads(pool);

  if(error != 0) {
    return ___pool_error_exit(pool, "ERROR: Fail to init Workers!\n");
  }

  return pool;
}

static int init_workers(workers_pool *pool) {
  int i;
  pool -> worker = vmalloc(sizeof(struct kthread_worker*) * pool -> num_workers);

  if(pool -> worker == NULL) {
    printk("ERROR: Fail to alloc %d workers from pool!\n", pool -> num_workers);
    return 1;
  }

  for (i = 0; i < pool -> num_workers; i++ ) {
    pool -> worker[i] = vmalloc(sizeof(struct kthread_worker));

    if(pool -> worker[i] == NULL) {
      printk("ERROR: Fail to alloc worker[%d] on the pool!\n", i);
      return 2;
    }
    init_kthread_worker( pool -> worker[i] );
  }
  return 0;
}

static int init_workers_threads(workers_pool *pool) {
  int i;
  pool -> workerThread = vmalloc(sizeof(struct task_struct*) * pool -> num_workers);

  if( pool -> workerThread == NULL ) {
    printk("ERROR: Fail to alloc Workers Threads\n");
    free_pool(pool);
    return 1;
  }

  for( i = 0; i < pool -> num_workers; i++ ) {
    pool -> workerThread[i] = kthread_run(kthread_worker_fn, pool -> worker[i], "worker-%d", i);
    if(IS_ERR(pool -> workerThread[i])) {
      printk("ERROR: Fail to create worker thread\n");
      free_pool(pool);
      return 2;
    }
  }
  return 0;
}

workers_pool *___pool_error_exit(workers_pool *pool, const char* message) {
  printk(message);
  free_pool(pool);
  return NULL;
}

void free_pool(workers_pool *pool) {
  if( pool != NULL ) {
    free_workers_threads(pool);
    free_workers(pool);
    vfree(pool);
  }
}

void free_workers_threads(workers_pool *pool) {
  int i = 0;
  if( pool -> workerThread != NULL ) {
    for ( i = 0; i < pool -> num_workers; i++ ) {
      if( pool -> workerThread[i] != NULL ) {
        kthread_stop(pool -> workerThread[i]);
      }
    }
    vfree(pool -> workerThread);
  }
}

void free_workers(workers_pool *pool) {
  int i = 0;
  if( pool -> worker != NULL ) {
    for (i = 0; i < pool -> num_workers; i++) {
      if (pool->worker[i] != NULL) {
        vfree(pool->worker[i]);
      }
    }
    vfree(pool->worker);
  }
}

void add_work(workers_pool* pool, struct kthread_work* work){

  if( pool != NULL && pool -> worker[pool -> current_worker] != NULL ) {
//        printk("\nQueue in the worker %d and the state is %ld\n\n", pool -> current_worker, pool -> worker[pool -> current_worker] -> task -> state);
    queue_kthread_work(pool -> worker[pool -> current_worker], work);
    pool->current_worker = (pool->current_worker + 1) % pool->num_workers;
  } else {
    printk("ERROR: workers pool is NULL!\n");
  }

}

persistence_work* createPersistenceWork(void* func_work) {
  persistence_work *persistenceWork = vmalloc(sizeof(persistence_work));

  if( persistenceWork != NULL ) {
    init_kthread_work(&(persistenceWork->work), func_work);
  }

  return persistenceWork;
}