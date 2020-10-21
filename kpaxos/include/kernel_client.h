#ifndef KERNEL_CLIENT
#define KERNEL_CLIENT
#ifdef user_space
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
//#include "paxos.h"
typedef unsigned int uint32_t;
#endif
#include "paxos_types.h"
struct client_value
{
  int            client_id;
  struct timeval t;
  size_t         size;
  char           value[0];
};

typedef struct storage_value {
  int bufferId;
  paxos_accepted* value;
} storage_value;


struct user_msg
{
  size_t size;
  char   value[0];
};

struct stats
{
  long   min_latency;
  long   max_latency;
  long   avg_latency;
  int    delivered_count;
  size_t delivered_bytes;
};

#endif
