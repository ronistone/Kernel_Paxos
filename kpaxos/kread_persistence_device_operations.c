//
// Created by roni on 07/10/19.
//

#include "read_persistence_device_operations.h"
#include "paxos_types.h"
#include "paxos.h"


paxos_kernel_device readPersistenceDevice_;


int read_persistence_open(struct inode *inodep, struct file *filep) {
  paxos_log_debug( "Mutex Address %p", &(readPersistenceDevice_.char_mutex));
  if (!mutex_trylock(&(readPersistenceDevice_.char_mutex))) {
    paxos_log_error("Read Persistence Device char: Device used by another process");
    return -EBUSY;
  }
  return 0;
}

// returns 0 if it has to stop, >0 when it reads something, and <0 on error
ssize_t read_persistence_read(struct file *filep, char *buffer, size_t len,
                               loff_t *offset) {
  int error_count_accepted, error_count_buffer_id;
  size_t llen;

  if (!readPersistenceDevice_.working)
    return 0;

  llen = sizeof(paxos_accepted) + readPersistenceDevice_.msg_buf[readPersistenceDevice_.first_buf]->value.paxos_value_len;
  error_count_buffer_id = copy_to_user(buffer, &readPersistenceDevice_.first_buf, sizeof(int));
  error_count_accepted = copy_to_user(&buffer[sizeof(int)], (char *)(readPersistenceDevice_.msg_buf[readPersistenceDevice_.first_buf]), llen);
  llen += sizeof(int);


  paxos_log_debug("Read Persistence Device char: read %zu bytes!", llen);

  paxos_log_info("READ REQUEST BUFFER ID: %d", readPersistenceDevice_.first_buf);
  if (error_count_accepted != 0 || error_count_buffer_id != 0) {
    paxerr("send fewer characters to the user");
    return -1;
  } else {
    readPersistenceDevice_.first_buf = (readPersistenceDevice_.first_buf + 1) % BUFFER_SIZE;
//    atomic_dec(&(readPersistenceDevice_.used_buf));
  }
  return llen;
}

ssize_t read_persistence_write(struct file *filep, const char *buffer, size_t len,
                                loff_t *offset) {
  if (readPersistenceDevice_.working == 0)
    return -1;

  int error_count_accepted, error_count_buffer_id, error_count_value = 0;
  int buffer_id;

  error_count_buffer_id = copy_from_user(&buffer_id, buffer, sizeof(int));

  if(error_count_buffer_id != 0) {
    goto error;
  }

  paxos_log_info("READ RESPONSE TO BUFFER ID: %d", buffer_id);

  kernel_device_callback* callback = readPersistenceDevice_.callback_buf[buffer_id];
  paxos_accepted* accepted = callback -> response;
  clearPaxosAccepted(callback -> response);

  char* paxos_val = accepted -> value.paxos_value_val;
  error_count_accepted  = copy_from_user(accepted, &buffer[sizeof(int)], sizeof(paxos_accepted));
  accepted -> value.paxos_value_val = paxos_val;

  if(error_count_accepted != 0) {
    goto error;
  }

  if(accepted -> value.paxos_value_len > 0) {
    error_count_value = copy_from_user(accepted -> value.paxos_value_val, &buffer[sizeof(int) + sizeof(paxos_accepted)], accepted -> value.paxos_value_len);
  }

//  readPersistenceDevice_.first_buf = (readPersistenceDevice_.first_buf + 1) % BUFFER_SIZE;
  atomic_dec(&(readPersistenceDevice_.used_buf));

  print_paxos_accepted(accepted, "READ RESPONSE");

  if ( error_count_value != 0 ) {
    goto error;
  }

  if(callback != NULL && !callback -> is_done) {
      callback->response = accepted;
      wake_up(&(callback -> response_wait));
  }
  return len;

  error:
  paxerr("receive fewer characters from the user");
  return -1;

}

unsigned int read_persistence_poll(struct file *file, poll_table *wait) {
  poll_wait(file, &(readPersistenceDevice_.access_wait), wait);
  if (atomic_read(&(readPersistenceDevice_.used_buf)) > 0)
    return POLLIN;

  return 0;
}

int read_persistence_release(struct inode *inodep, struct file *filep) {
  mutex_unlock(&(readPersistenceDevice_.char_mutex));
  atomic_set(&(readPersistenceDevice_.used_buf), 0);
  readPersistenceDevice_.current_buf = 0;
  readPersistenceDevice_.first_buf = 0;
  paxos_log_info("Read Persistence Device char: Device successfully closed");
  return 0;
}

kernel_device_callback* read_persistence_add_message(const char* msg, size_t size) {
  if (atomic_read(&(readPersistenceDevice_.used_buf)) >= BUFFER_SIZE) {
    //if (printk_ratelimit())
      paxos_log_debug("Read Persistence Buffer is full! Lost a value");
    return NULL;
  }
  atomic_inc(&(readPersistenceDevice_.used_buf));
  paxos_log_debug("READ: MESSAGE[%zu] -> %d", size, ((paxos_accepted*)msg)->iid);
  // BEGIN Save paxos accepted
  memset(readPersistenceDevice_.msg_buf[readPersistenceDevice_.current_buf], 0, sizeof(paxos_accepted));
  memcpy(readPersistenceDevice_.msg_buf[readPersistenceDevice_.current_buf], msg, size);

  // END Save paxos accepted

  kernel_device_callback* callback = readPersistenceDevice_.callback_buf[readPersistenceDevice_.current_buf];
  callback -> buffer_id = readPersistenceDevice_.current_buf;
  callback -> iid = readPersistenceDevice_.msg_buf[readPersistenceDevice_.current_buf] -> iid;
  callback -> is_done = 0;

  paxos_log_debug("Added %d message", readPersistenceDevice_.current_buf);
  readPersistenceDevice_.current_buf = (readPersistenceDevice_.current_buf + 1) % BUFFER_SIZE;

  wake_up_interruptible(&(readPersistenceDevice_.access_wait));
  return callback;
}


paxos_kernel_device* createReadPersistenceDevice(void) {

  readPersistenceDevice_.msg_buf = NULL;
  readPersistenceDevice_.charClass = NULL;
  readPersistenceDevice_.charDevice = NULL;
  readPersistenceDevice_.callback_buf = NULL;

  readPersistenceDevice_.fops.owner = THIS_MODULE;
  readPersistenceDevice_.fops.open = read_persistence_open;
  readPersistenceDevice_.fops.read = read_persistence_read;
  readPersistenceDevice_.fops.write = read_persistence_write;
  readPersistenceDevice_.fops.release = read_persistence_release;
  readPersistenceDevice_.fops.poll = read_persistence_poll;

  return &readPersistenceDevice_;
}