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

static int stop = 0, verbose = 0, mdb_nosync = 0;
static int READ = 1;
static int WRITE = 0;
static const char *read_device_path, *write_device_path;
static pthread_t read_thread, write_thread;

const char *get_device_path(int isRead);
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

static paxos_accepted* storage_get(struct lmdb_storage lmdbStorage, uint32_t id){

  if(lmdb_storage_tx_begin( &lmdbStorage) != 0){
    printf("Fail to open transaction!\n");
    return NULL;
  }
  paxos_accepted* out = malloc(sizeof(paxos_accepted));
  memset(out, 0, sizeof(paxos_accepted));
  if(lmdb_storage_get( &lmdbStorage, id, out)!=1){
//    printf("Fail to get in storage!\n");
    lmdb_storage_tx_abort( &lmdbStorage);
    goto error_storage_get;
  }

  if(lmdb_storage_tx_commit( &lmdbStorage) != 0){
    printf("Fail to commit transaction!\n");
    goto error_storage_get;
  }
  return out;

  error_storage_get:
  free(out);
  return NULL;
}

static int storage_put(struct lmdb_storage lmdbStorage, paxos_accepted* accepted){
  if(lmdb_storage_tx_begin( &lmdbStorage) != 0){
    printf("Fail to open transaction!\n");
    return 1;
  }

  if(lmdb_storage_put( &lmdbStorage, accepted)!=0){
    printf("Fail to put in storage!\n");
    lmdb_storage_tx_abort( &lmdbStorage);
  }

  if(lmdb_storage_tx_commit( &lmdbStorage) != 0){
    printf("Fail to commit transaction!\n");
    return 1;
  }

  return 0;
}

static void process_write_message(struct lmdb_storage lmdbStorage, storage_value* message){
  storage_put(lmdbStorage, message -> value);
}

static void process_read_message(struct lmdb_storage lmdbStorage, storage_value* message, int fd){
  LOG(READ, "buscando no lmdb");
  paxos_accepted* out = storage_get(lmdbStorage, message -> value -> iid);
  LOG(READ, "busca no lmdb terminada");
  storage_value out_message;
  out_message.bufferId = message -> bufferId;
  out_message.value = out;
  if(out != NULL) {
    LOG(READ, "found %d, sending to LKM\n", out -> iid);
    char* msg = storage_value_to_buffer(&out_message);
    write(fd, msg, sizeof(int) + sizeof(paxos_accepted) + out->value.paxos_value_len);
    free(msg);
  } else {
    LOG(READ, "not found %d, sending not found to LKM\n", message -> value -> iid);
    out = malloc(sizeof(paxos_accepted));
    memset(out, 0, sizeof(paxos_accepted));
    out_message.value = out;
    char* msg = storage_value_to_buffer(&out_message);
    write(fd, msg, sizeof(int) + sizeof(paxos_accepted) + out->value.paxos_value_len);
    free(msg);
  }
  if(out -> value.paxos_value_val != NULL){
    free( out -> value.paxos_value_val );
  }
  free(out);
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
  int fd, bufferId;
  struct pollfd polling;
  char *recv;
  paxos_accepted* accepted;
  storage_value* message;
  int len = 0;

  recv = malloc(sizeof(paxos_accepted) + (sizeof(char) * MAX_PAXOS_VALUE_SIZE));
  accepted = malloc(sizeof(paxos_accepted) + (sizeof(char) * MAX_PAXOS_VALUE_SIZE));

  LOG(isRead, "Open Device -> %s", device_path);
  fd = open(device_path, O_RDWR | O_NONBLOCK, 0);

  int countMessages = 0;
  if (fd > 0) {
    polling.fd = fd;
    polling.events = POLLIN;
    while (!stop) {
      LOG(isRead, "Check write device...");
      poll(&polling, 1, 2000);
      if (polling.revents & POLLIN) {

        len = read(fd, recv, WHATEVER_VALUE);

        if (len) {

          memcpy(&bufferId, recv, sizeof(int));
          buffer_to_paxos_accepted(&recv[sizeof(int)], accepted);
          message = malloc(sizeof(storage_value));
          message -> bufferId = bufferId;
          message -> value = accepted;

          if( isRead ){
            LOG(isRead, "Got this: bufferId[%d]: %d", bufferId, accepted->iid);
          } else {
            LOG(isRead, "Got this: bufferId[%d]: %d  --> %s   -> %p", bufferId, accepted->iid, accepted->value.paxos_value_val, accepted->value.paxos_value_val);
          }
          countMessages++;
          if(isRead){
            process_read_message(lmdbStorage, message, fd);
          } else {
            process_write_message(lmdbStorage, message);
          }
          free(message -> value -> value.paxos_value_val);
          free(message);
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
