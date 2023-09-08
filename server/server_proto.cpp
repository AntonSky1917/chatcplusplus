#include <iostream>
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <random>
#include <array>
#include <utility>
#include <mutex>
#include <memory>
#include <ctime>
#include <cstdint>
#include <unistd.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <map>
#include <condition_variable>
#include <queue>
#include <stdexcept>
#include "mixins.hpp"
#include "server_messages.hpp"

class User {
public:
    int socket;
    std::string name;
    std::string password;

    User(int socket, const std::string& name, const std::string& password)
        : socket(socket), name(std::move(name)), password(std::move(password)) {}

    std::string toString() const { return "User: " + name; }
    std::string getName() const { return name; }
};

void readBytes(int client_socket, void* data, size_t n) {
    size_t ready = 0;
    char* pdata = (char*)data;

    while (ready < n) {
        ssize_t nbytes = recv(client_socket, pdata + ready, n - ready, 0);

        if (nbytes <= 0) {
            throw std::runtime_error("socket error");
        }

        ready += nbytes;
    }
}

auto readPacket(int client_socket) {
    int size = 0;
    readBytes(client_socket, &size, sizeof(size));

    size = ntohl(size);

    std::vector<uint8_t> buffer(size);
    //std::string str;
    //str.resize(size);
    readBytes(client_socket, &buffer[0], size);

    return buffer;
}

// функция разделяет строку str на подстроки, используя разделитель delimiter
void split(const std::string& str, char delimiter, std::vector<std::string>& tokens) {
    size_t start = 0; // инициализируем переменную start
    size_t end = str.find(delimiter); // end инициализируется с помощью функции find(delimiter), которая находит позицию первого вхождения разделителя delimiter в строке str.
    while (end != std::string::npos) { // выполняется цикл, который продолжается до тех пор, пока end не станет равным std::string::npos  - это означает, что разделитель больше не найден.
        // c помощью функции substr() извлекается подстрока от позиции start до позиции end - start, и эта подстрока добавляется в вектор tokens с помощью функции emplace_back().
        tokens.emplace_back(str.substr(start, end - start));
        start = end + 1; // start устанавливается на позицию, следующую сразу за найденным разделителем (end + 1).
        end = str.find(delimiter, start); // переменная end обновляется с помощью функции find(delimiter, start), чтобы найти следующее вхождение разделителя после позиции start.
    }
    tokens.emplace_back(str.substr(start)); // после выхода из цикла добавляется последняя подстрока, начиная с позиции start, с помощью функции emplace_back().
}

class ChatServerProtocol : public DbInterfaceMixin, public ConvertMixin, public JimServerMessage {
public:

    ChatServerProtocol(Database db, int server_socket)
        : DbInterfaceMixin(std::move(db)), server_socket(server_socket) {}
    
    ~ChatServerProtocol() {}

    // функция выполняет проверку имени пользователя и пароля.
    bool cmd_login(int socket, const std::string& name, const std::string& password) {
        bool found = false; // переменная будет использоваться для отслеживания нахождения соответствующего имени пользователя и пароля.

        {
            std::lock_guard<std::mutex> l(usersMutex); // блокировка мьютекса usersMutex. Это необходимо для безопасного доступа к общему ресурсу users (вектор пользователей) из нескольких потоков.
            for (const auto& user : users) { // проходим по всем элементам вектора
                if (user->name == name && user->password == password) {
                    found = true; // если соответствие найдено то переменная устанавливается в true.
                    break; // и цикл прерывается.
                }
            }
        }

        if (!found) { // если не найдено
            std::cout << "To Client: STATUS_UNAUTHORIZED" << std::endl;
            sendMessage(socket, "STATUS_UNAUTHORIZED"); // отправка сообщения клиенту
        } else {
            std::cout << "To Client: STATUS_OK" << std::endl;
            sendMessage(socket, "STATUS_OK");
        }
        return found; // возвращаем значение переменной
    }

    // функция выполняет регистрацию нового пользователя.
    void cmd_register(int socket, const std::string& name, const std::string& password)
    {
        if (find_user_socket(name) >= 0) { // проверяем существует ли уже пользователь с таким именем в списке пользователей.
            std::cout << "To Client: STATUS_ALREADY_REGISTERED" << std::endl;
            sendMessage(socket, "STATUS_ALREADY_REGISTERED"); 
            return;
        }

        {
            std::lock_guard<std::mutex> l(usersMutex);
            // если пользователь с заданным именем не найден, создается новый объект User с использованием переданных параметров socket, name и password.
            users.emplace_back(std::make_shared<User>(std::move(socket), std::move(name), std::move(password)));
            add_client(name, password); // Добавить клиента в базу данных
            add_client_history(users.back()->socket); // Добавить запись в историю клиента
            //std::cout << "New user registered: " << name << std::endl;
        }

        std::cout << "To Client: STATUS_REGISTERED_SUCCESSFULL" << std::endl;
        std::cout << "Registered user: " + name + ", socket: " + std::to_string(socket) << std::endl;
        sendMessage(socket, "STATUS_REGISTERED_SUCCESSFULL");
    }

