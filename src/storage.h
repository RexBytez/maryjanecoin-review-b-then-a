#ifndef MARYJANECOIN_STORAGE_H
#define MARYJANECOIN_STORAGE_H

#include <stdint.h>
#include <vector>
#include <cstddef>

static const unsigned int STORAGE_ENVELOPE_LEN   = 35;
static const unsigned char STORAGE_OP_RETURN     = 0x6a;
static const unsigned char STORAGE_PUSH33        = 0x21;
static const unsigned int STORAGE_CHUNK_DATA_LEN = 33;

static const unsigned int STORAGE_HEADER_LEN      = 9;
static const unsigned int STORAGE_PAYLOAD_PER_CHUNK = STORAGE_CHUNK_DATA_LEN - STORAGE_HEADER_LEN;
static const unsigned char STORAGE_VERSION        = 1;
static const unsigned int STORAGE_MAX_CHUNKS      = 65535;

static const size_t STORAGE_MAX_PAYLOAD = (size_t)STORAGE_MAX_CHUNKS * STORAGE_PAYLOAD_PER_CHUNK - 8;

static const unsigned char STORAGE_FLAG_COMPRESSED = 0x01;
static const unsigned char STORAGE_FLAG_ENCRYPTED  = 0x02;

inline uint32_t StorageCrc32(const unsigned char* p, size_t n)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        crc ^= p[i];
        for (int k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88720u & (~(crc & 1) + 1));
    }
    return crc ^ 0xFFFFFFFFu;
}
inline uint32_t StorageCrc32(const std::vector<unsigned char>& v)
{
    return StorageCrc32(v.empty() ? (const unsigned char*)"" : &v[0], v.size());
}

inline uint32_t StorageDeriveStreamId(const std::vector<unsigned char>& payload)
{
    uint32_t crc = StorageCrc32(payload);
    return crc ^ (uint32_t)(payload.size() * 2654435761u);
}

