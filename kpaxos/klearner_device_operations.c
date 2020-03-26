#include "klearner_device_operations.h"
#include "kernel_client.h"
#include "paxos.h"

paxos_kernel_device klearner_device;

int
kdev_open(struct inode* inodep, struct file* filep)
{
  if (!mutex_trylock(&klearner_device.char_mutex)) {
    paxos_log_error("Device char: Device used by another process");
    return -EBUSY;
  }
  return 0;
}

void
kset_message(char* msg, size_t size)
{
  if (atomic_read(&klearner_device.used_buf) >= BUFFER_SIZE) {
    if (printk_ratelimit())
      paxos_log_error("Buffer is full! Lost a value");
  }
  set_value_pointer_ahead_of_accepted(klearner_device.msg_buf[klearner_device.current_buf]);

  paxos_log_debug("kset_message: size=%d\nmessage: %s", size, msg);
  klearner_device.msg_buf[klearner_device.current_buf]->value.paxos_value_len = size;
  memcpy(klearner_device.msg_buf[klearner_device.current_buf]->value.paxos_value_val, msg, size);
  klearner_device.current_buf = (klearner_device.current_buf + 1) % BUFFER_SIZE;
  atomic_inc(&klearner_device.used_buf);
  wake_up_interruptible(&klearner_device.access_wait);
}

// returns 0 if it has to stop, >0 when it reads something, and <0 on error
ssize_t
kdev_read(struct file* filep, char* buffer, size_t len, loff_t* offset)
{
  int error_count;

  if (!klearner_device.working) {
    return 0;
  }

  atomic_dec(&klearner_device.used_buf);

  size_t msg_len = klearner_device.msg_buf[klearner_device.first_buf]->value.paxos_value_len;
  size_t llen = sizeof(struct user_msg) + msg_len;
  error_count = copy_to_user(buffer, (char*)(&msg_len), sizeof(size_t));
  error_count += copy_to_user(&buffer[sizeof(size_t)], (char*)klearner_device.msg_buf[klearner_device.first_buf]->value.paxos_value_val, msg_len);
  paxos_log_debug("Seding message to user space: error_count=%d, sizeof(size_t)=%d, msg_len=%d\nmessage: %s",
    error_count, sizeof(size_t), msg_len,
    klearner_device.msg_buf[klearner_device.first_buf]->value.paxos_value_val);

  if (error_count != 0) {
    paxerr("send fewer characters to the user");
    return -1;
  } else
    klearner_device.first_buf = (klearner_device.first_buf + 1) % BUFFER_SIZE;

  return llen;
}

ssize_t
kdev_write(struct file* filep, const char* buffer, size_t len, loff_t* offset)
{
  if (klearner_device.working == 0)
    return -1;

  return len;
}

unsigned int
kdev_poll(struct file* file, poll_table* wait)
{
  poll_wait(file, &klearner_device.access_wait, wait);
  if (atomic_read(&klearner_device.used_buf) > 0)
    return POLLIN;

  return 0;
}

int
kdev_release(struct inode* inodep, struct file* filep)
{
  mutex_unlock(&klearner_device.char_mutex);
  // LOG_INFO("Messages left %d", atomic_read(&used_buf));
  atomic_set(&klearner_device.used_buf, 0);
  klearner_device.current_buf = 0;
  klearner_device.first_buf = 0;
  LOG_INFO("Device Char: Device successfully closed");
  return 0;
}

paxos_kernel_device* create_kleaner_device(void) {

  klearner_device.msg_buf = NULL;
  klearner_device.charClass = NULL;
  klearner_device.charDevice = NULL;
  klearner_device.callback_buf = NULL;

  klearner_device.fops.owner = THIS_MODULE;
  klearner_device.fops.open = kdev_open;
  klearner_device.fops.read = kdev_read;
  klearner_device.fops.write = kdev_write;
  klearner_device.fops.release = kdev_release;
  klearner_device.fops.poll = kdev_poll;

  return &klearner_device;
}