    void cmd_send(int socket, const std::string& sender, const std::string& recipient, const std::string& text) {
        std::cout << "Sender's name: " << sender << ", Recipient: " << recipient << std::endl;
        std::cout << "Sending message to recipient " << recipient << ": " << text << std::endl;
        int user_socket = find_user_socket(recipient); // поиск сокета получателя

        if (user_socket < 0) { // если не найден
            std::cout << "To Client: STATUS_INVALID_USER" << std::endl;
            // Отправка сообщения о неверном пользователе клиенту
            sendMessage(socket, "STATUS_INVALID_USER");
        } else { // если найден
            // Создаем данные для сообщения, пока это просто тестовый вариант!
            std::unordered_map<std::string, std::string> data;
            data["from"] = sender;
            data["to"] = recipient;
            data["message"] = text;

            // Вызываем функцию action_msg для обработки сообщения
            action_msg(data);
        }
    }

    // функция выполняет широковещательную рассылку сообщения text от отправителя sender всем пользователям, кроме отправителя.
    void cmd_broadcast(int socket, const std::string& sender, const std::string& text) {
        std::vector<int> tosend; // создаем пустой вектор tosend, который будет содержать сокеты всех пользователей, которым нужно отправить сообщение.
        {
            std::lock_guard<std::mutex> l(usersMutex); // блокировка гарантирует, что доступ к вектору users будет синхронизирован между потоками.
            for (const auto& user : users) { // итерируемся по всем пользователям в векторе users.
                tosend.push_back(user->socket); // и сокет каждого пользователя добавляется в вектор tosend.
            }
        }

        auto message = std::string("MESSAGE|") + sender + "|" + text; // создаем строку message

        for (int s : tosend) { // итерируемся по всем сокетам в векторе tosend.
            if (s != socket) // если сокет не совпадает с отправителем.
                sendMessage(s, std::move(message)); // вызываем sendMessage и отправляем сообщение каждому сокету.
        }
        sendMessage(socket, "BROADCAST_SENT");
    }

    // функция обрабатывает сообщения от клиента, подключенного к серверу.
    void processSocket(int client_socket)
    {   
        int socket = client_socket; // инициализируем локальную переменную со значением client_socket, переданным в функцию.
        std::string name = "?"; // локальная переменная name со значением "?", которая представляет имя пользователя.
        // зто значение будет обновляться после успешного выполнения команды COMMAND_LOGIN.
        std::cout << "Sending message as user: " << name << std::endl;

        while (true) // цикл будет выполняться до тех пор, пока не будет получено пустое сообщение от клиента (длина равна 0) или пока не произойдет ошибка.
        {   
            // Обработка полученного сообщения от клиента
            std::string command = receiveMessage(socket); // получаем сообщение от клиента.
            std::cout << "Received message from client " << name << ":" << command << std::endl; // выводим сообщение
            if (command.length() == 0) // если длинна = 0 то завершаем обработку пользователя.
                break;

            std::vector<std::string> tokens;
            split(command, '|', tokens); // Используем измененную функцию split с символом '|' в качестве разделителя

            if (tokens.size() == 0) { // Изменили условие проверки на пустоту вектора
                std::cout << "Invalid command" << std::endl;
                continue;
            }

            std::string commandType = tokens[0]; // извлекаем тип команды из первого токена
            if (commandType == "COMMAND_LOGIN") // проверяем первую подстроку на соответствие команде
            {   
                if (cmd_login(socket, tokens[1], tokens[2]))
                name = tokens[1]; // если вход выполнен успешно, значение name обновляется.
            }
            else if(commandType == "COMMAND_REGISTER")
            {
                cmd_register(socket, tokens[1], tokens[2]);
            }
            else if (commandType == "COMMAND_SEND")
            {   
                std::cout << "Sending message as user: " << name << std::endl;
                cmd_send(socket, name, tokens[1], tokens[2]);
            }
            else if (commandType == "COMMAND_BROADCAST")
            {
                // отправить сообщение всем пользователям, за исключением отправителя.
                cmd_broadcast(socket, name, tokens[1]);
            }
            else if (commandType == "COMMAND_CHECK_MESSAGES") {
                sendMessage(socket, "STATUS_NO");
            }
            else
            {
                std::cout << "Invalid command \"" << commandType << "\"" <<std::endl;
                sendMessage(socket, "ERROR_INVALID_COMMAND");
            }
        }

        {
            std::lock_guard<std::mutex> l(usersMutex);
            std::remove_if(users.begin(), users.end(), [socket](const std::shared_ptr<User>& user) {
                return user->socket == socket;
            });
        }

        close(socket);
    }

