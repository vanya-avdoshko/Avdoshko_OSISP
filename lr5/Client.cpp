#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <process.h>  // для _beginthreadex

#pragma comment(lib, "ws2_32.lib")

unsigned __stdcall receiveMessages(void* clientSocketPtr) {
    SOCKET clientSocket = *(SOCKET*)clientSocketPtr;
    char buffer[1024];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Message from server: " << buffer << std::endl;
        }
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create client socket." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(8080);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server." << std::endl;

    HANDLE receiveThread = (HANDLE)_beginthreadex(nullptr, 0, receiveMessages, &clientSocket, 0, nullptr);

    char message[1024];
    while (true) {
        std::cout << "Enter message (@<ID> to send directly): ";
        std::cin.getline(message, sizeof(message));
        send(clientSocket, message, strlen(message), 0);
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
