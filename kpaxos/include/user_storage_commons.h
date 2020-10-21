

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

extern int batch_size;
extern int readCount;
extern int readMissCount;
extern int writeCount;
extern int stop, verbose, mdb_nosync;
extern int READ;
extern int WRITE;
extern const size_t max_message_size;


int storage_get(struct lmdb_storage lmdbStorage, uint32_t id, char* out);
int storage_put(struct lmdb_storage lmdbStorage, uint32_t id, char* message, size_t len);
void process_write_message(struct lmdb_storage lmdbStorage, char* message, int len);
void process_read_message(struct lmdb_storage lmdbStorage, char* message, size_t len, int fd);
void log_found(paxos_accepted* out, char* message);