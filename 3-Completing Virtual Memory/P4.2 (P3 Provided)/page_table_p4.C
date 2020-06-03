#include "assert.H"
#include "console.H"
#include "exceptions.H"
#include "page_table.H"
#include "paging_low.H"
#include "vm_pool.H"

/*************************************************
 * The following methods are defined in the provided page_table_provided.o
 *
 * You can keep using your own implementation from P3, ignoring page_table_provided.o
 *


   void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size);

   PageTable::PageTable();

   void PageTable::load();

   void PageTable::enable_paging();

   void PageTable::handle_fault(REGS * _r);

  *************************************************
  */

// Returns whether or not the address is legit or not, check all the pools with their is_legitimate method
bool PageTable::check_address(unsigned long address) {
    // you need to implement this
    // it returns true if legitimate, false otherwise
    assert(false);

    for (int i = 0; i < VMPoolsSize; i++) {
        if (VMPools[i]->is_legitimate(address)) {
            return true;
        }
    }

    return false;  // you need to implement this
}

// Add this pool to a list of em
void PageTable::register_pool(VMPool *_vm_pool) {
    assert(false);

    VMPools[VMPoolsSize] = _vm_pool;
    VMPoolsSize++;

    Console::puts("registered VM pool\n");
}

// Check that page is valid, release the frame, mark the page invalid, and flush the TLB
// This can be done by reloading CR3 with its current value
void PageTable::free_page(unsigned long _page_no) {
    // _page_no is a virtual page number
    // 0*12 | X (10 bits) | Y (10 bits)
    // 0*12 | dir index | pt index
    // freeFrame(dir[X][Y]) will need to shift 12 bits to get number rather than addr
    // dir[X][Y] = 0x2
    // Look at dir and corresponding innerPT to find physical page number
    // free the frame, and mark it as not present (0x2)
    
    // Extract the "X" from the page number
    unsigned long pageDirectoryIndex = _page_no >> 10;
    // Get the inner page table at this index
    unsigned long *innerPT = (unsigned long *)(current_page_table->page_directory[pageDirectoryIndex] & 0xFFFFF000);
    // Get the index into this page table needed (mask extracts the 10 LSB)
    unsigned int pageTableIndex = (_page_no & 0x000003FF);

    // Check that the page is actually valid before doing anything
    if ((innerPT[pageTableIndex] & 0x1) != 0x1) {
        // Release the frame (since we have an address stored in the PT, we need to divide it by PAGE_SIZE to get the number)
        ContFramePool::release_frames(innerPT[pageTableIndex] / PAGE_SIZE);

        // Mark the page as not present
        innerPT[pageTableIndex] = 0x2;

        // Reload CR3 to flush TLB
        unsigned long oldCR3 = read_CR3();
        write_CR3(oldCR3);
    }

    Console::puts("freed page\n");
}
