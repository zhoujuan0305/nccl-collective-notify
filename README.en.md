# nccl-collective-notify

[中文](README.md) | English

## Project Overview

`nccl-collective-notify` is a minimal NCCL-based modification that emits host-side notification events for collectives and forwards them to a remote UDP server through a local agent.

It follows a lightweight path:

```text
NCCL rank 0
  -> AF_UNIX datagram socket
  -> local nccl-agent
  -> UDP
  -> remote server
```

Current default path and target:
- Local socket: `/tmp/nccl-agent.sock`
- Remote server: `<remote-host-url>:<remote-port>`

## Implementation Summary

- Add communicator-level `collectiveNotifySeq` and `notifier` to `ncclComm`
- Add `collectiveId` and `notifySent` to `ncclTaskColl`
- Assign `collectiveId = ++comm->collectiveNotifySeq` on every `ncclTaskColl` creation path
- Send one local notification in `hostStreamPlanTask()` only when `rank == 0` and `notifySent == false`
- Use `AF_UNIX + SOCK_DGRAM + nonblocking` for local notification
- The agent listens on local UDS and forwards messages to a remote UDP server

## Build

### Build NCCL

The simplest build command is:

```bash
make -j src.build
```

The default output directory is `build/`.

If CUDA is not installed under `/usr/local/cuda`, use:

```bash
make -j src.build CUDA_HOME=/path/to/cuda
```

### Build Agent

```bash
make -C agent build
```

Default output:

```bash
build/bin/nccl-agent
```

If you want to specify the remote server at build time:

```bash
make -C agent build AGENT_REMOTE_HOST=<remote-host-url> AGENT_REMOTE_PORT=<remote-port>
```

The corresponding CMake variables are:
- `NCCL_AGENT_REMOTE_HOST`
- `NCCL_AGENT_REMOTE_PORT`

## Usage Example

### 1. Start the Receiver

Run the minimal receiver:

```bash
python3 agent/server.py --host 0.0.0.0 --port <listen-port>
```

### 2. Start the Agent

For local loopback validation, build the agent to forward to your chosen target:

```bash
make -C agent build BUILDDIR=/tmp/nccl-agent-build AGENT_REMOTE_HOST=<remote-host-url> AGENT_REMOTE_PORT=<remote-port>
/tmp/nccl-agent-build/bin/nccl-agent
```

### Build nccl-tests

```bash
git clone https://github.com/NVIDIA/nccl-tests.git
cd nccl-tests
make -j MPI=1 MPI_HOME=<path-to-mpi> NCCL_HOME=<path-to-this-repo>/build
```

### Run nccl-tests

```bash
./build/all_reduce_perf -b 1M -e 1M -f 2 -g 2 -n 5
```

In theory, you should observe about 15 corresponding notification messages on the server side.

## Deploy Agent with systemd

This repository includes a deployment script:

[agent/install_systemd.sh](/data/zhiyuanzhou/nccl/agent/install_systemd.sh:1)

It will:
- rebuild the agent with your specified server host and port
- install the binary to `/usr/local/bin/nccl-agent` by default
- install the `systemd` service
- run `daemon-reload`
- enable and restart the service

The simplest deployment command is:

```bash
sudo bash agent/install_systemd.sh
```

Specify the remote server:

```bash
sudo bash agent/install_systemd.sh --server-host <remote-host-url> --server-port <remote-port>
```

Customize the service name and install path:

```bash
sudo bash agent/install_systemd.sh \
  --server-host <remote-host-url> \
  --server-port <remote-port> \
  --service-name nccl-agent-custom \
  --install-bin /usr/local/bin/nccl-agent-custom
```

## License and Acknowledgements

This repository is a derivative project based on NVIDIA NCCL.

Please also read:
- [LICENSE.txt](LICENSE.txt)
- [ThirdPartyNotices.txt](ThirdPartyNotices.txt)
- [CONTRIBUTING.md](CONTRIBUTING.md)

The original upstream NCCL copyright notices, SPDX markers, and third-party attribution information are preserved in this repository.

If you redistribute this repository or publish further modifications based on it, please retain the upstream license and attribution information and clearly mark your own changes.
