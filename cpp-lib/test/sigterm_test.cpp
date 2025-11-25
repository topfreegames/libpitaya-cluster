#include "pitaya/c_wrapper.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Processo filho - tenta inicializar com config INVÁLIDA
        std::cout << "[Child] Tentando inicializar com NATS inválido..." << std::endl;
        
        CNATSConfig nc = {};
        nc.addr = "nats://host-invalido:4222"; 
        nc.connectionTimeoutMs = 1000;
        nc.requestTimeoutMs = 5000;
        nc.serverShutdownDeadlineMs = 1000;
        nc.serverMaxNumberOfRpcs = 100;
        nc.maxReconnectionAttempts = 0;
        nc.maxPendingMsgs = 1000;
        nc.reconnectWaitInMs = 100;
        nc.reconnectBufSize = 8*1024*1024;
        nc.reconnectJitterInMs = 50;
        nc.pingIntervalInMs = 1000;
        nc.maxPingsOut = 2;
        
        CSDConfig sd = {};
        sd.endpoints = "http://etcd-invalido:2379";     
        sd.etcdPrefix = "pitaya/";
        sd.heartbeatTTLSec = 5;
        sd.logHeartbeat = false;
        sd.logServerSync = false;
        sd.logServerDetails = false;
        sd.syncServersIntervalSec = 20;
        sd.serverTypeFilters = nullptr;
        
        CServer sv = {};
        sv.id = (char*)"test";
        sv.type = (char*)"test";
        sv.metadata = (char*)"{}";
        sv.hostname = (char*)"localhost";
        sv.frontend = true;
        
        tfg_pitc_InitializeWithNats(&nc, &sd, &sv, LogLevel_Debug, nullptr);
        
        std::cout << "[Child] ERRO: Ainda executando!" << std::endl;
        return 99;
    }
    
    // Processo pai - verifica o sinal
    int status;
    waitpid(pid, &status, 0);
    
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM) {
        std::cout << "✓ SUCESSO: SIGTERM foi enviado!" << std::endl;
        return 0;
    }
    
    std::cout << "✗ FALHA: SIGTERM não foi enviado" << std::endl;
    return 1;
}