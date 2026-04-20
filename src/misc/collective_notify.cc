/*************************************************************************
 * SPDX-FileCopyrightText: Copyright (c) 2015-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * See LICENSE.txt for more license information
 *************************************************************************/

#include "comm.h"
#include "notify.h"

#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

static uint64_t ncclCollectiveNotifyTimestampUs() {
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return 0;
  return uint64_t(ts.tv_sec)*1000000ull + uint64_t(ts.tv_nsec)/1000ull;
}

ncclResult_t ncclCollectiveNotifierInit(struct ncclComm* comm) {
  comm->notifier.fd = -1;
  strncpy(comm->notifier.path, NCCL_COLLECTIVE_NOTIFY_SOCKET_PATH, sizeof(comm->notifier.path)-1);
  comm->notifier.path[sizeof(comm->notifier.path)-1] = '\0';

  if (comm->notifier.path[0] == '\0') return ncclSuccess;

  int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
  if (fd == -1) return ncclSuccess;

  int flags = fcntl(fd, F_GETFL, 0);
  if (flags != -1) (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  comm->notifier.fd = fd;
  return ncclSuccess;
}

void ncclCollectiveNotifierSend(struct ncclComm* comm, uint64_t collectiveId) {
  if (comm->notifier.fd < 0 || collectiveId == 0 || comm->notifier.path[0] == '\0') return;

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, comm->notifier.path, sizeof(addr.sun_path)-1);

  struct NcclCollectiveNotify msg;
  msg.timestamp_us = ncclCollectiveNotifyTimestampUs();
  msg.collective_id = collectiveId;

  socklen_t addrLen = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path) + 1;
  (void)sendto(comm->notifier.fd, &msg, sizeof(msg), MSG_DONTWAIT, reinterpret_cast<struct sockaddr*>(&addr), addrLen);
}

void ncclCollectiveNotifierClose(struct ncclComm* comm) {
  if (comm->notifier.fd >= 0) {
    (void)close(comm->notifier.fd);
    comm->notifier.fd = -1;
  }
}
