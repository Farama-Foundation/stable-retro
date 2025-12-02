  .text
  .arm
  .balign 4

  .func ctr_clear_cache_kernel
ctr_clear_cache_kernel:
  @ this less than what B2.7.3 of DDI0100I_ARM_ARM recommends, but so is Linux
  mrs r3, cpsr
  cpsid aif
  mov r0, #0
  mcr p15, 0, r0, c7, c10, 0    @ Clean entire data cache
  mcr p15, 0, r0, c7, c10, 4    @ Data Sync Barrier
  mcr p15, 0, r0, c7, c5, 0     @ Invalidate entire instruction cache / Flush BTB
  msr cpsr, r3
  bx lr
  .endfunc

  .func ctr_clear_cache_range_kernel
ctr_clear_cache_range_kernel:
  bic r0, r0, #31
  mov r12, r0
  mov r2, #0
  mrs r3, cpsr
  cpsid aif
0:
  mcr p15, 0, r0, c7, c10, 1    @ Clean Data Cache Line (using MVA)
  add r0, r0, #32
  cmp r0, r1
  blo 0b
  mcr p15, 0, r2, c7, c10, 4    @ Data Sync Barrier
  mov r0, r12
0:
  mcr p15, 0, r0, c7, c5, 1     @ Invalidate Instruction Cache Line (using MVA)
  add r0, r0, #32
  cmp r0, r1
  blo 0b
  mcr p15, 0, r2, c7, c5, 6     @ Flush Entire Branch Target Cache

  msr cpsr, r3
  bx lr
  .endfunc

  @@ Clear the entire data cache / invalidate the instruction cache. Uses
  @@ Rosalina svcCustomBackdoor to avoid svcBackdoor stack corruption
  @@ during interrupts.
  .global ctr_clear_cache
  .func ctr_clear_cache
ctr_clear_cache:
  adr r0, ctr_clear_cache_kernel
  svc 0x80                      @ svcCustomBackdoor
  bx lr
  .endfunc

  .global ctr_clear_cache_range
  .func ctr_clear_cache_range
ctr_clear_cache_range:
  mov r2, r1
  mov r1, r0
  adr r0, ctr_clear_cache_range_kernel
  svc 0x80                      @ svcCustomBackdoor
  bx lr
  .endfunc

  .func ctr_invalidate_icache_kernel
ctr_invalidate_icache_kernel:
  mrs r3, cpsr
  cpsid aif
  mov r0, #0
  mcr p15, 0, r0, c7, c10, 4    @ Data Sync Barrier
  mcr p15, 0, r0, c7, c5, 0     @ Invalidate entire instruction cache / Flush BTB
  msr cpsr, r3
  bx lr
  .endfunc

  .global ctr_invalidate_icache
  .func ctr_invalidate_icache
ctr_invalidate_icache:
  adr r0, ctr_invalidate_icache_kernel
  svc 0x80                      @ svcCustomBackdoor
  bx lr
  .endfunc

  .global svcCustomBackdoor
  .func svcCustomBackdoor
svcCustomBackdoor:
  svc 0x80                      @ svcCustomBackdoor
  bx lr
  .endfunc

  .global svcConvertVAToPA
  .func svcConvertVAToPA
svcConvertVAToPA:
  svc 0x90                      @ svcConvertVAToPA
  bx lr
  .endfunc
