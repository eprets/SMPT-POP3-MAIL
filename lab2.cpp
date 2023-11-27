#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")

using namespace std;

const char* SERVER = "pop.mail.ru";
const int PORT = 995;

const char* USERNAME = "katya_prets@mail.ru";
const char* PASSWORD = "6dr6GZdNqduhQPLFC7mA";


int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "Failed to initialize Winsock" << endl;
        return 1;
    }

    SOCKET pop3Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (pop3Socket == INVALID_SOCKET)
    {
        cerr << "Failed to create socket" << endl;
        WSACleanup();
        return 1;
    }

    struct hostent* host;
    host = gethostbyname(SERVER);

    SOCKADDR_IN server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    // Инициализация библиотеки OpenSSL
    SSL_library_init();
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, pop3Socket);

    if (connect(pop3Socket, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        cerr << "Failed to connect to server" << endl;
        closesocket(pop3Socket);
        WSACleanup();
        return 1;
    }

    // Установка безопасного соединения
    if (SSL_connect(ssl) != 1)
    {
        cerr << "Failed to establish an SSL/TLS connection" << endl;
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        closesocket(pop3Socket);
        WSACleanup();
        return 1;
    }

    char recvBuffer[4096];

    // Получаем ответ от сервера
    int bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';
    cout << recvBuffer << endl;

    // Отправляем команду USER
    string userCommand = "USER " + string(USERNAME) + "\r\n";
    SSL_write(ssl, userCommand.c_str(), userCommand.length());
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';
    cout << recvBuffer << endl;

    // Отправляем команду PASS
    string passCommand = "PASS " + string(PASSWORD) + "\r\n";
    SSL_write(ssl, passCommand.c_str(), passCommand.length());
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';
    cout << recvBuffer << endl;

    // Отправляем команду STAT для получения статистики по почтовому ящику
    const char* statCommand = "STAT\r\n";
    SSL_write(ssl, statCommand, strlen(statCommand));
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';
    cout << "STAT" << endl;
    cout << recvBuffer << endl;

    // Отправляем команду QUIT для завершения сессии
    const char* quitCommand = "QUIT\r\n";
    SSL_write(ssl, quitCommand, strlen(quitCommand));
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';
    cout << recvBuffer << endl;

    // Завершение работы с SSL/TLS
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);

    // Закрытие сокса и освобождение ресурсов
    closesocket(pop3Socket);
    WSACleanup();

    
    return 0;
}