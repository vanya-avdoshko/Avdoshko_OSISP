#include <iostream>
#include <windows.h>
#include <string>

int main() {
    const std::wstring pipeName = L"\\\\.\\pipe\\ServerPipe";


    HANDLE hPipe = CreateFile(
        pipeName.c_str(),          
        GENERIC_READ | GENERIC_WRITE, 
        0,                          
        NULL,                       
        OPEN_EXISTING,              
        0,                          
        NULL                        
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to connect to the server pipe. Error code: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Connected to the server pipe.\n";
    std::cout << "You can start typing messages. To exit, type 'exit'.\n";

    std::string message;
    while (true) {
        std::cout << "Enter message: ";
        std::getline(std::cin, message);

        if (message == "exit") {
            break;
        }

        DWORD bytesWritten;
        BOOL success = WriteFile(hPipe, message.c_str(), message.length(), &bytesWritten, NULL);
        if (!success) {
            std::cerr << "Error writing message to the pipe. Error code: " << GetLastError() << std::endl;
            break;
        }

        std::cout << "Message sent: " << message << std::endl;

        char buffer[1024];
        DWORD bytesRead;
        success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);
        if (!success || bytesRead == 0) {
            std::cerr << "Error reading server response. Error code: " << GetLastError() << std::endl;
            break;
        }

        buffer[bytesRead] = '\0'; 
        std::cout << "Server response: " << buffer << std::endl;
    }

    CloseHandle(hPipe);
    return 0;
}
