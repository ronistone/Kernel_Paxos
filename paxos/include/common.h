#ifndef COMMON_PAX
#define COMMON_PAX

#include <linux/slab.h> //kmalloc
#include <linux/if_ether.h>

#define pmalloc(size) kmalloc(size, GFP_ATOMIC)
#define prealloc(ptr, size) krealloc(ptr, size, GFP_ATOMIC)
#define pfree(ptr) kfree(ptr)

extern const char* MOD_NAME;

typedef uint8_t eth_address;

#define MAX_PAXOS_VALUE_SIZE ETH_DATA_LEN
#define NOOP (void)0

#define MIN(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })


#define LOG_DEBUG(fmt, args...)                                                \
  printk(KERN_DEBUG "[%s] %s: " fmt "\n", current -> comm, MOD_NAME, ##args)
//  printk_ratelimit()? NOOP : printk(KERN_DEBUG "[%s] %s: " fmt "\n", current -> comm, MOD_NAME, ##args)
#define LOG_INFO(fmt, args...)                                                 \
  printk(KERN_INFO "[%s] %s: " fmt "\n", current -> comm, MOD_NAME, ##args)
//  printk_ratelimit()? NOOP : printk(KERN_INFO "[%s] %s: " fmt "\n", current -> comm, MOD_NAME, ##args)
#define LOG_ERROR(fmt, args...)                                                \
  printk(KERN_ERR "[%s] %s: " fmt "\n", current -> comm, MOD_NAME, ##args)
//  printk_ratelimit()? NOOP : printk(KERN_ERR "[%s] %s: " fmt "\n", current -> comm, MOD_NAME, ##args)

#endif
