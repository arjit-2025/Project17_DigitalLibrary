// datagen.cpp - Builds a synthetic but realistic library: books with keywords and
// abstracts, a citation graph and readers with histories and interests.
#include "datagen.h"

#include <algorithm>
#include <cctype>
#include <random>
#include <set>
#include <string>

namespace {

const char* TOPICS[] = {
    "algorithms", "graphs", "sorting", "searching", "dynamic", "programming", "greedy", "networks",
    "databases", "compilers", "security", "cryptography", "learning", "vision", "robotics", "physics",
    "biology", "chemistry", "history", "economics", "statistics", "geometry", "optimization",
    "complexity", "parallel", "distributed", "systems", "language", "retrieval", "compression"};
const int NUM_TOPICS = sizeof(TOPICS) / sizeof(TOPICS[0]);

const char* FILLER[] = {
    "study", "method", "analysis", "approach", "model", "results", "efficient", "novel", "data",
    "performance", "framework", "evaluation", "problem", "solution", "theory", "experiments", "design",
    "structure", "technique", "system", "the", "of", "and", "for", "with", "in", "a", "new", "large", "scale"};
const int NUM_FILLER = sizeof(FILLER) / sizeof(FILLER[0]);

const char* PREFIX[] = {"Introduction to", "Advanced", "Practical", "Foundations of", "Modern",
                        "Applied", "Essentials of", "Principles of", "Handbook of", "Topics in"};
const int NUM_PREFIX = sizeof(PREFIX) / sizeof(PREFIX[0]);

const char* FIRST[] = {"Asha", "Ravi", "Meera", "Arjun", "Kavya", "Rohan", "Isha", "Vikram",
                       "Neha", "Karan", "Divya", "Aman", "Pooja", "Sahil", "Tara", "Nikhil"};
const char* LAST[] = {"Sharma", "Verma", "Iyer", "Gupta", "Rao", "Nair", "Singh", "Das",
                      "Mehta", "Joshi", "Kapoor", "Bose", "Reddy", "Pillai"};
const int NUM_FIRST = sizeof(FIRST) / sizeof(FIRST[0]);
const int NUM_LAST = sizeof(LAST) / sizeof(LAST[0]);

std::string capitalize(std::string s) {
    if (!s.empty()) s[0] = (char)std::toupper((unsigned char)s[0]);
    return s;
}

int sharedKeywords(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    int c = 0;
    for (size_t i = 0; i < a.size(); ++i)
        for (size_t j = 0; j < b.size(); ++j)
            if (a[i] == b[j]) ++c;
    return c;
}

}  // namespace

void generateLibrary(Library& lib, int nBooks, int nUsers, unsigned seed) {
    lib.clear();
    if (nBooks < 0) nBooks = 0;
    if (nUsers < 0) nUsers = 0;

    std::mt19937 rng(seed);
    auto rnd = [&rng](int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); };

    // ---- Books ----
    lib.books.reserve(nBooks);
    for (int i = 0; i < nBooks; ++i) {
        Book b;
        b.id = i;

        std::set<int> picked;
        int nk = rnd(3, 5);
        while ((int)picked.size() < nk) picked.insert(rnd(0, NUM_TOPICS - 1));
        for (std::set<int>::iterator it = picked.begin(); it != picked.end(); ++it) b.keywords.push_back(TOPICS[*it]);
        std::shuffle(b.keywords.begin(), b.keywords.end(), rng);

        b.title = std::string(PREFIX[rnd(0, NUM_PREFIX - 1)]) + " " + capitalize(b.keywords[0]) + " and " +
                  capitalize(b.keywords[1]) + " Vol " + std::to_string(i % 7 + 1);
        b.author = std::string(FIRST[rnd(0, NUM_FIRST - 1)]) + " " + LAST[rnd(0, NUM_LAST - 1)];
        b.year = rnd(1970, 2026);
        b.rating = rnd(10, 50) / 10.0;
        b.readMinutes = rnd(10, 120);

        int words = rnd(25, 50);
        std::string text;
        for (int w = 0; w < words; ++w) {
            if (!text.empty()) text += ' ';
            int r = rnd(0, 99);
            if (r < 4) text += "dynamic programming";
            else if (r < 7) text += "shortest path";
            else if (r < 40) text += b.keywords[rnd(0, (int)b.keywords.size() - 1)];
            else text += FILLER[rnd(0, NUM_FILLER - 1)];
        }
        b.abstractText = text;
        lib.books.push_back(b);
    }

    // ---- Citation graph: each book cites 1..3 earlier books ----
    lib.citations.assign(nBooks, std::vector<Edge>());
    for (int i = 1; i < nBooks; ++i) {
        int k = rnd(1, 3);
        for (int c = 0; c < k; ++c) {
            // 70% of citations go to recent books (locality), 30% anywhere earlier.
            int j = (rnd(0, 9) < 7) ? rnd(std::max(0, i - 50), i - 1) : rnd(0, i - 1);
            int w = std::max(1, 10 - 3 * sharedKeywords(lib.books[i].keywords, lib.books[j].keywords));
            lib.citations[i].push_back(Edge(j, w));
            lib.citations[j].push_back(Edge(i, w));
        }
    }

    // ---- Users ----
    lib.users.reserve(nUsers);
    for (int u = 0; u < nUsers; ++u) {
        User us;
        us.id = u;
        us.name = "Reader_" + std::to_string(u);
        std::set<int> topics;
        while ((int)topics.size() < 3) topics.insert(rnd(0, NUM_TOPICS - 1));
        for (std::set<int>::iterator it = topics.begin(); it != topics.end(); ++it) us.interests.push_back(TOPICS[*it]);
        if (nBooks > 0) {
            int h = std::min(rnd(3, 8), nBooks);
            std::set<int> hist;
            while ((int)hist.size() < h) hist.insert(rnd(0, nBooks - 1));
            us.history.assign(hist.begin(), hist.end());
        }
        lib.users.push_back(us);
    }
}
