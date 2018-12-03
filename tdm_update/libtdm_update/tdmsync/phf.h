#ifndef _TDM_PHF_H_816840_
#define _TDM_PHF_H_816840_

#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <vector>
#include <random>

//tdmsync perfect hash function library
namespace TdmPhf {

typedef std::mt19937 RndGen;

std::string assertFailedMessage(const char *code, const char *file, int line) {
    char buff[256];
    sprintf(buff, "Assertion %s failed in %s on line %d", code, file, line);
    return buff;
}
#define TdmPhfAssert(cond) if (!(cond)) throw std::runtime_error(assertFailedMessage(#cond, __FILE__, __LINE__));

//almost-universal hash function for integers
//https://en.wikipedia.org/wiki/Universal_hashing#Avoiding_modular_arithmetic
struct IntegerUhf {
    size_t mult = 0;
    size_t shift = 0;

    void create(RndGen &rnd, size_t logSize) {
        mult = std::uniform_int_distribution<size_t>(0, SIZE_MAX)(rnd);
        logSize = std::max(logSize, size_t(1));
        shift = 8 * sizeof(size_t) - logSize;
    }
    inline size_t evaluate (size_t key) const {
        return (mult * key) >> shift;
    }
};


//perfect hash function, graph-based
//http://cmph.sourceforge.net/papers/chm92.pdf
struct PerfectHashFunc {
    typedef uint32_t Key;
    typedef IntegerUhf HashFunc;

    size_t logSize, mask;
    HashFunc funcs[2];
    std::vector<uint32_t> data;

    inline uint32_t evaluate(Key key) const {
        size_t a = funcs[0].evaluate(key);
        size_t b = funcs[1].evaluate(key);
        size_t res = data[a] ^ data[b];
        return res;
    }

    void create(const uint32_t *keys, size_t num) {
        //choose size of auxilliary arrays: power-of-two, at least max(3*n, 32)
        logSize = 5;
        while ((1ULL << logSize) < 3 * num)
            logSize++;
        size_t cells = 1ULL << logSize;
        mask = cells - 1;
        //fprintf(stderr, "%d / %d\n", (int)num, (int)cells);

        RndGen rnd;
        bool ok;
        do {
            funcs[0].create(rnd, logSize);
            funcs[1].create(rnd, logSize);

            struct Edge {
                size_t end;
                size_t value;
            };
            std::vector<std::vector<Edge>> edgeLists(cells);

            for (size_t i = 0; i < num; i++) {
                //detect and avoid duplicates (note: keys must be sorted)
                if (i && keys[i] == keys[i-1])
                    continue;
                uint32_t key = keys[i];
                size_t a = funcs[0].evaluate(key);
                size_t b = funcs[1].evaluate(key);
                edgeLists[a].push_back(Edge{b, i});
                edgeLists[b].push_back(Edge{a, i});
            }
            
            data.assign(cells, 0);
            ok = true;

            std::vector<char> visited(cells, false);
            //series of BFS over graph
            for (size_t s = 0; ok && s < cells; s++) if (!visited[s]) {
                data[s] = 0;
                visited[s] = true;
                std::vector<size_t> qarr;
                qarr.push_back(s);

                for (size_t i = 0; ok && i < qarr.size(); i++) {
                    size_t u = qarr[i];
                    for (const Edge &e : edgeLists[u]) {
                        size_t v = e.end;
                        if (!visited[v]) {
                            data[v] = e.value ^ data[u];
                            visited[v] = true;
                            qarr.push_back(v);
                        }
                        if ((data[u] ^ data[v]) != e.value)
                            ok = false;
                    }
                }
            }

            if (ok) {
                for (size_t i = 0; i < num; i++) {
                    if (i && keys[i] == keys[i-1])
                        continue;
                    TdmPhfAssert(evaluate(keys[i]) == i);
                }
            }

        } while (!ok);          //note: expected O(1) iterations
    }
};

}

#endif
