/* pagetables -- A framework to experiment with memory management
 *
 * Copyright (C) 2017--2026 Leiden University, The Netherlands.
 */

#ifndef __TLB_H__
#define __TLB_H__

#include <stddef.h>

#include <cstdint>

/* Structure representing statistics for the TLB. */
struct TLBStatistics {
    int lookups; /* Amount of lookups */
    int hits;    /* Amount of hits */

    int population;
    int addEvictions; /* Amount of evictions due to add */

    int flushes;        /* Amount of flushes */
    int flushEvictions; /* Amount of evictions due to flush */
};

class MMU;

/*
 * Translation lookaside buffer.
 */

class TLB {
   protected:
    /* Reference to MMU; to be filled by initializer list in
     * constructor */
    const MMU &mmu;

    /* Maximum number of entries in TLB */
    const size_t max;

    struct __attribute__((__packed__)) TableEntry {
        uint64_t tag;
        uint64_t ppn;
        uintptr_t asid;
        uint8_t valid : 1;
    };

    TableEntry *storage = nullptr;

    uintptr_t asid = 0;
    size_t population = 0;
    size_t evict_target = 0;

   public:
    TLB(const MMU &mmu, const size_t max);
    ~TLB();

    TLBStatistics stats = {0};

    bool lookup(const uint64_t vPage, uint64_t &pPage);
    void add(const uint64_t vPage, const uint64_t pPage);
    void flush(void);

    void setASID(const uintptr_t asid);

    TLB(const TLB &) = delete;
    TLB &operator=(const TLB &) = delete;
};

#endif /* __TLB_H__ */
