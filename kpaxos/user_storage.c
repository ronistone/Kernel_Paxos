//
// Created by roni on 30/09/19.
//

#include "user_storage_commons.h"

static const char *read_device_path, *write_device_path;
static pthread_t read_thread, write_thread;
static struct lmdb_storage lmdbStorage;

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

static void* generic_storage_thread(void* param) {
  int isRead = *((int*)param);
  const char *device_path = get_device_path(isRead);

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
        // LOG(isRead, "No read event");
      }
    }
    printf("\n\n===================================\n\n");
    printf("%d Messages received!\n", countMessages);
    printf("\n===================================\n\n");
//    lmdb_storage_close( &lmdbStorage);
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
    if (lmdb_storage_open( &lmdbStorage, mdb_nosync ) != 0) {
      printf("Fail to open storage");
      return;
    }

    pthread_create(&read_thread, NULL, generic_storage_thread, &READ);
    pthread_create(&write_thread, NULL, generic_storage_thread, &WRITE);

    pthread_join(read_thread, NULL);
    pthread_join(write_thread,NULL);

    printf("\n\n===================================\n\n");
    printf("%d Read Messages!\n", readCount);
    printf("%d Read Miss Messages!\n", readMissCount);
    printf("%d write received!\n", writeCount);
    printf("\n===================================\n\n");
    lmdb_storage_close( &lmdbStorage);
}

int main(int argc, char *argv[]) {
  check_args(argc, argv);
  if (read_device_path == NULL || write_device_path == NULL)
    usage(argv[0]);
  signal(SIGINT, stop_execution);


  run();
  return 0;
}
