// model.h - Domain model for the Digital Library Search and Recommendation Engine.
// All records are plain structs so that algorithms can be swapped and tested fairly.
#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <utility>
#include <vector>

// A book / document in the catalogue.
// Invariant: inside a Library, books[i].id == i (ids are dense indices).
struct Book {
    int id = 0;
    std::string title;
    std::string author;
    int year = 0;
    double rating = 0.0;                 // 0.0 .. 5.0
    int readMinutes = 0;                 // time needed to read (knapsack weight)
    std::vector<std::string> keywords;   // subject terms (indexed for search)
    std::string abstractText;            // free text (used for phrase search, LCS, Huffman)
};

// A library member with a reading history and declared interests.
struct User {
    int id = 0;
    std::string name;
    std::vector<int> history;              // ids of books already read
    std::vector<std::string> interests;    // preferred keywords
};

// Edge of the citation graph: (neighbour book id, distance weight >= 0).
// Smaller weight = more closely related books.
typedef std::pair<int, int> Edge;

struct Library {
    std::vector<Book> books;
    std::vector<std::vector<Edge>> citations;   // undirected weighted adjacency list, size == books.size()
    std::vector<User> users;

    void clear() { books.clear(); citations.clear(); users.clear(); }
};

// ---- Types shared by the optimisation modules ------------------------------

// A candidate book in a reading plan (0/1 knapsack item).
// weight = minutes needed, value = interest score.
struct Item {
    int id;
    int weight;
    double value;
};

struct KnapsackResult {
    std::string algorithm;
    double value = 0.0;          // total value of chosen items
    int weight = 0;              // total minutes of chosen items
    std::vector<int> chosen;     // ids of chosen items
    long long ops = 0;           // basic operations (cells / nodes / comparisons)
    long long auxBytes = 0;      // extra memory used by the algorithm (estimate)
    bool complete = true;        // false if a search limit stopped an exact method early
};

// A reading-room booking [start, end) in minutes from midnight.
struct Interval {
    int id;
    int start;
    int end;
};

#endif
