#include <iostream>
#include <winsock2.h>
#include <process.h>  // для _beginthreadex
#include <map>
#include <string>
#include <vector>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

std::map<int, SOCKET> clients;  // Хранение клиентов с их ID
std::mutex clientsMutex;

struct Message {
    int senderID;
    int receiverID;  // -1 для широковещательного сообщения
    std::string text;
};

// Временное хранилище сообщений
std::vector<Message> messageQueue;
std::mutex messageQueueMutex;

unsigned __stdcall clientHandler(void* clientData) {
    auto clientPair = *reinterpret_cast<std::pair<int, SOCKET>*>(clientData);
    int clientID = clientPair.first;
    SOCKET clientSocket = clientPair.second;
    delete reinterpret_cast<std::pair<int, SOCKET>*>(clientData);  // очистка

    char buffer[1024];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cerr << "Client " << clientID << " disconnected." << std::endl;
            closesocket(clientSocket);
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.erase(clientID);
            return 0;
        }

        buffer[bytesReceived] = '\0';
        std::string messageText(buffer);

        // Обработка сообщения: предполагаем, что первые символы - ID получателя
        int receiverID = -1;  // -1 для широковещательной передачи
        if (messageText[0] == '@') {
            receiverID = std::stoi(messageText.substr(1, messageText.find(' ')));
            messageText = messageText.substr(messageText.find(' ') + 1);
        }

        // Добавление сообщения в очередь
        {
            std::lock_guard<std::mutex> lock(messageQueueMutex);
            messageQueue.push_back({ clientID, receiverID, messageText });
        }
    }
    return 0;
}

unsigned __stdcall messageDispatcher(void* param) {
    while (true) {
        std::lock_guard<std::mutex> lock(messageQueueMutex);
        for (const auto& msg : messageQueue) {
            std::lock_guard<std::mutex> lock(clientsMutex);

            if (msg.receiverID == -1) {
                // Широковещательная рассылка всем клиентам
                for (const auto& client : clients) {
                    if (client.first != msg.senderID) {
                        send(client.second, msg.text.c_str(), msg.text.length(), 0);
                    }
                }
            }
            else {
                // Отправка конкретному клиенту
                auto it = clients.find(msg.receiverID);
                if (it != clients.end()) {
                    send(it->second, msg.text.c_str(), msg.text.length(), 0);
                }
            }
        }
        messageQueue.clear();
        Sleep(100); // ожидание перед следующим циклом обработки
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create server socket." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening..." << std::endl;

    HANDLE dispatcherThread = (HANDLE)_beginthreadex(nullptr, 0, messageDispatcher, nullptr, 0, nullptr);

    int clientID = 1;
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients[clientID] = clientSocket;
        }

        std::cout << "Client " << clientID << " connected." << std::endl;

        auto clientData = new std::pair<int, SOCKET>(clientID, clientSocket);
        HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, clientHandler, clientData, 0, nullptr);
        clientID++;
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
