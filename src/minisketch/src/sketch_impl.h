#ifndef _MINISKETCH_SKETCH_IMPL_H_
#define _MINISKETCH_SKETCH_IMPL_H_

#include <random>

#include "util.h"
#include "sketch.h"
#include "int_utils.h"

template<typename F>
void PolyMod(const std::vector<typename F::Elem>& mod, std::vector<typename F::Elem>& val, const F& field) {
    size_t modsize = mod.size();
    CHECK_SAFE(modsize > 0 && mod.back() == 1);
    if (val.size() < modsize) return;
    CHECK_SAFE(val.back() != 0);
    while (val.size() >= modsize) {
        auto term = val.back();
        val.pop_back();
        if (term != 0) {
            typename F::Multiplier mul(field, term);
            for (size_t x = 0; x < mod.size() - 1; ++x) {
                val[val.size() - modsize + 1 + x] ^= mul(mod[x]);
            }
        }
    }
    while (val.size() > 0 && val.back() == 0) val.pop_back();
}

template<typename F>
void DivMod(const std::vector<typename F::Elem>& mod, std::vector<typename F::Elem>& val, std::vector<typename F::Elem>& div, const F& field) {
    size_t modsize = mod.size();
    CHECK_SAFE(mod.size() > 0 && mod.back() == 1);
    if (val.size() < mod.size()) {
        div.clear();
        return;
    }
    CHECK_SAFE(val.back() != 0);
    div.resize(val.size() - mod.size() + 1);
    while (val.size() >= modsize) {
        auto term = val.back();
        div[val.size() - modsize] = term;
        val.pop_back();
        if (term != 0) {
            typename F::Multiplier mul(field, term);
            for (size_t x = 0; x < mod.size() - 1; ++x) {
                val[val.size() - modsize + 1 + x] ^= mul(mod[x]);
            }
        }
    }
}

template<typename F>
typename F::Elem MakeMonic(std::vector<typename F::Elem>& a, const F& field) {
    CHECK_SAFE(a.back() != 0);
    if (a.back() == 1) return 0;
    auto inv = field.Inv(a.back());
    typename F::Multiplier mul(field, inv);
    a.back() = 1;
    for (size_t i = 0; i < a.size() - 1; ++i) {
        a[i] = mul(a[i]);
    }
    return inv;
}

template<typename F>
void GCD(std::vector<typename F::Elem>& a, std::vector<typename F::Elem>& b, const F& field) {
    if (a.size() < b.size()) std::swap(a, b);
    while (b.size() > 0) {
        if (b.size() == 1) {
            a.resize(1);
            a[0] = 1;
            return;
        }
        MakeMonic(b, field);
        PolyMod(b, a, field);
        std::swap(a, b);
    }
}

template<typename F>
void Sqr(std::vector<typename F::Elem>& poly, const F& field) {
    if (poly.size() == 0) return;
    poly.resize(poly.size() * 2 - 1);
    for (size_t i = 0; i < poly.size(); ++i) {
        auto x = poly.size() - i - 1;
        poly[x] = (x & 1) ? 0 : field.Sqr(poly[x / 2]);
    }
}

template<typename F>
void TraceMod(const std::vector<typename F::Elem>& mod, std::vector<typename F::Elem>& out, const typename F::Elem& param, const F& field) {
    out.reserve(mod.size() * 2);
    out.resize(2);
    out[0] = 0;
    out[1] = param;

    for (int i = 0; i < field.Bits() - 1; ++i) {
        Sqr(out, field);
        if (out.size() < 2) out.resize(2);
        out[1] = param;
        PolyMod(mod, out, field);
    }
}

