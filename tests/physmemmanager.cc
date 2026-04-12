/* pagetables -- A framework to experiment with memory management
 *
 * Copyright (C) 2017--2026 Leiden University, The Netherlands.
 */

#define BOOST_TEST_MODULE PhysMemManager
#include "os/physmemmanager.h"

#include <boost/test/unit_test.hpp>
#include <cstdint>

BOOST_AUTO_TEST_SUITE(physmemmanager_test)
struct PhysMemFixture {
    const uint64_t pageSize = 4096;
    const uint64_t memSize = 1024 * 1024;
    const uint64_t nPages = 1024 * 1024 / 4096;
    PhysMemManagerHole *manager = nullptr;

    PhysMemFixture() {
        manager = new PhysMemManagerHole(pageSize, memSize);
    }

    ~PhysMemFixture() { delete manager; }

    PhysMemFixture(const PhysMemFixture &) = delete;
    PhysMemFixture &operator=(const PhysMemFixture &) = delete;
};

BOOST_FIXTURE_TEST_CASE(test_initial_state, PhysMemFixture) {
    BOOST_CHECK_EQUAL(manager->allReleased(), true);
    BOOST_CHECK_EQUAL(manager->getMaxAllocatedPages(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_basic_allocation, PhysMemFixture) {
    uintptr_t addr1, addr2;

    // allocate 10 pages
    BOOST_CHECK_EQUAL(manager->allocatePages(10, addr1), true);
    BOOST_CHECK_EQUAL(manager->allReleased(), false);

    // allocate another 5 pages
    BOOST_CHECK_EQUAL(manager->allocatePages(5, addr2), true);

    // check that addresses are contiguous (addr2 should be 10 pages
    // after addr1)
    BOOST_CHECK_EQUAL(addr2, addr1 + (10 * pageSize));
}

BOOST_FIXTURE_TEST_CASE(test_out_of_memory, PhysMemFixture) {
    uintptr_t addr;
    size_t totalPages = memSize / pageSize;

    // try to allocate more than available
    BOOST_CHECK_EQUAL(manager->allocatePages(totalPages + 1, addr),
                      false);

    // fill the memory
    BOOST_CHECK_EQUAL(manager->allocatePages(totalPages, addr), true);

    // should be empty now
    uintptr_t addr1;
    BOOST_CHECK_EQUAL(manager->allocatePages(1, addr1), false);
}

BOOST_FIXTURE_TEST_CASE(test_release_and_merge, PhysMemFixture) {
    uintptr_t addr1, addr2, addr3;

    BOOST_CHECK_EQUAL(manager->allocatePages(10, addr1), true);
    BOOST_CHECK_EQUAL(manager->allocatePages(10, addr2), true);
    BOOST_CHECK_EQUAL(manager->allocatePages(10, addr3), true);

    // release middle block
    manager->releasePages(addr2, 10);

    // release first block -> this should merge a and b into
    // one hole of 20
    manager->releasePages(addr1, 10);

    // if merge worked, we should be able to allocate a
    // contiguous 20-page block at the original addr1.
    uintptr_t addr;
    BOOST_CHECK_EQUAL(manager->allocatePages(20, addr), true);
    BOOST_CHECK_EQUAL(addr, addr1);

    // cleanup
    manager->releasePages(addr, 20);
    manager->releasePages(addr3, 10);
    BOOST_CHECK_EQUAL(manager->allReleased(), true);
}

BOOST_FIXTURE_TEST_CASE(test_fragmentation_fill, PhysMemFixture) {
    uintptr_t addrs[nPages];

    // allocate 10 blocks of 1 page
    for (uint64_t i = 0; i < nPages; ++i) {
        BOOST_CHECK_EQUAL(manager->allocatePages(1, addrs[i]), true);
    }

    // release every other block
    for (uint64_t i = 0; i < nPages; i += 2) {
        manager->releasePages(addrs[i], 1);
    }

    uintptr_t addr;
    // none are continuously available should fail.
    BOOST_CHECK_EQUAL(manager->allocatePages(2, addr), false);

    // free the remaining blocks
    for (uint64_t i = 1; i < nPages; i += 2) {
        manager->releasePages(addrs[i], 1);
    }

    BOOST_CHECK_EQUAL(manager->allReleased(), true);
}

BOOST_AUTO_TEST_SUITE_END()
