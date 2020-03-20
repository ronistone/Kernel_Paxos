#ifndef DEVICE_INCLUDE
#define DEVICE_INCLUDE

#include <asm/atomic.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/time.h>
#include <linux/netdevice.h>
#include "workers_pool.h"
#include "paxos.h"

#define BUFFER_SIZE 1000


typedef struct kernel_device_message {
  int buffer_id;
  struct paxos_accepted* accepted;

} kernel_device_message;

struct paxos_kernel_device {
  struct mutex char_mutex;

  struct class *charClass;
  struct device *charDevice;
  wait_queue_head_t access_wait;
  char *de_name, *clas_name;
  int majorNumber, working, current_buf, first_buf;
  struct paxos_accepted** msg_buf;
  kernel_device_callback** callback_buf;
  atomic_t used_buf;
  workers_pool *pool;

  struct file_operations fops;
};

typedef struct paxos_kernel_device paxos_kernel_device;

extern void         paxerr(char *err);
extern int          kdevchar_init(int id, char* name, paxos_kernel_device* kernel_device);
extern void         kdevchar_exit(paxos_kernel_device* kernel_device);
extern void         persistence_device_destroy(paxos_kernel_device* kernel_device);

#endif
