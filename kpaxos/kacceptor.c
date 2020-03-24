#include <asm/atomic.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/udp.h>
#include <net/sock.h>

#include "evpaxos.h"
#include "evworkers.h"

const char* MOD_NAME = "KAcceptor";

static char* if_name = "enp4s0";
module_param(if_name, charp, 0000);
MODULE_PARM_DESC(if_name, "The interface name, default enp4s0");

static int id = 0;
module_param(id, int, S_IRUGO);
MODULE_PARM_DESC(id, "The acceptor id, default 0");

static char* path = "./paxos.conf";
module_param(path, charp, S_IRUGO);
MODULE_PARM_DESC(path, "The config file position, default ./paxos.conf");

static struct evacceptor* acc = NULL;
static struct evpaxos_config* config = NULL;

static void proccess_start(struct kthread_work* work){

  LOG_INFO("[%s] execute process_start\n", current -> comm);
  acc = evacceptor_init(id, if_name, config);
  if (acc == NULL) {
    LOG_ERROR("Could not start the acceptor\n");
  }

  vfree(work);
}

static void
start_acceptor(void)
{
  config = evpaxos_config_read(path);

  evworkers_pool_init();

  struct kthread_work *work = vmalloc(sizeof(struct kthread_work));
  init_kthread_work(work, proccess_start);

  evworker_add_work(work);
}

static int __init
           init_acceptor(void)
{
  start_acceptor();
  LOG_INFO("Module loaded");
  return 0;
}

static void __exit
            acceptor_exit(void)
{
  if (acc != NULL)
    evacceptor_free(acc);

  evworkers_destroy_pool();
  LOG_INFO("Module unloaded");
}

module_init(init_acceptor) module_exit(acceptor_exit) MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");
