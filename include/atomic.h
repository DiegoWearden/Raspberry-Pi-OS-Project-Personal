#ifndef _ATOMIC_H_
#define _ATOMIC_H_
#include "stdint.h"
#include "utils.h"
#include "printf.h"
#include "loop.h"

template <typename T>
class AtomicPtr {
    volatile T *ptr;
public:
    AtomicPtr() : ptr(nullptr) {}
    AtomicPtr(T *x) : ptr(x) {}
    AtomicPtr<T>& operator= (T v) {
        __atomic_store_n(ptr,v,__ATOMIC_SEQ_CST);
        return *this;
    }
    operator T () const {
        return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
    }
    T fetch_add(T inc) {
        return __atomic_fetch_add(ptr,inc,__ATOMIC_SEQ_CST);
    }
    T add_fetch(T inc) {
        return __atomic_add_fetch(ptr,inc,__ATOMIC_SEQ_CST);
    }
    void set(T inc) {
        return __atomic_store_n(ptr,inc,__ATOMIC_SEQ_CST);
    }
    T get(void) {
        return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
    }
    T exchange(T v) {
        T ret;
        __atomic_exchange(ptr,&v,&ret,__ATOMIC_SEQ_CST);
        return ret;
    }
};

template <typename T>
class Atomic {
    volatile T value;
public:
    Atomic(T x) : value(x) {}
    Atomic<T>& operator= (T v) {
        __atomic_store_n(&value,v,__ATOMIC_SEQ_CST);
        return *this;
    }
    operator T () const {
        return __atomic_load_n(&value,__ATOMIC_SEQ_CST);
    }
    T fetch_add(T inc) {
        return __atomic_fetch_add(&value,inc,__ATOMIC_SEQ_CST);
    }
    T add_fetch(T inc) {
        return __atomic_add_fetch(&value,inc,__ATOMIC_SEQ_CST);
    }
    void set(T inc) {
        return __atomic_store_n(&value,inc,__ATOMIC_SEQ_CST);
    }
    T get(void) {
        return __atomic_load_n(&value,__ATOMIC_SEQ_CST);
    }
    T exchange(T v) {
        T ret;
        // printf_no_lock("thing\n");
        __atomic_exchange(&value,&v,&ret,__ATOMIC_SEQ_CST);
        return ret;
    }
    void monitor_value() {
        monitor(reinterpret_cast<uintptr_t>(&value));
    }
};

template <>
class Atomic<uint64_t> {
public:
    Atomic() = delete;
    Atomic(uint64_t) = delete;
};

template <>
class Atomic<int64_t> {
public:
    Atomic() = delete;
    Atomic(int64_t) = delete;
};

template <typename T>
class LockGuard {
    T& it;
public:
    inline LockGuard(T& it): it(it) {
        it.lock();
    }
    inline ~LockGuard() {
        it.unlock();
    }
};

template <typename T>
class LockGuardP {
    T* it;
public:
    inline LockGuardP(T* it): it(it) {
        if (it) it->lock();
    }
    inline ~LockGuardP() {
        if (it) it->unlock();
    }
};

class SpinLock {
    Atomic<bool> taken;
public:
    SpinLock() : taken(false) {}

    SpinLock(const SpinLock&) = delete;

    // for debugging, etc. Allows false positives
    bool isMine() {
        return taken.get();
    }

    void lock(void) {
        taken.monitor_value();
        while (taken.exchange(true)) {
            iAmStuckInALoop(true);
            taken.monitor_value();
        }
    }
    
    void unlock(void) {
        taken.set(false);
    }
};

class Barrier {
    Atomic<uint32_t> counter;
public:
    Barrier(uint32_t counter): counter(counter) {}
    Barrier(const Barrier&) = delete;

    void sync() {
        counter.add_fetch(-1);
	while (counter.get() != 0) {
            iAmStuckInALoop(false);
        }
    }
};

#endif
