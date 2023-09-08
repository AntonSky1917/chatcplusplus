#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <unordered_map>

class JimClientMessage {
public:
    std::unordered_map<std::string, std::string> auth(const std::string& username, const std::string& password) {
        std::unordered_map<std::string, std::string> data = {
            {"action", "authenticate"},
            {"time", std::to_string(std::time(nullptr))},
            {"user", R"({"account_name": ")" + username + R"(", "password": ")" + password + R"("})"}
        };
        return data;
    }

    std::unordered_map<std::string, std::string> presence(const std::string& sender, const std::string& status = "Yep, I am here!") {
        std::unordered_map<std::string, std::string> data = {
            {"action", "presence"},
            {"time", std::to_string(std::time(nullptr))},
            {"type", "status"},
            {"user", R"({"account_name": ")" + sender + R"(", "status": ")" + status + R"("})"}
        };
        return data;
    }

    std::unordered_map<std::string, std::string> quit(const std::string& sender, const std::string& status = "disconnect") {
        std::unordered_map<std::string, std::string> data = {
            {"action", "quit"},
            {"time", std::to_string(std::time(nullptr))},
            {"type", "status"},
            {"user", R"({"account_name": ")" + sender + R"(", "status": ")" + status + R"("})"}
        };
        return data;
    }

    std::unordered_map<std::string, std::string> list_(const std::string& sender, const std::string& status = "show", const std::string& person = "") {
        std::unordered_map<std::string, std::string> data = {
            {"action", "list"},
            {"time", std::to_string(std::time(nullptr))},
            {"type", "status"},
            {"contact_list", "No contacts yet"},
            {"user", R"({"account_name": ")" + sender + R"(", "status": ")" + status + R"(", "contact": ")" + person + R"("})"}
        };
        return data;
    }

    std::unordered_map<std::string, std::string> message(const std::string& sender, const std::string& receiver = "user1", const std::string& text = "some msg text") {
        std::unordered_map<std::string, std::string> data = {
            {"action", "msg"},
            {"time", std::to_string(std::time(nullptr))},
            {"to", receiver},
            {"from", sender},
            {"encoding", "utf-8"},
            {"message", text}
        };
        return data;
    }
};

// int main() {
//     JimClientMessage clientMessage;
    
//     std::unordered_map<std::string, std::string> authData = clientMessage.auth("username", "password");
//     std::cout << "Auth Message:" << std::endl;
//     for (const auto& entry : authData) {
//         std::cout << entry.first << ": " << entry.second << std::endl;
//     }

//     std::unordered_map<std::string, std::string> presenceData = clientMessage.presence("sender");
//     std::cout << "Presence Message:" << std::endl;
//     for (const auto& entry : presenceData) {
//         std::cout << entry.first << ": " << entry.second << std::endl;
//     }
    
//     return 0;
// }

