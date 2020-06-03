/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"

/*--------------------------------------------------------------------------*/
/* EXTERNS */
/*--------------------------------------------------------------------------*/

extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {

    headOfList = NULL;
    tailOfList = NULL;

}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  // SimpleDisk::read(_block_no, _buf);

  // Add thread into queue
  Thread::CurrentThread()->diskNext = tailOfList;
  tailOfList = Thread::CurrentThread();

  // If the head has yet to be assigned, then assign it to this
  if(headOfList == NULL) {
    headOfList = Thread::CurrentThread();
  }

  while(true) {
    
    // If currently running thread is the head of the queue
    if(Thread::CurrentThread() == headOfList) {
      // Issue the operation
      issue_operation(READ, _block_no);

      // Begin loop to check if able to perform read
      while(true) {

        // If the read is ready
        if(is_ready()) {
          // Perform read
          int i;
          unsigned short tmpw;
          for (i = 0; i < 256; i++) {
            tmpw = Machine::inportw(0x1F0);
            _buf[i*2]   = (unsigned char)tmpw;
            _buf[i*2+1] = (unsigned char)(tmpw >> 8);
          }

          // remove from queue
          Thread * currentNode = tailOfList;
          Thread * previousNode = NULL;

          while(currentNode != headOfList) {
            previousNode = currentNode;
            currentNode = currentNode->diskNext;
          }

          previousNode->diskNext = currentNode->diskNext;
          headOfList = previousNode;

          break;
        }
        // Else, yield CPU
        else {
          SYSTEM_SCHEDULER->yield();
        }
      }
      break;
    }
    // Else, yield the CPU
    else {
      SYSTEM_SCHEDULER->yield();
    }
  }

  // While true

    // If head of the queue
      // issue operation

      // While true

        // If is_ready()
          // perform read
          // remove from queue
          // break
        // Else
          // yield

      //break

    // Else
      // yield

}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  // // -- REPLACE THIS!!!
  // SimpleDisk::write(_block_no, _buf);

  // Add thread into queue
  Thread::CurrentThread()->diskNext = tailOfList;
  tailOfList = Thread::CurrentThread();

  // If the head has yet to be assigned, then assign it to this
  if(headOfList == NULL) {
    headOfList = Thread::CurrentThread();
  }

  while(true) {
    
    // If currently running thread is the head of the queue
    if(Thread::CurrentThread() == headOfList) {
      // Issue the operation
      issue_operation(WRITE, _block_no);

      // Begin loop to check if able to perform read
      while(true) {

        // If the write is ready
        if(is_ready()) {
          // Perform write
          int i; 
          unsigned short tmpw;
          for (i = 0; i < 256; i++) {
            tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
            Machine::outportw(0x1F0, tmpw);
          }

          // remove from queue
          Thread * currentNode = tailOfList;
          Thread * previousNode = NULL;

          while(currentNode != headOfList) {
            previousNode = currentNode;
            currentNode = currentNode->diskNext;
          }

          previousNode->diskNext = currentNode->diskNext;
          headOfList = previousNode;

          break;
        }
        // Else, yield CPU
        else {
          SYSTEM_SCHEDULER->yield();
        }
      }
      break;
    }
    // Else, yield the CPU
    else {
      SYSTEM_SCHEDULER->yield();
    }
  }
}
