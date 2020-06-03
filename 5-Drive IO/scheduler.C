/*
 File: scheduler.C
 
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

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

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
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

/* Setup the scheduler. This sets up the ready queue, for example.
  If the scheduler implements some sort of round-robin scheme, then the 
  end_of_quantum handler is installed in the constructor as well. */

/* NOTE: We are making all functions virtual. This may come in handy when
  you want to derive RRScheduler from this class. */
Scheduler::Scheduler() {

  // This is a pointer to the final item in the queue, initialized at null
  listEnd = NULL;

  Console::puts("Constructed Scheduler.\n");
}

/* Called by the currently running thread in order to give up the CPU. 
  The scheduler selects the next thread from the ready queue to load onto 
  the CPU, and calls the dispatcher function defined in 'Thread.H' to
  do the context switch. */
void Scheduler::yield() {

  // Nothing to yield to, as there's nothing on the queue
  if(listEnd == NULL) {
    Console::puts("Queue is empty! Program will probably crash now.\n");
    return;
  }

  Console::puts("Called yield, front of queue is thread ");

  // Loop through the linked list to get to the end  
  Thread * currentNode = listEnd;
  Thread * previousNode = NULL;
  while(currentNode->next != NULL) {
    previousNode = currentNode;
    currentNode = currentNode->next;
  }

  Console::puti(currentNode->ThreadId()); Console::puts("\nAnd the currently running thread is ");
  Console::puti(Thread::CurrentThread()->ThreadId()); Console::puts("\n");

  // Then there's nothing else left on the queue
  if(previousNode == NULL) {
    listEnd = NULL;
  }

  // Remove the head from the queue and dispatch it
  previousNode->next = NULL;
  Thread::dispatch_to(currentNode);

}

/* Add the given thread to the ready queue of the scheduler. This is called
  for threads that were waiting for an event to happen, or that have 
  to give up the CPU in response to a preemption. */
void Scheduler::resume(Thread * _thread) {

  Console::puts("Called resume on thread "); Console::puti(_thread->ThreadId()); Console::puts("\n");

  // Just adds the thread back to the end of the ready queue, which is what add does
  this->add(_thread);
}

/* Make the given thread runnable by the scheduler. This function is called
  after thread creation. Depending on implementation, this function may 
  just add the thread to the ready queue, using 'resume'. */
void Scheduler::add(Thread * _thread) {

  Console::puts("Called add on thread "); Console::puti(_thread->ThreadId()); Console::puts("\n");

  // Simple implementation of adding to the end of a queue using a linked list
  _thread->next = listEnd;
  listEnd = _thread;

}

/* Remove the given thread from the scheduler in preparation for destruction
  of the thread. 
  Graciously handle the case where the thread wants to terminate itself.*/
void Scheduler::terminate(Thread * _thread) {
  Console::puts("terminate called on thread "); Console::puti(_thread->ThreadId()); Console::puts("\n");

  // Take the thread out of the queue, if it's there
  Thread * currentNode = listEnd;
  Thread * previousNode = NULL;
  while(currentNode != NULL) {
    // Found our thread
    if(currentNode == _thread) {
      // If the listEnd is the one being removed
      if(currentNode == listEnd) {
        listEnd = currentNode->next;
        currentNode->next = NULL;
      }
      // Else it it's in the middle or front of queue
      else {
        previousNode->next = currentNode->next;
        currentNode->next = NULL;
      }
      break;
    }
    previousNode = currentNode;
    currentNode = currentNode->next;
  }

  // Should probably be the kernel's job as it called new, but could be done here
  // delete currentNode;

  Console::puts("terminate finished\n");

  this->yield();

  // assert(false);
}



// /*
//  File: scheduler.C
 
//  Author:
//  Date  :
 
//  */

// /*--------------------------------------------------------------------------*/
// /* DEFINES */
// /*--------------------------------------------------------------------------*/

// /* -- (none) -- */

// /*--------------------------------------------------------------------------*/
// /* INCLUDES */
// /*--------------------------------------------------------------------------*/

// #include "scheduler.H"
// #include "thread.H"
// #include "console.H"
// #include "utils.H"
// #include "assert.H"
// #include "simple_keyboard.H"

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
// /* METHODS FOR CLASS   S c h e d u l e r  */
// /*--------------------------------------------------------------------------*/

// Scheduler::Scheduler() {
//   assert(false);
//   Console::puts("Constructed Scheduler.\n");
// }

// void Scheduler::yield() {
//   assert(false);
// }

// void Scheduler::resume(Thread * _thread) {
//   assert(false);
// }

// void Scheduler::add(Thread * _thread) {
//   assert(false);
// }

// void Scheduler::terminate(Thread * _thread) {
//   assert(false);
// }
