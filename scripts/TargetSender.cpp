#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return -1;
    }
#endif

    const char* serverIP = "127.0.0.1";
    const int port = 11000;
    
    std::cout << "Enter target coordinates as \"x,y,z\" or type \"home\" to return to starting position." << std::endl;
    std::cout << "Type \"quit\" to exit." << std::endl;
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input == "quit") {
            break;
        }
        
        std::istringstream iss(input);
        std::string trimmed;
        iss >> trimmed;
        if (trimmed.empty())
            continue;
            
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Error creating socket." << std::endl;
            continue;
        }
        
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(serverIP);
        
        if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error connecting to server." << std::endl;
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            continue;
        }
        

        std::string message;
        if (trimmed == "home") {
            message = "home";
        } else {
            if (std::count(trimmed.begin(), trimmed.end(), ',') < 2) {
                std::cerr << "Invalid input format. Please enter coordinates as x,y,z or the command \"home\"." << std::endl;
#ifdef _WIN32
                closesocket(sock);
#else
                close(sock);
#endif
                continue;
            }
            message = trimmed;
        }
        
        int bytesSent = send(sock, message.c_str(), message.size(), 0);
        if (bytesSent < 0) {
            std::cerr << "Error sending data." << std::endl;
        } else {
            std::cout << "Sent: " << message << std::endl;
        }
        
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
