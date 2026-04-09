#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <string>

#define DEFAULT_BUFLEN 512
#define MAX_CLIENTS 64

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

struct ClientInfo {
    SOCKET socket;
    std::string name;
    bool active;
    DWORD lastMessageTime;
};

ClientInfo clients[MAX_CLIENTS];
int clientCount = 0;

//Рассылка сообщений
void Messagening(std::string& message, SOCKET excludeSocket = INVALID_SOCKET) {
    for (int i = 0; i < clientCount; i++) {
        if (clients[i].active && clients[i].socket != excludeSocket) {
            send(clients[i].socket, message.c_str(), (int)message.length(), 0);
        }
    }
    std::cout << message << std::endl;
}

DWORD WINAPI ClientHandler(LPVOID indx) {
    int index = (int)indx;

    //Получаем имя
    char recvbuf[DEFAULT_BUFLEN];
    int iResult = recv(clients[index].socket, recvbuf, DEFAULT_BUFLEN, 0);
    if (iResult > 0) {
        recvbuf[iResult] = '\0';
        clients[index].name = recvbuf;
        clients[index].active = true;

        std::string joinMsg = "[SERVER]: " + clients[index].name + " присоединился к чату";
        Messagening(joinMsg, clients[index].socket);
    }

    //Цикл обработки сообщений
    while (clients[index].active) {
        iResult = recv(clients[index].socket, recvbuf, DEFAULT_BUFLEN, 0);

        if (iResult > 0) {
            recvbuf[iResult] = '\0';
            std::string msg = recvbuf;

            if (msg == "/exit") {
                std::cout << "[COMMAND]: " << clients[index].name << " ввёл команду /exit" << std::endl;

                std::string leaveMsg = "[SERVER]: " + clients[index].name + " покинул чат";
                Messagening(leaveMsg, clients[index].socket);
                clients[index].active = false;
            }
            else if (msg == "/users") {
                std::cout << "[COMMAND]: " << clients[index].name << " ввёл команду /users" << std::endl;

                std::string userList = "[SERVER]: Активные пользователи:\n";
                for (int i = 0; i < clientCount; i++) {
                    if (clients[i].active) {
                        userList += "  - " + clients[i].name + "\n";
                    }
                }
                send(clients[index].socket, userList.c_str(), (int)userList.length(), 0);
            }
            else {
                DWORD now = GetTickCount64();

                if (now - clients[index].lastMessageTime < 3000) {
                    std::string warn = "[SERVER]: Подождите 3 секунды перед следующим сообщением";
                    send(clients[index].socket, warn.c_str(), (int)warn.length(), 0);
                    continue;
                }

                clients[index].lastMessageTime = now;

                std::string formatted = "[" + clients[index].name + "]: " + msg;
                Messagening(formatted, clients[index].socket);
            }
        }
        else if (iResult == 0 || iResult == SOCKET_ERROR) {
            clients[index].active = false;
            std::string leaveMsg = "[SERVER]: " + clients[index].name + " отключился";
            clientCount--;
            Messagening(leaveMsg, clients[index].socket);
        }
    }

    closesocket(clients[index].socket);
    return 0;
}

int main() {
    //setlocale(0, "rus");
    SetConsoleCP(1251); //чот локализация без этого рандомно то работает то не работает
    SetConsoleOutputCP(1251);


    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = false;
        clients[i].socket = INVALID_SOCKET;
        clients[i].name = "";
        clients[i].lastMessageTime = 0;
    }


    //------------------------БЕЗ ИЗМЕНЕНИЙ ИЗ ПРЕЗЕНТАЦИИ------------------------
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
#define DEFAULT_PORT "27015"
    struct addrinfo* result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    SOCKET ListenSocket = INVALID_SOCKET;
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cout << "bind failed: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    //------------------------БЕЗ ИЗМЕНЕНИЙ ИЗ ПРЕЗЕНТАЦИИ------------------------


    std::cout << "Сервер запущен" << std::endl;
    std::cout << "Ожидание подключений..." << std::endl;

    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);

        if (ClientSocket != INVALID_SOCKET) {

            int freeSlot = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i].active) {
                    freeSlot = i;
                    break;
                }
            }

            if (freeSlot == -1) {
                std::string errMsg = "[SERVER]: Чат переполнен, попробуйте позже";
                send(ClientSocket, errMsg.c_str(), (int)errMsg.length(), 0);
                closesocket(ClientSocket);
            }
            else {
                clients[freeSlot].socket = ClientSocket;
                clients[freeSlot].active = false;
                clientCount++;
                CreateThread(NULL, 0, ClientHandler, (LPVOID)freeSlot, 0, NULL);

                std::cout << "Новый клиент подключен. Всего клиентов: " << clientCount << std::endl;
            }
        }
    }

    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}