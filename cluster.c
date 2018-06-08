#include "cluster.h"

void bridgeFunc(rpcCbFunc f, struct RPCReq req){
	f(req);
}
