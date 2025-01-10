#include "critical.h"

Atomic<uint32_t> critical_depth = 0;
Atomic<uint32_t> critical_owner = uint32_t(-1);
SpinLock critical_section_lock;