template<typename F>
bool RecFindRoots(std::vector<std::vector<typename F::Elem>>& stack, size_t pos, std::vector<typename F::Elem>& roots, bool fully_factorizable, int depth, typename F::Elem randv, const F& field) {
    auto& ppoly = stack[pos];

    CHECK_SAFE(ppoly.size() > 1 && ppoly.back() == 1);

    if (ppoly.size() == 2) {
        roots.push_back(ppoly[0]);
        return true;
    }

    if (ppoly.size() == 3) {
        CHECK_RETURN(ppoly[1] != 0, false);
        auto input = field.Mul(ppoly[0], field.Sqr(field.Inv(ppoly[1])));
        auto root = field.Qrt(input);
        if ((field.Sqr(root) ^ root) != input) {
            CHECK_SAFE(!fully_factorizable);
            return false;
        }
        auto sol = field.Mul(root, ppoly[1]);
        roots.push_back(sol);
        roots.push_back(sol ^ ppoly[1]);
        return true;
    }

    if (pos + 3 > stack.size()) {

        stack.resize((pos + 3) * 2);
    }
    auto& poly = stack[pos];
    auto& tmp = stack[pos + 1];
    auto& trace = stack[pos + 2];
    trace.clear();
    tmp.clear();
    for (int iter = 0;; ++iter) {

        TraceMod(poly, trace, randv, field);

        if (iter >= 1 && !fully_factorizable) {

            tmp = trace;
            Sqr(tmp, field);
            for (size_t i = 0; i < trace.size(); ++i) {
                tmp[i] ^= trace[i];
            }
            while (tmp.size() && tmp.back() == 0) tmp.pop_back();
            PolyMod(poly, tmp, field);

            if (tmp.size() != 0) return false;
            fully_factorizable = true;
        }

        if (fully_factorizable) {

            CHECK_RETURN(field.Bits() - depth >= std::numeric_limits<decltype(poly.size())>::digits ||
                (poly.size() - 2) >> (field.Bits() - depth) == 0, false);
        }

        depth++;

        randv = field.Mul2(randv);
        tmp = poly;
        GCD(trace, tmp, field);
        if (trace.size() != poly.size() && trace.size() > 1) break;
    }
    MakeMonic(trace, field);
    DivMod(trace, poly, tmp, field);

    std::swap(poly, trace);

    if (!RecFindRoots(stack, pos + 1, roots, fully_factorizable, depth, randv, field)) return false;

    bool ret = RecFindRoots(stack, pos, roots, true, depth, randv, field);

    CHECK_SAFE(ret);
    return ret;
}

template<typename F>
std::vector<typename F::Elem> FindRoots(const std::vector<typename F::Elem>& poly, typename F::Elem basis, const F& field) {
    std::vector<typename F::Elem> roots;
    CHECK_RETURN(poly.size() != 0, {});
    CHECK_RETURN(basis != 0, {});
    if (poly.size() == 1) return roots;
    roots.reserve(poly.size() - 1);
    std::vector<std::vector<typename F::Elem>> stack = {poly};

    if (!RecFindRoots(stack, 0, roots, false, 0, basis, field)) {

        return {};
    }
    CHECK_RETURN(poly.size() - 1 == roots.size(), {});
    return roots;
}

template<typename F>
std::vector<typename F::Elem> BerlekampMassey(const std::vector<typename F::Elem>& syndromes, size_t max_degree, const F& field) {
    std::vector<typename F::Multiplier> table;
    std::vector<typename F::Elem> current, prev, tmp;
    current.reserve(syndromes.size() / 2 + 1);
    prev.reserve(syndromes.size() / 2 + 1);
    tmp.reserve(syndromes.size() / 2 + 1);
    current.resize(1);
    current[0] = 1;
    prev.resize(1);
    prev[0] = 1;
    typename F::Elem b = 1, b_inv = 1;
    bool b_have_inv = true;
    table.reserve(syndromes.size());

    for (size_t n = 0; n != syndromes.size(); ++n) {
        table.emplace_back(field, syndromes[n]);
        auto discrepancy = syndromes[n];
        for (size_t i = 1; i < current.size(); ++i) discrepancy ^= table[n - i](current[i]);
        if (discrepancy != 0) {
            int x = static_cast<int>(n + 1 - (current.size() - 1) - (prev.size() - 1));
            if (!b_have_inv) {
                b_inv = field.Inv(b);
                b_have_inv = true;
            }
            bool swap = 2 * (current.size() - 1) <= n;
            if (swap) {
                if (prev.size() + x - 1 > max_degree) return {};
                tmp = current;
                current.resize(prev.size() + x);
            }
            typename F::Multiplier mul(field, field.Mul(discrepancy, b_inv));
            for (size_t i = 0; i < prev.size(); ++i) current[i + x] ^= mul(prev[i]);
            if (swap) {
                std::swap(prev, tmp);
                b = discrepancy;
                b_have_inv = false;
            }
        }
    }
    CHECK_RETURN(current.size() && current.back() != 0, {});
    return current;
}

