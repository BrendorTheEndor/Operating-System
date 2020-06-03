/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

/* Initializes the data structures needed for the management of this
* virtual-memory pool.
* _base_address is the logical start address of the pool.
* _size is the size of the pool in bytes.
* _frame_pool points to the frame pool that provides the virtual
* memory pool with physical memory frames.
* _page_table points to the page table that maps the logical memory
* references to physical addresses. */
VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {

    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;

    // Register the pool with its page table
    page_table->register_pool(this);

    // Set up the array to keep track of memory regions at the start of the pool
    memoryRegions = (unsigned long *) base_address;
    memoryRegionsSize = 0;

    // debug_out_E9("Going to try and assign\n");
    // memoryRegions[0] = 0;
    // debug_out_E9("Assigned\n");

    // Start after the first page of the pool, as the first page has mgmt info
    currentAddressToAssignTo = base_address + Machine::PAGE_SIZE;

    Console::puts("Constructed VMPool object.\n");
}

/* Allocates a region of _size bytes of memory from the virtual
* memory pool. If successful, returns the virtual address of the
* start of the allocated region of memory. If fails, returns 0. */
unsigned long VMPool::allocate(unsigned long _size) {

    // debug_out_E9("Allocate called\n");

    // Get the memory to allocate in terms of page multiples
    unsigned long memoryToAllocate = 0;
    unsigned long pagesToAllocate = 0;
    while(memoryToAllocate < _size) {
        // debug_out_E9("Page number loop fired\n");
        memoryToAllocate += Machine::PAGE_SIZE;
        pagesToAllocate++;
    }

    // Set up for the coming loops
    // Will store the start address for the region
    unsigned long startAddress = 0;
    // Will keep track of how many uniterrupted pages we can allocate
    unsigned long pagesAbleToBeAllocated = 0;
    // Keeps track of if this is the first page in the hole or not
    bool atStartPoint = true;
    // Used to keep track of any released regions we can use
    int firstReleasedIndex = memoryRegionsSize;
    // Used to find if this hole was big enough
    bool withinExistingRegion = false;

    // Go through entire region
    for(unsigned long i = base_address + Machine::PAGE_SIZE; i < (base_address + size); i += Machine::PAGE_SIZE) {

        // debug_out_E9("Outter loop fired\n");

        // Go through all memory regions we have
        for(int j = 0; j < memoryRegionsSize; j += 2) {

            // Console::puts("Checking for existing regions\n");
            // debug_out_E9("Checking for existing regions\n");

            // Look for any released regions for use
            if(memoryRegions[j] == 0) {
                firstReleasedIndex = j;
            }

            // If within range of an existing region, reset
            if((i >= memoryRegions[j]) && (i < (memoryRegions[j] + memoryRegions[j+1]))) {
                // debug_out_E9("In an existing region\n");
                // Console::puts("In an existing region\n");
                withinExistingRegion = true;
                break;
            }

        }

        // If this location was not in an existing region, then count this page
        if(!withinExistingRegion) {
            pagesAbleToBeAllocated++;
            if(atStartPoint) {
                startAddress = i;
                atStartPoint = false;
            }
        }
        // Otherwise, reset all the counts, and start looking for a new hole
        else {
            pagesAbleToBeAllocated = 0;
            startAddress = 0;
            atStartPoint = true;
        }

        withinExistingRegion = false;

        // If this hole is large enough
        if(pagesAbleToBeAllocated >= pagesToAllocate) {
            // debug_out_E9("Trying to allocate\n");
            // If this value didn't change, that means there were no released regions to take the spot of, so make a new one
            if(firstReleasedIndex == memoryRegionsSize) {
                // debug_out_E9("Allocating to new index\n");
                memoryRegions[memoryRegionsSize] = startAddress;
                // debug_out_E9("Assigned to array\n");
                memoryRegions[memoryRegionsSize + 1] = memoryToAllocate;
                memoryRegionsSize += 2;
            }
            // Otherwise, take the empty spot
            else {
                // debug_out_E9("Allocating to reused index\n");
                memoryRegions[firstReleasedIndex] = startAddress;
                memoryRegions[firstReleasedIndex + 1] = memoryToAllocate;
            }

            // debug_out_E9("Hit the return\n");

            return startAddress;
        }
    }

    Console::puts("VMPool is full");
    return 0;

    // // Old, much simpler implementation, but very limited

    // // Make sure that there is enough room left in the pool
    // if((currentAddressToAssignTo + memoryToAllocate) > (base_address + size)) {
    //     Console::puts("VMPool is full");
    //     return 0;
    // }

    // // Set up this region's start address and it's size, then increment the size of the array
    // memoryRegions[memoryRegionsSize] = currentAddressToAssignTo;
    // memoryRegions[memoryRegionsSize + 1] = memoryToAllocate;
    // memoryRegionsSize += 2;

    // // Move the next address to assign to up by the approriate amount
    // currentAddressToAssignTo += memoryToAllocate;

    // // return the value it was before, because that is the start of the allocated region
    // return (currentAddressToAssignTo - memoryToAllocate);

    // Console::puts("Allocated region of memory.\n");
}

/* Releases a region of previously allocated memory. The region
* is identified by its start address, which was returned when the
* region was allocated. */
void VMPool::release(unsigned long _start_address) {

    // Check all the registered regions to find this start address
    for(int i = 0; i <= memoryRegionsSize; i += 2) {
        // If the region with the start address is found
        if(memoryRegions[i] == _start_address) {
            // Release the pages from the table by freeing every page from the start address through to the end of the region
            // < and not <= because then it would free the page after the end as well
            for(unsigned long j = _start_address; j < (memoryRegions[i] + memoryRegions[i+1]); j += Machine::PAGE_SIZE) {
                // Shifting 12 bits to the right so that it goes from an address to a page number
                page_table->free_page(j >> 12);
            }
            // Reset the entry to be empty, so it is no longer legitimate
            memoryRegions[i] = 0;
            memoryRegions[i+1] = 0;
            return;
        }
    }

    Console::puts("Released region of memory.\n");
}

/* Returns false if the address is not valid. An address is not valid
* if it is not part of a region that is currently allocated. */
bool VMPool::is_legitimate(unsigned long _address) {

    if((_address >= base_address) && (_address < (base_address + Machine::PAGE_SIZE))) {
        return true;
    }

    // Check all entries to see if they contain the address
    for(int i = 0; i <= memoryRegionsSize; i += 2) {
        // Check if the address is in the range of this entry
        // < and not <= because the start address is size = 1, so start + size is actually one outside of the region
        if((_address >= memoryRegions[i]) && (_address < (memoryRegions[i] + memoryRegions[i+1]))) {
            return true;
        }
    }

    return false;

    Console::puts("Checked whether address is part of an allocated region.\n");
}

