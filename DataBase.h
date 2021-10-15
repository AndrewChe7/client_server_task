//
// Created by GodKnows on 15.10.2021.
//

#ifndef CLIENT_SERVER_TASK_DATABASE_H
#define CLIENT_SERVER_TASK_DATABASE_H

#include <map>
#include <mutex>
#include "ThirdParty/sole.hpp"

class DataBase {
private:
    std::map<std::string, std::string> loginsData;
    std::map<sole::uuid, std::string> uuidsData;
    std::mutex uuidsDataMutex;
public:
    DataBase();
    bool isLoginValid(const std::string& login);
    bool isPasswordValid(const std::string& login, const std::string& pass);
    bool isUuidValid(sole::uuid uuid);
    std::string getLogin(sole::uuid uuid);
    sole::uuid login(const std::string& login);
    void logout(sole::uuid uuid);
};


#endif //CLIENT_SERVER_TASK_DATABASE_H