template<typename F>
std::vector<typename F::Elem> ReconstructAllSyndromes(const std::vector<typename F::Elem>& odd_syndromes, const F& field) {
    std::vector<typename F::Elem> all_syndromes;
    all_syndromes.resize(odd_syndromes.size() * 2);
    for (size_t i = 0; i < odd_syndromes.size(); ++i) {
        all_syndromes[i * 2] = odd_syndromes[i];
        all_syndromes[i * 2 + 1] = field.Sqr(all_syndromes[i]);
    }
    return all_syndromes;
}

template<typename F>
void AddToOddSyndromes(std::vector<typename F::Elem>& osyndromes, typename F::Elem data, const F& field) {
    auto sqr = field.Sqr(data);
    typename F::Multiplier mul(field, sqr);
    for (auto& osyndrome : osyndromes) {
        osyndrome ^= data;
        data = mul(data);
    }
}

template<typename F>
std::vector<typename F::Elem> FullDecode(const std::vector<typename F::Elem>& osyndromes, const F& field) {
    auto asyndromes = ReconstructAllSyndromes<typename F::Elem>(osyndromes, field);
    auto poly = BerlekampMassey(asyndromes, field);
    std::reverse(poly.begin(), poly.end());
    return FindRoots(poly, field);
}

template<typename F>
class SketchImpl final : public Sketch
{
    const F m_field;
    std::vector<typename F::Elem> m_syndromes;
    typename F::Elem m_basis;

public:
    template<typename... Args>
    SketchImpl(int implementation, int bits, const Args&... args) : Sketch(implementation, bits), m_field(args...) {
        std::random_device rng;
        std::uniform_int_distribution<uint64_t> dist;
        m_basis = m_field.FromSeed(dist(rng));
    }

    size_t Syndromes() const override { return m_syndromes.size(); }
    void Init(size_t count) override { m_syndromes.assign(count, 0); }

    void Add(uint64_t val) override
    {
        auto elem = m_field.FromUint64(val);
        AddToOddSyndromes(m_syndromes, elem, m_field);
    }

    void Serialize(unsigned char* ptr) const override
    {
        BitWriter writer(ptr);
        for (const auto& val : m_syndromes) {
            m_field.Serialize(writer, val);
        }
        writer.Flush();
    }

    void Deserialize(const unsigned char* ptr) override
    {
        BitReader reader(ptr);
        for (auto& val : m_syndromes) {
            val = m_field.Deserialize(reader);
        }
    }

    int Decode(int max_count, uint64_t* out) const override
    {
        auto all_syndromes = ReconstructAllSyndromes(m_syndromes, m_field);
        auto poly = BerlekampMassey(all_syndromes, max_count, m_field);
        if (poly.size() == 0) return -1;
        if (poly.size() == 1) return 0;
        if ((int)poly.size() > 1 + max_count) return -1;
        std::reverse(poly.begin(), poly.end());
        auto roots = FindRoots(poly, m_basis, m_field);
        if (roots.size() == 0) return -1;

        for (const auto& root : roots) {
            *(out++) = m_field.ToUint64(root);
        }
        return static_cast<int>(roots.size());
    }

    size_t Merge(const Sketch* other_sketch) override
    {

        const SketchImpl* other = static_cast<const SketchImpl*>(other_sketch);
        m_syndromes.resize(std::min(m_syndromes.size(), other->m_syndromes.size()));
        for (size_t i = 0; i < m_syndromes.size(); ++i) {
            m_syndromes[i] ^= other->m_syndromes[i];
        }
        return m_syndromes.size();
    }

    void SetSeed(uint64_t seed) override
    {
        if (seed == (uint64_t)-1) {
            m_basis = 1;
        } else {
            m_basis = m_field.FromSeed(seed);
        }
    }
};

#endif
