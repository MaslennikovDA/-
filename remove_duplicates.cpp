#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> result;
    std::set<std::set<std::string>> all_doc;

    for (const int document_id : search_server) {
        std::set<std::string> one_doc;
        std::map<std::string,double> doc = search_server.GetWordFrequencies(document_id);

        //std::transform - изменяет согласно предикату элемент и помещает его в контейнер адресат. 
        //Только transform сам не создает элементы и не добавляет их в контейнер. 
        //Что бы ваш метод отработал,  one_doc контейнер уже должен содержать нужное количество элементов.
        //Или использовать специальный итератор который добавляет элементы в контейнер 

        std::transform(doc.begin(), doc.end(), std::inserter(one_doc, one_doc.begin()), [](const auto element) {return element.first; });

        if (all_doc.count(one_doc) != 0) {
            result.insert(document_id);
        }
        else {
            all_doc.insert(one_doc);
        }
    }

    for (auto document_id : result) {
        std::cout << "Found duplicate document id "s << document_id << std::endl;
        search_server.RemoveDocument(document_id);
    }
}