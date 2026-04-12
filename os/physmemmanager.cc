/* pagetables -- A framework to experiment with memory management
 *
 * Copyright (C) 2017--2026 Leiden University, The Netherlands.
 */

#include "physmemmanager.h"

#include <sys/mman.h>

#include <cstring>
#include <iostream>

PhysMemManager::PhysMemManager(const uint64_t pageSize,
                               const uint64_t memorySize)
    : baseAddress(nullptr),
      pageSize(pageSize),
      memorySize(memorySize),
      nPages(memorySize / pageSize),
      nAllocatedPages(0),
      maxAllocatedPages(0) {
    /* Circuit breaker: avoid too large memory allocations to avoid
     * the user of this program getting into trouble. We limit at 2
     * GiB.
     */
    if (memorySize > 2ULL * 1024 * 1024 * 1024)
        throw std::runtime_error("can't allocate more than 2 GiB.");

    /* Try to allocate "physical" memory. */
    baseAddress =
        mmap((void *)physMemBase, memorySize, PROT_READ | PROT_WRITE,
             MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (baseAddress == MAP_FAILED) {
        throw std::runtime_error("mmap for physical memory failed: " +
                                 std::string(strerror(errno)));
    }

    std::cerr << "BOOT: system memory @ " << std::hex << std::showbase
              << baseAddress << std::endl
              << "      page size of " << pageSize << " bytes"
              << std::endl
              << std::dec << "      " << (memorySize / pageSize)
              << " pages available." << std::endl;
}

PhysMemManager::~PhysMemManager() { munmap(baseAddress, memorySize); }

/* Returns whether all allocated pages were released. */
bool PhysMemManager::allReleased(void) const {
    return nAllocatedPages == 0;
}

/* Returns the largest amount of pages allocated at one time. */
uint64_t PhysMemManager::getMaxAllocatedPages(void) const {
    return maxAllocatedPages;
}

/*
 * Physical memory manager, bitmap version.
 */

PhysMemManagerBitmap::PhysMemManagerBitmap(const uint64_t pageSize,
                                           const uint64_t memorySize)
    : PhysMemManager(pageSize, memorySize),
      allocated(nPages, false) {}

/* Allocate @count pages, returning the starting address in @addr.
 * Returns whether the allocation succeeded.
 */
bool PhysMemManagerBitmap::allocatePages(size_t count,
                                         uintptr_t &addr) {
    /* Check upfront if pages are available at all. */
    if (nAllocatedPages + count > nPages) return false;

    size_t index = 0;
    for (; index < nPages; index++) {
        if (allocated[index] == true) continue;

        /* Test if a sequence of "count" free pages is available. */
        size_t start = index;
        for (; index < start + count && index < nPages; index++) {
            if (allocated[index] == true) break;
        }

        /* Only if we managed to move index "count" pages, the test
         * succeeded. */
        if (index != start + count) continue;

        /* Mark pages as allocated. */
        for (size_t i = start; i < index; i++) {
            allocated[i] = true;
        }

        addr = (uintptr_t)baseAddress + start * pageSize;
        nAllocatedPages += count;
        maxAllocatedPages =
            std::max(maxAllocatedPages, nAllocatedPages);
        return true;
    }

    return false;
}

/* Release (deallocate) the @count physical pages starting at address
 * @addr.
 */
void PhysMemManagerBitmap::releasePages(uintptr_t addr,
                                        size_t count) {
    /* Translate address back to index. */
    uint64_t index = (addr - (uintptr_t)baseAddress) / pageSize;

    for (size_t i = index; i < index + count; i++) {
        allocated[i] = false;
    }

    nAllocatedPages -= count;
}

/*
 * Physical memory manager, hole list version.
 */

PhysMemManagerHole::PhysMemManagerHole(const uint64_t pageSize,
                                       const uint64_t memorySize)
    : PhysMemManager(pageSize, memorySize) {
    root = new HoleNode(0, nPages);
}

PhysMemManagerHole::~PhysMemManagerHole() {
    HoleNode *p = root;
    do {
        HoleNode *n = p->next;
        delete p;
        p = n;
    } while (p);
}

/* Allocate @count pages, returning the starting address in @addr.
 * Returns whether the allocation succeeded.
 */
bool PhysMemManagerHole::allocatePages(size_t count,
                                       uintptr_t &addr) {
    /* Check upfront if pages are available at all. */
    if (nAllocatedPages + count > nPages) return false;

    HoleNode *n = root;
    while ((n->allocated || count > n->count) && (n = n->next));

    if (!n) {
        return false;
    }

    // if n->count is equal to count we can just use the whole hole,
    // otherwise split the current hole into two.
    if (count < n->count) {
        HoleNode *m = new HoleNode(n->page + count, n->count - count);

        if (n->next) {
            m->next = n->next;
            n->next->prev = m;
        }
        n->next = m;
        m->prev = n;
    }

    n->allocated = true;
    n->count = count;
    addr = (uintptr_t)baseAddress + n->page * pageSize;
    nAllocatedPages += count;
    maxAllocatedPages = std::max(maxAllocatedPages, nAllocatedPages);
    return true;
}

/* Release (deallocate) the @count physical pages starting at address
 * @addr.
 */
void PhysMemManagerHole::releasePages(uintptr_t addr, size_t count) {
    // There are some problems here, what if the addr is not aligned
    // to the hole in the hole list, or what if the count exceeds the
    // hole? Just going to ignore these problems for now.

    uint64_t page = (addr - (uintptr_t)baseAddress) / pageSize;

    HoleNode *n = root;
    while (n->page != page && (n = n->next));

    if (!n || !n->allocated) {
        return;
    }

    n->allocated = false;
    if (n->prev && !n->prev->allocated) {
        n->prev->count += n->count;
        n->prev->next = n->next;
        if (n->next) {
            n->next->prev = n->prev;
        }
        delete n;
    } else if (n->next && !n->next->allocated) {
        HoleNode *m = n->next;
        n->count += m->count;
        if (m->next) {
            n->next = m->next;
            m->next->prev = n;
        }
        delete m;
    }

    nAllocatedPages -= count;
}
