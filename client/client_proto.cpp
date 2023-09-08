#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <memory>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "mixins.hpp"
#include "client_messages.hpp"

class User {
public:
    int clientSocket;
    std::string name;
    std::string password;

    User(int clientSocket, const std::string& name, const std::string& password)
        : clientSocket(clientSocket), name(std::move(name)), password(std::move(password)) {}

    std::string toString() const { return "User: " + name; }
    std::string getName() const { return name; }
};

class ClientAuth : public ConvertMixin, public DbInterfaceMixin {
public:
    ClientAuth(Database db, const std::string& username = "", const std::string& password = "")
        : DbInterfaceMixin(std::move(db)), username(username), password(password) {}

    bool authenticate(const std::string& username, const std::string& password) {
        if (!username.empty() && !password.empty()) {
            User* usr = nullptr;
            {
                std::lock_guard<std::mutex> l(usersMutex);
                auto user_it = std::find_if(users.begin(), users.end(), [username](const std::shared_ptr<User>& user) {
                    return user->name == username;
                });
                if (user_it != users.end()) {
                    usr = user_it->get();
                }
            }

            if (usr) {
                Client* client = get_client_by_username(username);

                if (client) {
                    // Сравнение хранимого пароля с введенным паролем
                    if (client->password == password) {
                        std::cout << "Authentication successful for user: " << username << std::endl;
                        // Добавить запись в историю клиента
                        add_client_history(usr->clientSocket); // Предполагая, что у вас есть идентификатор клиента в объекте User
                        return true;
                    } else {
                        std::cout << "Authentication failed: Incorrect password for user: " << username << std::endl;
                        return false;
                    }
                } else {
                    std::cout << "Authentication failed: Client not found for user: " << username << std::endl;
                    return false; // Клиент не найден
                }
            } else {
                // Новый пользователь
                {
                    std::lock_guard<std::mutex> l(usersMutex);
                    users.push_back(std::make_shared<User>(-1, username, password));
                    add_client(username, password); // Добавить клиента в базу данных
                    add_client_history(users.back()->clientSocket); // Добавить запись в историю клиента
                    std::cout << "New user registered: " << username << std::endl;
                    return true;
                }
            }
        }
        std::cout << "Authentication failed: Empty username or password" << std::endl;
        return false;
    }

private:
    std::vector<std::shared_ptr<User>> users;
    std::string username;
    std::string password;
    std::mutex usersMutex;
};

class ChatClientProtocol : public ConvertMixin, public DbInterfaceMixin {
public:
    ChatClientProtocol(Database db, void* loop, std::vector<std::string>* tasks = nullptr,
                       const std::string& username = "", const std::string& password = "",
                       void* gui_instance = nullptr, const std::string& kwargs = "", const std::string &server_ip = "", int server_port = -1,
                       std::function<void(const std::unordered_map<std::string, std::string>&)> output_func = nullptr)
        : DbInterfaceMixin(std::move(db)), user(username), password(password), gui_instance(gui_instance),
          tasks(tasks), conn_is_open(false), loop(loop), sockname(""), transport(nullptr),
          output(output_func), server_ip(server_ip), server_port(server_port), client_socket(-1) {
    }

    ~ChatClientProtocol() {
        closeConnection(); // метод закрывает соединение с сервером при уничтожении объекта Client.
    }

