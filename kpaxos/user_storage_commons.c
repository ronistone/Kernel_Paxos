#include "user_storage_commons.h"


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
   printf("Fail to get in storage!\n");
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

void process_read_message(struct lmdb_storage lmdbStorage, char* message, size_t len, int fd){
  LOG(READ, "buscando no lmdb");

  paxos_accepted* accepted_to_read = (paxos_accepted*)&message[sizeof(int)];
  paxos_accepted* accepted_response;
  char out_message[max_message_size];
  char out[max_message_size - sizeof(int)];

//  memcpy(&iid, &message[sizeof(int) + sizeof(uint32_t)], sizeof(uint32_t));
  LOG(READ, "===> searching for %u", accepted_to_read->iid);
  int response = storage_get(lmdbStorage, accepted_to_read->iid, out);

  LOG(READ, "busca no lmdb terminada");

  memcpy(out_message, message, sizeof(int));
  memcpy(&out_message[sizeof(int)], out, max_message_size - sizeof(int));

  ++readCount;
  if(response != 0) {
    accepted_response = (paxos_accepted*) out;
    log_found(accepted_response, out);
    write(fd, out_message, sizeof(int) + sizeof(paxos_accepted) + accepted_response->value.paxos_value_len);
  } else {
    ++readMissCount;
    LOG(READ, "not found %u, sending not found to LKM\n", accepted_to_read->iid);
    memset(&out_message[sizeof(int)], 0, sizeof(paxos_accepted));
    write(fd, out_message, sizeof(int) + sizeof(paxos_accepted));
  }
}