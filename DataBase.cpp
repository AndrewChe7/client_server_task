//
// Created by GodKnows on 15.10.2021.
//

#include "DataBase.h"
#include <fstream>
#include "ThirdParty/picojson.h"

DataBase::DataBase() {
    std::ifstream file("logins.json");
    std::string json;
    picojson::value val;

    file.seekg(0, std::ios::end);
    json.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    json.assign(std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>());
    picojson::parse(val, json);
    picojson::value::object& o = val.get<picojson::object>();
    for (auto & i : o)
    {
        std::string login, pass;
        login = i.first;
        pass = i.second.to_str();
        loginsData.insert(std::pair<std::string, std::string> (login, pass));
    }
}

bool DataBase::isLoginValid(const std::string &login) {
    if (loginsData.find(login) != loginsData.end())
        return true;
    return false;
}

sole::uuid DataBase::login(const std::string &login) {
    auto uid = sole::uuid4();
    std::lock_guard<std::mutex> lock(uuidsDataMutex);
    uuidsData.insert(std::pair<sole::uuid, std::string> (uid, login));
    return uid;
}

bool DataBase::isPasswordValid(const std::string& login, const std::string& pass)
{
    if (loginsData[login] != pass)
    {
        return false;
    }
    return true;
}