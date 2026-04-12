/* pagetables -- A framework to experiment with memory management
 *
 * Copyright (C) 2017--2026 Leiden University, The Netherlands.
 */

#define BOOST_TEST_MODULE PhysMemManager
#include "include/tlb.h"

#include <boost/test/unit_test.hpp>

#include "arch/include/riscv.h"
#include "settings.h"
using namespace RISCV;

const int defaultSizeTlb = 16;

BOOST_AUTO_TEST_SUITE(tlb_test)

/*
 * Test MMU class separately
 */

struct TLBFixture {
    RISCV::MMU mmu;
    TLB *tlb = nullptr;
    size_t max = defaultSizeTlb;
    TLBFixture() : mmu() { tlb = new TLB(mmu, max); }

    ~TLBFixture() { delete tlb; }

    TLB *getTLB(size_t _max) {
        if (_max == max) {
            return tlb;
        }
        delete tlb;
        tlb = new TLB(mmu, max);
        return tlb;
    }

    size_t getMax(void) { return max; }

    TLBFixture(const TLBFixture &) = delete;
    TLBFixture &operator=(const TLBFixture &) = delete;
};

BOOST_FIXTURE_TEST_CASE(test_stats, TLBFixture) {
    uint64_t pPage = 0;

    BOOST_CHECK_EQUAL(tlb->stats.lookups, 0);
    BOOST_CHECK_EQUAL(tlb->stats.hits, 0);
    BOOST_CHECK_EQUAL(tlb->stats.population, 0);
    BOOST_CHECK_EQUAL(tlb->stats.addEvictions, 0);
    BOOST_CHECK_EQUAL(tlb->stats.flushes, 0);
    BOOST_CHECK_EQUAL(tlb->stats.flushEvictions, 0);

    tlb->add(0x0, 0xF00UL);
    BOOST_CHECK_EQUAL(tlb->stats.population, 1);

    // populate the whole table, since there is already a entry in the
    // table this should lead to a single add eviction.
    for (size_t i = 0; i < getMax(); i++) {
        tlb->add(0x0, 0xF00UL);
    }
    BOOST_CHECK_EQUAL(tlb->stats.population, getMax());
    BOOST_CHECK_EQUAL(tlb->stats.addEvictions, 1);

    BOOST_CHECK_EQUAL(tlb->lookup(0x0, pPage), true);
    BOOST_CHECK_EQUAL(tlb->stats.lookups, 1);
    BOOST_CHECK_EQUAL(tlb->stats.hits, 1);

    BOOST_CHECK_EQUAL(tlb->lookup(0x1, pPage), false);
    BOOST_CHECK_EQUAL(tlb->stats.lookups, 2);
    BOOST_CHECK_EQUAL(tlb->stats.hits, 1);

    // table is fully populated so the amount of flushevictions should
    // be equal to the size of the table.
    tlb->flush();
    BOOST_CHECK_EQUAL(tlb->stats.flushes, 1);
    BOOST_CHECK_EQUAL(tlb->stats.flushEvictions, getMax());
    // the following statistics should stay the same after a flush,
    // just a quick sanity check.
    BOOST_CHECK_EQUAL(tlb->stats.addEvictions, 1);
    BOOST_CHECK_EQUAL(tlb->stats.lookups, 2);
    BOOST_CHECK_EQUAL(tlb->stats.hits, 1);

    // table should now be empty so no flushevictions occur.
    tlb->flush();
    BOOST_CHECK_EQUAL(tlb->stats.flushes, 2);
    BOOST_CHECK_EQUAL(tlb->stats.flushEvictions, getMax());
}

BOOST_FIXTURE_TEST_CASE(test_tlb, TLBFixture) {
    uint64_t pPage = 0;

    tlb->add(0x0, 0xF00UL);

    BOOST_CHECK_EQUAL(tlb->lookup(0x0, pPage), true);
    BOOST_CHECK_EQUAL(pPage, 0xF00UL);

    BOOST_CHECK_EQUAL(tlb->lookup(0x1, pPage), false);

    // Verify the eviction through add, fully populated tlb should
    // lead to eviction.
    for (size_t i = 1; i < getMax(); i++) {
        tlb->add(i, 0x1UL);
        BOOST_CHECK_EQUAL(tlb->lookup(i, pPage), true);
        BOOST_CHECK_EQUAL(pPage, 0x1UL);
    }
    tlb->add(getMax() + 1, 0xFF0UL);
    // FIFO eviction policy, so the first entry should be evicted.
    BOOST_CHECK_EQUAL(tlb->lookup(0x0, pPage), false);
    BOOST_CHECK_EQUAL(tlb->lookup(getMax() + 1, pPage), true);
    BOOST_CHECK_EQUAL(pPage, 0xFF0UL);

    // Eviction target should do a rollover.
    for (size_t i = 1; i < getMax(); i++) {
        tlb->add(i, 0x2UL);
        BOOST_CHECK_EQUAL(tlb->lookup(i, pPage), true);
        BOOST_CHECK_EQUAL(pPage, 0x2UL);
    }
    tlb->add(0, 0x2UL);
    BOOST_CHECK_EQUAL(tlb->lookup(getMax() + 1, pPage), false);
    BOOST_CHECK_EQUAL(tlb->lookup(0x0, pPage), true);
    BOOST_CHECK_EQUAL(pPage, 0x2UL);

    // Test whether the flush removes all entries.
    tlb->flush();
    for (size_t i = 0; i < getMax(); i++) {
        BOOST_CHECK_EQUAL(tlb->lookup(i, pPage), false);
    }
}

