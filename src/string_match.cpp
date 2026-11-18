// string_match.cpp - Four exact string matchers with operation counters.
#include "string_match.h"

#include "divide_conquer.h"   // powMod (fast exponentiation) for the Rabin-Karp high-order factor

const char* matcherName(Matcher m) {
    switch (m) {
        case Matcher::Naive: return "Naive";
        case Matcher::RabinKarp: return "Rabin-Karp";
        case Matcher::Automaton: return "Finite automaton";
        case Matcher::KMP: return "KMP";
    }
    return "?";
}

std::vector<int> naiveSearch(const std::string& text, const std::string& pat, MatchStats& st) {
    std::vector<int> res;
    const int n = (int)text.size(), m = (int)pat.size();
    if (m == 0 || m > n) return res;
    for (int s = 0; s <= n - m; ++s) {          // try every shift
        int j = 0;
        while (j < m) {
            ++st.comparisons;
            if (text[s + j] != pat[j]) break;
            ++j;
        }
        if (j == m) res.push_back(s);
    }
    return res;
}

std::vector<int> rabinKarpSearch(const std::string& text, const std::string& pat, MatchStats& st, long long mod) {
    std::vector<int> res;
    const int n = (int)text.size(), m = (int)pat.size();
    if (m == 0 || m > n || mod < 2 || mod >= 3000000000LL) return res;

    const long long base = 256;
    const long long h = powMod(base, m - 1, mod);   // weight of the leading character
    long long ph = 0, th = 0;
    for (int i = 0; i < m; ++i) {
        ph = (ph * base + (unsigned char)pat[i]) % mod;
        th = (th * base + (unsigned char)text[i]) % mod;
    }
    st.preprocessing += m;

    for (int s = 0;; ++s) {
        ++st.comparisons;                           // one hash comparison per window
        if (ph == th) {
            int j = 0;
            while (j < m) {                         // verify to rule out a collision
                ++st.comparisons;
                if (text[s + j] != pat[j]) break;
                ++j;
            }
            if (j == m) res.push_back(s);
            else ++st.spuriousHits;
        }
        if (s == n - m) break;
        // Roll the window: drop text[s], add text[s+m].
        th = (th - (long long)(unsigned char)text[s] * h % mod + mod) % mod;
        th = (th * base + (unsigned char)text[s + m]) % mod;
    }
    return res;
}

std::vector<int> automatonSearch(const std::string& text, const std::string& pat, MatchStats& st) {
    std::vector<int> res;
    const int n = (int)text.size(), m = (int)pat.size();
    if (m == 0 || m > n) return res;

    const int A = 256;
    // dfa[q*A + c] = next state after reading c in state q (state = length of matched prefix).
    std::vector<int> dfa((size_t)(m + 1) * A, 0);
    dfa[(unsigned char)pat[0]] = 1;
    int x = 0;   // restart state: where the automaton would be after reading pat[1..q-1]
    for (int q = 1; q <= m; ++q) {
        for (int c = 0; c < A; ++c) dfa[(size_t)q * A + c] = dfa[(size_t)x * A + c];   // mismatch transitions
        if (q < m) {
            dfa[(size_t)q * A + (unsigned char)pat[q]] = q + 1;                         // match transition
            x = dfa[(size_t)x * A + (unsigned char)pat[q]];
        }
    }
    st.preprocessing += (long long)(m + 1) * A;

    int q = 0;
    for (int i = 0; i < n; ++i) {
        ++st.comparisons;   // one transition per text character, never backs up
        q = dfa[(size_t)q * A + (unsigned char)text[i]];
        if (q == m) res.push_back(i - m + 1);
    }
    return res;
}

std::vector<int> computePrefixFunction(const std::string& pat, long long* ops) {
    const int m = (int)pat.size();
    std::vector<int> pi(m, 0);
    int k = 0;
    for (int q = 1; q < m; ++q) {
        while (k > 0 && pat[k] != pat[q]) {
            if (ops) ++*ops;
            k = pi[k - 1];
        }
        if (ops) ++*ops;
        if (pat[k] == pat[q]) ++k;
        pi[q] = k;
    }
    return pi;
}

std::vector<int> kmpSearch(const std::string& text, const std::string& pat, MatchStats& st) {
    std::vector<int> res;
    const int n = (int)text.size(), m = (int)pat.size();
    if (m == 0 || m > n) return res;

    long long pre = 0;
    std::vector<int> pi = computePrefixFunction(pat, &pre);
    st.preprocessing += pre;

    int q = 0;   // number of pattern characters currently matched
    for (int i = 0; i < n; ++i) {
        while (q > 0 && pat[q] != text[i]) {   // fall back using pi instead of re-reading text
            ++st.comparisons;
            q = pi[q - 1];
        }
        ++st.comparisons;
        if (pat[q] == text[i]) ++q;
        if (q == m) {
            res.push_back(i - m + 1);
            q = pi[q - 1];
        }
    }
    return res;
}

std::vector<int> runMatcher(Matcher m, const std::string& text, const std::string& pat, MatchStats& st) {
    switch (m) {
        case Matcher::Naive: return naiveSearch(text, pat, st);
        case Matcher::RabinKarp: return rabinKarpSearch(text, pat, st);
        case Matcher::Automaton: return automatonSearch(text, pat, st);
        case Matcher::KMP: return kmpSearch(text, pat, st);
    }
    return std::vector<int>();
}
