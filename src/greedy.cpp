// greedy.cpp - Greedy scheduling, allocation and compression modules.
#include "greedy.h"

#include <functional>
#include <queue>

#include "divide_conquer.h"

namespace {

double ratio(const Item& it) { return it.weight == 0 ? 1e18 : it.value / it.weight; }

std::vector<Item> validItems(const std::vector<Item>& items) {
    std::vector<Item> v;
    for (size_t i = 0; i < items.size(); ++i)
        if (items[i].weight >= 0 && items[i].value >= 0) v.push_back(items[i]);
    return v;
}

std::vector<Interval> validIntervals(const std::vector<Interval>& in) {
    std::vector<Interval> v;
    for (size_t i = 0; i < in.size(); ++i)
        if (in[i].end > in[i].start) v.push_back(in[i]);   // reject empty / reversed bookings
    return v;
}

}  // namespace

KnapsackResult greedyKnapsack01(const std::vector<Item>& items, int capacity) {
    KnapsackResult r;
    r.algorithm = "Greedy (best value per minute)";
    std::vector<Item> v = validItems(items);
    SortStats st;
    mergeSort(v, [](const Item& a, const Item& b) { return ratio(a) > ratio(b); }, st);
    r.ops = st.comparisons;

    int remaining = capacity < 0 ? 0 : capacity;
    for (size_t i = 0; i < v.size(); ++i) {
        ++r.ops;
        if (v[i].weight <= remaining) {
            remaining -= v[i].weight;
            r.chosen.push_back(v[i].id);
            r.value += v[i].value;
            r.weight += v[i].weight;
        }
    }
    r.auxBytes = (long long)v.size() * 2 * (long long)sizeof(Item);   // copy + merge buffer
    return r;
}

double fractionalKnapsack(const std::vector<Item>& items, int capacity, long long& ops) {
    std::vector<Item> v = validItems(items);
    SortStats st;
    mergeSort(v, [](const Item& a, const Item& b) { return ratio(a) > ratio(b); }, st);
    ops += st.comparisons;
    double value = 0.0;
    double remaining = capacity < 0 ? 0 : capacity;
    for (size_t i = 0; i < v.size() && remaining > 0; ++i) {
        ++ops;
        if (v[i].weight <= remaining) {
            value += v[i].value;
            remaining -= v[i].weight;
        } else {
            value += v[i].value * (remaining / v[i].weight);   // take the fitting fraction
            remaining = 0;
        }
    }
    return value;
}

std::vector<Interval> activitySelection(std::vector<Interval> bookings, long long& ops) {
    std::vector<Interval> v = validIntervals(bookings);
    SortStats st;
    mergeSort(v, [](const Interval& a, const Interval& b) {
        return a.end != b.end ? a.end < b.end : a.start < b.start;
    }, st);
    ops += st.comparisons;

    std::vector<Interval> chosen;
    int lastEnd = -2147483647;
    for (size_t i = 0; i < v.size(); ++i) {
        ++ops;
        if (v[i].start >= lastEnd) {   // compatible with everything chosen so far
            chosen.push_back(v[i]);
            lastEnd = v[i].end;
        }
    }
    return chosen;
}

std::vector<Interval> firstComeSelection(std::vector<Interval> bookings, long long& ops) {
    std::vector<Interval> v = validIntervals(bookings);
    SortStats st;
    mergeSort(v, [](const Interval& a, const Interval& b) { return a.start < b.start; }, st);
    ops += st.comparisons;
    std::vector<Interval> chosen;
    int lastEnd = -2147483647;
    for (size_t i = 0; i < v.size(); ++i) {
        ++ops;
        if (v[i].start >= lastEnd) {
            chosen.push_back(v[i]);
            lastEnd = v[i].end;
        }
    }
    return chosen;
}

