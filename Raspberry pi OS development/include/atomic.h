#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include "utils.h"
#include "printf.h"
// #include "init.h"
// #include "loop.h"

template <typename T>
class Atomic {
    volatile T value;

public:
    Atomic(T x) : value(x) {}
    Atomic<T>& operator=(T v) {
        __atomic_store_n(&value, v, __ATOMIC_SEQ_CST);
        return *this;
    }
    operator T() const {
        return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
    }
    T fetch_add(T inc) {
        return __atomic_fetch_add(&value, inc, __ATOMIC_SEQ_CST);
    }
    T add_fetch(T inc) {
        return __atomic_add_fetch(&value, inc, __ATOMIC_SEQ_CST);
    }
    void set(T inc) {
        __atomic_store_n(&value, inc, __ATOMIC_SEQ_CST);
    }
    T get(void) {
        return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
    }
    T exchange(T v) {
        return __atomic_exchange_n(&value, v, __ATOMIC_SEQ_CST);
    }
    volatile T* get_value_ptr() {
        return &value;
    }
};

#endif
