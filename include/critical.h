#ifndef _CRITICAL_H_
#define _CRITICAL_H_

#include "atomic.h"
#include "mm.h"
// #include "smp.h"
// #include "debug.h"

extern Atomic<uint32_t> critical_depth;
extern Atomic<uint32_t> critical_owner;
extern SpinLock critical_section_lock;

template <typename Work>
inline void critical(Work work) {
    // uint32_t cpu_id = getCoreID();
    // if (critical_owner == cpu_id) {
    //     critical_depth.fetch_add(1);
    // } else {
    //     critical_section_lock.lock();
    //     critical_owner = cpu_id;
    //     critical_depth.fetch_add(1);
    // }
    critical_section_lock.lock();
    work();
    critical_section_lock.unlock();
    // if (critical_depth.add_fetch(-1) == 0) {
    //     critical_owner = uint32_t(-1);
    //     critical_section_lock.unlock();
    // }
}

#endif