PartitionResult intervalPartitioning(std::vector<Interval> bookings, long long& ops) {
    PartitionResult r;
    std::vector<Interval> v = validIntervals(bookings);
    SortStats st;
    mergeSort(v, [](const Interval& a, const Interval& b) {
        return a.start != b.start ? a.start < b.start : a.end < b.end;
    }, st);
    ops += st.comparisons;

    typedef std::pair<int, int> FreeAt;   // (time the room becomes free, room number)
    std::priority_queue<FreeAt, std::vector<FreeAt>, std::greater<FreeAt>> heap;
    for (size_t i = 0; i < v.size(); ++i) {
        ++ops;
        int room;
        if (!heap.empty() && heap.top().first <= v[i].start) {   // reuse the room that frees up earliest
            room = heap.top().second;
            heap.pop();
        } else {
            room = r.rooms++;                                    // all rooms busy: open a new one
        }
        r.assignment.push_back(std::make_pair(v[i].id, room));
        heap.push(FreeAt(v[i].end, room));
    }
    return r;
}

HuffmanResult huffmanCompress(const std::string& text) {
    HuffmanResult r;
    r.originalBits = 8LL * (long long)text.size();
    if (text.empty()) {
        r.roundTripOk = true;
        return r;
    }

    long long freq[256] = {0};
    for (size_t i = 0; i < text.size(); ++i) freq[(unsigned char)text[i]]++;

    struct Node {
        long long f;
        int left, right;
        int sym;
    };
    std::vector<Node> nodes;
    typedef std::pair<long long, int> QE;   // (frequency, node index) - index breaks ties deterministically
    std::priority_queue<QE, std::vector<QE>, std::greater<QE>> pq;
    for (int c = 0; c < 256; ++c) {
        if (freq[c]) {
            Node nd = {freq[c], -1, -1, c};
            nodes.push_back(nd);
            pq.push(QE(freq[c], (int)nodes.size() - 1));
        }
    }
    r.distinctSymbols = (int)nodes.size();

    std::string codeOf[256];
    int root = pq.top().second;
    if (nodes.size() == 1) {
        codeOf[nodes[0].sym] = "0";   // single symbol still needs one bit
    } else {
        // Greedy step: repeatedly merge the two least frequent trees.
        while (pq.size() > 1) {
            QE a = pq.top(); pq.pop();
            QE b = pq.top(); pq.pop();
            Node nd = {a.first + b.first, a.second, b.second, -1};
            nodes.push_back(nd);
            pq.push(QE(nd.f, (int)nodes.size() - 1));
        }
        root = pq.top().second;
        std::vector<std::pair<int, std::string>> stack;
        stack.push_back(std::make_pair(root, std::string()));
        while (!stack.empty()) {
            std::pair<int, std::string> cur = stack.back();
            stack.pop_back();
            const Node& nd = nodes[cur.first];
            if (nd.sym >= 0) {
                codeOf[nd.sym] = cur.second;
            } else {
                stack.push_back(std::make_pair(nd.left, cur.second + "0"));
                stack.push_back(std::make_pair(nd.right, cur.second + "1"));
            }
        }
    }
    for (int c = 0; c < 256; ++c)
        if (freq[c]) r.codes.push_back(std::make_pair((unsigned char)c, codeOf[c]));

    // Encode, then decode to prove the code is a correct prefix code.
    std::string bits;
    for (size_t i = 0; i < text.size(); ++i) bits += codeOf[(unsigned char)text[i]];
    r.encodedBits = (long long)bits.size();

    std::string decoded;
    decoded.reserve(text.size());
    if (nodes.size() == 1) {
        decoded.assign(bits.size(), (char)nodes[0].sym);
    } else {
        int cur = root;
        for (size_t i = 0; i < bits.size(); ++i) {
            cur = bits[i] == '0' ? nodes[cur].left : nodes[cur].right;
            if (nodes[cur].sym >= 0) {
                decoded += (char)nodes[cur].sym;
                cur = root;
            }
        }
    }
    r.roundTripOk = (decoded == text);
    return r;
}
