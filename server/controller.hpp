#include "models.hpp"
#include "db_connector.hpp"

class Database {
private:

    DataAccessLayer* dal;

    int generateClientId() {
        // Генерация уникального идентификатора клиента
        static int counter = 1;
        return counter++;
    }

    int generateContactId() {
        static int counter = 1;
        return counter++;
    }

    int generateHistoryId() {
        static int counter = 1;
        return counter++;
    }

    int generateMessageId() {
        static int counter = 1;
        return counter++;
    }

public:
    Database(DataAccessLayer* dal) : dal(dal) {}

    Client* getClientByUsername(const std::string& username) {
        // Поиск клиента по имени
        auto it = std::find_if(dal->data.begin(), dal->data.end(),
                               [&](CBase* item) {
                                   if (dynamic_cast<Client*>(item)) {
                                       Client* client =
                                           dynamic_cast<Client*>(item);
                                       return client->username == username;
                                   }
                                   return false;
                               });

        if (it != dal->data.end()) {
            return dynamic_cast<Client*>(*it);
        }

        return nullptr;
    }

    Client* getClientByPassword(const std::string& password) {
        Client* client = nullptr;
        {
            auto it = std::find_if(dal->data.begin(), dal->data.end(), [&](CBase* item) {
                if (Client* c = dynamic_cast<Client*>(item)) {
                    return c->password == password;
                }
                return false;
            });

            if (it != dal->data.end()) {
                client = dynamic_cast<Client*>(*it);
            }
        }

        return client; // Return the found client or nullptr if not found
    }

    Client* getClientById(int client_id) {
        auto it = std::find_if(dal->data.begin(), dal->data.end(),
                               [&](CBase* item) {
                                   if (auto client = dynamic_cast<Client*>(item)) {
                                       return client->id == client_id;
                                   }
                                   return false;
                               });

        if (it != dal->data.end()) {
            return dynamic_cast<Client*>(*it);
        }

        return nullptr;
    }

    //Client* getClientById(int client_id) {
        //for (CBase* item : dal->data) {
            //if (auto client = dynamic_cast<Client*>(item)) {
                //if (client->id == client_id) {
                    //return client;
                //}
            //}
        //}
        //return nullptr;
    //}

    std::vector<Messages*> getMessagesByClientId(int client_id) {
        std::vector<Messages*> messages;
        for (CBase* item : dal->data) {
            if (auto message = dynamic_cast<Messages*>(item)) {
                if (message->client_id == client_id) {
                    messages.push_back(message);
                }
            }
        }
        return messages;
    }

    Client* createClient(const std::string& username, const std::string& password, const std::string& info = "") {  // SH!
        // Проверка наличия клиента с таким же именем
        if (getClientByUsername(username)) {
            throw std::runtime_error(std::string("Пользователь ") + username + " уже существует");
        }
        
        // Создание нового клиента и добавление его в DAL
        int id = generateClientId();
        Client* newClient = new Client(id, username, password, info);
        dal->addData(newClient);
        dal->commit();
        return newClient;
    }

    void addClient(const std::string& username, const std::string& password,
                   const std::string& info = "") {
        // Проверка наличия клиента с таким же именем
        if (getClientByUsername(username)) {
            std::cout << "Пользователь " << username << " уже существует"
                      << std::endl;
        } else {
            // Создание нового клиента и добавление его в DAL
            int id = generateClientId();
            Client* newClient = new Client(id, username, password, info);
            dal->addData(newClient);
            dal->commit();
            std::cout << "Добавлен пользователь: " << newClient->toString()
                      << std::endl;
        }
    }

    std::vector<Client*> get_all_clients() {
        std::vector<Client*> clients;
        for (CBase* item : dal->data) {
            if (auto client = dynamic_cast<Client*>(item)) {
                clients.push_back(client);
            }
        }
        return clients;
    }

    void addHistory(int client_id, const std::string& ip_addr) {
        Client* client = getClientById(client_id);
        if (client) {
            int history_id = generateHistoryId();
            time_t current_time = std::time(nullptr);
            History* history = new History(history_id, current_time, ip_addr,
                                           client_id, client);
            dal->addData(history);
            client->history_ids.push_back(history_id);
            dal->commit();
            std::cout << "Добавлена история: " << history->toString() << std::endl;
        } else {
            std::cout << "Клиент с id " << client_id << " не найден" << std::endl;
        }
    }

