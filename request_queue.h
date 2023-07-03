#pragma once

#include "document.h"
#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        :search_server_(search_server), result_(0) {
    }

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> Result = search_server_.FindTopDocuments(raw_query, document_predicate);
        Counting_requests(Result);
        return Result;
    }

    // Вставил пробелы между методами

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status; });
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int GetNoResultRequests() const;

private:

    void Counting_requests(const std::vector<Document>& result_request);

    struct QueryResult {
        int counter = 0;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int result_;
};