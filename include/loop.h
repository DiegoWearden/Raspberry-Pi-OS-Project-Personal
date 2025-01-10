#ifndef _LOOP_H_
#define _LOOP_H_



// #include "machine.h"
#include "utils.h"

// Called when the code is spinning in a loop
//
// use_mwait (if true) tells us that the caller recommends using
// mwait. They are responsible for calling monitor
// as needed before checking the condition and calling us.
//
inline void iAmStuckInALoop(bool use_wfe) {
    if (onHypervisor) {
        // On ARM, QEMU also has inconsistent support for WFE and YIELD.
        if (use_wfe) {
            asm volatile("wfe");  // Wait for Event (low-power wait)
        } else {
            asm volatile("yield");  // Yield the processor (similar to x86 pause)
        }
    } else {
        if (use_wfe) {
            asm volatile("wfe");  // Wait for Event
        } else {
            asm volatile("yield");  // Yield the processor
        }
    }
}




#endif
