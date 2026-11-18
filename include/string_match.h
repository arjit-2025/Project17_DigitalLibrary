// string_match.h - Exact pattern matching: naive, Rabin-Karp, finite automaton, KMP.
// Used for phrase search inside book abstracts. All return 0-based start positions
// of every (possibly overlapping) occurrence of pat in text.
#ifndef STRING_MATCH_H
#define STRING_MATCH_H

#include <string>
#include <vector>

struct MatchStats {
    long long comparisons = 0;     // character comparisons / hash checks / DFA transitions
    long long spuriousHits = 0;    // Rabin-Karp: hash matched but text did not
    long long preprocessing = 0;   // work done before scanning the text
};

enum class Matcher { Naive, RabinKarp, Automaton, KMP };

const char* matcherName(Matcher m);

// Theta((n-m+1)*m) worst case, O(1) extra space.
std::vector<int> naiveSearch(const std::string& text, const std::string& pat, MatchStats& st);

// Rolling hash. Expected Theta(n+m), worst Theta(n*m) when many hashes collide.
// mod must be in [2, 3e9). A small mod (e.g. 101) shows spurious hits.
std::vector<int> rabinKarpSearch(const std::string& text, const std::string& pat, MatchStats& st,
                                 long long mod = 1000000007LL);

// DFA over 256 symbols. Preprocessing Theta(m * 256), scan exactly n transitions.
std::vector<int> automatonSearch(const std::string& text, const std::string& pat, MatchStats& st);

// Knuth-Morris-Pratt. Preprocessing Theta(m), scan Theta(n), at most 2n comparisons.
std::vector<int> kmpSearch(const std::string& text, const std::string& pat, MatchStats& st);

// pi[q] = length of the longest proper prefix of pat[0..q] that is also a suffix of it.
std::vector<int> computePrefixFunction(const std::string& pat, long long* ops = nullptr);

std::vector<int> runMatcher(Matcher m, const std::string& text, const std::string& pat, MatchStats& st);

#endif
