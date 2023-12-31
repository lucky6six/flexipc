#include <common/asm.h>

.macro dcache op                     
dsb     sy                       
mrs     x0, clidr_el1            
and     x3, x0, #0x7000000       
lsr     x3, x3, #23              

cbz     x3, finished_\op         
mov     x10, #0                  

loop1_\op:                           
add     x2, x10, x10, lsr #1     
lsr     x1, x0, x2               
and     x1, x1, #7               
cmp     x1, #2                   
b.lt    skip_\op                 

msr     csselr_el1, x10          
isb                              

mrs     x1, ccsidr_el1           
and     x2, x1, #7               
add     x2, x2, #4               
mov     x4, #0x3ff               
and     x4, x4, x1, lsr #3       
clz     w5, w4                   
mov     x7, #0x7fff              
and     x7, x7, x1, lsr #13      

loop2_\op:                           
mov     x9, x4                   

loop3_\op:                           
lsl     x6, x9, x5               
orr     x11, x10, x6             
lsl     x6, x7, x2               
orr     x11, x11, x6             
dc      \op, x11                 
subs    x9, x9, #1               
b.ge    loop3_\op                
subs    x7, x7, #1               
b.ge    loop2_\op                

skip_\op:                            
add     x10, x10, #2             
cmp     x3, x10                  
b.gt    loop1_\op                

finished_\op:                        
mov     x10, #0                  
msr     csselr_el1, x10          
dsb     sy                       
isb                              
.endm                                

BEGIN_FUNC(flush_idcache)
	ic      iallu
	dsb     nsh
	isb 
	dcache  cisw
	ret
END_FUNC(flush_idcache)

BEGIN_FUNC(set_ttbr0_el1)
	msr ttbr0_el1, x0
	dsb sy
	tlbi vmalle1is
	dsb sy
	isb
	ret
END_FUNC(set_ttbr0_el1)

/*
 * TODO: this is a simple implementation for now.
 * Note that tlbi is TLB invalidation instruction and vmalle1is is its arguement.
 * This function can flush all the TLB on all the (inner sharable) cores.
 * Different from x86 architecture, IPI is not required.
 */
BEGIN_FUNC(flush_tlb)
	tlbi vmalle1is
	dsb sy
	isb
	ret
END_FUNC(flush_tlb)


