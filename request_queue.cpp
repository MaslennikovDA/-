#include "request_queue.h"

int RequestQueue::GetNoResultRequests() const {
    return RequestQueue::result_;
}

void RequestQueue::Counting_requests(const std::vector<Document>& result_request) {
    int size = static_cast<int>(result_request.size());
    if (requests_.size() >= min_in_day_) {
        if (requests_.front().counter == 0) --result_;
        requests_.pop_front();
    }
    if (size == 0) {
        requests_.push_back({ 0 });
        ++result_;
    }
    else {
        requests_.push_back({ 1 });
    }
}