inline void StoragePutLE16(std::vector<unsigned char>& o, uint16_t v) { o.push_back(v & 0xff); o.push_back((v >> 8) & 0xff); }
inline void StoragePutLE32(std::vector<unsigned char>& o, uint32_t v) { for (int i = 0; i < 4; i++) o.push_back((v >> (8 * i)) & 0xff); }
inline uint16_t StorageGetLE16(const unsigned char* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
inline uint32_t StorageGetLE32(const unsigned char* p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

inline bool StorageEncode(const std::vector<unsigned char>& payload,
                          unsigned char flags,
                          uint32_t streamId,
                          std::vector<std::vector<unsigned char> >& outChunks)
{
    outChunks.clear();
    if (payload.size() > STORAGE_MAX_PAYLOAD)
        return false;

    std::vector<unsigned char> stream;
    StoragePutLE32(stream, (uint32_t)payload.size());
    stream.insert(stream.end(), payload.begin(), payload.end());
    uint32_t crc = StorageCrc32(stream);
    StoragePutLE32(stream, crc);

    size_t total = (stream.size() + STORAGE_PAYLOAD_PER_CHUNK - 1) / STORAGE_PAYLOAD_PER_CHUNK;
    if (total == 0) total = 1;
    if (total > STORAGE_MAX_CHUNKS)
        return false;

    unsigned char vf = (unsigned char)((STORAGE_VERSION << 4) | (flags & 0x0f));
    for (size_t idx = 0; idx < total; idx++) {
        std::vector<unsigned char> body;
        body.reserve(STORAGE_CHUNK_DATA_LEN);
        body.push_back(vf);
        StoragePutLE32(body, streamId);
        StoragePutLE16(body, (uint16_t)idx);
        StoragePutLE16(body, (uint16_t)total);
        size_t off = idx * STORAGE_PAYLOAD_PER_CHUNK;
        for (unsigned int b = 0; b < STORAGE_PAYLOAD_PER_CHUNK; b++)
            body.push_back(off + b < stream.size() ? stream[off + b] : 0x00);
        outChunks.push_back(body);
    }
    return true;
}

inline bool StorageEncode(const std::vector<unsigned char>& payload,
                          unsigned char flags,
                          std::vector<std::vector<unsigned char> >& outChunks)
{
    return StorageEncode(payload, flags, StorageDeriveStreamId(payload), outChunks);
}

inline std::vector<unsigned char> StorageChunkToScript(const std::vector<unsigned char>& body)
{
    std::vector<unsigned char> spk;
    spk.reserve(STORAGE_ENVELOPE_LEN);
    spk.push_back(STORAGE_OP_RETURN);
    spk.push_back(STORAGE_PUSH33);

    for (unsigned int i = 0; i < STORAGE_CHUNK_DATA_LEN; i++)
        spk.push_back(i < body.size() ? body[i] : 0x00);
    return spk;
}

inline bool StorageScriptToChunk(const std::vector<unsigned char>& spk,
                                 std::vector<unsigned char>& outBody)
{
    if (spk.size() != STORAGE_ENVELOPE_LEN) return false;
    if (spk[0] != STORAGE_OP_RETURN || spk[1] != STORAGE_PUSH33) return false;
    outBody.assign(spk.begin() + 2, spk.end());
    return outBody.size() == STORAGE_CHUNK_DATA_LEN;
}

inline bool StorageParseHeader(const std::vector<unsigned char>& body,
                               unsigned char& outFlags, uint32_t& outStreamId,
                               uint16_t& outIndex, uint16_t& outTotal)
{
    if (body.size() != STORAGE_CHUNK_DATA_LEN) return false;
    if ((body[0] >> 4) != STORAGE_VERSION) return false;
    outFlags    = body[0] & 0x0f;
    outStreamId = StorageGetLE32(&body[1]);
    outIndex    = StorageGetLE16(&body[5]);
    outTotal    = StorageGetLE16(&body[7]);
    return outTotal >= 1 && outIndex < outTotal;
}

inline bool StorageDecode(const std::vector<std::vector<unsigned char> >& chunks,
                          std::vector<unsigned char>& outPayload,
                          unsigned char& outFlags)
{
    outPayload.clear();
    if (chunks.empty()) return false;

    unsigned char flags0; uint32_t sid0; uint16_t idx0, total0;
    if (!StorageParseHeader(chunks[0], flags0, sid0, idx0, total0)) return false;

    std::vector<bool> seen(total0, false);
    std::vector<std::vector<unsigned char> > ordered(total0);
    for (size_t i = 0; i < chunks.size(); i++) {
        unsigned char f; uint32_t sid; uint16_t idx, total;
        if (!StorageParseHeader(chunks[i], f, sid, idx, total)) return false;
        if (sid != sid0 || total != total0 || f != flags0) return false;
        if (idx >= total0 || seen[idx]) return false;
        seen[idx] = true;
        ordered[idx].assign(chunks[i].begin() + STORAGE_HEADER_LEN, chunks[i].end());
    }
    for (uint16_t i = 0; i < total0; i++)
        if (!seen[i]) return false;

    std::vector<unsigned char> stream;
    stream.reserve((size_t)total0 * STORAGE_PAYLOAD_PER_CHUNK);
    for (uint16_t i = 0; i < total0; i++)
        stream.insert(stream.end(), ordered[i].begin(), ordered[i].end());

    if (stream.size() < 8) return false;
    uint32_t declaredLen = StorageGetLE32(&stream[0]);
    size_t framedLen = (size_t)4 + declaredLen + 4;
    if (framedLen > stream.size()) return false;
    uint32_t wantCrc = StorageGetLE32(&stream[4 + declaredLen]);
    uint32_t gotCrc  = StorageCrc32(&stream[0], 4 + declaredLen);
    if (wantCrc != gotCrc) return false;

    outPayload.assign(stream.begin() + 4, stream.begin() + 4 + declaredLen);
    outFlags = flags0;
    return true;
}

#endif
