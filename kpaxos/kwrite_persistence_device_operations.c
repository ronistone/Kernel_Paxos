//
// Created by roni on 06/10/19.
//

#include "write_persistence_device_operations.h"
#include "paxos_types.h"
#include "paxos.h"


static paxos_kernel_device writePersistenceDevice;


int write_persistence_open(struct inode *inodep, struct file *filep) {
    paxos_log_debug( "Mutex Address %p", &(writePersistenceDevice.char_mutex));
    if (!mutex_trylock(&(writePersistenceDevice.char_mutex))) {
        paxos_log_info("Write Persistence Device char: Device used by another process");
        return -EBUSY;
    }
    return 0;
}

// returns 0 if it has to stop, >0 when it reads something, and <0 on error
ssize_t write_persistence_read(struct file *filep, char *buffer, size_t len,
                      loff_t *offset) {
    int error_count, error_count_value = 0, error_count_buffer_id;
    int buffer_used = atomic_read(&(writePersistenceDevice.used_buf));
    ssize_t num_msgs = MIN(len, buffer_used);
    int buffer_index = 0;
    kernel_device_callback* callback;
    paxos_accepted* accepted;

    if (!writePersistenceDevice.working)
        return 0;

    for(int i = 0; i < num_msgs; i++){
      callback = writePersistenceDevice.callback_buf[writePersistenceDevice.first_buf];
      accepted = writePersistenceDevice.msg_buf[writePersistenceDevice.first_buf];

      error_count_buffer_id = copy_to_user(&buffer[buffer_index], &writePersistenceDevice.first_buf, sizeof(int));
      buffer_index += sizeof(int);

      error_count = copy_to_user(&buffer[buffer_index], (char *)(accepted), sizeof(paxos_accepted) + accepted->value.paxos_value_len);
      buffer_index += sizeof(paxos_accepted) + accepted->value.paxos_value_len;

      paxos_log_debug("The message sended to buffer[%zu]: %s", accepted -> iid, &buffer[sizeof(int) + sizeof(paxos_accepted)]);

      if (error_count != 0 || error_count_buffer_id != 0 ) {
        paxos_log_error("send fewer characters to the user: error_count=%d, error_count_value=%d, error_count_buffer_id=%d",
                error_count, error_count_value, error_count_buffer_id);
        return -1;
      } else {
        writePersistenceDevice.first_buf = (writePersistenceDevice.first_buf + 1) % BUFFER_SIZE;
        atomic_dec(&(writePersistenceDevice.used_buf));
      }

      if(callback != NULL && !callback -> is_done) {
        callback->response = accepted;
        wake_up(&(callback -> response_wait));
      }
    }

    return num_msgs;
}

ssize_t write_persistence_write(struct file *filep, const char *buffer, size_t len,
                       loff_t *offset) {
  if (writePersistenceDevice.working == 0)
    return -1;

  return len;
}

unsigned int write_persistence_poll(struct file *file, poll_table *wait) {
    poll_wait(file, &(writePersistenceDevice.access_wait), wait);
    int used = atomic_read(&(writePersistenceDevice.used_buf));
    paxos_log_info("WRITE: %d messages in buffer", used);
    if (used > 0)
        return POLLIN;

    return 0;
}

int write_persistence_release(struct inode *inodep, struct file *filep) {
//  paxos_log_debug("Messages left %d", atomic_read(&used_buf));
  atomic_set(&(writePersistenceDevice.used_buf), 0);
  writePersistenceDevice.current_buf = 0;
  writePersistenceDevice.first_buf = 0;
  mutex_unlock(&(writePersistenceDevice.char_mutex));
  paxos_log_info("Write Persistence Device char: Device successfully closed");
    return 0;
}

kernel_device_callback* write_persistence_add_message(const char* msg, int size) {
  if (atomic_read(&(writePersistenceDevice.used_buf)) >= BUFFER_SIZE) {
//    if (printk_ratelimit())
      paxos_log_debug("Write Persistence Buffer is full! Lost a value");
    return NULL;
  }
  atomic_inc(&(writePersistenceDevice.used_buf));

  if(size > sizeof(paxos_accepted) + MAX_PAXOS_VALUE_SIZE) {
    paxos_log_error( "Data will be truncated.");
    size = sizeof(paxos_accepted) + MAX_PAXOS_VALUE_SIZE;
  }

  // BEGIN Save paxos accepted
  memset(writePersistenceDevice.msg_buf[writePersistenceDevice.current_buf], 0,
      sizeof(paxos_accepted) + MAX_PAXOS_VALUE_SIZE);
  memcpy(writePersistenceDevice.msg_buf[writePersistenceDevice.current_buf], msg, size);

  paxos_log_debug("The message sended to buffer: %s", &msg[sizeof(paxos_accepted)]);

  writePersistenceDevice.msg_buf[writePersistenceDevice.current_buf] -> value.paxos_value_len = size - sizeof(paxos_accepted);
  // END Save paxos accepted

  kernel_device_callback* callback = writePersistenceDevice.callback_buf[writePersistenceDevice.current_buf];
  callback -> buffer_id = writePersistenceDevice.current_buf;
  callback -> iid = writePersistenceDevice.msg_buf[writePersistenceDevice.current_buf] -> iid;
  callback -> is_done = 0;
  writePersistenceDevice.current_buf = (writePersistenceDevice.current_buf + 1) % BUFFER_SIZE;

  wake_up_interruptible(&(writePersistenceDevice.access_wait));
  return callback;
}


paxos_kernel_device* createWritePersistenceDevice(void) {

    writePersistenceDevice.msg_buf = NULL;
    writePersistenceDevice.charClass = NULL;
    writePersistenceDevice.charDevice = NULL;
    writePersistenceDevice.callback_buf = NULL;

    writePersistenceDevice.fops.owner = THIS_MODULE;
    writePersistenceDevice.fops.open = write_persistence_open;
    writePersistenceDevice.fops.read = write_persistence_read;
    writePersistenceDevice.fops.write = write_persistence_write;
    writePersistenceDevice.fops.release = write_persistence_release;
    writePersistenceDevice.fops.poll = write_persistence_poll;

    return &writePersistenceDevice;
}