    // void processSocket(int client_socket) {
    //     std::cout << "processSocket " << client_socket << std::endl;

    //         try {
    //             while (true) {
    //                 // auto dataBytes = readPacket(client_socket);
    //                 // //std::cout << "received: " << dataBytes << std::endl;
    //                 // send(client_socket, "thx", 3, 0);
    //                 // data_received(dataBytes);
    //             }
    //         }
    //         catch (std::exception& e) {
    //             std::cerr << "exception! " << e.what() << std::endl;
    //         }
    // }

    std::string receiveMessage(int client_socket) {
        constexpr std::size_t bufferSize = 4096;
        std::array<char, bufferSize> buffer;
        std::string message;

        for (;;) {
            ssize_t nbytes = recv(client_socket, buffer.data(), 1, 0);
            if (nbytes < 0)
                return "";

            char c = buffer[0];

            if (nbytes == 1 && c != '\n')
                message.append(&c, 1);

            if (c == '\n')
                break;
        }
        return message;
    }

    // функция отправляет сообщение на указанный сокет.
    void sendMessage(int socket, const std::string& message) {
        std::string message_str = message + "\n"; // добавляет символ новой строки (\n) к сообщению.
        send(socket, message_str.c_str(), message_str.length(), 0);
    }

    void send_response(int client_socket, const std::string& error = "") {
        std::unordered_map<std::string, std::string> resp_msg = response(client_socket, error);
        std::vector<uint8_t> msg_bytes = dict_to_bytes(resp_msg);
        msg_bytes.push_back('\n'); // Добавляем символ новой строки
        sendMessage(client_socket, std::string(msg_bytes.begin(), msg_bytes.end()));
    }

    void connection_made(int client_socket) {
        std::cout << "New connection established, socket fd is " << client_socket << std::endl;

        {
            std::lock_guard<std::mutex> l(connectionsMutex);
            connections[client_socket] = {
                {"peername", "unknown"},
                {"username", ""},
                {"transport", std::to_string(client_socket)}
            };
            transport = client_socket;
            std::cout << "Client accepted" << std::endl;
        }

        // Обработка взаимодействия с клиентом
        processSocket(client_socket);
    }

    bool _login_required(int client_socket, std::function<void(User&)> func) {
        auto user_it = std::find_if(users.begin(), users.end(), [client_socket](const std::shared_ptr<User>& user) {
            return user->socket == client_socket;
        });
        if (user_it != users.end()) {
            User& user = **user_it;
            func(user);
            return true;
        } else {
            send_response(501, "login required");
            return false;
        }
    }

    void action_msg(const std::unordered_map<std::string, std::string>& data) {
        //_login_required(transport, [this, &data](User& user) {
            try {
                std::vector<uint8_t> msg_bytes = dict_to_bytes(data);
                msg_bytes.push_back('\n');

                if (data.find("from") != data.end() && !data.at("from").empty()) {
                    // получаем сообщение от клиента
                    std::cout << "Received message: " << data.at("message") << std::endl;
                    // Получаем ID отправителя и получателя по их именам
                    Client* sender = get_client_by_username(data.at("from"));
                    Client* recipient = get_client_by_username(data.at("to"));

                    if (sender) {
                        // Сохраняем сообщение в базе данных
                        add_client_message(sender->id, recipient ? recipient->id : sender->id, data.at("message"));
                        std::cout << "Message saved in the database: " << data.at("message") << std::endl;

                        int from_socket = find_user_socket(data.at("from"));
                        if (from_socket >= 0) {
                            sendMessage(from_socket, std::string(msg_bytes.begin(), msg_bytes.end())); // Если я сам себе отправил сообщение, то я его и получаю
                        }
                    } else {
                        std::cout << "Sender or recipient not found" << std::endl;
                    }
                }
                if (data.find("to") != data.end() && !data.at("to").empty() && data.at("from") != data.at("to")) {
                    // если пользователь отправляет сообщение не самому себе
                    int to_socket = find_user_socket(data.at("to"));
                    if (to_socket >= 0) {
                        sendMessage(to_socket, std::string(msg_bytes.begin(), msg_bytes.end())); // то его получает тот, кому отправили
                    } else {
                        send_response(404, "user is not connected");
                        std::cout << data.at("to") << " is not connected yet" << std::endl;
                        // на случай, если при отправке сообщения тот, кому отправили, отключился
                        // необходимо отлавливать такие случаи, чтобы приложение не аварийно завершилось
                    }
                }
                std::cout << "Message successfully processed." << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Error processing message: " << e.what() << std::endl;
                send_response(500, e.what());
            }
        //});
    }

