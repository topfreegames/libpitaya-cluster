#include "pitaya/c_wrapper.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <signal.h>
#include <cstring>

// Variável global atômica para detectar SIGTERM
static std::atomic<bool> gSigtermReceived{false};

// Signal handler para capturar SIGTERM
void sigterm_handler(int signum) {
    if (signum == SIGTERM) {
        gSigtermReceived = true;
    }
}

// Reseta o estado do teste
void reset_test_state() {
    gSigtermReceived = false;
}

// Instala o signal handler
void install_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
}

// Teste 1: Sem retry (maxReconnectionAttempts = 0) - falha imediata
bool test_sigterm_no_retry() {
    std::cout << "\n=== TESTE 1: Sem retry (falha imediata) ===" << std::endl;
    
    reset_test_state();
    install_signal_handler();
    
    std::thread worker([]() {
        std::cout << "[Thread] Tentando inicializar sem retry..." << std::endl;
        
        CNATSConfig nc = {};
        nc.addr = "nats://192.0.2.1:4222";  // IP inválido
        nc.connectionTimeoutMs = 100;
        nc.requestTimeoutMs = 1000;
        nc.serverShutdownDeadlineMs = 1000;
        nc.serverMaxNumberOfRpcs = 100;
        nc.maxReconnectionAttempts = 0;  // SEM RETRY
        nc.maxPendingMsgs = 1000;
        nc.reconnectWaitInMs = 100;
        nc.reconnectBufSize = 8*1024*1024;
        nc.reconnectJitterInMs = 50;
        nc.pingIntervalInMs = 1000;
        nc.maxPingsOut = 2;
        
        CSDConfig sd = {};
        sd.endpoints = "http://127.0.0.1:2379";
        sd.etcdPrefix = "pitaya/";
        sd.heartbeatTTLSec = 5;
        sd.logHeartbeat = false;
        sd.logServerSync = false;
        sd.logServerDetails = false;
        sd.syncServersIntervalSec = 20;
        sd.serverTypeFilters = nullptr;
        sd.initializationTimeoutSec = 2;
        
        CServer sv = {};
        sv.id = (char*)"test";
        sv.type = (char*)"test";
        sv.metadata = (char*)"{}";
        sv.hostname = (char*)"localhost";
        sv.frontend = true;
        
        tfg_pitc_InitializeWithNats(&nc, &sd, &sv, LogLevel_Info, nullptr);
        
        std::cout << "[Thread] Inicialização retornou" << std::endl;
    });
    
    // Aguarda a thread terminar
    worker.join();
    
    // Pequeno delay para garantir que o signal handler seja chamado
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (gSigtermReceived) {
        std::cout << "✓ TESTE 1 PASSOU: SIGTERM recebido pelo processo pai (sem retry)" << std::endl;
        return true;
    }
    
    std::cout << "✗ TESTE 1 FALHOU: SIGTERM não foi recebido" << std::endl;
    return false;
}

