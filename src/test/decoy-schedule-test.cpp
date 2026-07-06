#include <cstdio>
#include "../decoy.h"

static int pass = 0, total = 0;
static void check(const char* name, bool ok) {
    total++; if (ok) pass++;
    printf("%s %s\n", ok ? "\xE2\x9C\x93" : "\xE2\x9C\x97", name);
}

int main() {

    check("clamp(0)=default 3", DecoyClampLevel(0) == 3);
    check("clamp(6)=default 3", DecoyClampLevel(6) == 3);
    check("clamp(1)=1", DecoyClampLevel(1) == 1);
    check("clamp(5)=5", DecoyClampLevel(5) == 5);

    uint256 h(123456789ULL);
    bool d1 = DecoyShouldInject(h, 42, 3), d2 = DecoyShouldInject(h, 42, 3);
    check("decision deterministic", d1 == d2);
    check("beacon deterministic", DecoyBeacon(h, 42) == DecoyBeacon(h, 42));

    check("salt changes beacon", DecoyBeacon(h, 1) != DecoyBeacon(h, 2));

    int countL1 = 0, countL3 = 0, countL5 = 0;
    const int N = 200000;
    for (int i = 0; i < N; i++) {
        uint256 bh((uint64_t)i * 2654435761ULL + 7);
        if (DecoyShouldInject(bh, 99, 1)) countL1++;
        if (DecoyShouldInject(bh, 99, 3)) countL3++;
        if (DecoyShouldInject(bh, 99, 5)) countL5++;
    }
    check("L1 < L3 < L5 frequency", countL1 < countL3 && countL3 < countL5);

    double rateL3 = (double)countL3 / N * DECOY_BP_SCALE;
    check("L3 rate ~ 120bp", rateL3 > 120 * 0.75 && rateL3 < 120 * 1.25);
    double rateL5 = (double)countL5 / N * DECOY_BP_SCALE;
    check("L5 rate ~ 700bp", rateL5 > 700 * 0.75 && rateL5 < 700 * 1.25);

    bool burstOk = true;
    for (int i = 0; i < 50000; i++) {
        uint256 bh((uint64_t)i * 40503ULL + 11);
        int b3 = DecoyBurstCount(bh, 5, 3);
        int b5 = DecoyBurstCount(bh, 5, 5);
        bool inj3 = DecoyShouldInject(bh, 5, 3);
        if (inj3 && (b3 < 1 || b3 > 2)) burstOk = false;
        if (!inj3 && b3 != 0) burstOk = false;
        if (DecoyShouldInject(bh, 5, 5) && (b5 < 1 || b5 > 3)) burstOk = false;
    }
    check("burst count in valid range", burstOk);

    check("L1 recycle window > L5", DECOY_RECYCLE_BASE_BLOCKS / 1 > DECOY_RECYCLE_BASE_BLOCKS / 5);
    check("young decoy not recycled (L3)", DecoyShouldRecycle(100, 3) == false);
    check("old decoy recycled (L3)", DecoyShouldRecycle(DECOY_RECYCLE_BASE_BLOCKS, 3) == true);
    check("L5 recycles at age where L1 does not",
          DecoyShouldRecycle(300, 5) == true && DecoyShouldRecycle(300, 1) == false);

    int score = (int)((double)pass / total * 100 + 0.5);
    printf("\nSCORE: %d/100  (%d/%d checks)\n", score, pass, total);
    return score == 100 ? 0 : 1;
}
