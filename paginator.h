#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) {
        page_begin_ = begin;
        page_end_ = end;
        page_size_ = std::distance(begin, end);
    }

    Iterator begin() const {
        return page_begin_;
    }

    Iterator end() const {
        return page_end_;
    }

    size_t size() const {
        return page_size_;
    }

private:
    Iterator page_begin_;
    Iterator page_end_;
    size_t page_size_;
};

template <typename Iterator>
class Paginator {
    // тело класса
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        Iterator list_begin = begin;
        Iterator list_end = end;
        Iterator list_cut = list_begin;
        std::advance(list_cut, page_size);
        while (distance(list_cut, list_end) > 0) {
            result_.push_back({ list_begin, list_cut });
            list_begin = list_cut;
            if (std::distance(list_cut, list_end) > static_cast<int>(page_size)) {
                std::advance(list_cut, page_size);
            }
            else break;
        }
        result_.push_back({ list_cut, list_end });
    }

    auto begin() const {
        return result_.begin();
    }

    auto end() const {
        return result_.end();
    }

    size_t size() const {
        return result_.size();
    }

private:
    std::vector <IteratorRange<Iterator>> result_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(c.begin(), c.end(), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, const IteratorRange<Iterator>& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        output << *it;
    }
    return output;
}