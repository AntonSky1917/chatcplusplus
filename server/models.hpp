#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>

// Класс модели базы данных
class CBase {
public:
    virtual ~CBase() {} // Виртуальный деструктор для полиморфизма

    virtual std::string toString() const {
        return "Base";
    }

    // Виртуальные функции сериализации и десериализации
    virtual void serialize(std::ostream& os) const = 0;
    virtual void deserialize(std::istream& is) = 0;

};

// Класс модели Client
class Client : public CBase {

public:
    int id;
    std::string username;
    std::string password;
    std::string info;
    bool online_status;
    std::vector<int> history_ids; // Вектор идентификаторов записей в истории

    Client(int id, const std::string& username, const std::string& password,
           const std::string& info = "", bool online_status = false)
        : id(id), username(username), password(password), info(info),
          online_status(online_status) {}

    std::string toString() const override {
        return "Client(id='" + std::to_string(id) + "', username='" + username +
               "', info='" + info + "')";
    }

    // все данные сохраняются в бинарном виде
    // если бы это был json его пришлось бы парсить
    void serialize(std::ostream& os) const override {
        os << id << "\n";
        os << username.size() << "|" << username << "\n";
        os << password.size() << "|";
        os.write(password.data(), password.size());
        os << "\n";

        os.write(reinterpret_cast<const char*>(&id), sizeof(id));

        size_t usernameSize = username.size();
        os.write(reinterpret_cast<const char*>(&usernameSize), sizeof(usernameSize));
        os.write(username.c_str(), usernameSize);

        size_t passwordSize = password.size();
        os.write(reinterpret_cast<const char*>(&passwordSize), sizeof(passwordSize));
        os.write(password.data(), passwordSize);

        size_t infoSize = info.size();
        os.write(reinterpret_cast<const char*>(&infoSize), sizeof(infoSize));
        os.write(info.c_str(), infoSize);

        os.write(reinterpret_cast<const char*>(&online_status), sizeof(online_status));

        size_t historySize = history_ids.size();
        os.write(reinterpret_cast<const char*>(&historySize), sizeof(historySize));
        os.write(reinterpret_cast<const char*>(history_ids.data()), historySize * sizeof(int));
    }

    void deserialize(std::istream& is) override {
        is >> id;
        size_t s;
        char c;

        is >> s >> c;
        username.resize(s);
        is.read(const_cast<char*>(username.data()), s);

        is >> s >> c;
        password.resize(s);
        is.read(const_cast<char*>(password.data()), s);

        is.read(reinterpret_cast<char*>(&id), sizeof(id));

        size_t usernameSize;
        is.read(reinterpret_cast<char*>(&usernameSize), sizeof(usernameSize));
        username.resize(usernameSize);
        is.read(const_cast<char*>(username.data()), usernameSize);

        size_t passwordSize;
        is.read(reinterpret_cast<char*>(&passwordSize), sizeof(passwordSize));
        password.resize(passwordSize);
        is.read(const_cast<char*>(password.data()), passwordSize);

        size_t infoSize;
        is.read(reinterpret_cast<char*>(&infoSize), sizeof(infoSize));
        info.resize(infoSize);
        is.read(const_cast<char*>(info.data()), infoSize);

        is.read(reinterpret_cast<char*>(&online_status), sizeof(online_status));

        size_t historySize;
        is.read(reinterpret_cast<char*>(&historySize), sizeof(historySize));
        history_ids.resize(historySize);
        is.read(reinterpret_cast<char*>(history_ids.data()), historySize * sizeof(int));
    }
};

// Класс модели History
class History : public CBase {
public:
    int id;
    time_t time;
    std::string ip_addr;
    int client_id;
    Client* client;

    History(int id, time_t time, const std::string& ip_addr, int client_id,
            Client* client)
        : id(id), time(time), ip_addr(ip_addr), client_id(client_id),
          client(client) {}

    std::string toString() const override {
        return "History(client='" + (client ? client->username : "") + "', time='" +
               std::to_string(time) + "', ip_addr='" + ip_addr + "')";
    }

    void serialize(std::ostream& os) const override {
        os.write(reinterpret_cast<const char*>(&id), sizeof(id));
        os.write(reinterpret_cast<const char*>(&time), sizeof(time));

        size_t ipSize = ip_addr.size();
        os.write(reinterpret_cast<const char*>(&ipSize), sizeof(ipSize));
        os.write(ip_addr.c_str(), ipSize);

        os.write(reinterpret_cast<const char*>(&client_id), sizeof(client_id));
    }