// Teste 2: Com retry (maxReconnectionAttempts > 0) - falha após retries
bool test_sigterm_with_retry() {
    std::cout << "\n=== TESTE 2: Com retry (falha após retries) ===" << std::endl;
    
    reset_test_state();
    install_signal_handler();
    
    std::atomic<bool> threadFinished{false};
    
    std::thread worker([&threadFinished]() {
        std::cout << "[Thread] Tentando inicializar com 2 retries..." << std::endl;
        
        CNATSConfig nc = {};
        nc.addr = "nats://192.0.2.1:4222";  // IP inválido
        nc.connectionTimeoutMs = 100;       // 100ms timeout por tentativa
        nc.requestTimeoutMs = 1000;
        nc.serverShutdownDeadlineMs = 1000;
        nc.serverMaxNumberOfRpcs = 100;
        nc.maxReconnectionAttempts = 2;     // 2 RETRIES
        nc.maxPendingMsgs = 1000;
        nc.reconnectWaitInMs = 100;         // 100ms entre retries
        nc.reconnectBufSize = 8*1024*1024;
        nc.reconnectJitterInMs = 50;
        nc.pingIntervalInMs = 1000;
        nc.maxPingsOut = 2;
        
        CSDConfig sd = {};
        sd.endpoints = "http://127.0.0.1:2379";
        sd.etcdPrefix = "pitaya/";
        sd.heartbeatTTLSec = 5;
        sd.logHeartbeat = false;
        sd.logServerSync = false;
        sd.logServerDetails = false;
        sd.syncServersIntervalSec = 20;
        sd.serverTypeFilters = nullptr;
        sd.initializationTimeoutSec = 10;   // Maior timeout para permitir retries
        
        CServer sv = {};
        sv.id = (char*)"test";
        sv.type = (char*)"test";
        sv.metadata = (char*)"{}";
        sv.hostname = (char*)"localhost";
        sv.frontend = true;
        
        tfg_pitc_InitializeWithNats(&nc, &sd, &sv, LogLevel_Info, nullptr);
        
        std::cout << "[Thread] Inicialização retornou, aguardando SIGTERM dos retries..." << std::endl;
        
        // Aguarda o SIGTERM do ClosedCb quando os retries falharem
        for (int i = 0; i < 100 && !gSigtermReceived; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        threadFinished = true;
    });
    
    // Aguarda com timeout
    int timeout_sec = 15;
    for (int i = 0; i < timeout_sec * 10; i++) {
        if (gSigtermReceived) {
            worker.join();
            std::cout << "✓ TESTE 2 PASSOU: SIGTERM recebido pelo processo pai após retries falharem" << std::endl;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Timeout
    worker.join();
    std::cout << "✗ TESTE 2 FALHOU: Timeout esperando SIGTERM" << std::endl;
    return false;
}

// Teste 3: Conexão bem-sucedida - NÃO deve enviar SIGTERM
bool test_successful_connection() {
    std::cout << "\n=== TESTE 3: Conexão bem-sucedida (sem SIGTERM) ===" << std::endl;
    
    reset_test_state();
    install_signal_handler();
    
    std::atomic<bool> initSuccess{false};
    
    std::thread worker([&initSuccess]() {
        std::cout << "[Thread] Tentando conectar ao NATS e etcd reais..." << std::endl;
        
        CNATSConfig nc = {};
        nc.addr = "nats://127.0.0.1:4222";  // NATS local (Docker)
        nc.connectionTimeoutMs = 5000;
        nc.requestTimeoutMs = 5000;
        nc.serverShutdownDeadlineMs = 1000;
        nc.serverMaxNumberOfRpcs = 100;
        nc.maxReconnectionAttempts = 3;
        nc.maxPendingMsgs = 1000;
        nc.reconnectWaitInMs = 100;
        nc.reconnectBufSize = 8*1024*1024;
        nc.reconnectJitterInMs = 50;
        nc.pingIntervalInMs = 1000;
        nc.maxPingsOut = 2;
        
        CSDConfig sd = {};
        sd.endpoints = "http://127.0.0.1:2379";  // etcd local (Docker)
        sd.etcdPrefix = "pitaya/";
        sd.heartbeatTTLSec = 5;
        sd.logHeartbeat = false;
        sd.logServerSync = false;
        sd.logServerDetails = false;
        sd.syncServersIntervalSec = 20;
        sd.serverTypeFilters = nullptr;
        sd.initializationTimeoutSec = 10;
        
        CServer sv = {};
        sv.id = (char*)"test-server-1";
        sv.type = (char*)"connector";
        sv.metadata = (char*)"{}";
        sv.hostname = (char*)"localhost";
        sv.frontend = true;
        
        bool result = tfg_pitc_InitializeWithNats(&nc, &sd, &sv, LogLevel_Info, nullptr);
        
        if (result) {
            std::cout << "[Thread] ✓ Inicialização bem-sucedida!" << std::endl;
            initSuccess = true;
            
            // Aguarda um pouco para garantir estabilidade
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // Termina normalmente
            tfg_pitc_Terminate();
            std::cout << "[Thread] Terminado normalmente" << std::endl;
        } else {
            std::cout << "[Thread] ✗ Inicialização falhou!" << std::endl;
        }
    });
    
    // Aguarda a thread com timeout
    int timeout_sec = 20;
    for (int i = 0; i < timeout_sec * 10; i++) {
        if (gSigtermReceived) {
            worker.join();
            std::cout << "✗ TESTE 3 FALHOU: SIGTERM foi recebido (não deveria!)" << std::endl;
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Se a thread terminou com sucesso
        if (initSuccess) {
            break;
        }
    }
    
    worker.join();
    
    if (initSuccess && !gSigtermReceived) {
        std::cout << "✓ TESTE 3 PASSOU: Conexão bem-sucedida, sem SIGTERM" << std::endl;
        return true;
    }
    
    if (!initSuccess) {
        std::cout << "✗ TESTE 3 FALHOU: Inicialização não foi bem-sucedida" << std::endl;
    }
    
    return false;
}

int main(int argc, char* argv[]) {
    bool run_all = (argc == 1);
    bool run_test1 = run_all || (argc > 1 && strcmp(argv[1], "1") == 0);
    bool run_test2 = run_all || (argc > 1 && strcmp(argv[1], "2") == 0);
    bool run_test3 = run_all || (argc > 1 && strcmp(argv[1], "3") == 0);
    
    int failures = 0;
    
    if (run_test1) {
        if (!test_sigterm_no_retry()) failures++;
    }
    
    if (run_test2) {
        if (!test_sigterm_with_retry()) failures++;
    }
    
    if (run_test3) {
        if (!test_successful_connection()) failures++;
    }
    
    std::cout << "\n=== RESULTADO ===" << std::endl;
    if (failures == 0) {
        std::cout << "✓ Todos os testes passaram!" << std::endl;
        return 0;
    } else {
        std::cout << "✗ " << failures << " teste(s) falhou(aram)" << std::endl;
        return 1;
    }
}
