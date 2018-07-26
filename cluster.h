#include <stdlib.h>

#pragma once

typedef int bool;

struct Server {
  char* id;
  char* svType;
  char* metadata;
  bool frontend;
};

struct SDConfig {
  char* endpoints;
  int etcdDialTimeoutSec;
  char* etcdPrefix;
  int heartbeatTTLSec;
  bool logHeartbeat;
  int syncServersIntervalSec;
};

struct NatsRPCClientConfig {
  char* endpoint;
  int maxConnectionRetries;
  int requestTimeoutMs;
};

struct NatsRPCServerConfig {
  char* endpoint;
  int maxConnectionRetries;
  int messagesBufferSize;
  int rpcHandleWorkerNum;
};

struct GrpcRPCClientConfig {
  int requestTimeoutMs;
  int dialTimeoutMs;
};

struct GrpcRPCServerConfig {
  int port;
};

struct RPCReq {
  void* data;
  int dataLen;
  char* route;
};

struct RPCRes {
  void* data;
  int dataLen;
};

struct Route {
  char* svType;
  char* service;
  char* method;
};

typedef void* (*rpcCbFunc) (struct RPCReq);
void* bridgeRPCFunc(rpcCbFunc f, struct RPCReq req);
