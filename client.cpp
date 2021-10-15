//
// Created by GodKnows on 15.10.2021.
//

#include "ThirdParty/picojson.h"
#include "ThirdParty/sole.hpp"
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <mutex>

bool stop;
sole::uuid uid;
std::mutex messagesMutex;

void login(int socket)
{
    char response[1024];
    std::string login, pass, msg;
    while (!stop)
    {
        std::cout << "Please, log in" << std::endl << "Login:" << std::endl;
        std::cin >> login;
        std::cout << "Password:" << std::endl;
        std::cin >> pass;
        msg = R"({"id":2,"command":"login","login":")";
        msg += login;
        msg += R"(","password":")";
        msg += pass+"\"}";
        send(socket, msg.c_str(), msg.length(), 0);
        recv(socket, &response, sizeof(response), 0);
        std::string input(response);
        picojson::value value;
        std::string error = picojson::parse(value, input);
        if (!error.empty())
        {
            std::cout << error << std::endl;
            continue;
        }
        if (!value.is<picojson::object>())
        {
            std::cout << "Failed to get object" << std::endl;
            continue;
        }
        picojson::object& obj = value.get<picojson::object>();
        if (obj["status"].to_str() == "ok")
        {
            uid = sole::rebuild(obj["session"].to_str());
            return;
        }
    }
}


void logout(int socket)
{
    std::string  msg;
    msg = R"({"id":1,"command":"logout","session":")";
    msg += uid.str()+"\"}";
    std::lock_guard<std::mutex> lock(messagesMutex);
    send(socket, msg.c_str(), msg.length(), 0);
}

void sendMessage(int socket, const std::string& body)
{
    std::string msg;
    msg = R"({"id":1,"command":"message","body":")";
    msg += body + R"(", "session":")";
    msg += uid.str()+"\"}";
    send(socket, msg.c_str(), msg.length(), 0);
}

void getMessages(int socket)
{
    char response[1024];
    int res = 0;
    while (!stop)
    {
        fd_set set;
        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        FD_ZERO(&set);
        FD_SET(socket, &set);
        res = select(socket + 1, &set, nullptr, nullptr, &timeout);
        if (res > 0)
        {
            recv(socket, response, sizeof(response), 0);
            std::string input(response);
            picojson::value value;
            std::string error = picojson::parse(value, input);
            if (!error.empty())
            {
                std::cout << error << std::endl;
                return;
            }
            if (!value.is<picojson::object>())
            {
                std::cout << "Failed to get object" << std::endl;
                return;
            }
            picojson::object& obj = value.get<picojson::object>();
            std::string command = obj["command"].to_str();
            if (command == "logout")
            {
                if (obj["status"].to_str() == "ok")
                    stop = true;
            } else if (command == "message")
            {
                if (obj["status"].to_str() == "ok")
                    continue;
                std::cout << obj["sender_login"].to_str() << ": " << obj["body"].to_str() << std::endl;
            }
        }
    }
}

int main() {
    int res;
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1414);
    server_address.sin_addr.s_addr = INADDR_ANY;

    res = connect(client_socket, (struct sockaddr*) &server_address, sizeof(server_address));
    if (res < 0)
    {
        std::cout << "Cannot connect to server" << std::endl;
        return 1;
    }

    login(client_socket);
    std::cout << "Successful login" << std::endl;
    std::thread getMsgsThread(getMessages, client_socket);
    std::string message;
    while(!stop)
    {
        std::cin >> message;
        if (message == "logout")
        {
            logout(client_socket);
        } else {
            sendMessage(client_socket, message);
        }
    }
    getMsgsThread.join();
    close(client_socket);
    return 0;
}
