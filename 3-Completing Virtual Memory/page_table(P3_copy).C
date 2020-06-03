#include "assert.H"
#include "console.H"
#include "exceptions.H"
#include "page_table.H"
#include "paging_low.H"

PageTable* PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool* PageTable::kernel_mem_pool = NULL;
ContFramePool* PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;

// Is static and called before any objects actually constructed
void PageTable::init_paging(ContFramePool* _kernel_mem_pool,
                            ContFramePool* _process_mem_pool,
                            const unsigned long _shared_size) {
    // assert(false);

    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;

    Console::puts("Initialized Paging System\n");
}

// Frame pool usage:
// unsigned long frame = _pool->get_frames(n_frames);
// int* value_array = (int*)(frame * (4 KB));
PageTable::PageTable() {
    // Get the frames to store the page directory, the number of which is equal to
    // The shared size needed / page size * the entries per page (which is the size managed per page)
    unsigned long frameAddr = kernel_mem_pool->get_frames(shared_size / (PAGE_SIZE * ENTRIES_PER_PAGE)) * PAGE_SIZE;
    // Place the page directory in the address of the frame it's to be placed in
    page_directory = (unsigned long*)frameAddr;

    // Get frames needed for the page table and set up first inner PT
    unsigned long ptFrameAddr = kernel_mem_pool->get_frames(shared_size / (PAGE_SIZE * ENTRIES_PER_PAGE)) * PAGE_SIZE;
    unsigned long* innerPT = (unsigned long*)ptFrameAddr;

    // Fill the inner page table with approriately set up (but not used) frames
    unsigned long address = 0;
    for (unsigned int i = 0; i < ENTRIES_PER_PAGE; i++) {
        innerPT[i] = address | 3;  // Sets the last bit == 1, so present
        address += PAGE_SIZE;
    }

    // Set up the first bit of the page directory
    for (int i = 0; i < (shared_size / (PAGE_SIZE * ENTRIES_PER_PAGE)); i++) {
        page_directory[i] = (unsigned long)innerPT;
        page_directory[i] = page_directory[i] | 3;
    }

    // Set the rest of the page directory up
    for (unsigned int i = 1; i < ENTRIES_PER_PAGE; i++) {
        page_directory[i] = 0 | 2;  // Not present
    }

    //TODO: Make the last page of the directoy link back to the start

    Console::puts("Constructed Page Table object\n");
}

void PageTable::load() {
    // Put page directory into cr3
    write_cr3((unsigned long)page_directory);
    PageTable::current_page_table = this;
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging() {
    // Set paging bit in cr0 to 1
    write_cr0(read_cr0() | 0x80000000);
    PageTable::paging_enabled = 1;
    Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS* _r) {

    Machine::enable_interrupts();

    // Get the address of the fault, and shift it the approriate amount to get the page number of that address
    unsigned long address = read_cr2();
    unsigned long pageNumber = address >> 12;

    // Then get the page directory index of this page
    unsigned long pageDirectoryIndex = address >> 22;

    // If the page directory index is not marked present
    if((current_page_table->page_directory[pageDirectoryIndex] & 0x1) != 0x1) {

        // Get new frame for page table and initialize it
        // [B]
        // TODO: Get from process pool
        unsigned long * innerPT = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);
        // TODO: Reassign innterPT to be in the form 1023 | 1023 | X | 00
        // Mark all the entries as not present
        for(int i = 0; i < ENTRIES_PER_PAGE; i++) {
            innerPT[i] = 0x2;
        }
        // Update directory
        current_page_table->page_directory[pageDirectoryIndex] = (unsigned long) innerPT;
        current_page_table->page_directory[pageDirectoryIndex] |= 0x3;
        // Console::puts("After updating page directory\n");
    }

    // Now we need to update the page table entry in the inner PT
    // Get the inner page table that this page needs to go in (the hex digits are just to extract the right specific bits)
    unsigned long * innerPT = (unsigned long *)(current_page_table->page_directory[pageDirectoryIndex] & 0xFFFFF000);
    // And the index within this table
    unsigned int pageTableIndex = (address >> 12) & (0x000003FF);

    // [C]

    // Now get a new frame and update the page table entry with it
    unsigned long newFrame = process_mem_pool->get_frames(1);
    // Update the page table to have the frame marked present
    unsigned long newFrameAddress = newFrame * PAGE_SIZE;
    // TODO: reassign newFrameAddress to 1023 | X | Y | 00
    innerPT[pageTableIndex] = newFrameAddress | 0x3;

    Console::puts("handled page fault\n");
}
