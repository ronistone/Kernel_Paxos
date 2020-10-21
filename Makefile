PAX_OBJ= \
paxos/carray.o \
paxos/paxos.o \
paxos/quorum.o \
paxos/storage_mem.o \
paxos/storage_lmdb.o \
paxos/storage_utils.o \
paxos/storage.o \
paxos/workers_pool.o \
evpaxos/message.o \
evpaxos/paxos_types_pack.o \
evpaxos/config.o \
evpaxos/peers.o \
evpaxos/eth.o \
evpaxos/evworkers.o \
kpaxos/kernel_device.o \
kpaxos/kread_persistence_device_operations.o \
kpaxos/kwrite_persistence_device_operations.o \
kpaxos/kfile.o

CL_OBJ= \
kpaxos/kclient.o \
kpaxos/kstats.o \
evpaxos/evlearner.o \
paxos/learner.o \
$(PAX_OBJ)

PROP_OBJ= \
	kpaxos/kproposer.o \
	evpaxos/evproposer.o \
	paxos/proposer.o \
	$(PAX_OBJ)

ACC_OBJ= \
 	kpaxos/kacceptor.o \
	evpaxos/evacceptor.o \
	paxos/acceptor.o \
	$(PAX_OBJ)

LEARN_OBJ= \
	kpaxos/klearner.o \
	evpaxos/evlearner.o \
	paxos/learner.o \
	kpaxos/klearner_device_operations.o \
	$(PAX_OBJ)

REP_OBJ= \
	kpaxos/klearner_device_operations.o \
    kpaxos/kreplica.o \
	evpaxos/evlearner.o \
	evpaxos/evproposer.o \
	evpaxos/evacceptor.o \
	paxos/acceptor.o \
	paxos/learner.o \
	paxos/proposer.o \
	evpaxos/evreplica.o \
	$(PAX_OBJ)

################# MODIFY HERE FOR MORE MODULES ##############
obj-m += \
kproposer.o \
klearner.o \
kacceptor.o \
kreplica.o \
kclient.o

kclient-y:= $(CL_OBJ)
kproposer-y:= $(PROP_OBJ)
kacceptor-y:= $(ACC_OBJ)
klearner-y:= $(LEARN_OBJ)
kreplica-y:= $(REP_OBJ)

##############################################################

KDIR ?= /lib/modules/$(shell uname -r)/build
BUILD_DIR ?= $(PWD)/build
BUILD_DIR_MAKEFILE ?= $(PWD)/build/Makefile

C_COMP:= -std=c99
G_COMP:= -std=gnu99
USR_FLAGS:= -Wall -D user_space
USR_SRCS := $(wildcard kpaxos/user_*.c)
USRTST_SRCS := $(wildcard test/*.c)
USR_CL := $(filter-out kpaxos/user_learner.c kpaxos/user_storage.c kpaxos/user_storage_commons.c, $(USR_SRCS))
USR_LEARN := $(filter-out kpaxos/user_client.c kpaxos/user_storage.c kpaxos/user_storage_commons.c, $(USR_SRCS))
USR_STORE := $(filter-out kpaxos/user_learner.c kpaxos/user_client.c, $(USR_SRCS))
USR_STORE_TST := $(filter-out kpaxos/user_storage.c , $(USRTST_SRCS) $(USR_STORE))
USRC_OBJS := $(patsubst kpaxos/%.c, $(BUILD_DIR)/%.o, $(USR_CL))
USRL_OBJS := $(patsubst kpaxos/%.c, $(BUILD_DIR)/%.o, $(USR_LEARN))
USRS_OBJS := $(patsubst kpaxos/%.c, $(BUILD_DIR)/%.o, $(USR_STORE))
USRSTST_OBJS := $(patsubst kpaxos/%.c test/%.c, $(BUILD_DIR)/%.o, $(USR_STORE_TST))
LMDBOP_OBJS := $(BUILD_DIR)/lmdb_operations.o

EXTRA_CFLAGS:= -I$(PWD)/kpaxos/include -I$(PWD)/paxos/include -I$(PWD)/evpaxos/include -I$(HOME)/local/include
EXTRALMDB_FLAG:= -llmdb
EXTRASTORE_FLAG:= -pthread
ccflags-y:= $(G_COMP) -Wall -Wno-declaration-after-statement -Wframe-larger-than=3100 -O3

all: $(BUILD_DIR) kernel_app user_client user_learner user_storage user_storage_test

kernel_app: $(BUILD_DIR_MAKEFILE)
	make -C $(KDIR) M=$(BUILD_DIR) src=$(PWD) modules

$(BUILD_DIR):
	mkdir -p "$@/paxos"
	mkdir -p "$@/evpaxos"
	mkdir -p "$@/kpaxos"
	mkdir -p "$@/test"

$(BUILD_DIR_MAKEFILE): $(BUILD_DIR)
	touch "$@"

$(BUILD_DIR)/%.o: kpaxos/%.c
	$(CC) $(G_COMP) $(USR_FLAGS) $(EXTRA_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: test/%.c
	$(CC) $(G_COMP) $(USR_FLAGS) $(EXTRA_CFLAGS) -c $< -o $@

user_client: $(USRC_OBJS)
	$(CC) $(G_COMP) $(USR_FLAGS) $(EXTRA_CFLAGS) -o $(BUILD_DIR)/$@ $^

user_learner: $(USRL_OBJS)
	$(CC) $(G_COMP) $(USR_FLAGS) $(EXTRA_CFLAGS) -o $(BUILD_DIR)/$@ $^

user_storage: $(LMDBOP_OBJS) $(USRS_OBJS)
	$(CC) $(G_COMP) $(USR_FLAGS) $(EXTRA_CFLAGS) -o $(BUILD_DIR)/$@ $^ $(EXTRALMDB_FLAG) $(EXTRASTORE_FLAG)

user_storage_test: $(LMDBOP_OBJS) $(USRSTST_OBJS)
	$(CC) $(G_COMP) $(USR_FLAGS) $(EXTRA_CFLAGS) -o $(BUILD_DIR)/$@ $^ $(EXTRALMDB_FLAG) $(EXTRASTORE_FLAG)

###########################################################################
clean:
	make -C $(KDIR) M=$(BUILD_DIR) src=$(PWD) clean
	-rm -rf build
