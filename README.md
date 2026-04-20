# nccl-collective-notify

中文 | [English](README.en.md)

## 项目简介

`nccl-collective-notify` 是一个基于原始 NCCL 的最小改造版本，用来在 host 侧为 collective 生成通知事件，并通过本地 agent 转发到远端 UDP server。

它实现的是一条很轻量的链路：

```text
NCCL rank 0
  -> AF_UNIX datagram socket
  -> local nccl-agent
  -> UDP
  -> remote server
```

当前默认路径和目标是：
- 本地 socket: `/tmp/nccl-agent.sock`
- 远端 server: `<remote-host-url>:<remote-port>`

## 实现思路

- 在 `ncclComm` 上增加 communicator 级别的 `collectiveNotifySeq` 和 `notifier`
- 在 `ncclTaskColl` 上增加 `collectiveId` 和 `notifySent`
- 在所有 `ncclTaskColl` 创建路径里分配 `collectiveId = ++comm->collectiveNotifySeq`
- 在 `hostStreamPlanTask()` 中，仅 `rank == 0` 且 `notifySent == false` 时发送一次本地通知
- 本地通知使用 `AF_UNIX + SOCK_DGRAM + nonblocking`
- agent 监听本地 UDS，收到后转发到远端 UDP server

## 构建

### 构建NCCL

最简单的构建方式：

```bash
make -j src.build
```

默认输出目录是 `build/`。

如果 CUDA 不在 `/usr/local/cuda`，可以这样指定：

```bash
make -j src.build CUDA_HOME=/path/to/cuda
```

### 构建Agent


```bash
make -C agent build
```

默认会生成：

```bash
build/bin/nccl-agent
```

如果你想在编译时指定远端 server：

```bash
make -C agent build AGENT_REMOTE_HOST=<remote-host-url> AGENT_REMOTE_PORT=<remote-port>
```

也支持 CMake，变量名分别是：
- `NCCL_AGENT_REMOTE_HOST`
- `NCCL_AGENT_REMOTE_PORT`

## 使用示例

### 1. 启动receiver

运行最小接收端：

```bash
python3 agent/server.py --host 0.0.0.0 --port <listen-port>
```

### 2. 启动Agent

如果是本机闭环验证，推荐让 agent 发往本机：

```bash
make -C agent build BUILDDIR=/tmp/nccl-agent-build AGENT_REMOTE_HOST=<remote-host-url> AGENT_REMOTE_PORT=<remote-port>
/tmp/nccl-agent-build/bin/nccl-agent
```



### 编译nccl-tests


```bash
git clone https://github.com/NVIDIA/nccl-tests.git
cd nccl-tests
make -j MPI=1 MPI_HOME=<path-to-mpi> NCCL_HOME=<path-to-this-repo>/build
```


### 运行nccl-tests测试

```bash
./build/all_reduce_perf -b 1M -e 1M -f 2 -g 2 -n 5
```
理论上应该在服务端看到15次相应的消息通知


## agent侧部署到systemd

仓库里提供了一个部署脚本：

[agent/install_systemd.sh](/data/zhiyuanzhou/nccl/agent/install_systemd.sh:1)

它会：
- 按你指定的 server host 和 port 重新编译 agent
- 安装二进制到默认 `/usr/local/bin/nccl-agent`
- 安装 `systemd` service
- 执行 `daemon-reload`
- 启用并重启服务

**默认建议将 service 用户配置为和 NCCL 运行用户一致，或者至少确保 NCCL 运行用户对 `/tmp/nccl-agent.sock` 有写权限。**
- 当前部署脚本默认使用 `root:root`
- 可以通过 `--service-user` 和 `--service-group` 显式指定运行用户

最简单的部署方式：

```bash
sudo bash agent/install_systemd.sh
```

指定远端 server：

```bash
sudo bash agent/install_systemd.sh --server-host <remote-host-url> --server-port <remote-port>
```

如果需要让 agent 以和 NCCL 相同的用户运行：

```bash
sudo bash agent/install_systemd.sh \
  --server-host <remote-host-url> \
  --server-port <remote-port> \
  --service-user <nccl-run-user> \
  --service-group <nccl-run-group>
```

自定义服务名和安装路径：

```bash
sudo bash agent/install_systemd.sh \
  --server-host <remote-host-url> \
  --server-port <remote-port> \
  --service-user <service-user> \
  --service-group <service-group> \
  --service-name nccl-agent-custom \
  --install-bin /usr/local/bin/nccl-agent-custom
```


## License 与致谢
本仓库是基于 NVIDIA NCCL 的修改而来，属于其衍生项目。

请同时阅读以下文件：
- [LICENSE.txt](LICENSE.txt)
- [ThirdPartyNotices.txt](ThirdPartyNotices.txt)
- [CONTRIBUTING.md](CONTRIBUTING.md)

仓库中保留了上游 NCCL 的原始版权声明、SPDX 标记和第三方归属说明。

如果你分发本仓库或继续在其基础上修改并公开发布，请保留上游许可证与归属信息，并明确标注你的修改内容。
