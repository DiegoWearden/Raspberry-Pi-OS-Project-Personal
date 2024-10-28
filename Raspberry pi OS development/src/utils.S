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


.globl atomic_exchange
atomic_exchange:
    // Input: x0 = pointer to the integer, x1 = new value
    // Output: w0 = old value

    mov     w3, wzr         // Clear x3 (to use as a flag for successful store)
1:  ldaxr   w2, [x0]        // Load the current value from the address pointed by x0 into w2
    stxr   w3, w1, [x0]    // Attempt to store w1 (new value) to [x0], set w3 to 0 if successful
	cbnz w3, 1b
    mov     w0, w2          // Return the old value (in w2)
    ret                     // Return from the function






