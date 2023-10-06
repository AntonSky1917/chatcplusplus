#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <mutex>
#include <unistd.h>
#include <sys/socket.h>

class Logger {
public:
    Logger(const std::string& filename) : filename_(filename) {
        file_.open(filename_, std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "Error opening " << filename_ << std::endl;
        }
    }

    ~Logger() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    // эти для задания
    void logMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            std::time_t t = std::time(nullptr);
            file_ << std::ctime(&t);
            file_ << message << "\n\n";
            std::cout << "Message logged: " << message << std::endl;
        }
    }

    void logError(const std::string& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            std::time_t t = std::time(nullptr);
            file_ << std::ctime(&t);
            file_ << "Error " << error << "\n\n";
            std::cerr << "Error " << error << " written to " << filename_ << std::endl;
        }
    }

    // а эти на будущее
    static void error_log(const std::string& my_error) {
        std::time_t t = std::time(nullptr);
        std::ofstream file("/Users/anton_root/chatcplusplus/ErrorWsstd.log", std::ios::app);
        if (!file.is_open()) {
            std::cout << "Error open /Users/anton_root/chatcplusplus/ErrorWsstd.log." << std::endl;
        } else {
            file << std::ctime(&t);
            file << "Error " << my_error << "\n\n";
            std::cout << "Error " << my_error << " Write to /Users/anton_root/chatcplusplus/ErrorWsstd.log." << std::endl;
            file.close();
        }
        exit(0);
    }

    static void warning_access_log(const std::string& war_ac) {
        static int count_warning_log = 0;
        count_warning_log++;
        if (count_warning_log > 100) {
            std::system("gzip -f /Users/anton_root/chatcplusplus/Access_warning.log");
            count_warning_log = 0;
        }

        std::time_t t = std::time(nullptr);
        std::ofstream file("/Users/anton_root/chatcplusplus/Access_warning.log", std::ios::app);
        if (!file.is_open()) {
            std::cout << "Error open /Users/anton_root/chatcplusplus/Access_warning.log." << std::endl;
        } else {
            file << std::ctime(&t);
            file << war_ac << "\n\n";
            std::cout << "_______________________________________" << std::endl;
            std::cout << "Write to /Users/anton_root/chatcplusplus/Access_warning.log..." << std::endl;
            std::cout << war_ac << std::endl;
            file.close();
        }
    }

private:
    std::string filename_;
    std::ofstream file_;
    std::mutex mutex_;
};
