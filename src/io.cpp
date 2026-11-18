// io.cpp - CSV persistence with defensive validation.
#include "io.h"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace {

std::string clean(const std::string& s) {   // fields must not contain separators or newlines
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i)
        if (r[i] == ',' || r[i] == ';' || r[i] == '\n' || r[i] == '\r') r[i] = ' ';
    return r;
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == sep) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}

bool parseLong(const std::string& s, long& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    errno = 0;
    long v = std::strtol(s.c_str(), &end, 10);
    if (errno != 0 || *end != '\0') return false;
    out = v;
    return true;
}

bool parseDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    double v = std::strtod(s.c_str(), &end);
    if (*end != '\0') return false;
    out = v;
    return true;
}

bool getLine(std::istream& in, std::string& line) {
    if (!std::getline(in, line)) return false;
    if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
    return true;
}

}  // namespace

bool ensureDirectory(const std::string& path) {
#ifdef _WIN32
    int rc = _mkdir(path.c_str());
#else
    int rc = mkdir(path.c_str(), 0755);
#endif
    return rc == 0 || errno == EEXIST;
}

bool saveLibrary(const Library& lib, const std::string& prefix, std::string& error) {
    std::ofstream fb((prefix + "_books.csv").c_str());
    std::ofstream fc((prefix + "_citations.csv").c_str());
    std::ofstream fu((prefix + "_users.csv").c_str());
    if (!fb || !fc || !fu) {
        error = "cannot create files with prefix '" + prefix + "' (does the folder exist?)";
        return false;
    }
    fb << "id,title,author,year,rating,readMinutes,keywords,abstract\n";
    for (size_t i = 0; i < lib.books.size(); ++i) {
        const Book& b = lib.books[i];
        fb << b.id << ',' << clean(b.title) << ',' << clean(b.author) << ',' << b.year << ',' << b.rating << ','
           << b.readMinutes << ',';
        for (size_t k = 0; k < b.keywords.size(); ++k) fb << (k ? ";" : "") << clean(b.keywords[k]);
        fb << ',' << clean(b.abstractText) << '\n';
    }
    fc << "from,to,weight\n";
    for (size_t u = 0; u < lib.citations.size(); ++u)
        for (size_t k = 0; k < lib.citations[u].size(); ++k)
            if ((int)u < lib.citations[u][k].first)   // each undirected edge once
                fc << u << ',' << lib.citations[u][k].first << ',' << lib.citations[u][k].second << '\n';
    fu << "id,name,interests,history\n";
    for (size_t i = 0; i < lib.users.size(); ++i) {
        const User& us = lib.users[i];
        fu << us.id << ',' << clean(us.name) << ',';
        for (size_t k = 0; k < us.interests.size(); ++k) fu << (k ? ";" : "") << clean(us.interests[k]);
        fu << ',';
        for (size_t k = 0; k < us.history.size(); ++k) fu << (k ? ";" : "") << us.history[k];
        fu << '\n';
    }
    return true;
}

bool loadLibrary(Library& lib, const std::string& prefix, std::string& error, int& skippedRows) {
    skippedRows = 0;
    std::ifstream fb((prefix + "_books.csv").c_str());
    if (!fb) {
        error = "cannot open " + prefix + "_books.csv";
        return false;
    }
    Library tmp;
    std::map<long, int> remap;   // id in file -> dense id
    std::string line;
    getLine(fb, line);   // header
    while (getLine(fb, line)) {
        if (line.empty()) continue;
        std::vector<std::string> f = split(line, ',');
        long id, year, minutes;
        double rating;
        if (f.size() != 8 || !parseLong(f[0], id) || f[1].empty() || !parseLong(f[3], year) || year < 0 ||
            year > 3000 || !parseDouble(f[4], rating) || rating < 0 || rating > 5 || !parseLong(f[5], minutes) ||
            minutes <= 0 || minutes > 100000 || remap.count(id)) {
            ++skippedRows;
            continue;
        }
        Book b;
        b.id = (int)tmp.books.size();
        remap[id] = b.id;
        b.title = f[1];
        b.author = f[2];
        b.year = (int)year;
        b.rating = rating;
        b.readMinutes = (int)minutes;
        std::vector<std::string> kw = split(f[6], ';');
        for (size_t k = 0; k < kw.size(); ++k)
            if (!kw[k].empty()) b.keywords.push_back(kw[k]);
        b.abstractText = f[7];
        tmp.books.push_back(b);
    }
    if (tmp.books.empty()) {
        error = "no valid book rows found";
        return false;
    }
    tmp.citations.assign(tmp.books.size(), std::vector<Edge>());

    std::ifstream fc((prefix + "_citations.csv").c_str());
    if (fc) {
        getLine(fc, line);
        while (getLine(fc, line)) {
            if (line.empty()) continue;
            std::vector<std::string> f = split(line, ',');
            long a, b, w;
            if (f.size() != 3 || !parseLong(f[0], a) || !parseLong(f[1], b) || !parseLong(f[2], w) || w < 0 ||
                !remap.count(a) || !remap.count(b) || a == b) {
                ++skippedRows;
                continue;
            }
            int u = remap[a], v = remap[b];
            tmp.citations[u].push_back(Edge(v, (int)w));
            tmp.citations[v].push_back(Edge(u, (int)w));
        }
    }

    std::ifstream fu((prefix + "_users.csv").c_str());
    if (fu) {
        getLine(fu, line);
        while (getLine(fu, line)) {
            if (line.empty()) continue;
            std::vector<std::string> f = split(line, ',');
            if (f.size() != 4 || f[1].empty()) {
                ++skippedRows;
                continue;
            }
            User us;
            us.id = (int)tmp.users.size();
            us.name = f[1];
            std::vector<std::string> in = split(f[2], ';');
            for (size_t k = 0; k < in.size(); ++k)
                if (!in[k].empty()) us.interests.push_back(in[k]);
            std::vector<std::string> h = split(f[3], ';');
            for (size_t k = 0; k < h.size(); ++k) {
                long bid;
                if (parseLong(h[k], bid) && remap.count(bid)) us.history.push_back(remap[bid]);
            }
            tmp.users.push_back(us);
        }
    }
    lib.books.swap(tmp.books);
    lib.citations.swap(tmp.citations);
    lib.users.swap(tmp.users);
    return true;
}
