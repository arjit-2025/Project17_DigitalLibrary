// library_engine.h - The search & recommendation engine that ties the algorithms to the domain.
//   * inverted index (keyword -> sorted book ids) stored in a balanced BST (std::map)
//   * keyword search  = index lookups + sorted-list intersection + merge-sort ranking
//   * phrase search   = any of the four string matchers over abstracts
//   * similar books   = LCS similarity of abstracts + keyword Jaccard
//   * recommendations = interest overlap + citation proximity (multi-source Dijkstra) + rating
#ifndef LIBRARY_ENGINE_H
#define LIBRARY_ENGINE_H

#include <map>
#include <string>
#include <vector>

#include "model.h"
#include "string_match.h"

struct ScoredBook {
    int bookId;
    double score;
};

struct Recommendation {
    int bookId;
    double score;            // 0.5*topic + 0.3*graph + 0.2*rating
    double topicScore;       // fraction of the reader's interests covered by the book's keywords
    double graphScore;       // 1 / (1 + distance/5) to the nearest book already read
    double ratingScore;      // rating / 5
    long long citationDistance;
};

class SearchEngine {
public:
    // The engine keeps a reference: rebuild (or recreate) it whenever the library changes.
    explicit SearchEngine(const Library& lib);
    void rebuild();

    size_t indexedTerms() const { return index_.size(); }

    // AND query over keywords / title words; results ranked by rating (desc).
    std::vector<int> keywordSearch(const std::string& query, long long& ops) const;

    // Book ids whose abstract contains the phrase (case-insensitive).
    std::vector<int> phraseSearch(const std::string& phrase, Matcher m, MatchStats& st) const;

    std::vector<ScoredBook> similarBooks(int bookId, int k, long long& cells) const;
    std::vector<Recommendation> recommend(int userId, int k) const;

    // Top recommendations turned into knapsack items (weight = minutes, value = score*100).
    std::vector<Item> readingCandidates(int userId, int maxItems) const;

private:
    const Library& lib_;
    std::map<std::string, std::vector<int>> index_;
    std::vector<std::string> loweredAbstracts_;
};

std::string toLowerCopy(const std::string& s);

#endif