    void connectToServer()
    {
        try
        {
            createSocket(); // создаем сокет

            server_address_.sin_family = AF_INET; // устанавливаем тип и
            server_address_.sin_port = htons(server_port); // устанавливаем порт серверного адреса.

            if (inet_pton(AF_INET, server_ip.c_str(), &server_address_.sin_addr) <= 0) // IP-адрес сервера преобразуется из текстового формата в структуру sockaddr_in с помощью функции inet_pton().
            {
                throw std::runtime_error("Error: invalid server IP address");
            }

            if (connect(client_socket, (struct sockaddr *)&server_address_, sizeof(server_address_)) < 0) // подключаемся к серверу
            {
                throw std::runtime_error("Error: failed to connect to server");
            }

            conn_is_open = true; // еслиподключение прошло успешно, устанавливается флаг is_connected_ в значение true.

        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

    void closeConnection()
    {
        if (client_socket != -1)
        {   
            shutdown(client_socket, SHUT_RDWR);
            close(client_socket);
            client_socket = -1;
            conn_is_open = false;
        }
    }

    std::tuple<std::string, int> get_extra_info(void* transport) {
        // Simulated values for host and port
        std::string host = "127.0.0.1";
        int port = 12345;

        return std::make_tuple(host, port);
    }

    std::string serialize_map(const std::unordered_map<std::string, std::string>& map) {
        std::string serialized = "{";
        for (const auto& pair : map) {
            serialized += "\"" + pair.first + "\":\"" + pair.second + "\",";
        }
        serialized.pop_back(); // Remove trailing comma
        serialized += "}";
        return serialized;
    }

    void send_auth(const std::string& user, const std::string& password) {
        if (!user.empty() && !password.empty()) {
            std::unordered_map<std::string, std::string> auth_message = jim.auth(user, password);
            std::string serialized_message = serialize_map(auth_message);
            
            // Replace this with actual sending logic
            std::cout << "Sending auth message: " << serialized_message << std::endl;
        }
    }

    void connection_made(void* transport) {
        // Called when connection is initiated
        std::tuple<std::string, int> info = get_extra_info(transport);

        sockname = std::get<0>(info);
        this->transport = transport;
        send_auth(user, password);
        conn_is_open = true;
    }

    void connection_lost(std::exception_ptr exc) {
        try {
            conn_is_open = false;
            for (auto& task : *tasks) {
                // Implement task cancellation logic
            }
        } catch (...) {
        }
    }

    void data_received(const std::vector<uint8_t>& data) {
        std::unordered_map<std::string, std::string> msg = bytes_to_dict(data);
        
        if (!msg.empty()) {
            try {
                if (msg["action"] == "probe") {
                    // Implement your probe logic
                } else if (msg["action"] == "response") {
                    int code = std::stoi(msg["code"]);
                    if (code == 200) {
                        // Handle success
                    } else if (code == 402) {
                        connection_lost(std::make_exception_ptr(std::runtime_error("Connection cancelled")));
                    } else {
                        // Handle other codes
                        output(msg); // Call your output method
                    }
                } else if (msg["action"] == "msg") {
                    output(msg); // Call your output method
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception: " << e.what() << std::endl;
            }
        }
    }

    void connected() {
        conn_is_open = true;
        output({{"message", "Connected to server\n"}});

        input_thread = std::thread(&ChatClientProtocol::read_input, this);
    }

    void disconnect() {
        conn_is_open = false;
        if (input_thread.joinable()) {
            input_thread.join();
        }
    }

    void read_input() {
        while (conn_is_open) {
            std::string content;
            std::getline(std::cin, content);

            if (content == "exit") {
                disconnect();
                break;
            }

            if (output) {
                output({{"message", content}});
            }
        }
    }

    void sendMessage(const std::string& message, bool silent = false) {
        try {
            if (!conn_is_open) {
                throw std::runtime_error("Error: client is not connected to server");
            }

            std::string message_str = message + "\n";
            if (!silent) {
                std::cout << "Sending message: " << message_str;
            }

            size_t nbytes = 0;
            while (nbytes < message_str.length()) {
                ssize_t result = send(client_socket, message_str.c_str() + nbytes, message_str.length() - nbytes, 0);
                if (result < 0) {
                    throw std::runtime_error("Failed to send message");
                }
                nbytes += result;
            }
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            throw;
        }
    }

    // void send_message(const std::unordered_map<std::string, std::string>& request) {
    //     if (!request.empty()) {
    //         std::vector<uint8_t> msg = dict_to_bytes(request);
    //         msg.push_back('\n');
    //         sendMessage(transport, std::string(msg.begin(), msg.end())); // Предполагается, что у вас есть доступ к объекту transport
    //     }
    // }

    // void send_msg(const std::string& to_user, const std::string& content) {
    //     if (!to_user.empty() && !content.empty()) {
    //         std::unordered_map<std::string, std::string> request = jim.message(user, to_user, content);
    //         std::vector<uint8_t> msg = dict_to_bytes(request);
    //         msg.push_back('\n');
    //         sendMessage(transport, std::string(msg.begin(), msg.end()));
    //     }
    // }

    // функция которая читает сообщение от сервера побайтно до символа новой строки и сохраняет его в строке message. Когда встречается символ новой строки, это означает, что команда завершена, и функция возвращает полученное сообщение.
    std::string receiveMessage() { // не принимает никаких параметров и возвращает строку.
        std::string message; // создаем строку

        for (;;) { // созлаем цикл который будет продолжаться до тех пор, пока не будет прочитан символ новой строки ('\n').
            char c = 0;
            // в каждой итерации цикла происходит:
            int nbytes = recv(client_socket, &c, 1, 0); // читается один символ из сокета с помощью функции recv(). Результат чтения сохраняется в переменную nbytes.
            if (nbytes < 0)
                return "";

            if (nbytes == 1 && c != '\n')           
                message.append(&c, 1); // если был прочитан один байт (nbytes == 1) и этот байт не является символом новой строки (c != '\n'), он добавляется к строке message.

            if (c == '\n') // если был прочитан символ новой строки (c == '\n'), это означает конец команды, и цикл прерывается.
                break;
        }
        
        // // Обработка сообщения перед выводом
        // if (!message.empty()) {
        //     // Если сообщение начинается с "message:", добавляем разделитель и форматируем его
        //     if (message.substr(0, 7) == "message") {
        //         std::string formatted_message = "message:";
        //         std::string temp = message.substr(8);
        //         size_t to_pos = temp.find("to:");
        //         size_t from_pos = temp.find("from:");
        //         if (to_pos != std::string::npos && from_pos != std::string::npos) {
        //             formatted_message += temp.substr(0, to_pos - 1) + ", ";
        //             formatted_message += temp.substr(to_pos, from_pos - to_pos - 1) + ", ";
        //             formatted_message += temp.substr(from_pos);
        //             message = formatted_message;
        //         }
        //     }
        // }
        
        return message;
    }

    // Потоковая функция для периодического опроса сервера на наличие новых сообщений, получает их, выводит на консоль и выполняет дополнительную обработку (если предусмотрено).
    void receiveMessages() {
        //std::cout << "Received message from start!!! " << std::endl;
        while (true) {
            try 
            {
                std::this_thread::sleep_for(std::chrono::seconds(1)); // устанавливаем интервал между запросами на получение новых сообщений.

                // Отправка запроса на получение новых сообщений от сервера
                std::string response;
                {
                    std::lock_guard<std::mutex> l(socket_guard);
                    sendMessage("COMMAND_CHECK_MESSAGES", true); // отправляется запрос на сервер используя функцию sendMessage. Флаг true указывает на то, что сообщение должно быть отправлено без вывода на консоль.
                    // Получение ответа от сервера
                    while (true) {
                        response = receiveMessage(); // ответ сохраняется в переменной response.
                        if (response.substr(0, 9) == "STATUS_NO") {
                            break; // если префикс ответа response равен "STATUS_NO" (длина префикса равна 9), цикл прерывается с помощью break. Это означает, что больше нет сообщений для получения, и переходим к следующей итерации внешнего цикла while (true).
                        }
                        //std::cout << "Received message from server: " << response << std::endl;
                        std::cout << response << std::endl;
                    }
                }
                    //std::cout << "Received message from server: " << response << std::endl;
                // Обработка полученного сообщения и передача его клиенту
                // ...

            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                break;
            }
        }
    }

    // функция обработки которая отвечает за основной цикл выполнения клиентской программы.
    void run()
    {
        while (true) {
            try 
            {
                // Получение сообщения от пользователя
                std::string message;
                std::cout << "Enter command: ";
                std::getline(std::cin, message); // получаем сообщение
                std::cout << "message: " << message << std::endl;

                // Обработка команды "COMMAND_EXIT"
                if (message == "COMMAND_EXIT") {
                    break;
                }

                // Обработка команды "COMMAND_LOGIN"
                if (message == "COMMAND_LOGIN") {

                    // Получение имени пользователя
                    std::string name;
                    std::cout << "Enter name: ";
                    std::getline(std::cin, name);

                    // Получение пароля пользователя
                    std::string password;
                    std::cout << "Enter password: ";
                    std::getline(std::cin, password);

                    // Отправка запроса на авторизацию на сервер
                    std::string request = "COMMAND_LOGIN|" + name + "|" + password;
                    std::string response;
                    {
                        std::lock_guard<std::mutex> l(socket_guard);
                        sendMessage(request);
                        // Получение ответа от сервера
                        response = receiveMessage();
                    }
                    std::cout << "Server response: " << response << std::endl;
                    
                    // Проверяем, успешно ли выполнена команда "COMMAND_LOGIN"
                    {
                        std::thread receive_thread(&ChatClientProtocol::receiveMessages, this); // создается отдельный поток receive_thread, который вызывает функцию receiveMessages для получения сообщений от сервера.
                        receive_thread.detach();  // Отсоединяем поток от главного потока клиента
                    }   // эта процедура позволяет выполнять получение сообщений параллельно с обработкой пользовательских команд.
                    
                }
                // Обработка команды "COMMAND_REGISTER"
                else if (message == "COMMAND_REGISTER") {
                    // Получение имени пользователя
                    std::string name;
                    std::cout << "Enter name: ";
                    std::getline(std::cin, name);

                    // Получение пароля пользователя
                    std::string password;
                    std::cout << "Enter password: ";
                    std::getline(std::cin, password);

                    // Формирование запроса на регистрацию пользователя
                    std::string request = "COMMAND_REGISTER|" + name + "|" + password;
                    std::string response;
                    {
                        std::lock_guard<std::mutex> l(socket_guard);
                        sendMessage(request);

                        // Получение ответа от сервера
                        response = receiveMessage();
                    }
                    std::cout << "Server response: " << response << std::endl;
                }
                // Обработка команды "COMMAND_SEND"
                else if (message == "COMMAND_SEND") {

                    // Получение имени получателя
                    std::string recipient;
                    std::cout << "Enter recipient name: ";
                    std::getline(std::cin, recipient);

                    // Получение текста сообщения
                    std::string text;
                    std::cout << "Enter message: ";
                    std::getline(std::cin, text);

                    // Формирование запроса на отправку сообщения
                    std::string request = "COMMAND_SEND|" + recipient + "|" + text;
                    std::string response;
                    {
                        std::lock_guard<std::mutex> l(socket_guard);
                        sendMessage(request);

                        // Получение ответа от сервера
                        response = receiveMessage();
                    }
                    std::cout << "Server response: " << response << std::endl;
                }
                // Обработка команды "COMMAND_BROADCAST"
                else if (message == "COMMAND_BROADCAST") {

                    // Получение текста сообщения для широковещательной рассылки
                    std::string text;
                    std::cout << "Enter message: ";
                    std::getline(std::cin, text);
                
                    // Формирование запроса на широковещательную рассылку сообщения
                    std::string request = "COMMAND_BROADCAST|" + text;
                    std::string response;
                    {
                        std::lock_guard<std::mutex> l(socket_guard);
                        sendMessage(request);

                        // Получение ответа от сервера
                        response = receiveMessage();
                    }
                    std::cout << "Server response: " << response << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                break;
            }
        }
        closeConnection();
    }

    void output_to_console(const std::string& msg, bool response = false) {
        try {
            if (!response) {
                output({{"message", msg}});
            }
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    int getSocket() const {
        return client_socket;
    }


private:

    void createSocket()
    {
        if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            throw std::runtime_error("Error: failed to create client socket");
        }
    }

    std::string user;
    std::string password;
    JimClientMessage jim;
    void* gui_instance;
    std::vector<std::string>* tasks;
    std::string server_ip;
    int server_port;
    int client_socket;
    struct sockaddr_in server_address_;
    bool conn_is_open;
    void* loop;
    std::string sockname;
    void* transport;
    std::function<void(const std::unordered_map<std::string, std::string>&)> output;
    std::thread input_thread;
    std::mutex socket_guard;
};

void custom_output_function(const std::unordered_map<std::string, std::string>& msg) {
        std::cout << "Custom Output: " << msg.at("message") << std::endl;
    }

int main() {
    std::string server_ip = "127.0.0.1";
    int server_port = 8080;
    std::string username = "123";
    std::string password = "123";
    std::vector<std::string> tasks;
    // Создание объекта аутентификации
    DataAccessLayer dal;
    Database db(&dal);
    ClientAuth auth(db);
    std::unique_ptr<ChatClientProtocol> client(new ChatClientProtocol(std::move(db), nullptr, &tasks, username, password, nullptr, "", server_ip, server_port, custom_output_function));

    try
    {
        client->connectToServer();
        std::cout << "Connected to server\n";
        client->run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    //ConvertMixin convertMixin;

    //auto client_socket = client->client_socket;
    auto client_socket = client->getSocket();

    //Отправка запроса на авторизацию первого пользователя
    std::string user1_auth = R"(action:authenticate,user.account_name:user1,user.password:password1)";

    // std::unordered_map<std::string, std::string> user1_auth = {
    //     {"action", "authenticate"},
    //     {"user.account_name", "user1"},
    //     {"user.password", "password1"}
    // };

    //std::vector<uint8_t> auth_bytes = convertMixin.dict_to_bytes(user1_auth);

    int s = htonl(user1_auth.size());
    send(client_socket, &s, sizeof(s), 0);
    send(client_socket, user1_auth.c_str(), user1_auth.size(), 0);
    //send(client_socket, auth_bytes.data(), auth_bytes.size(), 0);

    // Ожидание и чтение ответа от сервера
    char response_buffer[4096];
    ssize_t bytes_received = recv(client_socket, response_buffer, sizeof(response_buffer) - 1, 0);
    if (bytes_received > 0) {
        response_buffer[bytes_received] = '\0';
        std::cout << "Response from server: " << response_buffer << std::endl;
    }

    //Отправка сообщения от пользователя user1
    std::string message_from_user1 = R"(action:msg,from:user1,to:user2,message:Hello from user1!)";

    s = htonl(message_from_user1.size());
    send(client_socket, &s, sizeof(s), 0);
    send(client_socket, message_from_user1.c_str(), message_from_user1.size(), 0);

    // Ожидание и чтение ответа от сервера
    bytes_received = recv(client_socket, response_buffer, sizeof(response_buffer) - 1, 0);
    if (bytes_received > 0) {
        response_buffer[bytes_received] = '\0';
        std::cout << "Response from server: " << response_buffer << std::endl;
    }

    close(client_socket);
    
    return 0;
}