    // в разработке
    void action_list(const std::unordered_map<std::string, std::string>& data) {
        //_login_required(transport, [this, &data](User& user) {
            try {
                std::vector<uint8_t> msg_bytes = dict_to_bytes(data);
                msg_bytes.push_back('\n'); // Добавляем символ новой строки
                // Заглушка: просто отправляем сообщение с успешным ответом
                send_response(200);
                
                std::cout << "List operation successfully processed." << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Error processing list operation: " << e.what() << std::endl;
                send_response(500, e.what());
            }
        //});
    }

    void eof_received() {
        // Закрываем соединение
        close(transport);
    }

    // функция, вызываемая при потере соединения с клиентом
    void connection_lost(const std::exception_ptr& exc) {
        if (exc) {
            try {
                std::rethrow_exception(exc);
            } catch (const std::exception& e) {
                if (typeid(e) == typeid(std::system_error) && dynamic_cast<const std::system_error*>(&e)->code() == std::errc::connection_reset) {
                    std::cout << "ConnectionResetError" << std::endl;
                    std::cout << "Connections: " << connections.size() << std::endl;
                    std::cout << "Users: " << users.size() << std::endl;
                }
            }
        }

        // Удаление закрытых соединений
        std::vector<int> rm_con;
        for (auto con_kv : connections) {
            if (con_kv.second["transport"] == std::to_string(transport)) {
                rm_con.push_back(con_kv.first);
            }
        }

        for (int con : rm_con) {
            connections.erase(con);
        }

        std::vector<std::string> rm_user;
        
        for (auto it = rm_user.begin(); it != rm_user.end(); ++it) {
            const std::string& u = *it;
            users.erase(std::remove_if(users.begin(), users.end(), [u](const std::shared_ptr<User>& user) {
                return std::to_string(user->socket) == u;
            }), users.end());
            set_user_offline(std::stoi(u));
            std::cout << u << " disconnected" << std::endl;
            eof_received();
        }

    }

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
                        add_client_history(usr->socket);
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
                    add_client_history(users.back()->socket); // Добавить запись в историю клиента
                    std::cout << "New user registered: " << username << std::endl;
                    return true;
                }
            }
        }
        std::cout << "Authentication failed: Empty username or password" << std::endl;
        return false;
    }

    void data_received(const std::vector<uint8_t>& data) {
        std::cout << "Received data: ";
        for (const uint8_t byte : data) {
            std::cout << static_cast<char>(byte);
        }
        std::cout << std::endl;

        std::unordered_map<std::string, std::string> _data = bytes_to_dict(data);

        if (!_data.empty()) {

            std::cout << "Decoded data: ";
            for (const auto& entry : _data) {
                std::cout << entry.first << ": " << entry.second << ", ";
            }
            std::cout << std::endl;

            try {
                std::cout << "action: " << _data["action"] << std::endl;
                if (_data["action"] == "msg") {
                    //_login_required(transport, [this, &_data](User& user) {
                        std::cout << "Action: msg" << std::endl;
                        action_msg(_data);
                    //});
                } else if (_data["action"] == "list") {
                    //_login_required(transport, [this, &_data](User& user) {
                        std::cout << "Action: list" << std::endl;
                        
                        std::string sender = _data["user.account_name"];
                        std::string status = _data["user.status"];
                        std::string person = _data["user.contact"];
                        // Вызов функции action_list(_data)
                        action_list(_data);
                        // Отправка ответа
                        send_response(200);
                    //});
                } else if (_data["action"] == "presence") {
                    //_login_required(transport, [this, &_data](User& user) {
                        std::cout << "Action: presence" << std::endl;
                        if (!_data["from"].empty()) {
                            auto statusIterator = _data.find("status");
                            if (statusIterator != _data.end()) {
                                //std::cout << user.toString() << " " << statusIterator->second << std::endl;
                            } else {
                                std::cout << "Status not found in data." << std::endl;
                            }
                            send_response(200);
                        } else {
                            send_response(500, "wrong presence msg");
                        }
                    //});
                } else if (_data.find("action") != _data.end() && _data["action"] == "authenticate") {
                    std::string userAccountName = _data["user.account_name"];
                    std::string userPassword = _data["user.password"];

                    std::cout << "Action: authenticate" << std::endl;

                    if (authenticate(userAccountName, userPassword)) {
                        // Поиск пользователя по имени.
                        auto userIter = std::find_if(users.begin(), users.end(),
                            [&userAccountName](const std::shared_ptr<User>& user) {
                                return user->getName() == userAccountName;
                            });

                        if (userIter == users.end()) {
                            // Получение нового сокета для нового пользователя.
                            int newUserSocket = accept(server_socket, NULL, NULL);
                            if (newUserSocket == -1) {

                                std::cout << "Error accepting new user socket" << std::endl;
                                // Обработка ошибки при подключении нового пользователя.
                            } else {
                                // Создание нового пользователя.
                                std::shared_ptr<User> newUser = std::make_shared<User>(newUserSocket, userAccountName, userPassword);
                                users.push_back(newUser);

                                std::unordered_map<std::string, std::string> resp_msg = probe(userAccountName);
                                std::vector<uint8_t> resp_bytes = dict_to_bytes(resp_msg);
                                resp_bytes.push_back('\n');
                                sendMessage(transport, std::string(resp_bytes.begin(), resp_bytes.end()));
                            }
                        } else {
                            std::cout << "User is already connected" << std::endl;
                        }
                    } else {
                        send_response(402, "wrong login/password");
                    }
                }
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
                send_response(500, e.what());
            }
        } else {
            std::cout << "Decoded data is empty" << std::endl;
            send_response(500, "Вы отправили сообщение без имени или данных");
        }
    }

    int get_transport() const {
        return transport;
    }

    // функция выполняет поиск пользователя по имени в векторе users.
    // она принимает аргумент name - имя пользователя, и возвращает соответствующий сокет пользователя (socket) или -1, если пользователь не найден.
    int find_user_socket(const std::string& name) {
        std::lock_guard<std::mutex> l(usersMutex); // используется мьютекс usersMutex для обеспечения безопасности доступа к вектору users.
        for (const auto& u : users) {
            if (u->name == name)
                return u->socket;
        }
        return -1;
    }

