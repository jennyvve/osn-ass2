/* pagetables -- A framework to experiment with memory management
 *
 * Copyright (C) 2017--2026 Leiden University, The Netherlands.
 */

#include "riscv.h"
using namespace RISCV;

#include <iostream>

/* Initialize a page table entry @entry with the given @address.
 */
static inline void initPageTableEntry(TableEntry &entry,
                                      const uintptr_t address) {
    entry.valid = 1;
    entry.ppn = address >> pageBits;

    entry.read = 1;
    entry.write = 1;
    entry.execute = 1;
}

/* Returns the address for a given page table entry @entry.
 */
static inline uintptr_t getAddress(TableEntry &entry) {
    return (uintptr_t)entry.ppn << pageBits;
}

/*
 * MMU driver software (part of the OS kernel). The OS kernel is in
 * charge of actually allocating and organizing the page tables for
 * the MMU to use.
 */

RISCV::MMUDriver::MMUDriver()
    : pageTables(), bytesAllocated(0), kernel(nullptr) {}

RISCV::MMUDriver::~MMUDriver() {
    if (pageTables.empty()) return;

    std::cerr
        << "MMUDriver: error: kernel did not release all page tables."
        << std::endl;
}

/* Set the host kernel of this driver to @kernel.
 */
void RISCV::MMUDriver::setHostKernel(OSKernel *kernel) {
    this->kernel = kernel;
}

const static int entries = pageSize / sizeof(TableEntry);

/* Allocate a new page table root for the process @proc.
 */
void RISCV::MMUDriver::allocatePageTable(const PID proc) {
    TableEntry *table =
        reinterpret_cast<TableEntry *>(kernel->allocateMemory(
            entries * sizeof(TableEntry), pageTableAlign));
    /* Note: allocateMemory always allocates entire pages. */
    bytesAllocated += entries * sizeof(TableEntry);

    for (int i = 0; i < entries; i++) {
        table[i].valid = 0;
    }

    /* Add to list of page table roots. */
    pageTables.emplace(proc, table);
}

void RISCV::MMUDriver::releasePageTableInternal(TableEntry *table) {
    for (int y = 0; y < entries; y++) {
        if (table[y].valid && !table[y].read) {
            TableEntry *currentTable = reinterpret_cast<TableEntry *>(
                reinterpret_cast<uintptr_t>(table[y].ppn)
                << pageBits);
            releasePageTableInternal(currentTable);
        }
    }

    kernel->releaseMemory(reinterpret_cast<void *>(table),
                          entries * sizeof(TableEntry));
}

/* Release the page table associated with the process @proc.
 */
void RISCV::MMUDriver::releasePageTable(const PID proc) {
    TableEntry *table =
        reinterpret_cast<TableEntry *>(getPageTable(proc));
    releasePageTableInternal(table);
    auto it = pageTables.find(proc);
    pageTables.erase(it);
}

/* Returns the root of the page table associated with the process
 * @proc.
 */
uintptr_t RISCV::MMUDriver::getPageTable(const PID proc) {
    auto kv = pageTables.find(proc);
    if (kv == pageTables.end()) return 0x0;

    return reinterpret_cast<uintptr_t>(kv->second);
}

/* Create a new mapping for the process @proc, mapping the virtual
 * address
 *  @vAddr to the physical page @pPage.
 */
void RISCV::MMUDriver::setMapping(const PID proc, uintptr_t vAddr,
                                  PhysPage &pPage) {
    const int tblLevels = 4;

    TableEntry *currentTable =
        reinterpret_cast<TableEntry *>(getPageTable(proc));
    if (!currentTable) {
        throw std::runtime_error(
            "Process has no allocated root page table.");
    }

    for (int lv = tblLevels - 1; lv > 0; lv--) {
        int i = (vAddr >> (pageBits + lv * vpnBits)) & vpnMask;

        if (!currentTable[i].valid) {
            TableEntry *newTable =
                reinterpret_cast<TableEntry *>(kernel->allocateMemory(
                    entries * sizeof(TableEntry), pageTableAlign));

            bytesAllocated += entries * sizeof(TableEntry);

            for (int y = 0; y < entries; y++) {
                newTable[y].valid = 0;
            }

            currentTable[i].valid = 1;
            currentTable[i].read = 0;
            currentTable[i].write = 0;
            currentTable[i].execute = 0;
            currentTable[i].ppn =
                reinterpret_cast<uintptr_t>(newTable) >> pageBits;
        }

        currentTable = reinterpret_cast<TableEntry *>(
            reinterpret_cast<uintptr_t>(currentTable[i].ppn)
            << pageBits);
    }

    int i = (vAddr >> pageBits) & vpnMask;
    initPageTableEntry(currentTable[i], pPage.addr);
}

/* Returns the number of bytes allocated for the page table.
 */
uint64_t RISCV::MMUDriver::getBytesAllocated(void) const {
    return bytesAllocated;
}
