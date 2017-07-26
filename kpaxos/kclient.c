#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <net/udp.h>
#include <asm/atomic.h>
#include <linux/time.h>
#include <net/sock.h>

#include "kernel_udp.h"

#define MODULE_NAME "Client: "
#define MAX_RCV_WAIT 100000 // in microseconds
#define MAX_UDP_SIZE 65507

static unsigned char proposerip[5] = {127,0,0,2,'\0'};
static int proposerport = 3002;
static unsigned char myip[5] = {127,0,0,1,'\0'};
static int myport = 3001;

module_param(myport, int, S_IRUGO);
MODULE_PARM_DESC(myport,"The receiving port, default 3001");

static int len = 50;
module_param(len, int, S_IRUGO);
MODULE_PARM_DESC(len,"Data packet length, default 50, max 65507 (automatically added space for terminating 0)");

static udp_service * kclient;
static atomic_t released_socket = ATOMIC_INIT(0); // 0 no, 1 yes
static atomic_t thread_running = ATOMIC_INIT(0);   // 0 no, 1 yes
static atomic_t struct_allocated = ATOMIC_INIT(0); // 0 no, 1 yes

int connection_handler(void *data)
{
  struct sockaddr_in address;
  struct socket *client_socket = kclient->u_socket;

  int ret;
  unsigned char * in_buf = kmalloc(len, GFP_KERNEL);
  unsigned char * out_buf = kmalloc(len, GFP_KERNEL);
  size_t size_msg, size_buf;

  address.sin_addr.s_addr = htonl(create_address(proposerip));
  _send_message(client_socket, &address, out_buf, proposerport, "VALUE FROM CLIENT", len, MODULE_NAME);

  while (1){

    if(kthread_should_stop() || signal_pending(current)){
      // printk(KERN_INFO MODULE_NAME"STOP [connection_handler]");
      if(atomic_read(&released_socket) == 0){
        printk(KERN_INFO MODULE_NAME"Released socket [connection_handler]");
        atomic_set(&released_socket, 1);
        sock_release(kclient->u_socket);
      }
      kfree(in_buf);
      kfree(out_buf);
      return 0;
    }


    memset(in_buf, '\0', len);
    memset(&address, 0, sizeof(struct sockaddr_in));
    ret = udp_server_receive(client_socket, &address, in_buf, len, MSG_WAITALL, &released_socket, MODULE_NAME);
    if(ret > 0){
      size_msg = strlen("ALL DONE");
      size_buf = strlen(in_buf);
      if(memcmp(in_buf, "ALL DONE", size_buf > size_msg ? size_msg : size_buf) == 0){
        printk(KERN_INFO MODULE_NAME"All done, terminating client [connection_handler]");
      }
    }
  }

  return 0;
}

int udp_server_listen(void)
{
  int server_err;
  struct socket *conn_socket;
  struct sockaddr_in server;
  struct timeval tv;

  server_err = sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &kclient->u_socket);
  if(server_err < 0){
    printk(KERN_INFO MODULE_NAME"Error: %d while creating socket [udp_server_listen]", server_err);
    atomic_set(&thread_running, 0);
    return 0;
  }else{
    atomic_set(&released_socket, 0);
    printk(KERN_INFO MODULE_NAME"Created socket [udp_server_listen]");
  }

  conn_socket = kclient->u_socket;
  server.sin_addr.s_addr = htonl(create_address(myip));
  server.sin_family = AF_INET;
  server.sin_port = htons(myport);

  server_err = conn_socket->ops->bind(conn_socket, (struct sockaddr*)&server, sizeof(server));
  if(server_err < 0) {
    printk(KERN_INFO MODULE_NAME"Error: %d while binding socket [udp_server_listen]", server_err);
    atomic_set(&released_socket, 1);
    sock_release(kclient->u_socket);
    atomic_set(&thread_running, 0);
    return 0;
  }else{
    printk(KERN_INFO MODULE_NAME"Socket is bind to 127.0.0.1 [udp_server_listen]");
  }

  tv.tv_sec = 0;
  tv.tv_usec = MAX_RCV_WAIT;
  kernel_setsockopt(conn_socket, SOL_SOCKET, SO_RCVTIMEO, (char * )&tv, sizeof(tv));

  connection_handler(NULL);
  atomic_set(&thread_running, 0);
  return 0;
}

void udp_server_start(void){
  kclient->u_thread = kthread_run((void *)udp_server_listen, NULL, MODULE_NAME);
  if(kclient->u_thread >= 0){
    atomic_set(&thread_running,1);
    printk(KERN_INFO MODULE_NAME "Thread running [udp_server_start]");
  }else{
    printk(KERN_INFO MODULE_NAME "Error in starting thread. Terminated [udp_server_start]");
  }
}

static int __init network_server_init(void)
{
  if(len < 0 || len > MAX_UDP_SIZE){
    printk(KERN_INFO MODULE_NAME"Wrong len, using default one");
    len = 50;
  }
  len++;
  atomic_set(&released_socket, 1);
  kclient = kmalloc(sizeof(udp_service), GFP_KERNEL);
  if(!kclient){
    printk(KERN_INFO MODULE_NAME"Failed to initialize server [network_server_init]");
  }else{
    atomic_set(&struct_allocated,1);
    memset(kclient, 0, sizeof(udp_service));
    printk(KERN_INFO MODULE_NAME "Server initialized [network_server_init]");
    udp_server_start();
  }
  return 0;
}

static void __exit network_server_exit(void)
{
  int ret;
  if(atomic_read(&struct_allocated) == 1){

    if(atomic_read(&thread_running) == 1){
      if((ret = kthread_stop(kclient->u_thread)) == 0){
        printk(KERN_INFO MODULE_NAME"Terminated thread [network_server_exit]");
      }else{
        printk(KERN_INFO MODULE_NAME"Error %d in terminating thread [network_server_exit]", ret);
      }
    }else{
      printk(KERN_INFO MODULE_NAME"Thread was not running [network_server_exit]");
    }

    if(atomic_read(&released_socket) == 0){
      atomic_set(&released_socket, 1);
      sock_release(kclient->u_socket);
      printk(KERN_INFO MODULE_NAME"Released socket [network_server_exit]");
    }

    kfree(kclient);
  }else{
    printk(KERN_INFO MODULE_NAME"Struct was not allocated [network_server_exit]");
  }

  printk(KERN_INFO MODULE_NAME"Module unloaded [network_server_exit]");
}

module_init(network_server_init)
module_exit(network_server_exit)
MODULE_LICENSE("MIT");
MODULE_AUTHOR("Emanuele Giuseppe Esposito");