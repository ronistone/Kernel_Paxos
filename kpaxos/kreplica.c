#include "evpaxos.h"
#include "evworkers.h"
#include "klearner_device_operations.h"
#include "kernel_device.h"
#include <asm/atomic.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/udp.h>
#include <net/sock.h>

const char* MOD_NAME = "KReplica";

static int id = 0;
module_param(id, int, S_IRUGO);
MODULE_PARM_DESC(id, "The replica id, default 0");

static char* if_name = "enp4s0";
module_param(if_name, charp, 0000);
MODULE_PARM_DESC(if_name, "The interface name, default enp4s0");

static char* path = "./paxos.conf";
module_param(path, charp, S_IRUGO);
MODULE_PARM_DESC(path, "The config file position, default ./paxos.conf");

static struct evpaxos_replica* replica = NULL;
static paxos_kernel_device* kleaner_device = NULL;
static struct evpaxos_config* config;

void
deliver(unsigned iid, char* value, size_t size, void* arg)
{
  kset_message(value, size);
}

static void
process_start(struct kthread_work* work) {

  LOG_INFO("[%s] execute process_start\n", current -> comm);
  kleaner_device = create_kleaner_device();
  kdevchar_init(id, "klearner", kleaner_device);
  replica = evpaxos_replica_init(id, deliver, NULL, if_name, config);

  if (replica == NULL) {
    LOG_ERROR("Could not start the replica!");
  }

  vfree(work);
}

static void
start_replica(void)
{
  config = evpaxos_config_read(path);
  evworkers_pool_init();

  struct kthread_work *work = vmalloc(sizeof(struct kthread_work));
  init_kthread_work(work, process_start);

  evworker_add_work(work);
}

static int __init
           init_replica(void)
{
  if (id < 0) {
    LOG_ERROR("you must give a valid id!");
    return 0;
  }
  start_replica();
  LOG_INFO("Module loaded");
  return 0;
}

static void __exit
            replica_exit(void)
{
  evworkers_destroy_pool();
  kdevchar_exit(kleaner_device);
  if (replica != NULL) {
    evpaxos_replica_free(replica);
  }
  LOG_INFO("Module unloaded");
}

module_init(init_replica);
module_exit(replica_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
