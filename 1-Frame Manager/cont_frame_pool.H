/*
 File: cont_frame_pool.H
 
 Author: R. Bettati
 Department of Computer Science
 Texas A&M University
 Date  : 17/02/04 
 
 Description: Management of the CONTIGUOUS Free-Frame Pool.
 
 As opposed to a non-contiguous free-frame pool, here we can allocate
 a sequence of CONTIGUOUS frames.
 
 */

#ifndef _CONT_FRAME_POOL_H_  // include file only once
#define _CONT_FRAME_POOL_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB *(0x1 << 20)
#define KB *(0x1 << 10)

#define TOTAL_NUMBER_OF_POSSIBLE_FRAMES ((32 MB) / (4 KB))

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"
// #include <stdio.h>

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* C o n t F r a m e   P o o l  */
/*--------------------------------------------------------------------------*/

class ContFramePool {
   private:
    /* -- DEFINE YOUR CONT FRAME POOL DATA STRUCTURE(s) HERE. */
    unsigned char* bitmap;  // We implement the simple frame pool with 2 bitmaps
    unsigned char* bitmap2; // 10 = free; 01 = head of sequence; 00 = allocated
    unsigned long base_frame_no;  // Where does the frame pool start in phys mem?
    unsigned long nframes;        // Size of the frame pool
    unsigned long info_frame_no;  // Where do we store the management information?
    unsigned long n_info_frames;  // The number of needed info frames

    static ContFramePool* framePools[TOTAL_NUMBER_OF_POSSIBLE_FRAMES];
    // Is initialized at 0
    static unsigned int framePoolsSize;

    void mark_inaccessible(unsigned long _frame_no);
    void release_frame(unsigned long frame_no);

   public:
    // The frame size is the same as the page size, duh...
    static const unsigned int FRAME_SIZE = Machine::PAGE_SIZE;

    // Getters used for determing who owns the frames to be released
    unsigned long GetBaseFrameNo() { return base_frame_no; }
    unsigned long GetNFrames() { return nframes; }

    ContFramePool(unsigned long _base_frame_no,
                  unsigned long _n_frames,
                  unsigned long _info_frame_no,
                  unsigned long _n_info_frames);
    /*
     Initializes the data structures needed for the management of this
     frame pool.
     _base_frame_no: Number of first frame managed by this frame pool.
     _n_frames: Size, in frames, of this frame pool.
     EXAMPLE: If _base_frame_no is 16 and _n_frames is 4, this frame pool manages
     physical frames numbered 16, 17, 18 and 19.
     _info_frame_no: Number of the first frame that should be used to store the
     management information for the frame pool.
     NOTE: If _info_frame_no is 0, the frame pool is free to
     choose any frames from the pool to store management information.
     _n_info_frames: If _info_frame_no is NOT 0, this argument specifies the
     number of consecutive frames needed to store the management information
     for the frame pool.
     EXAMPLE: If _info_frame_no is 699 and _n_info_frames is 3,
     then Frames 699, 700, and 701 are used to store the management information
     for the frame pool.
     NOTE: This function must be called before the paging system
     is initialized.
     */

    unsigned long get_frames(unsigned int _n_frames);
    /*
     Allocates a number of contiguous frames from the frame pool.
     _n_frames: Size of contiguous physical memory to allocate,
     in number of frames.
     If successful, returns the frame number of the first frame.
     If fails, returns 0.
     */

    void mark_inaccessible(unsigned long _base_frame_no,
                           unsigned long _n_frames);
    /*
     Marks a contiguous area of physical memory, i.e., a contiguous
     sequence of frames, as inaccessible.
     _base_frame_no: Number of first frame to mark as inaccessible.
     _n_frames: Number of contiguous frames to mark as inaccessible.
     */

    static void release_frames(unsigned long _first_frame_no);
    /*
     Releases a previously allocated contiguous sequence of frames
     back to its frame pool.
     The frame sequence is identified by the number of the first frame.
     NOTE: This function is static because there may be more than one frame pool
     defined in the system, and it is unclear which one this frame belongs to.
     This function must first identify the correct frame pool and then call the frame
     pool's release_frame function.
     */

    static unsigned long needed_info_frames(unsigned long _n_frames);
    /*
     Returns the number of frames needed to manage a frame pool of size _n_frames.
     The number returned here depends on the implementation of the frame pool and 
     on the frame size.
     EXAMPLE: For FRAME_SIZE = 4096 and a bitmap with a single bit per frame 
     (not appropriate for contiguous allocation) one would need one frame to manage a 
     frame pool with up to 8 * 4096 = 32k frames = 128MB of memory!
     This function would therefore return the following value:
       _n_frames / 32k + (_n_frames % 32k > 0 ? 1 : 0) (always round up!)
     Other implementations need a different number of info frames.
     The exact number is computed in this function..
     */
};
#endif
