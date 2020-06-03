/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "cont_frame_pool.H"
#include "utils.H"

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
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

unsigned int ContFramePool::framePoolsSize = 0;
ContFramePool *ContFramePool::framePools[TOTAL_NUMBER_OF_POSSIBLE_FRAMES];

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
ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames) {
    this->base_frame_no = _base_frame_no;
    this->nframes = _n_frames;
    this->info_frame_no = _info_frame_no;
    this->n_info_frames = _n_info_frames;

    // Add this pool to the list of pools
    ContFramePool::framePools[ContFramePool::framePoolsSize] = this;
    ContFramePool::framePoolsSize++;

    // Get the amount of frames needed to manage this pool
    unsigned long neededInfoFrames = this->needed_info_frames(this->nframes);

    // If not given anywhere specific to store the info, put it at the start
    // Since each frame is FRAME_SIZE large, set the initial point of the array to the
    // starting frame times the size, so that it starts at the start of the frame
    if (this->info_frame_no == 0) {
        bitmap = (unsigned char *)(this->base_frame_no * FRAME_SIZE);
        bitmap2 = (unsigned char *)((nframes / 8) + (this->base_frame_no * FRAME_SIZE));
    }  // Else put it where was specified
    else {
        bitmap = (unsigned char *)(this->info_frame_no * FRAME_SIZE);
        bitmap2 = (unsigned char *)((nframes / 8) + (this->info_frame_no * FRAME_SIZE));
        // Also make sure we were given enough room to store the information
        assert(neededInfoFrames <= n_info_frames);
    }

    // Number of frames must be "fill" the bitmap!
    assert((nframes % 8) == 0);  // Not sure if this is actually needed

    // Mark each frame as free
    // 10 = free
    // 01 = head of sequence
    // 00 = allocated, where the bits are ordered [bitmap][bitmap2]
    for (int i = 0; i * 8 < nframes; i++) {
        bitmap[i] = 0xFF;   // 1111 1111
        bitmap2[i] = 0x00;  // 0000 0000
    }                       // Every single bit in both arrays is now set to 1

    // Mark the first frames in use, if any
    if (info_frame_no == 0) {
        bitmap[0] = 0x7F;   // 0111 1111
        bitmap2[0] = 0x80;  // 1000 0000, so the first frame should now be 01
        for (int i = 1; i < neededInfoFrames; i++) {
            // This clears the index'th frame's bit for free/allocated
            int index = i / 8;
            int pos = i % 8;

            unsigned char mask = 0x80;  // 1000 0000
            mask = mask >> pos;
            mask = ~mask;

            bitmap[index] = bitmap[index] & mask;
        }
    }
}

/*
    Allocates a number of contiguous frames from the frame pool.
    _n_frames: Size of contiguous physical memory to allocate,
    in number of frames.
    If successful, returns the frame number of the first frame.
    If fails, returns 0.
*/
unsigned long ContFramePool::get_frames(unsigned int _n_frames) {
    unsigned int consecutiveFreeFrames = 0;
    unsigned long indexOfHeadOfSequence = 0;  // This is index in pool, not bitmap[index]

    // Go through all frames in the manager
    for (int i = 0; i < nframes; i++) {
        int index = i / 8;
        int pos = i % 8;
        unsigned char mask = 0x80;  // 1000 0000
        mask = mask >> pos;

        // If a free frame is found, start counting contiguous free frames
        if (bitmap[index] & mask) {
            if (indexOfHeadOfSequence == 0) {
                indexOfHeadOfSequence = i;
            }
            consecutiveFreeFrames++;

            // If we've found a hole large enough, then allocate and return
            if (consecutiveFreeFrames >= _n_frames) {
                // Assign the first free frame as head of sequence
                // All the bitmap vals here should be 10
                int index = indexOfHeadOfSequence / 8;
                int pos = indexOfHeadOfSequence % 8;
                unsigned char mask = 0x80;  // 1000 0000
                mask = mask >> pos;

                // Clear bit in bitmap, activate bit in bitmap2, so we have 01
                bitmap2[index] = bitmap2[index] | mask;
                mask = ~mask;
                bitmap[index] = bitmap[index] & mask;

                // Loop through the rest of the frames and assign them as allocated
                for (int j = indexOfHeadOfSequence + 1; j <= i; j++) {
                    index = j / 8;
                    pos = j % 8;
                    mask = 0x80;  // 1000 0000
                    mask = mask >> pos;

                    // Clear bit in bitmap
                    mask = ~mask;
                    bitmap[index] = bitmap[index] & mask;  // Now 00
                }

                return (base_frame_no + indexOfHeadOfSequence);
            }
        }
        // If we aren't at a free frame, reset the count
        else {
            consecutiveFreeFrames = 0;
            indexOfHeadOfSequence = 0;
        }
    }

    // If didn't find a large enough hole, return 0
    return 0;
}

