#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string>

#define DEFAULT_BUFLEN 512

#pragma comment(lib, "Ws2_32.lib")

SOCKET ConnectSocket = INVALID_SOCKET;
bool connected = true;

DWORD WINAPI forRecvThr(LPVOID) {
    char recvbuf[DEFAULT_BUFLEN];

    while (connected) {
        int iResult = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);

        if (iResult > 0) {
            recvbuf[iResult] = '\0';
            std::cout << recvbuf << std::endl;
        }
        else {
            std::cout << "[SERVER]: Соединение разорвано" << std::endl;
            connected = false;
            break;
        }
    }

    return 0;
}

int main() {
    //setlocale(0, "rus");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);


    //------------------------БЕЗ ИЗМЕНЕНИЙ ИЗ ПРЕЗЕНТАЦИИ------------------------
    WSADATA wsaData;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
#define DEFAULT_PORT "27015" 
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    //------------------------БЕЗ ИЗМЕНЕНИЙ ИЗ ПРЕЗЕНТАЦИИ------------------------


    std::string ip;
    std::cout << "Введите IP сервера (по умолчанию поставил 192.168.0.102): ";
    std::getline(std::cin, ip);
    if (ip.empty()) {
        ip = "192.168.0.102";
    }


    //---------------------(почти)БЕЗ ИЗМЕНЕНИЙ ИЗ ПРЕЗЕНТАЦИИ--------------------
    if (getaddrinfo(ip.c_str(), DEFAULT_PORT, &hints, &result) != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    if (connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    //---------------------(почти)БЕЗ ИЗМЕНЕНИЙ ИЗ ПРЕЗЕНТАЦИИ--------------------


    std::cout << "Подключено к серверу!" << std::endl;
    std::cout << "Введите ваше имя: ";

    std::string name;
    std::getline(std::cin, name);
    send(ConnectSocket, name.c_str(), (int)name.length(), 0);

    CreateThread(NULL, 0, forRecvThr, NULL, 0, NULL);


    std::cout << "\nДоступные команды:" << std::endl;
    std::cout << "  /exit - выход из чата" << std::endl;
    std::cout << "  /users - список пользователей" << std::endl;
    std::cout << "\nВведите сообщение: " << std::endl;

    std::string message;
    while (connected) {
        std::getline(std::cin, message);

        if (message.empty()) continue;

        send(ConnectSocket, message.c_str(), (int)message.length(), 0);

        if (message == "/exit") {
            connected = false;
            break;
        }
    }

    shutdown(ConnectSocket, SD_SEND);
    closesocket(ConnectSocket);
    WSACleanup();
    std::cout << "Вы вышли из чата." << std::endl;
    return 0;
}