#include "models.hpp"
#include "controller.hpp"
#include <memory>
#include <unordered_map>

class DbInterfaceMixin {
public:

    DbInterfaceMixin(Database db) : _db(std::move(db)) {}

    // Обертки для методов базы данных
    void add_client(const std::string& username, const std::string& password, const std::string& info = "") {
        _db.addClient(username, password, info);
    }

    Client* get_client_by_username(const std::string& username) {
        return _db.getClientByUsername(username);
    }

    Client* get_client_by_password(const std::string& password) {
        return _db.getClientByPassword(password);
    }

    Client* get_client_by_id(int client_id) {
        return _db.getClientById(client_id);
    }

    std::vector<Messages*> get_messages_by_client_id(int client_id) {
        return _db.getMessagesByClientId(client_id);
    }

    Client* create_client(const std::string& username, const std::string& password, const std::string& info = "") {
        return _db.createClient(username, password, info);
    }

    std::vector<Client*> get_all_clients() {
        return _db.get_all_clients();
    }

    void add_client_history(int client_id, const std::string& ip_addr = "8.8.8.8") {
        _db.addHistory(client_id, ip_addr);
    }

    void add_contact(int client_id, int contact_id) {
        _db.add_contact(client_id, contact_id);
    }

    void del_contact(int client_id, int contact_id) {
        _db.del_contact(client_id, contact_id);
    }

    std::vector<Contacts*> get_contacts(int client_id) {
        return _db.get_contacts(client_id);
    }

    void send_message(int client_id, int contact_id, const std::string& message) {
        _db.send_message(client_id, contact_id, message);
    }

    std::vector<Messages*> get_conversation(int client_id, int contact_id) {
        return _db.get_conversation(client_id, contact_id);
    }

    void add_client_message(int client_id, int contact_id, const std::string& message) {
        _db.add_client_message(client_id, contact_id, message);
    }

    std::vector<Messages*> get_client_messages(int client_id) {
        return _db.get_client_messages(client_id);
    }

    std::vector<Messages*> get_contact_messages(int client_id, int contact_id) {
        return _db.get_contact_messages(client_id, contact_id);
    }

    bool get_user_status(int client_id) {
        return _db.get_user_status(client_id);
    }

    void set_user_online(int client_id) {
        _db.set_user_online(client_id);
    }

    void set_user_offline(int client_id) {
        _db.set_user_offline(client_id);
    }

private:
    Database _db;
};

class ConvertMixin {
public:
    std::vector<uint8_t> dict_to_bytes(const std::unordered_map<std::string, std::string>& msg_dict) {
        std::vector<uint8_t> bytes;

        for (const auto& pair : msg_dict) {
            encode_string(pair.first, bytes);
            encode_string(pair.second, bytes);
        }

        return bytes;
    }

    std::unordered_map<std::string, std::string> bytes_to_dict(const std::vector<uint8_t>& msg_bytes) {
        std::unordered_map<std::string, std::string> message_dict;

        size_t pos = 0;
        while (pos < msg_bytes.size()) {
            std::string key = decode_string(msg_bytes, pos);
            std::string value = decode_string(msg_bytes, pos);

            message_dict[key] = value;
        }

        return message_dict;
    }

    // std::unordered_map<std::string, std::string> bytes_to_dict(const std::vector<uint8_t>& msg_bytes) {
    //     std::unordered_map<std::string, std::string> message_dict;

    //     size_t pos = 0;
    //     while (pos < msg_bytes.size()) {
    //         std::unordered_map<std::string, std::string> key_value_pair = decode_string(msg_bytes, pos);

    //         // Извлекаем ключ и значение из пары и добавляем их в словарь
    //         for (const auto& pair : key_value_pair) {
    //             message_dict[pair.first] = pair.second;
    //         }
    //     }

    //     return message_dict;
    // }

private:
    void encode_string(const std::string& input, std::vector<uint8_t>& output) {
        size_t size = input.size();
        for (size_t i = 0; i < size; ++i) {
            output.push_back(static_cast<uint8_t>(input[i]));
        }
        output.push_back(0);  // Null-terminate the string
    }

    std::string decode_string(const std::vector<uint8_t>& input, size_t& pos) {
        std::string result;
        //while (pos < input.size() && input[pos] != 0) {
        while (pos < input.size() && input[pos] != ':' && input[pos] != ',' && input[pos] != '\n') {
            result += static_cast<char>(input[pos++]);
        }
        ++pos;  // Skip the null-terminator
        return result;
    }
};