    void add_contact(int client_id, int contact_id) {
        Client* client = getClientById(client_id);
        Client* contact = getClientById(contact_id);
        if (client && contact) {
            Contacts* newContact = new Contacts(generateContactId(), client_id, contact_id, client, contact);
            dal->addData(newContact);
            dal->commit();
            std::cout << "Добавлен контакт: " << newContact->toString() << std::endl;
        } else {
            std::cout << "Клиент или контакт с заданным id не найдены" << std::endl;
        }
    }

    void del_contact(int client_id, int contact_id) {
        auto it = std::find_if(dal->data.begin(), dal->data.end(),
                            [&](CBase* item) {
                                if (auto contact = dynamic_cast<Contacts*>(item)) {
                                    return contact->client_id == client_id && contact->contact_id == contact_id;
                                }
                                return false;
                            });

        if (it != dal->data.end()) {
            delete *it;
            dal->data.erase(it);
            dal->commit();
            std::cout << "Контакт успешно удален" << std::endl;
        } else {
            std::cout << "Контакт не найден" << std::endl;
        }
    }

    std::vector<Contacts*> get_contacts(int client_id) {
        std::vector<Contacts*> contacts;
        for (CBase* item : dal->data) {
            if (auto contact = dynamic_cast<Contacts*>(item)) {
                if (contact->client_id == client_id) {
                    contacts.push_back(contact);
                }
            }
        }
        return contacts;
    }

    void send_message(int client_id, int contact_id, const std::string& message) {
        Client* client = getClientById(client_id);
        Client* contact = getClientById(contact_id);
        if (client && contact) {
            time_t current_time = std::time(nullptr);
            Messages* newMessage = new Messages(generateMessageId(), client_id, contact_id, current_time, client, contact, message);
            dal->addData(newMessage);
            dal->commit();
            std::cout << "Отправлено сообщение: " << newMessage->toString() << std::endl;
        } else {
            std::cout << "Клиент или контакт с заданным id не найдены" << std::endl;
        }
    }

    std::vector<Messages*> get_conversation(int client_id, int contact_id) {
        std::vector<Messages*> conversation;
        for (CBase* item : dal->data) {
            if (auto message = dynamic_cast<Messages*>(item)) {
                if ((message->client_id == client_id && message->contact_id == contact_id) ||
                    (message->client_id == contact_id && message->contact_id == client_id)) {
                    conversation.push_back(message);
                }
            }
        }
        return conversation;
    }

    void add_client_message(int client_id, int contact_id, const std::string& message) {
        Client* client = getClientById(client_id); // это требует прохождения по списку всех объектов потому что они хранятся в одной куче
        Client* contact = getClientById(contact_id);
        if (client && contact) {
            time_t current_time = std::time(nullptr);
            Messages* newMessage = new Messages(generateMessageId(), client_id, contact_id, current_time, client, contact, message);
            dal->addData(newMessage);
            dal->commit();
            std::cout << "Добавлено сообщение: " << newMessage->toString() << std::endl;
        } else {
            std::cout << "Клиент или контакт с заданным id не найдены" << std::endl;
        }
    }

    std::vector<Messages*> get_client_messages(int client_id) {
        std::vector<Messages*> messages;
        for (CBase* item : dal->data) {
            if (auto message = dynamic_cast<Messages*>(item)) {
                if (message->client_id == client_id) {
                    messages.push_back(message);
                }
            }
        }
        return messages;
    }

    std::vector<Messages*> get_contact_messages(int client_id, int contact_id) {
        std::vector<Messages*> contact_messages;
        std::vector<Messages*> messages = getMessagesByClientId(client_id);
        for (Messages* message : messages) {
            if (message->contact_id == contact_id) {
                contact_messages.push_back(message);
            }
        }
        return contact_messages;
    }

    bool get_user_status(int client_id) {
        Client* client = getClientById(client_id);
        if (client) {
            return client->online_status;
        } else {
            std::cout << "Клиент с id " << client_id << " не найден" << std::endl;
            return false;
        }
    }

    void set_user_online(int client_id) {
        Client* client = getClientById(client_id);
        if (client) {
            client->online_status = true;
            dal->commit();
            std::cout << "Клиент " << client->username << " теперь онлайн" << std::endl;
        } else {
            std::cout << "Клиент с id " << client_id << " не найден" << std::endl;
        }
    }

    void set_user_offline(int client_id) {
        Client* client = getClientById(client_id);
        if (client) {
            client->online_status = false;
            dal->commit();
            std::cout << "Клиент " << client->username << " теперь оффлайн" << std::endl;
        } else {
            std::cout << "Клиент с id " << client_id << " не найден" << std::endl;
        }
    }

};

