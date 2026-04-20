#ifndef NCCL_NOTIFY_H_
#define NCCL_NOTIFY_H_

#include <stdint.h>

#include "nccl.h"

struct ncclComm;

ncclResult_t ncclCollectiveNotifierInit(struct ncclComm* comm);
void ncclCollectiveNotifierSend(struct ncclComm* comm, uint64_t collectiveId);
void ncclCollectiveNotifierClose(struct ncclComm* comm);

#endif
