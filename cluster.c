#include "cluster.h"
#include "stdio.h"

void* bridgeRPCFunc(rpcCbFunc f, struct RPCReq req){
	return f(req);
}

void bridgeFreeRPCFunc(freeRPCCbFunc f, void* ptr){
	return f(ptr);
}