BOOST_FIXTURE_TEST_CASE(test_asid, TLBFixture) {
    uint64_t pPage = 0;

    // Verify whether the entries are selected through the ASID.
    tlb->setASID(0x1);
    tlb->add(0x0, 0xF00UL);
    BOOST_CHECK_EQUAL(tlb->lookup(0x0, pPage), true);
    BOOST_CHECK_EQUAL(pPage, 0xF00UL);

    tlb->setASID(0x2);
    tlb->add(0x0, 0xFF0UL);
    BOOST_CHECK_EQUAL(tlb->lookup(0x0, pPage), true);
    BOOST_CHECK_EQUAL(pPage, 0xFF0UL);

    tlb->setASID(0x1);
    BOOST_CHECK_EQUAL(tlb->lookup(0x0, pPage), true);
    BOOST_CHECK_EQUAL(pPage, 0xF00UL);
}

const static uint64_t MemorySize = (1 * 1024 * 1024) << 10;

struct MMUDriverFixture {
    RISCV::MMU mmu;
    RISCV::MMUDriver driver;

    Processor processor;
    ProcessList list = {};
    OSKernel kernel;

    MMUDriverFixture()
        : mmu(),
          driver(),
          processor(mmu),
          kernel(processor, driver, MemorySize, list) {}

    MMUDriverFixture(const MMUDriverFixture &) = delete;
    MMUDriverFixture &operator=(const MMUDriverFixture &) = delete;
};

BOOST_FIXTURE_TEST_CASE(test_mmu, MMUDriverFixture) {
    driver.allocatePageTable(0);
    processor.getMMU().setPageTablePointer(driver.getPageTable(0));

    MemAccess access{
        .type = MemAccessType::Load, .addr = 1234, .size = 8};
    uint64_t pAddr = 0;
    uint64_t pPageInt = 0;

    driver.allocatePageTable(0);
    PhysPage pPage{.pid = 0, .addr = 2 * pageSize};
    driver.setMapping(0, 0, pPage);

    EnableTLB = true;

    // after a translation the entry should be stored in the tlb.
    BOOST_CHECK_EQUAL(mmu.getTranslation(access, pAddr), true);
    BOOST_CHECK_EQUAL(
        mmu.getTLB().lookup(access.addr >> pageBits, pPageInt), true);
    BOOST_CHECK_EQUAL(pPageInt, (2 * pageSize) >> pageBits);

    // after another translation the pAddr should be retrieved from
    // the tlb.
    BOOST_CHECK_EQUAL(mmu.getTLB().stats.lookups, 2);
    BOOST_CHECK_EQUAL(mmu.getTranslation(access, pAddr), true);
    BOOST_CHECK_EQUAL(pAddr, (2 * pageSize) | 1234UL);
    BOOST_CHECK_EQUAL(mmu.getTLB().stats.lookups, 3);

    // when the tlb is disabled it shouldn't use the tlb. So first
    // verify that it really does not look up the value, then flush
    // the TLB and guarantee that the value doesn't get added to the
    // TLB.
    EnableTLB = false;
    BOOST_CHECK_EQUAL(mmu.getTLB().stats.lookups, 3);
    BOOST_CHECK_EQUAL(mmu.getTranslation(access, pAddr), true);
    BOOST_CHECK_EQUAL(pAddr, (2 * pageSize) | 1234UL);
    BOOST_CHECK_EQUAL(mmu.getTLB().stats.lookups, 3);

    mmu.getTLB().flush();
    BOOST_CHECK_EQUAL(mmu.getTranslation(access, pAddr), true);
    BOOST_CHECK_EQUAL(
        mmu.getTLB().lookup(access.addr >> pageBits, pPageInt),
        false);

    /* Tear down */
    EnableTLB = false;
    processor.getMMU().setPageTablePointer(0);
    driver.releasePageTable(0);
}

BOOST_AUTO_TEST_SUITE_END()
