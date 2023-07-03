#include "search_server.h"

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    if (documents_.count(document_id) != 0) {
        throw std::invalid_argument("this document_id already exists"s);
    }
    if (document_id < 0) {
        throw std::invalid_argument("document_id < 0"s);
    }
    if (IsValidWord(document) == false) {
        throw std::invalid_argument("invalid characters in add document"s);
    }
    id_doc_.insert(document_id);
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("No document with id "s +  std::to_string(document_id));
    }
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy, const std::string& raw_query, int document_id) const {
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("No document with id "s + std::to_string(document_id));
    }
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;

    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy, const std::string& raw_query, int document_id) const {
    if (documents_.count(document_id) == 0) {
        throw std::out_of_range("No document with id "s + std::to_string(document_id));
    }
    const Query query = ParseQuery(raw_query, false);//передаем второй параметр типа bool для разделения однопоточного и многопоточного метода
    std::vector<std::string> matched_words = {};
    
    //функции any_of и for_each работают правильно, тестировал на стуктуре set, тесты проходят все, кроме ratio
    bool key = std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [this, document_id, &matched_words](auto& word) {
        return ((word_to_document_freqs_.count(word) != 0) && (word_to_document_freqs_.at(word).count(document_id) != 0));
     });
    if (key) return { matched_words, documents_.at(document_id).status };

    matched_words.resize(query.plus_words.size());
    std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this,document_id](std::string word)
        {
            return ((word_to_document_freqs_.at(word).count(document_id) != 0) && (word_to_document_freqs_.count(word) != 0));
        }); 
    //вот здесь удаляем дубликаты
    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    auto last = std::unique(std::execution::par, matched_words.begin(), matched_words.end());
    matched_words.erase(last, matched_words.end());
    //если была пустая строка, удаляем ее, она будет на 1 первой позиции после сортировки и удаления дубликатов
    if (matched_words[0].empty())
    {
        matched_words.erase(matched_words.begin());
    }
    return { matched_words, documents_.at(document_id).status };
}

//убрал удаленный метод

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (IsNotValidQuery(text)) {
        throw std::invalid_argument("request error"s);
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text, bool key) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    if (key == true){
        for (auto* words : { &query.plus_words, &query.minus_words }) {
            std::sort(words->begin(), words->end());
            words->erase(std::unique(words->begin(), words->end()), words->end());
        }
    }

    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

bool SearchServer::IsNotValidQuery(const std::string& query_word)
{
    if (query_word[0] == '-' || query_word.size() == 0 || IsValidWord(query_word) == false)
    {
        return true;
    }
    return false;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }

    
    int rating_sum = 0;
    //использовал функцию accumulate
    rating_sum = std::accumulate(ratings.begin(), ratings.end(), rating_sum);
    
    return rating_sum / static_cast<int>(ratings.size());
}

bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string, double> empty_map = {};

    if (document_to_word_freqs_.count(document_id) != 0) {
        return document_to_word_freqs_.at(document_id);
    }
    else {
        return empty_map;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    documents_.erase(document_id);
    id_doc_.erase(document_id);
    //теперь сначала удаляем элементы из поля word_to_document_freqs_, а только потом из поля document_to_word_freqs_
    for (auto [word, freq] : GetWordFrequencies(document_id)) {
        word_to_document_freqs_[word].erase(document_id);
        // если значений у этого ключа нет, удаляем ключ
        if (word_to_document_freqs_.count(word) == 0) {
            word_to_document_freqs_.erase(word);
        }
    }
    document_to_word_freqs_.erase(document_id);   
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy, int document_id) {
    documents_.erase(document_id);
    id_doc_.erase(document_id);
    //теперь сначала удаляем элементы из поля word_to_document_freqs_, а только потом из поля document_to_word_freqs_
    for (auto [word, freq] : GetWordFrequencies(document_id)) {
        word_to_document_freqs_[word].erase(document_id);
        // если значений у этого ключа нет, удаляем ключ
        if (word_to_document_freqs_.count(word) == 0) {
            word_to_document_freqs_.erase(word);
        }
    }
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy, int document_id) {
    documents_.erase(document_id);
    id_doc_.erase(document_id);
    //создаем вектор указателей, с const
    std::vector<const std::string*> word_adr(GetWordFrequencies(document_id).size());

    //используем алгоритм transform, чтобы не создавался указатель на временный объект, используем константную ссылку в лямбду-функцию
    std::transform(std::execution::par, document_to_word_freqs_.at(document_id).begin(), document_to_word_freqs_.at(document_id).end(),
        word_adr.begin(), [](const auto& data) {return &(data.first); });
      
    //for_each как замена for, захватываем this, чтобы получить доступ к полям
    std::for_each(std::execution::par, word_adr.begin(), word_adr.end(), [this, document_id](const std::string* word) {
        word_to_document_freqs_.at(*word).erase(document_id);
        // если значений у этого ключа нет, удаляем ключ
        if (word_to_document_freqs_.count(*word) == 0) {
            word_to_document_freqs_.erase(*word);}
        });

    document_to_word_freqs_.erase(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}