#include "printf.h"
#include "critical.h"
#include "atomic.h"

static uint32_t counter = 0;

void work() {
    printf("*** hello %d\n",counter);
    printf("*** goodbye %d\n",counter);
    counter ++;
}

/* Called by all cores */
void kernelMain(void) {
    // The C way, use function pointers
    critical(work);
}
