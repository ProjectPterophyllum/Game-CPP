#include <iostream>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

const int PORT = 8080;
const int TIMEOUT_SEC = 60;

void handle_game(int client1_fd, int client2_fd)
{
    int secret_number = rand() % 9 + 1;

    std::cout << "New game started. Secret number: " << secret_number << std::endl;

    int winner = -1;
    int closest_number = 10;

    for (int client_fd : {client1_fd, client2_fd})
    {
        int client_number;

        // Установка таймаута на чтение
        timeval timeout;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // Получение числа от клиента
        int bytes_received = recv(client_fd, &client_number, sizeof(client_number), 0);
        if (bytes_received <= 0)
        {
            // Клиент отключился или превысил таймаут
            if (client_fd == client1_fd)
            {
                close(client2_fd);
                send(client1_fd, "Waiting for another player...", 26, 0);
            }
            else
            {
                close(client1_fd);
                send(client2_fd, "Waiting for another player...", 26, 0);
            }
            return;
        }

        int diff = abs(secret_number - client_number);

        if (diff <= closest_number)
        {
            closest_number = diff;
            winner = client_fd;
        }
    }

    for (int client_fd : {client1_fd, client2_fd})
    {
        if (client_fd == winner)
        {
            // Отправка сообщения об победе
            send(client_fd, "You win!", 9, 0);
            std::cout << "Player " << (client_fd == client1_fd ? "1" : "2") << " wins!" << std::endl;
        }
        else
        {
            // Отправка сообщения об поражении
            send(client_fd, "You lose", 9, 0);
            std::cout << "Player " << (client_fd == client1_fd ? "1" : "2") << " loses." << std::endl;
        }
    }

    std::cout << "Игра закончена, закрытие соединения с игроками.";
    close(client1_fd);
    close(client2_fd);
}

int main()
{
    srand(time(0));

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Создание сокета
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }

    // Настройка сокета
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1)
    {
        std::cerr << "Failed to set socket options" << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Привязка сокета к адресу и порту
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        std::cerr << "Failed to bind socket" << std::endl;
        return -1;
    }

    // Ожидание подключений
    if (listen(server_fd, 10) == -1)
    {
        std::cerr << "Failed to listen on socket" << std::endl;
        return -1;
    }

    std::queue<int> clients;

    std::cout << "Waiting players...\n";
    while (true)
    {

        // Принятие нового подключения
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) == -1)
        {
            std::cerr << "Failed to accept new connection" << std::endl;
            return -1;
        }

        std::cout << "New player connected" << std::endl;

        clients.push(new_socket);

        if (clients.size() >= 2)
        {
            int client1_fd = clients.front();
            clients.pop();
            int client2_fd = clients.front();
            clients.pop();

            // Запуск нового потока для обработки игры
            std::thread game_thread(handle_game, client1_fd, client2_fd);
            game_thread.detach();
        }
    }

    return 0;
}