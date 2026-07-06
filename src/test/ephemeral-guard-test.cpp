#include <cstdio>
#include <vector>
#include "../ephemeral-guard.h"

static int pass = 0, total = 0;
static void check(const char* name, bool ok) {
    total++; if (ok) pass++;
    printf("%s %s\n", ok ? "\xE2\x9C\x93" : "\xE2\x9C\x97", name);
}

static std::vector<unsigned char> mkKey(uint64_t seed) {
    std::vector<unsigned char> k(33);
    uint64_t x = seed * 0x9E3779B97F4A7C15ULL + 0xABCDEF;
    for (int i = 0; i < 33; i++) {
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL; x ^= x >> 27;
        k[i] = (unsigned char)(x >> (8 * (i % 8)));
    }
    return k;
}

int main() {
    EphemeralKeyGuard g(100000, 1e-6);

    check("filter has bits", g.bits() >= 64);
    check("filter has >=1 hash", g.hashes() >= 1 && g.hashes() <= 32);

    std::vector<unsigned char> r0 = mkKey(1);
    check("fresh key not reuse", g.WouldReuse(r0) == false);

    g.Record(r0);
    check("recorded key flagged reuse", g.WouldReuse(r0) == true);
    check("recorded key still flagged (idempotent)", g.WouldReuse(r0) == true);

    const int N = 50000;
    for (int i = 0; i < N; i++) { std::vector<unsigned char> k = mkKey(1000 + i); g.Record(k); }
    bool noFalseNeg = true;
    for (int i = 0; i < N; i++) {
        std::vector<unsigned char> k = mkKey(1000 + i);
        if (!g.WouldReuse(k)) { noFalseNeg = false; break; }
    }
    check("NO false negatives over 50k recorded keys", noFalseNeg);

    int fp = 0; const int M = 50000;
    for (int i = 0; i < M; i++) {
        std::vector<unsigned char> k = mkKey(5000000 + i);
        if (g.WouldReuse(k)) fp++;
    }
    double fpRate = (double)fp / M;
    check("FP rate < 1% (sized for 1e-6 @100k, 50k load)", fpRate < 0.01);

    std::vector<unsigned char> rd = mkKey(424242);
    bool a = g.WouldReuse(rd); bool b = g.WouldReuse(rd);
    check("verdict deterministic", a == b);

    EphemeralKeyGuard g2(1000, 1e-6);
    std::vector<unsigned char> x = mkKey(7), y = mkKey(8);
    g2.Record(x);
    check("recording X does not flag unrelated Y", g2.WouldReuse(x) && !g2.WouldReuse(y));

    EphemeralKeyGuard g3(0, 0.0);
    check("degenerate ctor clamps", g3.bits() >= 64 && g3.hashes() >= 1);

    int score = (int)((double)pass / total * 100 + 0.5);
    printf("\nSCORE: %d/100  (%d/%d checks)\n", score, pass, total);
    return score == 100 ? 0 : 1;
}
