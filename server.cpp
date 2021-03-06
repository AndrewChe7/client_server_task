//
// Created by GodKnows on 15.10.2021.
//

#include <iostream>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <list>

#include "ThirdParty/picojson.h"
#include "DataBase.h"
#include "MessageQueue.h"

struct client_connection {
    int socket;
    struct sockaddr_in address;
    socklen_t address_len;
};

std::list<std::thread> threads;
std::list<struct client_connection> connections;
DataBase db;
MessageQueue messageQueue;
std::mutex connections_mutex;
std::mutex error_mutex;
bool stop = false;

void printError(const std::string& message)
{
    std::lock_guard<std::mutex> lock(error_mutex);
    std::cout << "[ERROR] " << message << std::endl;
}

void handleMessage(const std::string& message, const struct client_connection& connection)
{
    picojson::value value;
    std::string error = picojson::parse(value, message);
    if (!error.empty())
    {
        printError(error);
        return;
    }
    if (!value.is<picojson::object>())
    {
        printError("Failed to get object");
        return;
    }
    picojson::object& obj = value.get<picojson::object>();
    picojson::object answerObj;
    if (!obj["command"].is<std::string>())
    {
        printError("Command is not string");
        return;
    }
    auto command = obj["command"].to_str();
    if (command == "HELLO")
    {
        answerObj.insert(std::pair<std::string, picojson::value> ("id",
                                                                  picojson::value(obj["id"].to_str())));
        answerObj.insert(std::pair<std::string, picojson::value> ("command",
                                                                  picojson::value("HELLO")));
        answerObj.insert(std::pair<std::string, picojson::value> ("auth_method",
                                                                  picojson::value("plain-text")));
    } else if (command == "login")
    {
        auto login = obj["login"].to_str();
        auto pass = obj["password"].to_str();
        sole::uuid uid{};
        if (db.isLoginValid(login))
        {
            if (db.isPasswordValid(login, pass)) {
                uid = db.login(login);
                answerObj.insert(std::pair<std::string, picojson::value> ("id",
                                                                          picojson::value(obj["id"].to_str())));
                answerObj.insert(std::pair<std::string, picojson::value> ("command",
                                                                          picojson::value("login")));
                answerObj.insert(std::pair<std::string, picojson::value> ("status",
                                                                          picojson::value("ok")));
                answerObj.insert(std::pair<std::string, picojson::value> ("session",
                                                                          picojson::value(uid.str())));
            } else {
                answerObj.insert(std::pair<std::string, picojson::value> ("id",
                                                                          picojson::value(obj["id"].to_str())));
                answerObj.insert(std::pair<std::string, picojson::value> ("command",
                                                                          picojson::value("login")));
                answerObj.insert(std::pair<std::string, picojson::value> ("status",
                                                                          picojson::value("failed")));
                answerObj.insert(std::pair<std::string, picojson::value> ("message",
                                                                          picojson::value("Wrong password")));
            }
        } else {
            answerObj.insert(std::pair<std::string, picojson::value> ("id",
                                                                      picojson::value(obj["id"].to_str())));
            answerObj.insert(std::pair<std::string, picojson::value> ("command",
                                                                      picojson::value("login")));
            answerObj.insert(std::pair<std::string, picojson::value> ("status",
                                                                      picojson::value("failed")));
            answerObj.insert(std::pair<std::string, picojson::value> ("message",
                                                                      picojson::value("Wrong login")));
        }
    } else if (command == "logout")
    {
        auto uuidStr = obj["session"].to_str();
        auto uid = sole::rebuild(uuidStr);
        if (db.isUuidValid(uid))
        {
            answerObj.insert(std::pair<std::string, picojson::value> ("id",
                                                                      picojson::value(obj["id"].to_str())));
            answerObj.insert(std::pair<std::string, picojson::value> ("command",
                                                                      picojson::value("logout")));
            answerObj.insert(std::pair<std::string, picojson::value> ("status",
                                                                      picojson::value("ok")));
            picojson::value answerValue(answerObj);
            auto answer = answerValue.serialize();
            send(connection.socket, answer.c_str(), answer.length(), 0);
            db.logout(uid);
            close(connection.socket);
            return;
        } else {
            answerObj.insert(std::pair<std::string, picojson::value> ("id",
                                                                      picojson::value(obj["id"].to_str())));
            answerObj.insert(std::pair<std::string, picojson::value> ("command",
                                                                      picojson::value("logout")));
            answerObj.insert(std::pair<std::string, picojson::value> ("status",
                                                                      picojson::value("failed")));
            answerObj.insert(std::pair<std::string, picojson::value> ("message",
                                                                      picojson::value("Wrong uuid")));
        }
    } else if (command == "message")
    {
        auto body = obj["body"].to_str();
        auto uuidStr = obj["session"].to_str();
        auto uid = sole::rebuild(uuidStr);
        if (db.isUuidValid(uid)) {
            messageQueue.send_message(uid, obj["body"].to_str());
        } else {
            answerObj.insert(std::pair<std::string, picojson::value> ("id",
                                                                      picojson::value(obj["id"].to_str())));
            answerObj.insert(std::pair<std::string, picojson::value> ("command",
                                                                      picojson::value("message")));
            answerObj.insert(std::pair<std::string, picojson::value> ("status",
                                                                      picojson::value("failed")));
            answerObj.insert(std::pair<std::string, picojson::value> ("message",
                                                                      picojson::value("Wrong uuid")));
        }
    }

    picojson::value answerValue(answerObj);
    auto answer = answerValue.serialize();
    send(connection.socket, answer.c_str(), answer.length(), 0);
}