private:
    std::vector<std::shared_ptr<User>> users;
    std::unordered_map<int, std::unordered_map<std::string, std::string>> connections;
    int transport;
    int user;
    bool stopFlag;
    std::mutex connectionsMutex;
    std::mutex usersMutex;
    int server_socket;
};

int main(int argc, char *argv[])
{   
    long numProcessors = sysconf(_SC_NPROCESSORS_ONLN);
    std::cout << numProcessors << " concurrent threads are supported.\n";

    int port = 8080;

    if (argc >= 2) {
        port = atoi(argv[1]); // читаем порт из командной строки
    }

    int server_socket = socket(PF_INET, SOCK_STREAM, 0); // создаем серверный сокет
    if (server_socket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    // создается структура для хранения информации об адресе сервера.
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0) { // сокет сервера связывается с адресом.
        perror("bind");
        std::cerr << "Error binding socket" << std::endl;
        return 1;
    }

    if (listen(server_socket, 0) < 0) { // устанавливается прослушивание порта на сервере.
        std::cerr << "Error listening on socket" << std::endl;
        return 1;
    }

    std::cout << "Server started on port " << port << std::endl;

    ConvertMixin convertMixin;
    DataAccessLayer dal;
    Database db(&dal);
    std::unique_ptr<ChatServerProtocol> protocol(new ChatServerProtocol(std::move(db), server_socket));

    while (true) // запускаем цикл который будет принимать подключения от клиентов.
    {
        struct sockaddr_in clientAddress;
        socklen_t addressLength = sizeof(clientAddress);
        int client_socket = accept(server_socket, (struct sockaddr*)&clientAddress, &addressLength); // вызывается функция accept, которая принимает входящее соединение и возвращает файловый дескриптор клиентского сокета.
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        std::cout << "New connection established, socket fd is " << client_socket << std::endl;
        std::thread t(&ChatServerProtocol::connection_made, protocol.get(), client_socket); // создается отдельный поток (std::thread) для обработки пользователя с использованием функции connection_made.
        t.detach(); // далее поток отсоединяется (detach), чтобы продолжить работу независимо от него.
        std::cout << "Client accepted" << std::endl;
    }

    return 0;
}