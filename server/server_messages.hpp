#include <iostream>
#include <unordered_map>
#include <string>
#include <chrono>
#include <ctime>
#include <variant>

class JimServerMessage {
public:
    // Функция для создания запроса "probe"
    std::unordered_map<std::string, std::string> probe(const std::string& sender, const std::string& status = "Are you there?") {
        std::unordered_map<std::string, std::string> data = {
            {"action", "probe"},
            {"time", std::to_string(std::time(nullptr))},
            {"type", "status"},
            {"user", R"({"account_name": ")" + sender + R"(", "status": ")" + status + R"("})"}
        };
        return data;
    }

    // Функция для создания ответа
    std::unordered_map<std::string, std::string> response(int code = 0, const std::string& error = "") {
        std::unordered_map<std::string, std::string> _data = {
            {"action", "response"},
            {"code", std::to_string(code)},
            {"time", std::to_string(std::time(nullptr))},
            {"error", error}
        };
        return _data;
    }
};
