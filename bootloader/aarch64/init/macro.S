#include <common/asm.h>

BEGIN_FUNC(get_current_el)
    mrs x0, currentel
    mov x1, #2
    lsr x0, x0, #2
    ret
END_FUNC(get_current_el)

BEGIN_FUNC(tlb_invalidate)
    dsb ishst
    tlbi vmalle1is
    dsb ish
    isb
    ret
END_FUNC(tlb_invalidate)


BEGIN_FUNC(early_put32)
    str w1,[x0]
    ret
END_FUNC(early_put32)

BEGIN_FUNC(early_get32)
    ldr w0,[x0]
    ret
END_FUNC(early_get32)

BEGIN_FUNC(delay)
	subs x0, x0, #1
	bne delay
	ret
END_FUNC(delay)

BEGIN_FUNC(get_sctlr_el2)
    mrs x0, sctlr_el2
    ret
END_FUNC(get_sctlr_el2)

BEGIN_FUNC(get_sctlr_el1)
    mrs x0, sctlr_el1
    ret
END_FUNC(get_sctlr_el1)
