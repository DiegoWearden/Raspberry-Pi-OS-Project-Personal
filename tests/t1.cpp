#include "printf.h"
#include "critical.h"

static uint32_t counter = 0;

/* Called by all cores */
void kernelMain(void) {
    // The C++ way, use closures (C++ and Python call them lambdas)
    critical([] {
        printf("*** hello %d\n",counter);
        printf("*** goodbye %d\n",counter);
	counter++;
    });
}
