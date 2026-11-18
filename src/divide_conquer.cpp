// divide_conquer.cpp - Non-template divide-and-conquer modules.
#include "divide_conquer.h"

#include <climits>

namespace {

SubarrayResult maxSubRec(const std::vector<long long>& a, int lo, int hi, long long& ops) {
    if (lo == hi) {
        SubarrayResult r;
        r.sum = a[lo];
        r.left = r.right = lo;
        return r;
    }
    int mid = lo + (hi - lo) / 2;
    SubarrayResult L = maxSubRec(a, lo, mid, ops);
    SubarrayResult R = maxSubRec(a, mid + 1, hi, ops);

    // Best subarray crossing the midpoint: best suffix of left + best prefix of right.
    long long s = 0, bestL = LLONG_MIN;
    int li = mid;
    for (int i = mid; i >= lo; --i) {
        ++ops;
        s += a[i];
        if (s > bestL) { bestL = s; li = i; }
    }
    s = 0;
    long long bestR = LLONG_MIN;
    int ri = mid + 1;
    for (int j = mid + 1; j <= hi; ++j) {
        ++ops;
        s += a[j];
        if (s > bestR) { bestR = s; ri = j; }
    }
    SubarrayResult C;
    C.sum = bestL + bestR;
    C.left = li;
    C.right = ri;

    if (L.sum >= R.sum && L.sum >= C.sum) return L;
    if (R.sum >= C.sum) return R;
    return C;
}

}  // namespace

SubarrayResult maxSubarrayDC(const std::vector<long long>& a) {
    SubarrayResult r;
    if (a.empty()) return r;
    long long ops = 0;
    r = maxSubRec(a, 0, (int)a.size() - 1, ops);
    r.ops = ops;
    return r;
}

SubarrayResult maxSubarrayKadane(const std::vector<long long>& a) {
    SubarrayResult r;
    if (a.empty()) return r;
    long long best = a[0], cur = a[0];
    int bl = 0, br = 0, cl = 0;
    for (int i = 1; i < (int)a.size(); ++i) {
        ++r.ops;
        // Either extend the current run or start a new one at i.
        if (cur < 0) { cur = a[i]; cl = i; }
        else cur += a[i];
        if (cur > best) { best = cur; bl = cl; br = i; }
    }
    r.sum = best;
    r.left = bl;
    r.right = br;
    return r;
}

long long powMod(long long base, long long exp, long long mod, long long* multiplications) {
    if (mod <= 1) return 0;
    base %= mod;
    if (base < 0) base += mod;
    if (exp <= 0) return 1 % mod;
    long long half = powMod(base, exp / 2, mod, multiplications);   // solve half-size problem once
    long long result = half * half % mod;
    if (multiplications) ++*multiplications;
    if (exp & 1) {
        result = result * base % mod;
        if (multiplications) ++*multiplications;
    }
    return result;
}
