#include "models.hpp"

class DataAccessLayer {
public:
    DataAccessLayer() = default; // Конструктор по умолчанию
    DataAccessLayer(const std::string& filename)
         : filename(filename) {}

    ~DataAccessLayer() {
        clearData();
    }

    //db.connect("filename") - так должно быть с обычной БД
    std::string filename = "database.txt"; // бинарный файл
    std::vector<CBase*> data; // Вектор данных

    void addData(CBase* item) {
        data.push_back(item);
    }

    void removeData(CBase* item) {
        auto it = std::find(data.begin(), data.end(), item);
        if (it != data.end()) {
            delete *it;
            data.erase(it);
        }
    }

    // либо записываем ВСЕ!
    // появился новый клиент, зарегистрировался и коммит добавляет запись в файл
    void commit() {
        std::ofstream file(filename, std::ios::binary | std::ios::out);
        if (!file) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        for (CBase* item : data) {
            std::string className = typeid(*item).name();
            file.write(className.c_str(), className.size());
            item->serialize(file);
        }

        file.close();
        std::cout << "Data committed to file: " << filename << std::endl;
    }

    // либо читаем ВСЕ!
    void load() {
        std::ifstream file(filename, std::ios::binary | std::ios::in);
        if (!file) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }

        clearData();

        while (file) {
            char className[256];
            file.read(className, sizeof(className));

            if (file.gcount() > 0) {
                CBase* item = createObject(className);
                if (item) {
                    item->deserialize(file);
                    data.push_back(item);
                } else {
                    std::cerr << "Unknown class type in file: " << className << std::endl;
                }
            }
        }

        file.close();
        std::cout << "Data loaded from file: " << filename << std::endl;
    }

    void clearData() {
        for (CBase* item : data) {
            delete item;
        }
        data.clear();
    }

    CBase* createObject(const std::string& className) {
        if (className == typeid(Client).name()) {
            return new Client(0, "", "");
        } else if (className == typeid(History).name()) {
            return new History(0, 0, "", 0, nullptr);
        } else if (className == typeid(Contacts).name()) {
            return new Contacts(0, 0, 0, nullptr, nullptr);
        } else if (className == typeid(Messages).name()) {
            return new Messages(0, 0, 0, 0, nullptr, nullptr, "");
        }
        return nullptr;
    }
};