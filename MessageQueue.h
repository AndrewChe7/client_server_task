//
// Created by GodKnows on 15.10.2021.
//

#ifndef CLIENT_SERVER_TASK_MESSAGEQUEUE_H
#define CLIENT_SERVER_TASK_MESSAGEQUEUE_H

#include <queue>
#include <mutex>
#include "ThirdParty/sole.hpp"

struct Message {
    sole::uuid sender;
    std::string body;
};

class MessageQueue {
private:
    std::queue<Message> queue;
    std::mutex queueMutex;
public:
    MessageQueue() = default;
    void send_message(sole::uuid sender, const std::string& body);
    bool isEmpty();
    Message get_last();
};


#endif //CLIENT_SERVER_TASK_MESSAGEQUEUE_H
