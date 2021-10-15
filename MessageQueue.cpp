//
// Created by GodKnows on 15.10.2021.
//

#include "MessageQueue.h"

void MessageQueue::send_message(sole::uuid sender, const std::string& body) {
    std::lock_guard<std::mutex> lock(queueMutex);
    queue.push({sender, body});
}

Message MessageQueue::get_last() {
    std::lock_guard<std::mutex> lock(queueMutex);
    auto res = queue.front();
    queue.pop();
    return res;
}

bool MessageQueue::isEmpty() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return queue.empty();
}
