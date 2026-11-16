#include "NetworkServer.h"
#include <iostream>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

namespace ASIS {

NetworkServer::NetworkServer(int port)
    : m_port(port),
      m_running(false),
      m_connectedClients(0),
      m_listenSocket(INVALID_SOCKET)
{
}

NetworkServer::~NetworkServer() {
    stop();
}

bool NetworkServer::start() {
    if (m_running) return true;

    // 1. Inicializar Winsock API
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        std::cerr << "[ASIS Network ERROR] WSAStartup falló con código: " << wsaResult << std::endl;
        return false;
    }

    // 2. Crear Socket TCP
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) {
        std::cerr << "[ASIS Network ERROR] Creación de socket falló: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // 3. Reutilizar dirección y puerto
    BOOL reuse = TRUE;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    // 4. Configurar dirección local (127.0.0.1)
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Solo conexiones locales seguras
    serverAddr.sin_port = htons(static_cast<u_short>(m_port));

    // 5. Vincular (Bind)
    if (bind(m_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[ASIS Network ERROR] Falló el bind en el puerto " << m_port << ": " << WSAGetLastError() << std::endl;
        closesocket(m_listenSocket);
        WSACleanup();
        return false;
    }

    // 6. Escuchar conexiones entrantes
    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[ASIS Network ERROR] Falló listen en el puerto " << m_port << ": " << WSAGetLastError() << std::endl;
        closesocket(m_listenSocket);
        WSACleanup();
        return false;
    }

    m_running = true;

    // 7. Lanzar hilo de escucha de conexiones
    m_serverThread = std::thread(&NetworkServer::serverLoop, this);

    std::cout << "[ASIS Network] Servidor local TCP iniciado en 127.0.0.1:" << m_port << " para el Módulo de IA.\n";
    return true;
}

void NetworkServer::stop() {
    if (!m_running) return;

    m_running = false;

    // Cerrar socket de escucha para desbloquear accept()
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    // Cerrar clientes conectados
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (SOCKET client : m_clientSockets) {
            closesocket(client);
        }
        m_clientSockets.clear();
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }

    WSACleanup();
    std::cout << "[ASIS Network] Servidor socket cerrado correctamente.\n";
}

void NetworkServer::serverLoop() {
    while (m_running) {
        // Usar select con timeout de 500ms para revisar m_running periódicamente
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_listenSocket, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000; // 500 ms

        int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (selectResult > 0 && FD_ISSET(m_listenSocket, &readSet)) {
            sockaddr_in clientAddr{};
            int addrLen = sizeof(clientAddr);
            SOCKET clientSocket = accept(m_listenSocket, (sockaddr*)&clientAddr, &addrLen);

            if (clientSocket != INVALID_SOCKET) {
                {
                    std::lock_guard<std::mutex> lock(m_clientsMutex);
                    m_clientSockets.push_back(clientSocket);
                    m_connectedClients++;
                }

                std::cout << "[ASIS Network] Cliente de IA conectado. Clientes activos: " << m_connectedClients << "\n";

                // Procesar cliente en un hilo independiente
                std::thread(&NetworkServer::handleClient, this, clientSocket).detach();
            }
        }
    }
}

void NetworkServer::handleClient(SOCKET clientSocket) {
    char buffer[4096];
    std::string accumulatedData;

    while (m_running) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            break; // Cliente desconectado o error
        }

        buffer[bytesReceived] = '\0';
        accumulatedData.append(buffer, bytesReceived);

        // Procesar paquetes delimitados por fin de bloque "###END###" o salto de línea doble "\n\n"
        size_t delimiterPos;
        while ((delimiterPos = accumulatedData.find("###END###")) != std::string::npos) {
            std::string command = accumulatedData.substr(0, delimiterPos);
            accumulatedData.erase(0, delimiterPos + 9); // Longitud de "###END###"

            if (!command.empty()) {
                std::lock_guard<std::mutex> lock(m_commandMutex);
                m_incomingCommands.push_back(command);
            }

            // Enviar acuse de recibo (ACK) al cliente Python
            std::string ack = "{\"status\": \"received\"}\n";
            send(clientSocket, ack.c_str(), static_cast<int>(ack.length()), 0);
        }
    }

    // Limpieza al desconectar el cliente
    closesocket(clientSocket);
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = std::find(m_clientSockets.begin(), m_clientSockets.end(), clientSocket);
        if (it != m_clientSockets.end()) {
            m_clientSockets.erase(it);
            m_connectedClients--;
        }
    }
    std::cout << "[ASIS Network] Cliente de IA desconectado.\n";
}

std::vector<std::string> NetworkServer::pollIncomingCommands() {
    std::lock_guard<std::mutex> lock(m_commandMutex);
    std::vector<std::string> commands = std::move(m_incomingCommands);
    m_incomingCommands.clear();
    return commands;
}

void NetworkServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    std::string payload = message + "\n";
    for (SOCKET client : m_clientSockets) {
        send(client, payload.c_str(), static_cast<int>(payload.length()), 0);
    }
}

} // namespace ASIS
