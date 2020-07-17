//
// Created by roni on 30/09/19.
//

#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <linux/if_ether.h>
//#include "paxos.h"
#include "kernel_client.h"
#include "lmdb_operations.h"

#define MAX_PAXOS_VALUE_SIZE ETH_DATA_LEN
#define WHATEVER_VALUE 0
#define LOG(isRead, fmt, args...)                                                \
  verbose? isRead? printf("READ: " fmt "\n", ##args): printf("WRITE: " fmt "\n", ##args): WHATEVER_VALUE

static const size_t max_message_size =
  sizeof(paxos_accepted) + (sizeof(char) * MAX_PAXOS_VALUE_SIZE);
static int stop = 0, verbose = 0, mdb_nosync = 0;
static int READ = 1;
static int WRITE = 0;
static const char *read_device_path, *write_device_path;
static pthread_t read_thread, write_thread;

const char *get_device_path(int isRead);
void        log_found(paxos_accepted* out, char* message);
static void usage(const char *name) {
  printf("Usage: %s [options] \n", name);
  printf("Options:\n");
  printf("  %-30s%s\n", "-h, --help", "Output this message and exit");
  printf("  %-30s%s\n", "-r, --read-chardev_path #", "Kernel paxos lmdb query kernel device path");
  printf("  %-30s%s\n", "-w, --write-chardev_path #", "Kernel paxos lmdb write kernel device path");
  printf("  %-30s%s\n", "-v, --verbose", "Verbose execution");
  printf("  %-30s%s\n", "-m, --mdb-nosync", "Using lmdb nosync mode");
  exit(1);
}

static void check_args(int argc, char *argv[]) {
  int opt = 0, idx = 0;
  static struct option options[] = {{"write-chardev-path", required_argument, 0, 'w'},
                                    {"read-chardev-path", required_argument, 0, 'r'},
                                    {"help", no_argument, 0, 'h'},
                                    {"verbose", no_argument, 0, 'v'},
                                    {"mdb-nosync", no_argument, 0, 'm'},
                                    {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "w:r:hvm", options, &idx)) != -1) {
    switch (opt) {
    case 'w':
      write_device_path = optarg;
      break;
    case 'r':
      read_device_path = optarg;
      break;
    case 'v':
      verbose = 1;
      break;
    case 'm':
      mdb_nosync = 1;
      break;
    default:
      usage(argv[0]);
    }
  }
}

static void stop_execution(int signal) {
  stop = 1;
  printf("Finishing Threads...\n");
}

static int storage_get(struct lmdb_storage lmdbStorage, uint32_t id, char* out){

  if(lmdb_storage_tx_begin( &lmdbStorage) != 0){
    printf("Fail to open transaction!\n");
    return 0;
  }
  if(lmdb_storage_get( &lmdbStorage, id, out)!=1){
//    printf("Fail to get in storage!\n");
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

static int storage_put(struct lmdb_storage lmdbStorage, uint32_t id, char* message, size_t len){
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

static void process_write_message(struct lmdb_storage lmdbStorage, char* message, int len){
  paxos_accepted* accepted_to_write = (paxos_accepted*)&message[sizeof(int)];

//  memcpy(&iid, &message[sizeof(int) + sizeof(uint32_t)], sizeof(uint32_t));
  if(verbose) {
    int value_size;
    memcpy(&value_size, &message[sizeof(int) + (6* sizeof(uint32_t))], sizeof(int));
    int i;
    // TODO descobrir porque esse valor está vindo com valor invalido
    LOG(READ, "Putting %u in the storage with size = %d", accepted_to_write->iid, accepted_to_write->value.paxos_value_len);
    for(i=0;i< sizeof(int) + len;i++){
      printf("%u ", message[i]);
    }
    for(i=0;i< sizeof(int) + len;i++){
      printf("%c ", message[i]);
    }
    printf("\n");
  }
  storage_put(lmdbStorage, accepted_to_write->iid, &message[sizeof(int)], len);
}

static void process_read_message(struct lmdb_storage lmdbStorage, char* message, size_t len, int fd){
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

  if(response != 0) {
    accepted_response = (paxos_accepted*) out;
    log_found(accepted_response, out);
    write(fd, out_message, sizeof(int) + sizeof(paxos_accepted) + accepted_response->value.paxos_value_len);
  } else {
    LOG(READ, "not found %u, sending not found to LKM\n", accepted_to_read->iid);
    memset(&out_message[sizeof(int)], 0, sizeof(paxos_accepted));
    write(fd, out_message, sizeof(int) + sizeof(paxos_accepted));
  }
}
void
log_found(paxos_accepted* out, char* message)
{
  if(verbose) {
    int i;
    int len = sizeof(paxos_accepted) + out -> value.paxos_value_len;
    printf("\n=============================================================\n");
    LOG(READ, "found %u, sending to LKM\n", out->iid);

    LOG(READ, "Reading %u in the storage with size = %d", out->iid, out->value.paxos_value_len);
    for(i=0;i< sizeof(int) + len;i++){
      printf("%u, ", message[i]);
    }
    for(i=0;i< sizeof(int) + len;i++){
      printf("%c, ", message[i]);
    }
    printf("\n=============================================================\n");
  }
}

static void* generic_storage_thread(void* param) {
  int isRead = *((int*)param);
  const char *device_path = get_device_path(isRead);

  struct lmdb_storage lmdbStorage;
  if (lmdb_storage_open( &lmdbStorage, mdb_nosync ) != 0) {
    LOG(isRead, "Fail to open storage");
    pthread_exit(0);
    return NULL;
  }
  int fd;
  struct pollfd polling;
  char *recv;
  paxos_accepted* accepted;
  int len = 0;

  recv = malloc(max_message_size);
  accepted = malloc(max_message_size);

  LOG(isRead, "Open Device -> %s", device_path);
  fd = open(device_path, O_RDWR | O_NONBLOCK, 0);

  int countMessages = 0;
  if (fd > 0) {
    polling.fd = fd;
    polling.events = POLLIN;
  while (!stop) {
//      LOG(isRead, "Check device...");
      poll(&polling, 1, 2000);
//      LOG(isRead, "exit poll");
      if (polling.revents & POLLIN) {

        len = read(fd, recv, WHATEVER_VALUE);

        if (len) {
          countMessages++;
          if(isRead){
            process_read_message(lmdbStorage, recv, len - sizeof(int), fd);
          } else {
            process_write_message(lmdbStorage, recv, len - sizeof(int));
          }
        }
      } else {
        LOG(isRead, "No read event");
      }
    }
    printf("\n\n===================================\n\n");
    printf("%d Messages received!\n", countMessages);
    printf("\n===================================\n\n");
    lmdb_storage_close( &lmdbStorage);
    close(fd);
  } else {
    LOG(isRead, "Error while opening the write storage chardev.");
  }
  free(recv);
  free(accepted);

  pthread_exit(0);
}
const char *get_device_path(int isRead) {
  const char* device_path;
  if (isRead) {
    device_path = read_device_path;
  } else {
    device_path = write_device_path;
  }
  return device_path;
}

static void run(){

    pthread_create(&read_thread, NULL, generic_storage_thread, &READ);
    pthread_create(&write_thread, NULL, generic_storage_thread, &WRITE);

    pthread_join(read_thread, NULL);
    pthread_join(write_thread,NULL);
}

int main(int argc, char *argv[]) {
  check_args(argc, argv);
  if (read_device_path == NULL || write_device_path == NULL)
    usage(argv[0]);
  signal(SIGINT, stop_execution);


  run();
  return 0;
}
