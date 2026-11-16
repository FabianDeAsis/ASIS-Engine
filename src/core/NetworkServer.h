#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

namespace ASIS {

class NetworkServer {
public:
    NetworkServer(int port = 5555);
    ~NetworkServer();

    // Iniciar el servidor socket en segundo plano
    bool start();

    // Detener el servidor y cerrar conexiones
    void stop();

    // Extraer todos los comandos Lua pendientes recibidos desde el cliente de IA (thread-safe)
    std::vector<std::string> pollIncomingCommands();

    // Enviar respuesta o información de la escena al cliente conectado
    void broadcast(const std::string& message);

    bool isRunning() const { return m_running; }
    int getPort() const { return m_port; }
    int getConnectedClientsCount() const { return m_connectedClients; }

private:
    int m_port;
    std::atomic<bool> m_running;
    std::atomic<int> m_connectedClients;

    SOCKET m_listenSocket;
    std::vector<SOCKET> m_clientSockets;
    std::mutex m_clientsMutex;

    std::thread m_serverThread;

    std::mutex m_commandMutex;
    std::vector<std::string> m_incomingCommands;

    void serverLoop();
    void handleClient(SOCKET clientSocket);
};

} // namespace ASIS
