// library_engine.cpp - Search and recommendation built on the algorithm modules.
#include "library_engine.h"

#include <cctype>
#include <cmath>
#include <set>

#include "divide_conquer.h"
#include "dp.h"
#include "graph.h"

std::string toLowerCopy(const std::string& s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i) r[i] = (char)std::tolower((unsigned char)r[i]);
    return r;
}

namespace {

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

double jaccard(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    std::set<std::string> sa(a.begin(), a.end()), sb(b.begin(), b.end());
    if (sa.empty() && sb.empty()) return 0.0;
    int inter = 0;
    for (std::set<std::string>::const_iterator it = sa.begin(); it != sa.end(); ++it)
        if (sb.count(*it)) ++inter;
    return (double)inter / (double)(sa.size() + sb.size() - inter);
}

// Intersection of two sorted id lists in Theta(|a| + |b|).
std::vector<int> intersectSorted(const std::vector<int>& a, const std::vector<int>& b, long long& ops) {
    std::vector<int> out;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        ++ops;
        if (a[i] < b[j]) ++i;
        else if (b[j] < a[i]) ++j;
        else { out.push_back(a[i]); ++i; ++j; }
    }
    return out;
}

}  // namespace

SearchEngine::SearchEngine(const Library& lib) : lib_(lib) { rebuild(); }

void SearchEngine::rebuild() {
    index_.clear();
    loweredAbstracts_.clear();
    loweredAbstracts_.reserve(lib_.books.size());
    for (size_t i = 0; i < lib_.books.size(); ++i) {
        const Book& b = lib_.books[i];
        std::vector<std::string> terms;
        for (size_t k = 0; k < b.keywords.size(); ++k) {
            std::vector<std::string> t = tokenize(b.keywords[k]);
            terms.insert(terms.end(), t.begin(), t.end());
        }
        std::vector<std::string> titleWords = tokenize(b.title);
        terms.insert(terms.end(), titleWords.begin(), titleWords.end());
        for (size_t k = 0; k < terms.size(); ++k) {
            std::vector<int>& posting = index_[terms[k]];
            // Books are visited in id order, so postings stay sorted; skip duplicates.
            if (posting.empty() || posting.back() != b.id) posting.push_back(b.id);
        }
        loweredAbstracts_.push_back(toLowerCopy(b.abstractText));
    }
}

std::vector<int> SearchEngine::keywordSearch(const std::string& query, long long& ops) const {
    std::vector<std::string> terms = tokenize(query);
    std::vector<int> result;
    if (terms.empty()) return result;
    for (size_t k = 0; k < terms.size(); ++k) {
        ++ops;   // one O(log T) tree lookup
        std::map<std::string, std::vector<int>>::const_iterator it = index_.find(terms[k]);
        if (it == index_.end()) return std::vector<int>();
        result = (k == 0) ? it->second : intersectSorted(result, it->second, ops);
        if (result.empty()) return result;
    }
    const Library& lib = lib_;
    SortStats st;
    mergeSort(result, [&lib](int a, int b) {
        if (lib.books[a].rating != lib.books[b].rating) return lib.books[a].rating > lib.books[b].rating;
        return a < b;
    }, st);
    ops += st.comparisons;
    return result;
}

std::vector<int> SearchEngine::phraseSearch(const std::string& phrase, Matcher m, MatchStats& st) const {
    std::vector<int> hits;
    std::string p = toLowerCopy(trim(phrase));
    if (p.empty()) return hits;
    for (size_t i = 0; i < loweredAbstracts_.size(); ++i)
        if (!runMatcher(m, loweredAbstracts_[i], p, st).empty()) hits.push_back((int)i);
    return hits;
}

std::vector<ScoredBook> SearchEngine::similarBooks(int bookId, int k, long long& cells) const {
    std::vector<ScoredBook> out;
    if (bookId < 0 || bookId >= (int)lib_.books.size() || k <= 0) return out;
    const Book& target = lib_.books[bookId];
    std::vector<std::string> tt = tokenize(target.abstractText);
    for (size_t j = 0; j < lib_.books.size(); ++j) {
        if ((int)j == bookId) continue;
        const Book& b = lib_.books[j];
        double s = 0.6 * lcsSimilarity(tt, tokenize(b.abstractText), &cells) + 0.4 * jaccard(target.keywords, b.keywords);
        ScoredBook sb = {(int)j, s};
        out.push_back(sb);
    }
    SortStats st;
    mergeSort(out, [](const ScoredBook& a, const ScoredBook& b) {
        return a.score != b.score ? a.score > b.score : a.bookId < b.bookId;
    }, st);
    if ((int)out.size() > k) out.resize(k);
    return out;
}

std::vector<Recommendation> SearchEngine::recommend(int userId, int k) const {
    std::vector<Recommendation> out;
    if (userId < 0 || userId >= (int)lib_.users.size() || k <= 0) return out;
    const User& u = lib_.users[userId];
    const int n = (int)lib_.books.size();

    std::vector<char> alreadyRead(n, 0);
    std::vector<int> sources;
    for (size_t i = 0; i < u.history.size(); ++i) {
        int b = u.history[i];
        if (b >= 0 && b < n) { alreadyRead[b] = 1; sources.push_back(b); }
    }
    // Distance from the nearest already-read book to every other book (one Dijkstra run).
    PathResult pr = dijkstra(lib_.citations, sources);

    std::set<std::string> interests;
    for (size_t i = 0; i < u.interests.size(); ++i) interests.insert(toLowerCopy(u.interests[i]));

    for (int b = 0; b < n; ++b) {
        if (alreadyRead[b]) continue;
        const Book& bk = lib_.books[b];
        int overlap = 0;
        for (size_t k2 = 0; k2 < bk.keywords.size(); ++k2)
            if (interests.count(toLowerCopy(bk.keywords[k2]))) ++overlap;
        Recommendation r;
        r.bookId = b;
        r.topicScore = interests.empty() ? 0.0 : (double)overlap / interests.size();
        r.citationDistance = (b < (int)pr.dist.size()) ? pr.dist[b] : INF_DIST;
        r.graphScore = r.citationDistance >= INF_DIST ? 0.0 : 1.0 / (1.0 + r.citationDistance / 5.0);
        r.ratingScore = bk.rating / 5.0;
        r.score = 0.5 * r.topicScore + 0.3 * r.graphScore + 0.2 * r.ratingScore;
        out.push_back(r);
    }
    SortStats st;
    mergeSort(out, [](const Recommendation& a, const Recommendation& b) {
        return a.score != b.score ? a.score > b.score : a.bookId < b.bookId;
    }, st);
    if ((int)out.size() > k) out.resize(k);
    return out;
}

std::vector<Item> SearchEngine::readingCandidates(int userId, int maxItems) const {
    std::vector<Recommendation> recs = recommend(userId, maxItems);
    std::vector<Item> items;
    for (size_t i = 0; i < recs.size(); ++i) {
        Item it;
        it.id = recs[i].bookId;
        it.weight = lib_.books[recs[i].bookId].readMinutes;
        it.value = std::floor(recs[i].score * 1000.0 + 0.5) / 10.0;   // score*100, one decimal
        items.push_back(it);
    }
    return items;
}