/*
    Marks a contiguous area of physical memory, i.e., a contiguous
    sequence of frames, as inaccessible.
    _base_frame_no: Number of first frame to mark as inaccessible.
    _n_frames: Number of contiguous frames to mark as inaccessible.
*/
void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames) {
    // Loop through all the specified frames and mark them as inaccessible
    for (int i = _base_frame_no; i < _base_frame_no + _n_frames; i++) {
        mark_inaccessible(i);
    }
}

void ContFramePool::mark_inaccessible(unsigned long _frame_no) {
    // Let's first do a range check.
    assert((_frame_no >= base_frame_no) && (_frame_no < base_frame_no + nframes));

    // Since this is passed as an actual frame number in the total system, not
    // an index in this specific pool, first need to convert to that type of index
    int index = (_frame_no - base_frame_no) / 8;
    int pos = (_frame_no - base_frame_no) % 8;

    unsigned char mask = 0x80;  // 1000 0000
    mask = mask >> pos;
    mask = ~mask;  // Mask is now 0111 1111 where the 0 is shifted to the appropriate pos

    // This frame should now be 00
    bitmap[index] = bitmap[index] & mask;
    bitmap2[index] = bitmap2[index] & mask;
}

/*
    Releases a previously allocated contiguous sequence of frames
    back to its frame pool.
    The frame sequence is identified by the number of the first frame.
    NOTE: This function is static because there may be more than one frame pool
    defined in the system, and it is unclear which one this frame belongs to.
    This function must first identify the correct frame pool and then call the frame
    pool's release_frame function.
*/
void ContFramePool::release_frames(unsigned long _first_frame_no) {
    // Check to find what pool the frame belongs to
    for (int i = 0; i < ContFramePool::framePoolsSize; i++) {
        unsigned int baseFrame = ContFramePool::framePools[i]->GetBaseFrameNo();
        unsigned int nFrames = ContFramePool::framePools[i]->GetNFrames();

        // Check if the frames to be released are in this pool or not
        if ((_first_frame_no >= baseFrame) && (_first_frame_no <= (baseFrame + nFrames))) {
            ContFramePool::framePools[i]->release_frame(_first_frame_no);
        }
    }
}

// Actually releases all the frames attatched to this one as well
void ContFramePool::release_frame(unsigned long frame_no) {
    unsigned int frameIndex = frame_no - base_frame_no;

    int index = frameIndex / 8;
    int pos = frameIndex % 8;

    unsigned char mask = 0x80;      // 1000 0000
    mask = mask >> pos;             // 0010 0000 or something
    unsigned char notMask = ~mask;  // 1101 1111 or something

    if (bitmap[index] & mask) {  // If 1x
        Console::puts("Error, Frame being released is not being used (marked A)\n");
        assert(false);
    } else {                          // Bit is 0x
        if (bitmap2[index] & mask) {  // This frame is marked as HoS (01)

            // Mark HoS frame as free
            bitmap[index] = bitmap[index] | mask;       // Set to 11
            bitmap2[index] = bitmap2[index] & notMask;  // Set to 10

            // Get set up to check next frame
            frameIndex++;
            index = frameIndex / 8;
            pos = frameIndex % 8;

            mask = 0x80;         // 1000 0000
            mask = mask >> pos;  // 0010 0000 or something

            // Should be true if the pos bit is 1
            bool bitmapResult = bitmap[index] & mask;
            bool bitmap2Result = bitmap[index] & mask;

            while ((!bitmapResult) && (!bitmapResult)) {  // While the bit is allocated (00)
                bitmap[index] = bitmap[index] | mask;     // Set to 10

                // Set up for next frame
                frameIndex++;
                index = frameIndex / 8;
                pos = frameIndex % 8;

                mask = 0x80;         // 1000 0000
                mask = mask >> pos;  // 0010 0000 or something

                bitmapResult = bitmap[index] & mask;
                bitmap2Result = bitmap[index] & mask;
            }
        } else {
            Console::puts("Error, Frame being released is not being used (not HoS)\n");
            assert(false);
        }
    }
}

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
unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames) {
    unsigned int numberOfBitsNeededPerFrame = 2;
    unsigned int numberOfFramesRepresentedPerFrameUsed = FRAME_SIZE / numberOfBitsNeededPerFrame;

    unsigned int pageNumber = 0;

    while (true) {
        pageNumber++;
        if ((numberOfFramesRepresentedPerFrameUsed * pageNumber) >= _n_frames) {
            return pageNumber;
        }
    }
}





// /*
//  File: ContFramePool.C
 
//  Author:
//  Date  : 
 
//  */

// /*--------------------------------------------------------------------------*/
// /* 
//  POSSIBLE IMPLEMENTATION
//  -----------------------

