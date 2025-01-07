#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "atomic.h" // Include your Atomic class
#include "utils.h"  // Include any necessary utility functions
#include "printf.h" // For debugging, if needed

class Spinlock {
private:
    Atomic<int> theLock;

public:
    // Constructor
    Spinlock() : theLock(0) {}

    // Acquire the lock
    void lock() {
        while (theLock.exchange(1) != 0) {
        }
    }

    // Release the lock
    void unlock() {
        theLock.set(0); // Set the lock to 0, indicating it is free
    }

    // Try to acquire the lock (non-blocking)
    bool try_acquire() {
        return theLock.exchange(1) == 0; // Acquire lock if it was free
    }
};

#endif
