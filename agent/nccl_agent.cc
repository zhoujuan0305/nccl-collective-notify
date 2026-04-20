#include "../src/include/collective_notify.h"

#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef NCCL_AGENT_FORWARD_HOST
#define NCCL_AGENT_FORWARD_HOST "192.168.3.3"
#endif

#ifndef NCCL_AGENT_FORWARD_PORT
#define NCCL_AGENT_FORWARD_PORT 19090
#endif

static volatile sig_atomic_t g_running = 1;
static constexpr int kPollTimeoutMs = 500;

static void onSignal(int) {
  g_running = 0;
}

static int installSignalHandlers() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = onSignal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, nullptr) != 0) return -1;
  if (sigaction(SIGTERM, &sa, nullptr) != 0) return -1;
  return 0;
}

int main() {
  if (installSignalHandlers() != 0) {
    perror("sigaction");
    return 1;
  }

  int unixFd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (unixFd < 0) {
    perror("socket(AF_UNIX)");
    return 1;
  }

  struct sockaddr_un localAddr;
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sun_family = AF_UNIX;
  strncpy(localAddr.sun_path, NCCL_COLLECTIVE_NOTIFY_SOCKET_PATH, sizeof(localAddr.sun_path)-1);

  unlink(localAddr.sun_path);
  if (bind(unixFd, reinterpret_cast<struct sockaddr*>(&localAddr), sizeof(localAddr)) != 0) {
    perror("bind(AF_UNIX)");
    close(unixFd);
    return 1;
  }

  int udpFd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpFd < 0) {
    perror("socket(AF_INET)");
    unlink(localAddr.sun_path);
    close(unixFd);
    return 1;
  }

  struct sockaddr_in remoteAddr;
  memset(&remoteAddr, 0, sizeof(remoteAddr));
  remoteAddr.sin_family = AF_INET;
  remoteAddr.sin_port = htons(NCCL_AGENT_FORWARD_PORT);
  if (inet_pton(AF_INET, NCCL_AGENT_FORWARD_HOST, &remoteAddr.sin_addr) != 1) {
    fprintf(stderr, "invalid NCCL_AGENT_FORWARD_HOST: %s\n", NCCL_AGENT_FORWARD_HOST);
    close(udpFd);
    unlink(localAddr.sun_path);
    close(unixFd);
    return 1;
  }

  while (g_running) {
    struct pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = unixFd;
    pfd.events = POLLIN;

    int pr = poll(&pfd, 1, kPollTimeoutMs);
    if (pr < 0) {
      if (errno == EINTR) continue;
      perror("poll(AF_UNIX)");
      break;
    }
    if (pr == 0) continue;
    if ((pfd.revents & POLLIN) == 0) continue;

    struct NcclCollectiveNotify msg;
    ssize_t n = recv(unixFd, &msg, sizeof(msg), 0);
    if (n < 0) {
      if (errno == EINTR) continue;
      perror("recv(AF_UNIX)");
      break;
    }
    if (n != (ssize_t)sizeof(msg)) continue;
    (void)sendto(udpFd, &msg, sizeof(msg), 0, reinterpret_cast<struct sockaddr*>(&remoteAddr), sizeof(remoteAddr));
  }

  close(udpFd);
  unlink(localAddr.sun_path);
  close(unixFd);
  return 0;
}
