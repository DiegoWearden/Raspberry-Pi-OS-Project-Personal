#include "heap.h"
#include "printf.h"
#include "stdint.h"
#include "atomic.h"

/* A first-fit heap */


namespace gheith {
static int64_t *array;
static int64_t len;
static int safe = 0;
static int avail = 0;
static SpinLock *theLock = nullptr;

void makeTaken(int i, int ints);
void makeAvail(int i, int ints);

int abs(int x) {
    if (x < 0) return -x; else return x;
}

int size(int i) {
    return abs(array[i]);
}

int headerFromFooter(int i) {
    return i - size(i) + 1;
}

int footerFromHeader(int i) {
    return i + size(i) - 1;
}
    
int sanity(int i) {
    if (safe) {
        if (i == 0) return 0;
        if ((i < 0) || (i >= len)) {
            panic("bad header index %d\n",i);
            return i;
        }
        int footer = footerFromHeader(i);
        if ((footer < 0) || (footer >= len)) {
            panic("bad footer index %d\n",footer);
            return i;
        }
        int hv = array[i];
        int fv = array[footer];
  
        if (hv != fv) {
            panic("bad block at %d, hv:%d fv:%d\n", i,hv,fv);
            return i;
        }
    }

    return i;
}

int left(int i) {
    return sanity(headerFromFooter(i-1));
}

int right(int i) {
    return sanity(i + size(i));
}

int next(int i) {
    return sanity(array[i+1]);
}

int prev(int i) {
    return sanity(array[i+2]);
}

void setNext(int i, int x) {
    array[i+1] = x;
}

void setPrev(int i, int x) {
    array[i+2] = x;
}

void remove(int i) {
    int prevIndex = prev(i);
    int nextIndex = next(i);

    if (prevIndex == 0) {
        /* at head */
        avail = nextIndex;
    } else {
        /* in the middle */
        setNext(prevIndex,nextIndex);
    }
    if (nextIndex != 0) {
        setPrev(nextIndex,prevIndex);
    }
}

void makeAvail(int i, int ints) {
    array[i] = ints;
    array[footerFromHeader(i)] = ints;    
    setNext(i,avail);
    setPrev(i,0);
    if (avail != 0) {
        setPrev(avail,i);
    }
    avail = i;
}

void makeTaken(int i, int ints) {
    array[i] = -ints;
    array[footerFromHeader(i)] = -ints;    
}

int isAvail(int i) {
    return array[i] > 0;
}

int isTaken(int i) {
    return array[i] < 0;
}
};

void heapInit(void* base, size_t bytes) {
    using namespace gheith;

    printf_no_lock("| heap range 0x%x 0x%x\n", (uint64_t)base, (uint64_t)base + bytes);

    // Cast to uint64_t pointer for 64-bit systems
    array = (int64_t*) base;
    len = bytes / sizeof(uint64_t); // Divide by 8 instead of 4 for 64-bit alignment

    // Initialize the heap: Reserved blocks at both ends
    makeTaken(0, 2);             // Mark the beginning as taken
    makeAvail(2, len - 4);       // The main heap space available
    makeTaken(len - 2, 2);       // Mark the end as taken

    theLock = new SpinLock();
}


void* malloc(size_t bytes) {
    using namespace gheith;

    // Return early for zero allocation
    if (bytes == 0) return nullptr;

    // Align size for 64-bit (8 bytes)
    int units = ((bytes + 7) / 8) + 2;  // Aligning to 8 bytes instead of 4
    if (units < 4) units = 4;  // Ensure a minimum block size
    
    //LockGuard needs mmu enabled in order to run correctly as it uses atomic operations
    if(coresAwoken){
        LockGuardP g{theLock};
    }

    void* res = nullptr;
    int minSize = 0x7FFFFFFF;
    int bestFitIndex = 0;

    {
        int attempts = 20;  // Prevent infinite loops
        int currentIndex = avail;
        while (currentIndex != 0) {
            if (!isAvail(currentIndex)) {
                panic("block is not available in malloc %p\n", currentIndex);
            }
            int blockSize = size(currentIndex);

            if (blockSize >= units && blockSize < minSize) {
                minSize = blockSize;
                bestFitIndex = currentIndex;
            }
            attempts--;
            if (attempts == 0) break;

            currentIndex = next(currentIndex);
        }
    }

    if (bestFitIndex != 0) {
        remove(bestFitIndex);
        int extra = minSize - units;
        
        // Split block if there's enough space left
        if (extra >= 4) {
            makeTaken(bestFitIndex, units);
            makeAvail(bestFitIndex + units, extra);
        } else {
            makeTaken(bestFitIndex, minSize);
        }

        // Return pointer to the allocated memory area
        res = reinterpret_cast<void*>(&array[bestFitIndex + 1]);
    }
    return res;
}


void free(void* p) {
    using namespace gheith;
    if (p == 0) return;
    if (p == (void*) array) return;

    if(coresAwoken){
        LockGuardP g{theLock};
    }

    int idx = ((((uintptr_t) p) - ((uintptr_t) array)) / 4) - 1;
    sanity(idx);
    if (!isTaken(idx)) {
        panic("freeing free block, p:%x idx:%d\n",(uint32_t) p,(int32_t) idx);
        return;
    }

    int sz = size(idx);

    int leftIndex = left(idx);
    int rightIndex = right(idx);

    if (isAvail(leftIndex)) {
        remove(leftIndex);
        idx = leftIndex;
        sz += size(leftIndex);
    }

    if (isAvail(rightIndex)) {
        remove(rightIndex);
        sz += size(rightIndex);
    }

    makeAvail(idx,sz);
}


/*****************/
/* C++ operators */
/*****************/

void* operator new(size_t size) {
    void* p =  malloc(size);
    if (p == 0) panic("out of memory");
    return p;
}

void operator delete(void* p) noexcept {
    return free(p);
}

void operator delete(void* p, size_t sz) {
    return free(p);
}

void* operator new[](size_t size) {
    void* p =  malloc(size);
    if (p == 0) panic("out of memory");
    return p;
}

void operator delete[](void* p) noexcept {
    return free(p);
}

void operator delete[](void* p, size_t sz) {
    return free(p);
}
