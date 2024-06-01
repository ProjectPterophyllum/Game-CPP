#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <chrono>
#include <thread>

const int PORT = 8080;
const int TIMEOUT_SEC = 60;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Создание сокета
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Ввод адреса сервера
    //std::cout << "Enter server address: ";
    std::string server_addr = "127.0.0.1";
    //std::cin >> server_addr;
    inet_pton(AF_INET, server_addr.c_str(), &serv_addr.sin_addr);

    // Подключение к серверу
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Failed to connect to server" << std::endl;
        return -1;
    }

    // Установка таймаута на чтение
    timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (true) {
        int number;

        // Ввод числа
        std::cout << "Enter your number (1-9): ";
        std::cin >> number;

        if (number < 1 || number > 9) {
            std::cout << "Invalid number, try again" << std::endl;
            continue;
        }

        // Отправка числа на сервер
        send(sock, &number, sizeof(number), 0);

        char buffer[1024];

        // Получение сообщения от сервера
        int bytes_received = recv(sock, buffer, 1024, 0);
        if (bytes_received <= 0) {
            // Сервер отключился или превысил таймаут
            std::cout << "Server disconnected or timeout exceeded. Waiting for another player..." << std::endl;
            continue;
        }

        std::cout << buffer << std::endl;

        // Если игрок выиграл или проиграл, закрываем соединение и выходим из цикла
        if (std::string(buffer, 9) == "You win!" || std::string(buffer, 9) == "You lose") {
            close(sock);
            break;
        }
    }

    return 0;
}