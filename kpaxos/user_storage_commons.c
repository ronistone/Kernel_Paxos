#include "user_storage_commons.h"

int batch_size = 1;
int readCount = 0;
int readMissCount = 0;
int writeCount = 0;
int stop = 0, verbose = 0, mdb_nosync = 0;
int READ = 1;
int WRITE = 0;
const size_t max_message_size =
  sizeof(paxos_accepted) + (sizeof(char) * MAX_PAXOS_VALUE_SIZE);

void
log_found(paxos_accepted* out, char* message)
{
  if(verbose) {
    printf("\n=============================================================\n");
    LOG(READ, "found %u, sending to LKM\n", out->iid);
  }
}

int storage_get(struct lmdb_storage lmdbStorage, uint32_t id, char* out){

  if(lmdb_storage_tx_begin( &lmdbStorage) != 0){
    printf("Fail to open transaction!\n");
    return 0;
  }
  if(lmdb_storage_get( &lmdbStorage, id, out)!=1){
//   printf("Fail to get in storage!\n");
    lmdb_storage_tx_abort( &lmdbStorage);
    goto error_storage_get;
  }

  if(lmdb_storage_tx_commit( &lmdbStorage) != 0){
    printf("Fail to commit transaction!\n");
    goto error_storage_get;
  }
  return 1;

  error_storage_get:
  return 0;
}

int storage_put(struct lmdb_storage lmdbStorage, uint32_t id, char* message, size_t len){
  if(lmdb_storage_tx_begin( &lmdbStorage) != 0){
    printf("Fail to open transaction!\n");
    return 1;
  }

  if(lmdb_storage_put( &lmdbStorage, id, message, len)!=0){
    printf("Fail to put in storage!\n");
    lmdb_storage_tx_abort( &lmdbStorage);
  }

  if(lmdb_storage_tx_commit( &lmdbStorage) != 0){
    printf("Fail to commit transaction!\n");
    return 1;
  }

  return 0;
}

void process_write_message(struct lmdb_storage lmdbStorage, char* message, int len){
  paxos_accepted* accepted_to_write = (paxos_accepted*)&message[sizeof(int)];

//  memcpy(&iid, &message[sizeof(int) + sizeof(uint32_t)], sizeof(uint32_t));
  if(verbose) {
    int value_size;
    memcpy(&value_size, &message[sizeof(int) + (6* sizeof(uint32_t))], sizeof(int));
    LOG(WRITE, "Putting %u in the storage with size = %d", accepted_to_write->iid, accepted_to_write->value.paxos_value_len);
  }
  ++writeCount;
  storage_put(lmdbStorage, accepted_to_write->iid, &message[sizeof(int)], len);
}

void process_read_message(struct lmdb_storage lmdbStorage, char* message, size_t num_msgs, int fd){
  LOG(READ, "buscando no lmdb");

  char out[max_message_size - sizeof(int)];
  char out_message[num_msgs * max_message_size];
  int buffer_index = 0;
  int response_index = 0;
  int buffer_id;
  paxos_accepted accepted_to_read, accepted_response;

  for(int i = 0; i < num_msgs; i++) {
    
    memcpy(&buffer_id, &message[buffer_index], sizeof(int));  // read buffer id
    buffer_index += sizeof(int);

    memcpy(&accepted_to_read, &message[buffer_index], sizeof(paxos_accepted));  // read message
    buffer_index += sizeof(paxos_accepted);

    LOG(READ, "===> searching for %u", accepted_to_read.iid);
    int response = storage_get(lmdbStorage, accepted_to_read.iid, out);

    LOG(READ, "busca no lmdb terminada");

    memcpy(&out_message[response_index], &buffer_id, sizeof(int)); // save buffer id
    response_index += sizeof(int);

    ++readCount;
    if(response != 0) {
      memcpy(&out_message[response_index], out, sizeof(paxos_accepted)); // save message headers
      response_index += sizeof(paxos_accepted);

      memcpy(&accepted_response, out, sizeof(paxos_accepted));

      if(accepted_response.value.paxos_value_len > 0) { // save message payload
        memcpy(&out_message[response_index], &out[sizeof(paxos_accepted)], accepted_response.value.paxos_value_len);
        response_index += accepted_response.value.paxos_value_len;
      }

      log_found(&accepted_response, out);
    } else {
      ++readMissCount;
      LOG(READ, "not found %u, sending not found to LKM\n", accepted_to_read.iid);
      memset(&out_message[response_index], 0, sizeof(paxos_accepted));
      response_index += sizeof(paxos_accepted);
    }
  }
  write(fd, out_message, num_msgs);
}