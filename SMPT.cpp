#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")

using namespace std;

const char* SERVER = "smtp.mail.ru";
const int PORT = 465; // порт для SSL/TLS

const char* USERNAME = "katya_prets@mail.ru";
const char* PASSWORD = "NmRyNkdaZE5xZHVoUVBMRkM3bUE=";
const char* RECIPIENT = "katya_prets@mail.ru";

const int MAX_SUBJECT_LENGTH = 100;
const int MAX_TEXT_LENGTH = 500;

const char* END_OF_DATA = "\r\n.\r\n"; // Завершение данных для письма
const char* QUIT = "QUIT\r\n"; // Завершение сессии

string base64Encode(const string& input)
{
    const string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    string encoded;
    size_t inputLength = input.length();

    for (size_t i = 0; i < inputLength; i += 3)
    {
        unsigned char octet1 = input[i];
        unsigned char octet2 = (i + 1 < inputLength) ? input[i + 1] : 0;
        unsigned char octet3 = (i + 2 < inputLength) ? input[i + 2] : 0;

        unsigned char sextet1 = (octet1 & 0xFC) >> 2;
        unsigned char sextet2 = ((octet1 & 0x03) << 4) | ((octet2 & 0xF0) >> 4);
        unsigned char sextet3 = ((octet2 & 0x0F) << 2) | ((octet3 & 0xC0) >> 6);
        unsigned char sextet4 = octet3 & 0x3F;

        encoded.push_back(base64Chars[sextet1]);
        encoded.push_back(base64Chars[sextet2]);
        encoded.push_back(base64Chars[sextet3]);
        encoded.push_back(base64Chars[sextet4]);
    }
    // Паддинг для выравнивания до длины кратной 4
    size_t padding = inputLength % 3;
    if (padding > 0)
    {
        encoded.replace(encoded.length() - padding, padding, padding, '=');
    }
    return encoded;
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "Failed to initialize Winsock" << endl;
        return 1;
    }

    SOCKET smtpSocket = socket(AF_INET, SOCK_STREAM, 0); // создание сокета
    if (smtpSocket == INVALID_SOCKET)
    {
        throw runtime_error("Failed to create socket");
    }

    struct hostent* host;
    host = gethostbyname(SERVER);

    SOCKADDR_IN server;                     // определяет адрес и порт сервера
    server.sin_family = AF_INET;            // тип адреса
    server.sin_port = htons(PORT);          // содержит номер порта, преобразование в сетевой формат 
    server.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    SSL_library_init();                     // Инициализация OpenSSL
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
    //SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, smtpSocket); // указываем через какой сокет идут зашифрованные данные
    if (connect(smtpSocket, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        throw runtime_error("Failed to connect to server");
    }
    // Установка безопасного соединения
    if (SSL_connect(ssl) != 1)
    {
        ERR_print_errors_fp(stderr);
        throw runtime_error("Failed to establish an SSL/TLS connection");
    }
    // Массив для получения ответов от сервера (SSL_write - отправляем команду и ожидаем ответа от сервера)
    //                                     (SSL_read - ответ от сервера помещается в recvBuffer, после чего мы сможем вывести его в консоль)
    char recvBuffer[4096];

    int bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';
    cout << recvBuffer << endl;

    const char* ehlo = "EHLO mail.ru\r\n";

    // Отправка команды EHLO с аргументом "mail.ru"
    SSL_write(ssl, ehlo, strlen(ehlo));
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';
    cout << recvBuffer << endl;

    // Отправка команды AUTH LOGIN
    const char* auth = "AUTH LOGIN\r\n";

    SSL_write(ssl, auth, strlen(auth));
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';

    // Отправка имени пользователя и пароля в кодировке Base64
    string username1 = base64Encode(string(USERNAME));
    //string username1 = USERNAME;
    username1 = username1 + "\r\n";
    SSL_write(ssl, username1.c_str(), username1.length());
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';

    //string password1 = base64Encode(string(PASSWORD));
    string password1 = PASSWORD;
    password1 = password1 + "\r\n";
    SSL_write(ssl, password1.c_str(), password1.length());
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';

    // Отправка команды MAIL FROM
    cout << "MAIL FROM : " << USERNAME << endl;
    const char* mailfrom = "MAIL FROM\r\n";
    SSL_write(ssl, "MAIL FROM:<katya_prets@mail.ru>\r\n", strlen("MAIL FROM:<katya_prets@mail.ru>\r\n"));
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';

    // Отправка команды RCPT TO
    cout << "RECIPIENT TO : " << RECIPIENT << endl << endl;
    SSL_write(ssl, "RCPT TO:<katya_prets@mail.ru>\r\n", strlen("RCPT TO:<katya_prets@mail.ru>\r\n"));
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';

    // Отправка команды DATA
    SSL_write(ssl, "DATA\r\n", strlen("DATA\r\n"));
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';

    // ====== СОЗДАНИЕ СОДЕРЖИМОГО ПИСЬМА ======
    char Subject[MAX_SUBJECT_LENGTH];
    char Text[MAX_TEXT_LENGTH];

    cout << "Enter the subject of the message: ";
    cin.getline(Subject, MAX_SUBJECT_LENGTH);

    cout << "Enter the text of the message: ";
    cin.getline(Text, MAX_TEXT_LENGTH);

    auto end = chrono::system_clock::now();// Получение времени
    time_t end_time = chrono::system_clock::to_time_t(end);

    // Преобразование времени в строку
    stringstream ss;
    ss << put_time(localtime(&end_time), "%Y-%m-%d %X");
    const string Date = ss.str();

    // Формирование HTML-контента и кодирование его в base64
    string htmlContent = "<html><body><p>" + string(Text) + "</p></body></html>" + string(Date);
    string base64EncodedContent = base64Encode(htmlContent);

    // Формирование MIME-сообщения
    ostringstream mimeContent;
    mimeContent << "From: katya_prets@mail.ru\r\n";
    mimeContent << "To: katya_prets@mail.ru\r\n";
    mimeContent << "Subject: " << Subject << "\r\n";
    mimeContent << "MIME-Version: 1.0\r\n";
    mimeContent << "Content-Type: text/html; charset=utf-8\r\n";
    mimeContent << "Content-Transfer-Encoding: base64\r\n";
    mimeContent << base64EncodedContent << "\r\n";

    string mailContent = mimeContent.str();

    // Преобразование строки в const char*
    const char* MAIL_CONTENT = mailContent.c_str();

    // =========================================
    // Отправка содержания письма
    SSL_write(ssl, MAIL_CONTENT, strlen(MAIL_CONTENT));

    // Завершение данных письма
    SSL_write(ssl, END_OF_DATA, strlen(END_OF_DATA));
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';

    // Отправка команды QUIT
    SSL_write(ssl, QUIT, strlen(QUIT));
    bytesReceived = SSL_read(ssl, recvBuffer, sizeof(recvBuffer));
    recvBuffer[bytesReceived] = '\0';

    // Завершение работы с SSL/TLS
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);

    closesocket(smtpSocket);
    WSACleanup();

    return 0;
}