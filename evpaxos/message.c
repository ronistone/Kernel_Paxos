/*
 * Copyright (c) 2013-2014, University of Lugano
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the names of it
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "message.h"
#include "eth.h"
#include "paxos.h"
#include "paxos_types_pack.h"
#include <paxos_types.h>

paxos_message*
print_paxos_message(paxos_message* message);

void
send_paxos_message(struct net_device* dev, eth_address* addr,
                   paxos_message* msg)
{
  eth_address packer[ETH_DATA_LEN];
  long        size_msg = msgpack_pack_paxos_message(packer, msg);

  print_paxos_message(msg);
  eth_send(dev, addr, (uint16_t)msg->type, packer, size_msg);
}

void
send_paxos_learner_hi(struct net_device* dev, eth_address* addr)
{
  paxos_message msg = { .type = PAXOS_LEARNER_HI };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Learner: Send hi to the acceptors");
}

void
send_paxos_acceptor_ok(struct net_device* dev, eth_address* addr)
{
  paxos_message msg = { .type = PAXOS_ACCEPTOR_OK };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Acceptor: Send ok to the learner");
}

void
send_paxos_learner_del(struct net_device* dev, eth_address* addr)
{
  paxos_message msg = { .type = PAXOS_LEARNER_DEL };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Learner: Send del to the acceptors");
}

void
send_paxos_prepare(struct net_device* dev, eth_address* addr, paxos_prepare* pp)
{

  paxos_message msg = { .type = PAXOS_PREPARE, .u.prepare = *pp };

  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Proposer: Send prepare for iid %d ballot %d", pp->iid,
                  pp->ballot);
}

void
send_paxos_promise(struct net_device* dev, eth_address* addr, paxos_promise* p)
{
  paxos_message msg = { .type = PAXOS_PROMISE, .u.promise = *p };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Acceptor: Send promise for iid %d ballot %d", p->iid,
                  p->ballot);
}

void
send_paxos_accept(struct net_device* dev, eth_address* addr, paxos_accept* pa)
{
  paxos_message msg = { .type = PAXOS_ACCEPT, .u.accept = *pa };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Proposer: Send accept for iid %d ballot %d", pa->iid,
                  pa->ballot);
}

void
send_paxos_accepted(struct net_device* dev, eth_address* addr,
                    paxos_accepted* p)
{
  paxos_message msg = { .type = PAXOS_ACCEPTED, .u.accepted = *p };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Acceptor: Send accepted for inst %d ballot %d", p->iid,
                  p->ballot);
}

void
send_paxos_preempted(struct net_device* dev, eth_address* addr,
                     paxos_preempted* p)
{
  paxos_message msg = { .type = PAXOS_PREEMPTED, .u.preempted = *p };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Acceptor Send preempted for inst %d ballot %d", p->iid,
                  p->ballot);
}

void
send_paxos_repeat(struct net_device* dev, eth_address* addr, paxos_repeat* p)
{
  paxos_message msg = { .type = PAXOS_REPEAT, .u.repeat = *p };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Learner: Send repeat for inst %d-%d", p->from, p->to);
}

void
send_paxos_trim(struct net_device* dev, eth_address* addr, paxos_trim* t)
{
  paxos_message msg = { .type = PAXOS_TRIM, .u.trim = *t };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Learner: Send trim for inst %d", t->iid);
}

void
paxos_submit(struct net_device* dev, eth_address* addr, char* data, int size)
{
  paxos_message msg = { .type = PAXOS_CLIENT_VALUE,
                        .u.client_value.value.paxos_value_len = size,
                        .u.client_value.value.paxos_value_val = data };
  send_paxos_message(dev, addr, &msg);
  paxos_log_debug("Client: Sent client value size data %d", size);
}

char*
get_paxos_message_type(paxos_message* message)
{

  switch (message -> type) {

    case PAXOS_CLIENT_VALUE:
      return "PAXOS_CLIENT_VALUE";
    case PAXOS_PROMISE:
      return "PAXOS_PROMISE";
    case PAXOS_ACCEPT:
      return "PAXOS_ACCEPT";
    case PAXOS_ACCEPTED:
      return "PAXOS_ACCEPTED";
    case PAXOS_PREPARE:
      return "PAXOS_PREPARE";
    case PAXOS_PREEMPTED:
      return "PAXOS_PREEMPTED";
    case PAXOS_REPEAT:
      return "PAXOS_REPEAT";
    case PAXOS_TRIM:
      return "PAXOS_TRIM";
    case PAXOS_ACCEPTOR_STATE:
      return "PAXOS_ACCEPTOR_STATE";
    case PAXOS_LEARNER_HI:
      return "PAXOS_LEARNER_HI";
    case PAXOS_LEARNER_DEL:
      return "PAXOS_LEARNER_DEL";
    case PAXOS_ACCEPTOR_OK:
      return "PAXOS_ACCEPTOR_OK";
    default:
      return "TYPE NOT FOUND";
  }
}

int
recv_paxos_message(paxos_message* msg, char* msg_data, paxos_message_type proto,
                   char* data, size_t size)
{
  return msgpack_unpack_paxos_message(msg, msg_data, proto, data, size);
}

paxos_message*
copy_paxos_message(paxos_message* msg)
{
  paxos_message* message = pmalloc(sizeof(paxos_message));

  memcpy(message, msg, sizeof(paxos_message));
  switch (message -> type) {

    case PAXOS_CLIENT_VALUE:
      ALLOC_VAL_POINTER(message, client_value);
      COPY_VAL_POINTER(message, msg, client_value);
      paxos_log_debug("Paxos message copied: %s [ %d ] -> %s", get_paxos_message_type(message),
        msg->u.client_value.value.paxos_value_len,
        msg->u.client_value.value.paxos_value_val + (msg->u.client_value.value.paxos_value_len - 64));
      break;

    case PAXOS_PROMISE:
      ALLOC_VAL_POINTER(message, promise);
      COPY_VAL_POINTER(message, msg, promise);
      paxos_log_debug("Paxos message copied: %s [ %d ] -> %s", get_paxos_message_type(message),
        msg->u.promise.value.paxos_value_len,
        msg->u.promise.value.paxos_value_val + (msg->u.promise.value.paxos_value_len - 64));
      break;

    case PAXOS_ACCEPT:
      ALLOC_VAL_POINTER(message, accept);
      COPY_VAL_POINTER(message, msg, accept);
      paxos_log_debug("Paxos message copied: %s [ %d ] -> %s", get_paxos_message_type(message),
        msg->u.accept.value.paxos_value_len,
        msg->u.accept.value.paxos_value_val + (msg->u.accept.value.paxos_value_len - 64));
      break;

    case PAXOS_ACCEPTED:
      ALLOC_VAL_POINTER(message, accepted);
      COPY_VAL_POINTER(message, msg, accepted);
      paxos_log_debug("Paxos message copied: %s [ %d ] -> %s", get_paxos_message_type(message),
        msg->u.accepted.value.paxos_value_len,
        msg->u.accepted.value.paxos_value_val + (msg->u.accepted.value.paxos_value_len - 64));
      break;

    case PAXOS_PREPARE:
      break;

    case PAXOS_PREEMPTED:
      break;

    case PAXOS_REPEAT:
      break;

    case PAXOS_TRIM:
      break;

    case PAXOS_ACCEPTOR_STATE:
      break;

    case PAXOS_LEARNER_HI:
      break;

    case PAXOS_LEARNER_DEL:
      break;

    case PAXOS_ACCEPTOR_OK:
      break;

    default:
      paxos_log_error("type invalid");
      break;
  }
  return message;
}


paxos_message*
print_paxos_message(paxos_message* message)
{
  if(message == NULL) {
    return NULL;
  }
  switch (message -> type) {

    case PAXOS_CLIENT_VALUE:
      paxos_log_debug("Paxos message: %s [ %d ] -> %s", get_paxos_message_type(message),
                     message->u.client_value.value.paxos_value_len,
                     message->u.client_value.value.paxos_value_val);
      break;

    case PAXOS_PROMISE:
      paxos_log_debug("Paxos message: %s [ %d ] -> %s", get_paxos_message_type(message),
                     message->u.promise.value.paxos_value_len,
                     message->u.promise.value.paxos_value_val);
      break;

    case PAXOS_ACCEPT:
      paxos_log_debug("Paxos message: %s [ %d ] -> %s", get_paxos_message_type(message),
                     message->u.accept.value.paxos_value_len,
                     message->u.accept.value.paxos_value_val);
      break;

    case PAXOS_ACCEPTED:
      paxos_log_debug("Paxos message: %s [ %d ] -> %s", get_paxos_message_type(message),
                     message->u.accepted.value.paxos_value_len,
                     message->u.accepted.value.paxos_value_val);
      break;

    case PAXOS_PREPARE:
      break;

    case PAXOS_PREEMPTED:
      break;

    case PAXOS_REPEAT:
      break;

    case PAXOS_TRIM:
      break;

    case PAXOS_ACCEPTOR_STATE:
      break;

    case PAXOS_LEARNER_HI:
      break;

    case PAXOS_LEARNER_DEL:
      break;

    case PAXOS_ACCEPTOR_OK:
      break;

    default:
      paxos_log_error("type invalid");
      break;
  }
  return message;
}

char*
paxos_accepted_to_char_array(paxos_accepted* acc, char* buffer)
{
  size_t len = acc->value.paxos_value_len;
//  char* buffer = malloc(sizeof(paxos_accepted) + len);
  if (buffer == NULL) {
    return NULL;
  }
  memcpy(buffer, acc, sizeof(paxos_accepted));
  if (len > 0) {
    memcpy(&buffer[sizeof(paxos_accepted)], acc->value.paxos_value_val, len);
  }
  return buffer;
}