// int main() {
//     // зачем все эти сущности для работы с клиентом, клиент должен работать с чем то одним - надо смотреть с конца, тоесть с точки зрения клиента
//     // ему важно подключиться к БД и выполнять запросы (инкапсуляция - тоесть клиенту не важно как это работает - а важно то какой у него интерфейс)
//     DataAccessLayer dal; // один объект отвечает за запись и чтение - слой который взаимодействует с бинарными данными
//     Database clientMessages(&dal); // другой объект отвечает за запросы

//     Database db(&dal); // клиент должен проводить операции только через один объект нижнего уровня
//     // таким образом клиент через этот объект подгружает данные и строит таблицы
//     // в свою очередь в dal мы можем построить разные карты для разного представления, тоесть в объекте верхнего уровня реализовать дополнительную логику
//     // например что происходит при загрузке данных, логику взаимодействия с сетью, логику взаимодействия с физическим уровнем сетевых протоколов

//     // Добавление данных
//     //Client* client1 = new Client(1, "user1", {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'});
//     //dal.addData(client1);

//     //Client* client2 = new Client(2, "user2", {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'});
//     //dal.addData(client2);
//     // Добавление данных
//     Client* client1 = db.createClient("user1", {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'});  // SH!
//     Client* client2 = db.createClient("user2", {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'});

//     History* history1 = new History(1, 1, "127.0.0.1", 1234567890, nullptr);
//     dal.addData(history1);

//     History* history2 = new History(2, 1, "192.168.0.1", 1234567890, nullptr);
//     dal.addData(history2);

//     // Получение всех клиентов
//     std::vector<Client*> clients = db.get_all_clients();
//     std::cout << "Все клиенты:" << std::endl;
//     for (Client* client : clients) {
//         std::cout << client->toString() << std::endl;
//     }

//     // Установка онлайн-статуса
//     db.set_user_online(1);
//     db.set_user_offline(2);

//     // Добавление истории для клиента 1
//     db.addHistory(1, "127.0.0.1");
//     db.addHistory(2, "192.168.0.1");

//     // Добавление контактов
//     db.add_contact(1, 2);
//     db.add_contact(2, 1);

//     // Отправка сообщений
//     db.send_message(1, 2, "Здратуте!");
//     db.send_message(2, 1, "Дотвидания!");

//     // Получение и вывод сообщений для клиента 1
//     std::vector<Messages*> client1Messages = db.get_client_messages(1);
//     std::cout << "Сообщения для клиента 1:" << std::endl;
//     for (Messages* message : client1Messages) {
//         std::cout << message->toString() << std::endl;
//     }

//     // Получение и вывод сообщений между клиентом 1 и контактом 2
//     std::vector<Messages*> conversation = db.get_conversation(1, 2);
//     std::cout << "Сообщения между клиентом 1 и контактом 2:" << std::endl;
//     for (Messages* message : conversation) {
//         std::cout << message->toString() << std::endl;
//     }
    
//     // Проверка статуса клиента 1
//     bool status = db.get_user_status(1);
//     std::cout << "Статус клиента 1: " << (status ? "Онлайн" : "Оффлайн") << std::endl;

//     // Удаление контакта между клиентом 1 и контактом 2
//     //clientMessages.del_contact(1, 2);

//     // Получение всех сообщений между клиентом 1 и контактом 2 после удаления контакта
//     conversation = db.get_conversation(1, 2);
//     std::cout << "Сообщения между клиентом 1 и контактом 2 после удаления контакта:" << std::endl;
//     for (Messages* message : conversation) {
//         std::cout << message->toString() << std::endl;
//     }

//     // Установка статуса оффлайн для клиента 1
//     //clientMessages.set_user_offline(1);

//     // Вывод данных до коммита
//     std::cout << "Данные до коммита:" << std::endl;
//     for (CBase* item : dal.data) {
//         std::cout << item->toString() << std::endl;
//     }

//     // Коммит данных в файл
//     dal.commit();

//     // Удаление данных
//     dal.removeData(history1);

//     // Загрузка данных из файла
//     dal.load();

//     // Вывод данных после загрузки
//     std::cout << "Данные после загрузки:" << std::endl;
//     for (CBase* item : dal.data) {
//         std::cout << item->toString() << std::endl;
//     }

//     return 0;
// }