    void deserialize(std::istream& is) override {
        is.read(reinterpret_cast<char*>(&id), sizeof(id));
        is.read(reinterpret_cast<char*>(&time), sizeof(time));

        size_t ipSize;
        is.read(reinterpret_cast<char*>(&ipSize), sizeof(ipSize));
        ip_addr.resize(ipSize);
        is.read(const_cast<char*>(ip_addr.data()), ipSize);

        is.read(reinterpret_cast<char*>(&client_id), sizeof(client_id));
    }
};

// Класс модели Contacts
class Contacts : public CBase {
public:
    int id;
    int client_id;
    int contact_id;
    Client* client;
    Client* contact;

    Contacts(int id, int client_id, int contact_id, Client* client,
             Client* contact)
        : id(id), client_id(client_id), contact_id(contact_id), client(client),
          contact(contact) {}

    std::string toString() const override {
        return "Contacts(client_username='" + (client ? client->username : "") +
               "', contact_username='" + (contact ? contact->username : "") + "')";
    }

    void serialize(std::ostream& os) const override {
        os.write(reinterpret_cast<const char*>(&id), sizeof(id));
        os.write(reinterpret_cast<const char*>(&client_id), sizeof(client_id));
        os.write(reinterpret_cast<const char*>(&contact_id), sizeof(contact_id));
    }

    void deserialize(std::istream& is) override {
        is.read(reinterpret_cast<char*>(&id), sizeof(id));
        is.read(reinterpret_cast<char*>(&client_id), sizeof(client_id));
        is.read(reinterpret_cast<char*>(&contact_id), sizeof(contact_id));
    }
};

// Класс модели Messages
class Messages : public CBase {
public:
    int id;
    int client_id;
    int contact_id;
    time_t time;
    Client* client;
    Client* contact;
    std::string message;

    Messages(int id, int client_id, int contact_id, time_t time, Client* client,
             Client* contact, const std::string& message)
        : id(id), client_id(client_id), contact_id(contact_id), time(time),
          client(client), contact(contact), message(message) {}

    std::string toString() const override {
        return "Messages(client='" + (client ? client->username : "") + "', contact='" +
               (contact ? contact->username : "") + "', time='" + std::to_string(time) +
               "', message='" + message + "')";
    }

    void serialize(std::ostream& os) const override {

        //немного форматирования вывола для того что-бы читать в файле
        os << id << "\n";
        os << client_id << "|" << contact_id << "|" << time << "\n"; // пишем в одной строке все числа - это должно быть безопасно
        os << message.size() << "|"; // что если сообщение с переводом строки? случай иньекции? допустим строка потом перевод строки и числа - это ненадежный способ
        // json в данном случае при записи преобразует строку что-бы внутри не было "", а при чтении откатит это обратно и выведет ""
        // таким образом мы сначала храним явным образом размер, потом храним строку - это безопастно
        os.write(message.c_str(), message.size());
        os << "\n";


        os.write(reinterpret_cast<const char*>(&id), sizeof(id));
        os.write(reinterpret_cast<const char*>(&client_id), sizeof(client_id));
        os.write(reinterpret_cast<const char*>(&contact_id), sizeof(contact_id));
        os.write(reinterpret_cast<const char*>(&time), sizeof(time));

        size_t messageSize = message.size();
        os.write(reinterpret_cast<const char*>(&messageSize), sizeof(messageSize));
        os.write(message.c_str(), messageSize);
    }

    void deserialize(std::istream& is) override {
        is >> id;
        size_t s;
        char c;
        is >> s >> c;

        message.resize(s);
        is.read(const_cast<char*>(message.data()), s);
        is >> s >> c;


        is.read(reinterpret_cast<char*>(&id), sizeof(id));
        is.read(reinterpret_cast<char*>(&client_id), sizeof(client_id));
        is.read(reinterpret_cast<char*>(&contact_id), sizeof(contact_id));
        is.read(reinterpret_cast<char*>(&time), sizeof(time));

        size_t messageSize;
        is.read(reinterpret_cast<char*>(&messageSize), sizeof(messageSize));
        message.resize(messageSize);
        is.read(const_cast<char*>(message.data()), messageSize);
    }
};