//  The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
//  incomplete vanilla implementation of a frame pool that allocates 
//  *single* frames at a time. Because it does allocate one frame at a time, 
//  it does not guarantee that a sequence of frames is allocated contiguously.
//  This can cause problems.
 
//  The class ContFramePool has the ability to allocate either single frames,
//  or sequences of contiguous frames. This affects how we manage the
//  free frames. In SimpleFramePool it is sufficient to maintain the free 
//  frames.
//  In ContFramePool we need to maintain free *sequences* of frames.
 
//  This can be done in many ways, ranging from extensions to bitmaps to 
//  free-lists of frames etc.
 
//  IMPLEMENTATION:
 
//  One simple way to manage sequences of free frames is to add a minor
//  extension to the bitmap idea of SimpleFramePool: Instead of maintaining
//  whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
//  we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
//  The meaning of FREE is the same as in SimpleFramePool. 
//  If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
//  and that it is the first such frame in a sequence of frames. Allocated
//  frames that are not first in a sequence are marked as ALLOCATED.
 
//  NOTE: If we use this scheme to allocate only single frames, then all 
//  frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
//  NOTE: In SimpleFramePool we needed only one bit to store the state of 
//  each frame. Now we need two bits. In a first implementation you can choose
//  to use one char per frame. This will allow you to check for a given status
//  without having to do bit manipulations. Once you get this to work, 
//  revisit the implementation and change it to using two bits. You will get 
//  an efficiency penalty if you use one char (i.e., 8 bits) per frame when
//  two bits do the trick.
 
//  DETAILED IMPLEMENTATION:
 
//  How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
//  allocator? Let's look a the individual functions:
 
//  Constructor: Initialize all frames to FREE, except for any frames that you 
//  need for the management of the frame pool, if any.
 
//  get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
//  sequence of at least _n_frames entries that are FREE. If you find one, 
//  mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
//  ALLOCATED.

//  release_frames(_first_frame_no): Check whether the first frame is marked as
//  HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
//  Traverse the subsequent frames until you reach one that is FREE or 
//  HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
//  mark_inaccessible(_base_frame_no, _n_frames): This is no different than
//  get_frames, without having to search for the free sequence. You tell the
//  allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
//  frames after that to mark as ALLOCATED.
 
//  needed_info_frames(_n_frames): This depends on how many bits you need 
//  to store the state of each frame. If you use a char to represent the state
//  of a frame, then you need one info frame for each FRAME_SIZE frames.
 
//  A WORD ABOUT RELEASE_FRAMES():
 
//  When we releae a frame, we only know its frame number. At the time
//  of a frame's release, we don't know necessarily which pool it came
//  from. Therefore, the function "release_frame" is static, i.e., 
//  not associated with a particular frame pool.
 
//  This problem is related to the lack of a so-called "placement delete" in
//  C++. For a discussion of this see Stroustrup's FAQ:
//  http://www.stroustrup.com/bs_faq2.html#placement-delete
 
//  */
// /*--------------------------------------------------------------------------*/


// /*--------------------------------------------------------------------------*/
// /* DEFINES */
// /*--------------------------------------------------------------------------*/

// /* -- (none) -- */

// /*--------------------------------------------------------------------------*/
// /* INCLUDES */
// /*--------------------------------------------------------------------------*/

// #include "cont_frame_pool.H"
// #include "console.H"
// #include "utils.H"
// #include "assert.H"

// /*--------------------------------------------------------------------------*/
// /* DATA STRUCTURES */
// /*--------------------------------------------------------------------------*/

// /* -- (none) -- */

// /*--------------------------------------------------------------------------*/
// /* CONSTANTS */
// /*--------------------------------------------------------------------------*/

// /* -- (none) -- */

// /*--------------------------------------------------------------------------*/
// /* FORWARDS */
// /*--------------------------------------------------------------------------*/

// /* -- (none) -- */

// /*--------------------------------------------------------------------------*/
// /* METHODS FOR CLASS   C o n t F r a m e P o o l */
// /*--------------------------------------------------------------------------*/

// ContFramePool::ContFramePool(unsigned long _base_frame_no,
//                              unsigned long _nframes,
//                              unsigned long _info_frame_no,
//                              unsigned long _n_info_frames)
// {
//     // TODO: IMPLEMENTATION NEEEDED!
//     assert(false);
// }

// unsigned long ContFramePool::get_frames(unsigned int _n_frames)
// {
//     // TODO: IMPLEMENTATION NEEEDED!
//     assert(false);
// }

// void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
//                                       unsigned long _n_frames)
// {
//     // TODO: IMPLEMENTATION NEEEDED!
//     assert(false);
// }

// void ContFramePool::release_frames(unsigned long _first_frame_no)
// {
//     // TODO: IMPLEMENTATION NEEEDED!
//     assert(false);
// }

// unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
// {
//     // TODO: IMPLEMENTATION NEEEDED!
//     assert(false);
// }
