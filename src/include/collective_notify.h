#ifndef NCCL_COLLECTIVE_NOTIFY_H_
#define NCCL_COLLECTIVE_NOTIFY_H_

#include <stdint.h>

#ifndef NCCL_COLLECTIVE_NOTIFY_SOCKET_PATH
#define NCCL_COLLECTIVE_NOTIFY_SOCKET_PATH "/tmp/nccl-agent.sock"
#endif

struct NcclCollectiveNotify {
  uint64_t timestamp_us;
  uint64_t collective_id;
};

#endif
