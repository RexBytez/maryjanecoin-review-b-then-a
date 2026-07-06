#include <cstdio>
#include <vector>
#include <string>
#include "../storage.h"

static int pass = 0, total = 0;
static void check(const char* name, bool ok) {
    total++; if (ok) pass++;
    printf("%s %s\n", ok ? "\xE2\x9C\x93" : "\xE2\x9C\x97", name);
}

typedef std::vector<unsigned char> Bytes;
typedef std::vector<Bytes> Chunks;

static Bytes mkPayload(size_t n) {
    Bytes b; b.reserve(n);
    for (size_t i = 0; i < n; i++) b.push_back((unsigned char)((i * 37 + 11) & 0xff));
    return b;
}

static bool roundTrip(const Bytes& payload, unsigned char flags) {
    Chunks chunks;
    if (!StorageEncode(payload, flags, chunks)) return false;
    Bytes out; unsigned char outFlags;
    if (!StorageDecode(chunks, out, outFlags)) return false;
    return out == payload && outFlags == (flags & 0x0f);
}

int main() {

    check("envelope == 35 (L5 canonical)", STORAGE_ENVELOPE_LEN == 35);
    check("chunk data == 33", STORAGE_CHUNK_DATA_LEN == 33);
    check("payload/chunk == 24", STORAGE_PAYLOAD_PER_CHUNK == 24);

    check("round-trip empty",        roundTrip(mkPayload(0), 0));
    check("round-trip 1 byte",       roundTrip(mkPayload(1), 0));
    check("round-trip 16 (1 chunk)", roundTrip(mkPayload(16), 0));
    check("round-trip 17 (2 chunks)",roundTrip(mkPayload(17), 0));
    check("round-trip 24",           roundTrip(mkPayload(24), 0));
    check("round-trip 100",          roundTrip(mkPayload(100), 0));
    check("round-trip 5000 (large)", roundTrip(mkPayload(5000), 0));

    check("flags COMPRESSED survive", roundTrip(mkPayload(50), STORAGE_FLAG_COMPRESSED));
    check("flags ENCRYPTED survive",  roundTrip(mkPayload(50), STORAGE_FLAG_ENCRYPTED));
    check("flags both survive",       roundTrip(mkPayload(50), STORAGE_FLAG_COMPRESSED | STORAGE_FLAG_ENCRYPTED));

    {
        Bytes p = mkPayload(300);
        Chunks a, b;
        StorageEncode(p, 0, a); StorageEncode(p, 0, b);
        check("encode deterministic", a == b);
        check("streamId deterministic", StorageDeriveStreamId(p) == StorageDeriveStreamId(p));
        check("distinct payloads → distinct streamId", StorageDeriveStreamId(mkPayload(10)) != StorageDeriveStreamId(mkPayload(11)));
    }

    {
        Bytes p = mkPayload(200);
        Chunks chunks; StorageEncode(p, 0, chunks);
        bool envOk = true, unwrapOk = true;
        for (size_t i = 0; i < chunks.size(); i++) {
            Bytes spk = StorageChunkToScript(chunks[i]);
            if (spk.size() != STORAGE_ENVELOPE_LEN || spk[0] != 0x6a || spk[1] != 0x21) envOk = false;
            Bytes body;
            if (!StorageScriptToChunk(spk, body) || body != chunks[i]) unwrapOk = false;
        }
        check("all chunks → 35B OP_RETURN envelope", envOk);
        check("script unwrap restores chunk body", unwrapOk);
    }

    {
        Bytes p = mkPayload(500);
        Chunks chunks; StorageEncode(p, 0, chunks);
        Chunks shuffled;
        for (size_t i = 0; i < chunks.size(); i += 2) shuffled.push_back(chunks[i]);
        for (size_t i = 1; i < chunks.size(); i += 2) shuffled.push_back(chunks[i]);
        Bytes out; unsigned char f;
        check("out-of-order reassembly", StorageDecode(shuffled, out, f) && out == p);
    }

    {
        Bytes p = mkPayload(120);
        Chunks chunks; StorageEncode(p, 0, chunks);
        chunks[1][STORAGE_HEADER_LEN] ^= 0xFF;
        Bytes out; unsigned char f;
        check("tamper detected (CRC)", StorageDecode(chunks, out, f) == false);
    }

    {
        Bytes p = mkPayload(300);
        Chunks chunks; StorageEncode(p, 0, chunks);
        chunks.pop_back();
        Bytes out; unsigned char f;
        check("missing chunk detected", StorageDecode(chunks, out, f) == false);
    }

    {
        Bytes p = mkPayload(300);
        Chunks chunks; StorageEncode(p, 0, chunks);
        chunks.push_back(chunks[0]);
        Bytes out; unsigned char f;
        check("duplicate chunk detected", StorageDecode(chunks, out, f) == false);
    }

    {
        Chunks a, b; StorageEncode(mkPayload(60), 0, a); StorageEncode(mkPayload(61), 0, b);
        Chunks mixed = a; if (!b.empty()) mixed.push_back(b[0]);
        Bytes out; unsigned char f;
        check("cross-stream contamination detected", StorageDecode(mixed, out, f) == false);
    }

    {
        Bytes fakeBody(STORAGE_CHUNK_DATA_LEN, 0x00);
        fakeBody[0] = 0x00;
        unsigned char f; uint32_t sid; uint16_t idx, tot;
        check("foreign chunk rejected by header parse", StorageParseHeader(fakeBody, f, sid, idx, tot) == false);
    }

    int score = (int)((double)pass / total * 100 + 0.5);
    printf("\nSCORE: %d/100  (%d/%d checks)\n", score, pass, total);
    return score == 100 ? 0 : 1;
}
