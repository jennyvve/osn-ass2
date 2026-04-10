/* pagetables -- A framework to experiment with memory management
 *
 * Copyright (C) 2017--2026 Leiden University, The Netherlands.
 */

#include "riscv.h"
using namespace RISCV;

/*
 * MMU hardware. The MMU translates virtual addresses to physical addresses. It
 * does this based on mappings stored in the page table.
 */

RISCV::MMU::MMU()
{
}

RISCV::MMU::~MMU()
{
}

/* Translate a virtual page number @vPage to a physical page number @pPage.
 * If @isWrite is true, the translation is for a write (this is ignored).
 * Returns whether the translation succeeded.
 */
bool
RISCV::MMU::performTranslation(const uint64_t vPage,
                                uint64_t &pPage,
                                bool isWrite)
{
  /* TODO: Implement the MMU hardware. */
  return false;
}
