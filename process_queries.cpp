#include "process_queries.h"

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
    vector<vector<Document>> documents_lists(queries.size());
    transform(execution::par, queries.begin(), queries.end(), documents_lists.begin(), [&search_server](string query) {return search_server.FindTopDocuments(query); });
    return documents_lists;
}
vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    vector<Document> result = {};
    for (vector<Document> documents : ProcessQueries(search_server, queries)) {
        for(const Document& document: documents){
        result.push_back(document);
        }
    }
    return result;
}