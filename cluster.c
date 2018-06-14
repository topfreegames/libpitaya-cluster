#include "cluster.h"
#include "stdio.h"

void* bridgeFunc(rpcCbFunc f, struct RPCReq req){
	return f(req);
}
