BUILDDIR ?= $(abspath ../build)
BINDIR := $(BUILDDIR)/bin
AGENT_BIN := $(BINDIR)/nccl-agent

CXX ?= g++
CXXSTD ?= -std=c++14
CXXFLAGS ?= -O2 -g -Wall -Wextra $(CXXSTD)
AGENT_REMOTE_HOST ?= 192.168.3.3
AGENT_REMOTE_PORT ?= 19090

.PHONY: build clean install

build: $(AGENT_BIN)

$(AGENT_BIN): nccl_agent.cc ../src/include/collective_notify.h
	mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -I../src/include -DNCCL_AGENT_FORWARD_HOST='"$(AGENT_REMOTE_HOST)"' -DNCCL_AGENT_FORWARD_PORT=$(AGENT_REMOTE_PORT) $< -o $@

clean:
	rm -f $(AGENT_BIN)

install: build
