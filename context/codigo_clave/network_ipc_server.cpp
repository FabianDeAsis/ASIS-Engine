// Snippet Clave: Servidor Socket Local (IPC) de C++ para Comunicación con la IA
// Archivo de referencia: src/core/NetworkServer.cpp

#include <winsock2.h>
#include <vector>
#include <string>
#include <mutex>

// Extraer comandos entrantes de la IA de forma segura (Thread-Safe)
// para ejecutarlos en el hilo principal de OpenGL
std::vector<std::string> pollAICommands(std::vector<std::string>& queue, std::mutex& mtx) {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> pending = std::move(queue);
    queue.clear();
    return pending;
}
