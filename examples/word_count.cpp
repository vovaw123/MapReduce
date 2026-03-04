/// word_count.cpp – Classic MapReduce word-count demo.
///
/// Splits a set of text "documents" (plain strings) into words, counts how
/// many times each word appears across all documents, and prints the result.

#include "mr/mapreduce.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

int main()
{
    // Input: document_id → text
    std::vector<std::pair<int, std::string>> docs = {
        {1, "hello world"},
        {2, "hello mapreduce world"},
        {3, "mapreduce is great"},
        {4, "hello great world"},
    };

    // Map: (doc_id, text) → [(word, 1), ...]
    auto map_fn = [](const int& /*id*/, const std::string& text)
        -> std::vector<std::pair<std::string, int>>
    {
        std::vector<std::pair<std::string, int>> result;
        std::istringstream iss(text);
        std::string word;
        while (iss >> word) result.emplace_back(word, 1);
        return result;
    };

    // Reduce: (word, [1,1,...]) → total count
    auto reduce_fn = [](const std::string& /*word*/,
                        const std::vector<int>& counts) -> int
    {
        int total = 0;
        for (int c : counts) total += c;
        return total;
    };

    mr::MapReduce<int, std::string,
                  std::string, int,
                  int> engine(map_fn, reduce_fn, /*threads=*/4);

    auto result = engine.run(docs);

    std::cout << "Word counts:\n";
    for (const auto& [word, count] : result) {
        std::cout << "  " << word << ": " << count << "\n";
    }
}