void handleClient(struct client_connection connection)
{
    char buf[1024];
    size_t count = 0;
    while (!stop)
    {
        fd_set set;
        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        FD_ZERO(&set);
        FD_SET(connection.socket, &set);
        count = select(connection.socket + 1, &set, nullptr, nullptr, &timeout);
        if (count > 0) {
            count = read(connection.socket, buf, 1024);
            if (count == 0)
            {
                break;
            }
            buf[count] = '\0';
            std::string input(buf);
            handleMessage(input, connection);
        }
    }
    close(connection.socket);
}

void handleServerCommands(int server_socket)
{
    while (!stop)
    {
        std::string s;
        std::cin >> s;
        if (s == "stop"){
            stop = true;
            close(server_socket);
        }
    }
}

void handleMessagesFromQueue()
{
    while (!stop)
    {
        if (!messageQueue.isEmpty()) {
            Message message = messageQueue.get_last();
            std::string answer = R"({"command":"message","sender_login":")" +
                    db.getLogin(message.sender) + R"(","body":")" + message.body + "\"}";
            std::lock_guard<std::mutex> lock(connections_mutex);
            for (auto& i: connections)
            {
                send(i.socket, answer.c_str(), answer.length(), 0);
            }
        }
    }
}

int main() {
    int server_socket;
    int res;

    struct sockaddr_in server_address{};

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        printError("Failed to init socket.");
        return -1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1414);
    server_address.sin_addr.s_addr = INADDR_ANY;

    res = bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
    if (res == -1)
    {
        printError("Failed to bind address.");
        close(server_socket);
        return -2;
    }

    res = listen(server_socket, 10);
    if (res == -1)
    {
        printError("Failed to listen.");
        close(server_socket);
        return -3;
    }

    std::thread serverCommandsThread(handleServerCommands, server_socket);
    std::thread messageQueueThread(handleMessagesFromQueue);

    while (!stop)
    {
        struct client_connection new_connection{};
        new_connection.address_len = sizeof(new_connection.address);
        fd_set set;
        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        FD_ZERO(&set);
        FD_SET(server_socket, &set);
        res = select(server_socket + 1, &set, nullptr, nullptr, &timeout);
        if (res > 0)
        {
            new_connection.socket = accept(server_socket,
                                           (struct sockaddr*) &new_connection.address,
                                           &new_connection.address_len);
            if (new_connection.socket == -1 || stop)
            {
                printError("Failed to accept.");
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(connections_mutex);
                connections.push_back(new_connection);
            }
            threads.emplace_back(handleClient, new_connection);
        }
    }
    printError("stopped");
    messageQueueThread.join();
    serverCommandsThread.join();
    for(auto& t: threads)
    {
        t.join();
    }
    if (server_socket)
        close(server_socket);
    return 0;
}
