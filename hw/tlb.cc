/* pagetables -- A framework to experiment with memory management
 *
 * Copyright (C) 2017--2026 Leiden University, The Netherlands.
 */

#include "tlb.h"

#include "mmu.h"

TLB::TLB(const MMU &mmu, const size_t max)
    : mmu(mmu), max(max), stats() {
    storage = new TableEntry[max];
}

TLB::~TLB() { delete[] storage; }

/* Look up the virtual page number @vPage for a physical page number
 * @pPage. Returns whether the lookup was successful.
 */
bool TLB::lookup(const uint64_t vPage, uint64_t &pPage) {
    stats.lookups++;

    for (size_t i = 0; i < population; i++) {
        if (storage[i].valid && storage[i].tag == vPage &&
            storage[i].asid == asid) {
            stats.hits++;
            pPage = storage[i].ppn;
            return true;
        }
    }

    return false;
}

/* Add a translation of @vPage to @pPage to the TLB.
 */
void TLB::add(const uint64_t vPage, const uint64_t pPage) {
    if (population < max) {
        storage[population].tag = vPage;
        storage[population].ppn = pPage;
        storage[population].valid = true;
        storage[population].asid = asid;
        population++;
        stats.population++;
        return;
    }

    for (size_t i = 0; i < population; i++) {
        if (!storage[i].valid) {
            storage[population].tag = vPage;
            storage[population].ppn = pPage;
            storage[population].valid = true;
            storage[population].asid = asid;
            return;
        }
    }

    stats.addEvictions++;
    storage[evict_target].tag = vPage;
    storage[evict_target].ppn = pPage;
    storage[evict_target].valid = true;
    storage[evict_target].asid = asid;
    evict_target = (evict_target + 1) % max;
}

/* Flush all TLB entries.
 */
void TLB::flush(void) {
    stats.flushes++;
    stats.flushEvictions += population;
    population = 0;
    stats.population = 0;
}

/* Set the currently active ASID to @asid.
 */
void TLB::setASID(const uintptr_t _asid) { asid = _asid; }
