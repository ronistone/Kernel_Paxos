//
// Created by roni on 3/26/20.
//

//#include "storage_lmdb.h"
#include "read_persistence_device_operations.h"
#include "storage.h"
#include "write_persistence_device_operations.h"
#include <message.h>

static paxos_kernel_device *write_device = NULL;
static paxos_kernel_device *read_device = NULL;
static int write_device_id = 0;
static int read_device_id = 0;



static void
create_devices(void)
{
  read_device = createReadPersistenceDevice();
  kdevchar_init(read_device_id, "storage-read", read_device);

  write_device = createWritePersistenceDevice();
  kdevchar_init(write_device_id, "storage-write", write_device);
}

static int
lmdb_storage_open(void* handle)
{
  return 0;
}

static void
lmdb_storage_close(void* handle)
{
  kdevchar_exit(write_device);
  kdevchar_exit(read_device);
}

static int
lmdb_storage_tx_begin(void* handle)
{
  return 0;
}

static int
lmdb_storage_tx_commit(void* handle)
{
  return 0;
}

static void
lmdb_storage_tx_abort(void* handle)
{}

static int
lmdb_storage_get(void* handle, iid_t iid, paxos_accepted* out)
{
  paxos_accepted accepted_query;
  memset(&accepted_query, 0, sizeof(paxos_accepted));
  accepted_query.iid = iid;

  size_t len = sizeof(paxos_accepted);
  char buffer[len];

  paxos_accepted_to_char_array(&accepted_query, buffer);

  kernel_device_callback* callback = read_persistence_add_message(buffer, len);
  if (callback == NULL) {
    paxos_log_debug("Read Buffer full!\n");
    return 0;
  }

  int wait_response = wait_event_timeout(callback->response_wait, callback->response != NULL, paxos_config.storage_wait_timeout);
  //if (printk_ratelimit()) {
    if (wait_response == 0) {
//      paxos_log_info("Wait Response: Timeout and condition is false");
    } else if (wait_response == 1) {
//      paxos_log_info("Wait Response: Timeout and condition is true");
    } else if(callback->response != NULL) {
//      paxos_log_info("Wait Response: Condition is true with time left = %d", wait_response);
    } else {
      paxos_log_error("Wait Response: Condition is false with time left = %d", wait_response);
    }
  //}

  if (!callback -> is_done &&
      callback -> response -> iid == iid) {

      memcpy(out, callback->response, sizeof(paxos_accepted));
      if (out->value.paxos_value_len > 0) {
        out->value.paxos_value_val = pmalloc(out->value.paxos_value_len);
        memcpy(out->value.paxos_value_val, callback->response->value.paxos_value_val, out->value.paxos_value_len);
        paxos_log_debug("Paxos_accepted %d - [%d] -> {%d} = %s\n", callback -> buffer_id, callback->response->iid,
                        callback->response->value.paxos_value_len, callback->response->value.paxos_value_val);
      } else {
        paxos_log_debug("Paxos_accepted %d - [%d] -> Without message value\n",
                        callback->buffer_id, callback->response->iid);
      }

      callback -> is_done = 1;
      return 1;
  } else {
    paxos_log_debug("is_done: %d,  response->iid: %u,  iid: %u,  value_len: %d",
      callback -> is_done,
      callback -> response -> iid,
      iid,
      callback->response->value.paxos_value_len
    );
    paxos_log_debug("Paxos_accepted %d - [%d] -> no Message", callback->buffer_id, callback->iid);
  }

  callback -> is_done = 1;

  return 0;
}

static int
lmdb_storage_put(void* handle, paxos_accepted* acc)
{
  if(acc == NULL) {
    return 1;
  }

  int len = sizeof(paxos_accepted) + acc->value.paxos_value_len;
  char buffer[len];

  paxos_accepted_to_char_array(acc, buffer);

  kernel_device_callback* callback = write_persistence_add_message(buffer, len);
  if (callback == NULL) {
    paxos_log_debug("Write Buffer full!\n");
    return 1;
  }

  // TODO validate if put need to validate success
  int wait_response = wait_event_timeout(callback->response_wait, callback->response != NULL, paxos_config.storage_wait_timeout);
  //if (printk_ratelimit()) {
  if (wait_response == 0) {
    paxos_log_info("Wait Response: Timeout and condition is false");
  } else if (wait_response == 1) {
    paxos_log_info("Wait Response: Timeout and condition is true");
  } else {
    paxos_log_debug("Wait Response: Condition is true with time left = %d", wait_response);
  }
  //}

  callback -> is_done = 1;

  return 0;
}

static int
lmdb_storage_trim(void* handle, iid_t iid)
{
  return 0;
}

static iid_t
lmdb_storage_get_trim_instance(void* handle)
{
  return 0;
}


void
storage_init_lmdb(struct storage* s, int acceptor_id) {

  create_devices();

//  s->handle = lmdb_storage_new(acceptor_id);
  s->api.open = lmdb_storage_open;
  s->api.close = lmdb_storage_close;
  s->api.tx_begin = lmdb_storage_tx_begin;
  s->api.tx_commit = lmdb_storage_tx_commit;
  s->api.tx_abort = lmdb_storage_tx_abort;
  s->api.get = lmdb_storage_get;
  s->api.put = lmdb_storage_put;
  s->api.trim = lmdb_storage_trim;
  s->api.get_trim_instance = lmdb_storage_get_trim_instance;

}
