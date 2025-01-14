#ifndef _MMU_H
#define _MMU_H

#ifdef __cplusplus								// If we are including to a C++
extern "C" {									// Put extern C directive wrapper around
#endif
#include "stdint.h"								// Needed for uint8_t, uint32_t, etc
#include "rpi-SmartStart.h"						// Needed for RegType_t

#define MT_DEVICE_NGNRNE	0
#define MT_DEVICE_NGNRE		1
#define MT_DEVICE_GRE		2
#define MT_NORMAL_NC		3
#define MT_NORMAL		    4

/*-[ MMU_setup_pagetable ]--------------------------------------------------}
.  Sets up a default TLB table. This needs to be called by only once by one 
.  core on a multicore system. Each core can use the same default table.
.--------------------------------------------------------------------------*/
void MMU_setup_pagetable (void);

/*-[ MMU_enable ]-----------------------------------------------------------}
.  Enables the MMU system to the previously created TLB tables. This needs 
.  to be called by each individual core on a multicore system.
.--------------------------------------------------------------------------*/
void MMU_enable(void);


#if __aarch64__ == 1
RegType_t virtualmap (uint32_t phys_addr, uint8_t memattrs);
#endif

#ifdef __cplusplus								// If we are including to a C++ file
}												// Close the extern C directive wrapper
#endif

#endif
