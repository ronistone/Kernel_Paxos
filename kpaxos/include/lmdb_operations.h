//
// Created by roni on 03/11/19.
//

#ifndef KERNEL_PAXOS_PERSISTENCE_LMDB_OPERATIONS_H
#define KERNEL_PAXOS_PERSISTENCE_LMDB_OPERATIONS_H

typedef unsigned int uint32_t;


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include "paxos_types.h"

#include "kernel_client.h"

#include <lmdb.h>

struct lmdb_storage
{
    MDB_env* env;
    MDB_txn* txn;
    MDB_dbi dbi;
    int acceptor_id;
};

extern int      lmdb_storage_open(struct lmdb_storage *handle, int mdb_nosync_enable);
extern int      lmdb_storage_tx_begin(struct lmdb_storage *handle);
extern int      lmdb_storage_tx_commit(struct lmdb_storage *handle);
extern void     lmdb_storage_tx_abort(struct lmdb_storage *handle);
extern void     lmdb_storage_close(struct lmdb_storage *handle);
extern int      lmdb_storage_put(struct lmdb_storage *s, uint32_t id, char* value, size_t len);
extern int      lmdb_storage_get(struct lmdb_storage *s, iid_t iid, char* out);


#endif //KERNEL_PAXOS_PERSISTENCE_LMDB_OPERATIONS_H
