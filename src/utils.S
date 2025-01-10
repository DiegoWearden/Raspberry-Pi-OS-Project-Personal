.globl put32
put32:
    str w1, [x0]
	ret

.globl get32
get32:
	ldr w0,[x0]
	ret

.globl delay
delay:
	subs x0, x0, #1
	bne delay
	ret

.globl getCoreID
getCoreID:
    mrs x0, mpidr_el1       // Move the value of MPIDR_EL1 into x0
    and x0, x0, #0xFF       // Mask to get the lower 8 bits, which represent the core ID
    ret                     // Return from the function

.globl get_el
	get_el:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret


.global monitor
	monitor:
    dsb sy
    ldxr x0, [x0]
    wfe
    ret

// outb - ARMv8-A version for memory-mapped I/O
.global outb
.type outb, %function

outb:
    // x0 = port (address), x1 = value
    strb w1, [x0]       // Store byte (8 bits) from w1 into memory address in x0
    ret






