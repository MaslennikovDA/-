#pragma once
#include<execution>
#include<vector>
#include<string>
#include<execution>
#include"search_server.h"
#include"document.h"
using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries);
vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries);