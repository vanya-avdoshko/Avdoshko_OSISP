#include <iostream>
#include <windows.h>

int main() {
    
    HANDLE hPipe = INVALID_HANDLE_VALUE;

    while (true) {
        hPipe = CreateNamedPipe(
            L"\\\\.\\pipe\\ServerPipe", 
            PIPE_ACCESS_DUPLEX,         
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
            PIPE_UNLIMITED_INSTANCES,   
            1024,                       
            1024,                       
            0,                          // ќжидание по умолчанию
            NULL                        // јтрибуты безопасности
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create named pipe. Error code: " << GetLastError() << std::endl;
            return 1;
        }

        std::cout << "Server started, waiting for connections...\n";
        // ќжидаем подключени€ клиента
        BOOL success = ConnectNamedPipe(hPipe, NULL);
        if (!success) {
            std::cerr << "Failed to connect to the named pipe. Error code: " << GetLastError() << std::endl;
            CloseHandle(hPipe);
            continue; 
        }

        std::wcout << L"Client connected.\n";


        char buffer[1024];
        DWORD bytesRead;

        
        while (true) {
            success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);
            if (!success || bytesRead == 0) {
                std::cerr << "Error reading data from the pipe. Error code: " << GetLastError() << std::endl;
                break; 
            }

            buffer[bytesRead] = '\0'; 
            std::wcout << L"Received: " << buffer << std::endl;

            
            std::string response = "Message received: " + std::string(buffer);
            DWORD bytesWritten;
            success = WriteFile(hPipe, response.c_str(), response.length(), &bytesWritten, NULL);

            if (!success) {
                std::cerr << "Error writing response to pipe. Error code: " << GetLastError() << std::endl;
                break;
            }
        }

        
        DisconnectNamedPipe(hPipe);
        std::cout << "Client disconnected.\n";

        
        CloseHandle(hPipe);
    }

    return 0;
}
