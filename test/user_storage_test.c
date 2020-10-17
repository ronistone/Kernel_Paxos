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

#include "user_storage_commons.h"



static const char* PATH = "/tmp/paxos";
static const char* MSG = "1234567890123456789012345678901234567890123456789012345678901234";
static const size_t MSG_SIZE = 64;
static struct lmdb_storage lmdbStorage;


void stop_execution(int sig) {
    stop = 1;
}

char*
paxos_accepted_to_char_array(paxos_accepted* acc, char* buffer)
{
  int len = acc->value.paxos_value_len;
  if (buffer == NULL) {
    return NULL;
  }
  memcpy(buffer, acc, sizeof(paxos_accepted));
  if (len > 0) {
    memcpy(&buffer[sizeof(paxos_accepted)], acc->value.paxos_value_val, len);
  }
  return buffer;
}

int main() {

    if (lmdb_storage_open( &lmdbStorage, 1 ) != 0) {
      printf("Fail to open storage");
      return 1;
    }
    signal(SIGINT, stop_execution);
    int count = 0;
    int fd = open(PATH, O_RDWR | O_NONBLOCK, 0);
    paxos_accepted accepted;
    char buffer[sizeof(int) + sizeof(paxos_accepted) + MSG_SIZE];
    struct timeval initial_time;
    // verbose = 1;

    gettimeofday(&initial_time, NULL);

    if(fd > 0) {
        while (!stop) {
            memset(&accepted, 0, sizeof(paxos_accepted));
            memset(&buffer, 0, sizeof(paxos_accepted) + MSG_SIZE);
            accepted.iid = count++;
            accepted.value.paxos_value_len = MSG_SIZE;
            accepted.value.paxos_value_val = MSG;
            memcpy(buffer, &count, sizeof(int));
            paxos_accepted_to_char_array(&accepted, &buffer[sizeof(int)]);
            process_write_message(lmdbStorage, buffer, sizeof(paxos_accepted) + MSG_SIZE);
            process_read_message(lmdbStorage, buffer, sizeof(paxos_accepted) + MSG_SIZE, fd);
        }
    }

    struct timeval final_time;
    gettimeofday(&final_time, NULL);

    long diff_time = timeval_diff(&initial_time, &final_time);

    printf("\n\n%d messages writted in %ld us\n", count, (long) (diff_time/1e6));
    printf("%lf msgs/s\n", count/(diff_time/1e6));

    return 0;
}