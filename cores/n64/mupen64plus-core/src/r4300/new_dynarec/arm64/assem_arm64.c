/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - assem_arm64.c                                           *
 *   Copyright (C) 2009-2011 Ari64                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

typedef enum {
  EQ,
  NE,
  CS,
  CC,
  MI,
  PL,
  VS,
  VC,
  HI,
  LS,
  GE,
  LT,
  GT,
  LE,
  AW,
  NV
} eCond;

static void *dynamic_linker(void * src, u_int vaddr);
static void *dynamic_linker_ds(void * src, u_int vaddr);
static void invalidate_addr(u_int addr);

static unsigned int needs_clear_cache[1<<(TARGET_SIZE_2-17)];

static const uintptr_t jump_vaddr_reg[32] = {
  (intptr_t)jump_vaddr_x0,
  (intptr_t)jump_vaddr_x1,
  (intptr_t)jump_vaddr_x2,
  (intptr_t)jump_vaddr_x3,
  (intptr_t)jump_vaddr_x4,
  (intptr_t)jump_vaddr_x5,
  (intptr_t)jump_vaddr_x6,
  (intptr_t)jump_vaddr_x7,
  (intptr_t)jump_vaddr_x8,
  (intptr_t)jump_vaddr_x9,
  (intptr_t)jump_vaddr_x10,
  (intptr_t)jump_vaddr_x11,
  (intptr_t)jump_vaddr_x12,
  (intptr_t)jump_vaddr_x13,
  (intptr_t)jump_vaddr_x14,
  (intptr_t)jump_vaddr_x15,
  (intptr_t)jump_vaddr_x16,
  (intptr_t)jump_vaddr_x17,
  (intptr_t)jump_vaddr_x18,
  (intptr_t)jump_vaddr_x19,
  (intptr_t)jump_vaddr_x20,
  (intptr_t)jump_vaddr_x21,
  (intptr_t)jump_vaddr_x22,
  (intptr_t)jump_vaddr_x23,
  (intptr_t)jump_vaddr_x24,
  (intptr_t)jump_vaddr_x25,
  (intptr_t)jump_vaddr_x26,
  (intptr_t)jump_vaddr_x27,
  (intptr_t)jump_vaddr_x28,
  (intptr_t)breakpoint,
  (intptr_t)breakpoint,
  (intptr_t)breakpoint};

static const uintptr_t invalidate_addr_reg[32] = {
  (intptr_t)invalidate_addr_x0,
  (intptr_t)invalidate_addr_x1,
  (intptr_t)invalidate_addr_x2,
  (intptr_t)invalidate_addr_x3,
  (intptr_t)invalidate_addr_x4,
  (intptr_t)invalidate_addr_x5,
  (intptr_t)invalidate_addr_x6,
  (intptr_t)invalidate_addr_x7,
  (intptr_t)invalidate_addr_x8,
  (intptr_t)invalidate_addr_x9,
  (intptr_t)invalidate_addr_x10,
  (intptr_t)invalidate_addr_x11,
  (intptr_t)invalidate_addr_x12,
  (intptr_t)invalidate_addr_x13,
  (intptr_t)invalidate_addr_x14,
  (intptr_t)invalidate_addr_x15,
  (intptr_t)invalidate_addr_x16,
  (intptr_t)invalidate_addr_x17,
  (intptr_t)invalidate_addr_x18,
  (intptr_t)invalidate_addr_x19,
  (intptr_t)invalidate_addr_x20,
  (intptr_t)invalidate_addr_x21,
  (intptr_t)invalidate_addr_x22,
  (intptr_t)invalidate_addr_x23,
  (intptr_t)invalidate_addr_x24,
  (intptr_t)invalidate_addr_x25,
  (intptr_t)invalidate_addr_x26,
  (intptr_t)invalidate_addr_x27,
  (intptr_t)invalidate_addr_x28,
  (intptr_t)breakpoint,
  (intptr_t)breakpoint,
  (intptr_t)breakpoint};

static uintptr_t jump_table_symbols[] = {
  (intptr_t)invalidate_addr,
  (intptr_t)jump_vaddr,
  (intptr_t)dynamic_linker,
  (intptr_t)dynamic_linker_ds,
  (intptr_t)verify_code,
  (intptr_t)verify_code_vm,
  (intptr_t)verify_code_ds,
  (intptr_t)cc_interrupt,
  (intptr_t)fp_exception,
  (intptr_t)fp_exception_ds,
  (intptr_t)jump_syscall,
  (intptr_t)jump_eret,
  (intptr_t)indirect_jump_indexed,
  (intptr_t)indirect_jump,
  (intptr_t)do_interrupt,
  (intptr_t)NULL /*MFC0*/,
  (intptr_t)NULL /*MTC0*/,
  (intptr_t)NULL /*TLBR*/,
  (intptr_t)NULL /*TLBP*/,
  (intptr_t)TLBWI_new,
  (intptr_t)TLBWR_new,
  (intptr_t)jump_vaddr_x0,
  (intptr_t)jump_vaddr_x1,
  (intptr_t)jump_vaddr_x2,
  (intptr_t)jump_vaddr_x3,
  (intptr_t)jump_vaddr_x4,
  (intptr_t)jump_vaddr_x5,
  (intptr_t)jump_vaddr_x6,
  (intptr_t)jump_vaddr_x7,
  (intptr_t)jump_vaddr_x8,
  (intptr_t)jump_vaddr_x9,
  (intptr_t)jump_vaddr_x10,
  (intptr_t)jump_vaddr_x11,
  (intptr_t)jump_vaddr_x12,
  (intptr_t)jump_vaddr_x13,
  (intptr_t)jump_vaddr_x14,
  (intptr_t)jump_vaddr_x15,
  (intptr_t)jump_vaddr_x16,
  (intptr_t)jump_vaddr_x17,
  (intptr_t)jump_vaddr_x18,
  (intptr_t)jump_vaddr_x19,
  (intptr_t)jump_vaddr_x20,
  (intptr_t)jump_vaddr_x21,
  (intptr_t)jump_vaddr_x22,
  (intptr_t)jump_vaddr_x23,
  (intptr_t)jump_vaddr_x24,
  (intptr_t)jump_vaddr_x25,
  (intptr_t)jump_vaddr_x26,
  (intptr_t)jump_vaddr_x27,
  (intptr_t)jump_vaddr_x28,
  (intptr_t)invalidate_addr_x0,
  (intptr_t)invalidate_addr_x1,
  (intptr_t)invalidate_addr_x2,
  (intptr_t)invalidate_addr_x3,
  (intptr_t)invalidate_addr_x4,
  (intptr_t)invalidate_addr_x5,
  (intptr_t)invalidate_addr_x6,
  (intptr_t)invalidate_addr_x7,
  (intptr_t)invalidate_addr_x8,
  (intptr_t)invalidate_addr_x9,
  (intptr_t)invalidate_addr_x10,
  (intptr_t)invalidate_addr_x11,
  (intptr_t)invalidate_addr_x12,
  (intptr_t)invalidate_addr_x13,
  (intptr_t)invalidate_addr_x14,
  (intptr_t)invalidate_addr_x15,
  (intptr_t)invalidate_addr_x16,
  (intptr_t)invalidate_addr_x17,
  (intptr_t)invalidate_addr_x18,
  (intptr_t)invalidate_addr_x19,
  (intptr_t)invalidate_addr_x20,
  (intptr_t)invalidate_addr_x21,
  (intptr_t)invalidate_addr_x22,
  (intptr_t)invalidate_addr_x23,
  (intptr_t)invalidate_addr_x24,
  (intptr_t)invalidate_addr_x25,
  (intptr_t)invalidate_addr_x26,
  (intptr_t)invalidate_addr_x27,
  (intptr_t)invalidate_addr_x28,
  (intptr_t)div64,
  (intptr_t)divu64,
  (intptr_t)cvt_s_w,
  (intptr_t)cvt_d_w,
  (intptr_t)cvt_s_l,
  (intptr_t)cvt_d_l,
  (intptr_t)cvt_w_s,
  (intptr_t)cvt_w_d,
  (intptr_t)cvt_l_s,
  (intptr_t)cvt_l_d,
  (intptr_t)cvt_d_s,
  (intptr_t)cvt_s_d,
  (intptr_t)round_l_s,
  (intptr_t)round_w_s,
  (intptr_t)trunc_l_s,
  (intptr_t)trunc_w_s,
  (intptr_t)ceil_l_s,
  (intptr_t)ceil_w_s,
  (intptr_t)floor_l_s,
  (intptr_t)floor_w_s,
  (intptr_t)round_l_d,
  (intptr_t)round_w_d,
  (intptr_t)trunc_l_d,
  (intptr_t)trunc_w_d,
  (intptr_t)ceil_l_d,
  (intptr_t)ceil_w_d,
  (intptr_t)floor_l_d,
  (intptr_t)floor_w_d,
  (intptr_t)c_f_s,
  (intptr_t)c_un_s,
  (intptr_t)c_eq_s,
  (intptr_t)c_ueq_s,
  (intptr_t)c_olt_s,
  (intptr_t)c_ult_s,
  (intptr_t)c_ole_s,
  (intptr_t)c_ule_s,
  (intptr_t)c_sf_s,
  (intptr_t)c_ngle_s,
  (intptr_t)c_seq_s,
  (intptr_t)c_ngl_s,
  (intptr_t)c_lt_s,
  (intptr_t)c_nge_s,
  (intptr_t)c_le_s,
  (intptr_t)c_ngt_s,
  (intptr_t)c_f_d,
  (intptr_t)c_un_d,
  (intptr_t)c_eq_d,
  (intptr_t)c_ueq_d,
  (intptr_t)c_olt_d,
  (intptr_t)c_ult_d,
  (intptr_t)c_ole_d,
  (intptr_t)c_ule_d,
  (intptr_t)c_sf_d,
  (intptr_t)c_ngle_d,
  (intptr_t)c_seq_d,
  (intptr_t)c_ngl_d,
  (intptr_t)c_lt_d,
  (intptr_t)c_nge_d,
  (intptr_t)c_le_d,
  (intptr_t)c_ngt_d,
  (intptr_t)add_s,
  (intptr_t)sub_s,
  (intptr_t)mul_s,
  (intptr_t)div_s,
  (intptr_t)sqrt_s,
  (intptr_t)abs_s,
  (intptr_t)mov_s,
  (intptr_t)neg_s,
  (intptr_t)add_d,
  (intptr_t)sub_d,
  (intptr_t)mul_d,
  (intptr_t)div_d,
  (intptr_t)sqrt_d,
  (intptr_t)abs_d,
  (intptr_t)mov_d,
  (intptr_t)neg_d
};

/* Linker */

static void set_jump_target(intptr_t addr,uintptr_t target)
{
  u_char *ptr=(u_char *)addr;
  u_int *ptr2=(u_int *)ptr;

  int offset=target-(intptr_t)addr;

  if((ptr[3]&0xfc)==0x14) {
    assert(offset>=-134217728&&offset<134217728);
    *ptr2=(*ptr2&0xFC000000)|(((u_int)offset>>2)&0x3ffffff);
  }
  else if((ptr[3]&0xff)==0x54) {
    //Conditional branch are limited to +/- 1MB
    //block max size is 256k so branching beyond the +/- 1MB limit
    //should only happen when jumping to an already compiled block (see add_link)
    //a workaround would be to do a trampoline jump via a stub at the end of the block
    assert(offset>=-1048576&&offset<1048576);
    *ptr2=(*ptr2&0xFF00000F)|((((u_int)offset>>2)&0x7ffff)<<5);
  }
  else if((ptr[3]&0x9f)==0x10) { //adr
    //generated by do_miniht_insert
    assert(offset>=-1048576&&offset<1048576);
    *ptr2=(*ptr2&0x9F00001F)|((u_int)offset&0x3)<<29|(((u_int)offset>>2)&0x7ffff)<<5;
  }
  else
    assert(0); /*Should not happen*/
}

static void *dynamic_linker(void * src, u_int vaddr)
{
  u_int page=(vaddr^0x80000000)>>12;
  u_int vpage=page;
  if(page>262143&&tlb_LUT_r[vaddr>>12]) page=(tlb_LUT_r[vaddr>>12]^0x80000000)>>12;
  if(page>2048) page=2048+(page&2047);
  if(vpage>262143&&tlb_LUT_r[vaddr>>12]) vpage&=2047; // jump_dirty uses a hash of the virtual address instead
  if(vpage>2048) vpage=2048+(vpage&2047);
  struct ll_entry *head;
  head=jump_in[page];

  while(head!=NULL) {
    if(head->vaddr==vaddr&&head->reg32==0) {
      int *ptr=(int*)src;
      assert(((*ptr&0xfc000000)==0x14000000)||((*ptr&0xff000000)==0x54000000)); //b or b.cond
      //TOBEDONE: Avoid disabling link between blocks for conditional branches
      if((*ptr&0xfc000000)==0x14000000) { //b
        int offset=((signed int)(*ptr<<6)>>6)<<2;
        u_int *ptr2=(u_int*)((intptr_t)ptr+offset);
        assert((ptr2[0]&0xffe00000)==0x52a00000); //movz
        assert((ptr2[1]&0xffe00000)==0x72800000); //movk
        assert((ptr2[2]&0x9f000000)==0x10000000); //adr
        //assert((ptr2[3]&0xfc000000)==0x94000000); //bl
        //assert((ptr2[4]&0xfffffc1f)==0xd61f0000); //br
        add_link(vaddr, ptr2);
        set_jump_target((intptr_t)ptr, (uintptr_t)head->addr);
        __clear_cache((void*)ptr, (void*)((uintptr_t)ptr+4));
      }
      #ifdef NEW_DYNAREC_DEBUG
      print_debug_info(vaddr);
      #endif
      return head->addr;
    }
    head=head->next;
  }

  uintptr_t *ht_bin=hash_table[((vaddr>>16)^vaddr)&0xFFFF];
  if(ht_bin[0]==vaddr){
    #ifdef NEW_DYNAREC_DEBUG
    print_debug_info(vaddr);
    #endif
    return (void *)ht_bin[1];
  }
  if(ht_bin[2]==vaddr){
    #ifdef NEW_DYNAREC_DEBUG
    print_debug_info(vaddr);
    #endif
    return (void *)ht_bin[3];
  }

  head=jump_dirty[vpage];
  while(head!=NULL) {
    if(head->vaddr==vaddr&&head->reg32==0) {
      //DebugMessage(M64MSG_VERBOSE, "TRACE: count=%d next=%d (get_addr match dirty %x: %x)",g_cp0_regs[CP0_COUNT_REG],next_interrupt,vaddr,(int)head->addr);
      // Don't restore blocks which are about to expire from the cache
      if((((uintptr_t)head->addr-(uintptr_t)out)<<(32-TARGET_SIZE_2))>0x60000000+(MAX_OUTPUT_BLOCK_SIZE<<(32-TARGET_SIZE_2))) {
        if(verify_dirty(head->addr)) {
          //DebugMessage(M64MSG_VERBOSE, "restore candidate: %x (%d) d=%d",vaddr,page,invalid_code[vaddr>>12]);
          invalid_code[vaddr>>12]=0;
          memory_map[vaddr>>12]|=WRITE_PROTECT;
          if(vpage<2048) {
            if(tlb_LUT_r[vaddr>>12]) {
              invalid_code[tlb_LUT_r[vaddr>>12]>>12]=0;
              memory_map[tlb_LUT_r[vaddr>>12]>>12]|=WRITE_PROTECT;
            }
            restore_candidate[vpage>>3]|=1<<(vpage&7);
          }
          else restore_candidate[page>>3]|=1<<(page&7);
          uintptr_t *ht_bin=hash_table[((vaddr>>16)^vaddr)&0xFFFF];
          if(ht_bin[0]==vaddr) {
            ht_bin[1]=(intptr_t)head->addr; // Replace existing entry
          }
          else
          {
            ht_bin[3]=ht_bin[1];
            ht_bin[2]=ht_bin[0];
            ht_bin[1]=(intptr_t)head->addr;
            ht_bin[0]=vaddr;
          }
          #ifdef NEW_DYNAREC_DEBUG
          print_debug_info(vaddr);
          #endif
          return head->addr;
        }
      }
    }
    head=head->next;
  }

  int r=new_recompile_block(vaddr);
  if(r==0) return dynamic_linker(src, vaddr);
  // Execute in unmapped page, generate pagefault exception
  g_cp0_regs[CP0_STATUS_REG]|=2;
  g_cp0_regs[CP0_CAUSE_REG]=0x8;
  g_cp0_regs[CP0_EPC_REG]=vaddr;
  g_cp0_regs[CP0_BADVADDR_REG]=vaddr;
  g_cp0_regs[CP0_CONTEXT_REG]=(g_cp0_regs[CP0_CONTEXT_REG]&0xFF80000F)|((g_cp0_regs[CP0_BADVADDR_REG]>>9)&0x007FFFF0);
  g_cp0_regs[CP0_ENTRYHI_REG]=g_cp0_regs[CP0_BADVADDR_REG]&0xFFFFE000;
  return get_addr_ht(0x80000000);
}

static void *dynamic_linker_ds(void * src, u_int vaddr)
{
  u_int page=(vaddr^0x80000000)>>12;
  u_int vpage=page;
  if(page>262143&&tlb_LUT_r[vaddr>>12]) page=(tlb_LUT_r[vaddr>>12]^0x80000000)>>12;
  if(page>2048) page=2048+(page&2047);
  if(vpage>262143&&tlb_LUT_r[vaddr>>12]) vpage&=2047; // jump_dirty uses a hash of the virtual address instead
  if(vpage>2048) vpage=2048+(vpage&2047);
  struct ll_entry *head;
  head=jump_in[page];

  while(head!=NULL) {
    if(head->vaddr==vaddr&&head->reg32==0) {
      int *ptr=(int*)src;
      assert(((*ptr&0xfc000000)==0x14000000)||((*ptr&0xff000000)==0x54000000)); //b or b.cond
      //TOBEDONE: Avoid disabling link between blocks for conditional branches
      if((*ptr&0xfc000000)==0x14000000) { //b
        int offset=((signed int)(*ptr<<6)>>6)<<2;
        u_int *ptr2=(u_int*)((intptr_t)ptr+offset);
        assert((ptr2[0]&0xffe00000)==0x52a00000); //movz
        assert((ptr2[1]&0xffe00000)==0x72800000); //movk
        assert((ptr2[2]&0x9f000000)==0x10000000); //adr
        //assert((ptr2[3]&0xfc000000)==0x94000000); //bl
        //assert((ptr2[4]&0xfffffc1f)==0xd61f0000); //br
        add_link(vaddr, ptr2);
        set_jump_target((intptr_t)ptr, (uintptr_t)head->addr);
        __clear_cache((void*)ptr, (void*)((uintptr_t)ptr+4));
      }
      #ifdef NEW_DYNAREC_DEBUG
      print_debug_info(vaddr);
      #endif
      return head->addr;
    }
    head=head->next;
  }

  uintptr_t *ht_bin=hash_table[((vaddr>>16)^vaddr)&0xFFFF];
  if(ht_bin[0]==vaddr){
    #ifdef NEW_DYNAREC_DEBUG
    print_debug_info(vaddr);
    #endif
    return (void *)ht_bin[1];
  }
  if(ht_bin[2]==vaddr){
    #ifdef NEW_DYNAREC_DEBUG
    print_debug_info(vaddr);
    #endif
    return (void *)ht_bin[3];
  }

  head=jump_dirty[vpage];
  while(head!=NULL) {
    if(head->vaddr==vaddr&&head->reg32==0) {
      //DebugMessage(M64MSG_VERBOSE, "TRACE: count=%d next=%d (get_addr match dirty %x: %x)",g_cp0_regs[CP0_COUNT_REG],next_interrupt,vaddr,(int)head->addr);
      // Don't restore blocks which are about to expire from the cache
      if((((uintptr_t)head->addr-(uintptr_t)out)<<(32-TARGET_SIZE_2))>0x60000000+(MAX_OUTPUT_BLOCK_SIZE<<(32-TARGET_SIZE_2))) {
        if(verify_dirty(head->addr)) {
          //DebugMessage(M64MSG_VERBOSE, "restore candidate: %x (%d) d=%d",vaddr,page,invalid_code[vaddr>>12]);
          invalid_code[vaddr>>12]=0;
          memory_map[vaddr>>12]|=WRITE_PROTECT;
          if(vpage<2048) {
            if(tlb_LUT_r[vaddr>>12]) {
              invalid_code[tlb_LUT_r[vaddr>>12]>>12]=0;
              memory_map[tlb_LUT_r[vaddr>>12]>>12]|=WRITE_PROTECT;
            }
            restore_candidate[vpage>>3]|=1<<(vpage&7);
          }
          else restore_candidate[page>>3]|=1<<(page&7);
          uintptr_t *ht_bin=hash_table[((vaddr>>16)^vaddr)&0xFFFF];
          if(ht_bin[0]==vaddr) {
            ht_bin[1]=(intptr_t)head->addr; // Replace existing entry
          }
          else
          {
            ht_bin[3]=ht_bin[1];
            ht_bin[2]=ht_bin[0];
            ht_bin[1]=(intptr_t)head->addr;
            ht_bin[0]=vaddr;
          }
          #ifdef NEW_DYNAREC_DEBUG
          print_debug_info(vaddr);
          #endif
          return head->addr;
        }
      }
    }
    head=head->next;
  }

  int r=new_recompile_block((vaddr&0xFFFFFFF8)+1);
  if(r==0) return dynamic_linker_ds(src, vaddr);
  // Execute in unmapped page, generate pagefault exception
  g_cp0_regs[CP0_STATUS_REG]|=2;
  g_cp0_regs[CP0_CAUSE_REG]=0x80000008;
  g_cp0_regs[CP0_EPC_REG]=(vaddr&0xFFFFFFF8)-4;
  g_cp0_regs[CP0_BADVADDR_REG]=vaddr&0xFFFFFFF8;
  g_cp0_regs[CP0_CONTEXT_REG]=(g_cp0_regs[CP0_CONTEXT_REG]&0xFF80000F)|((g_cp0_regs[CP0_BADVADDR_REG]>>9)&0x007FFFF0);
  g_cp0_regs[CP0_ENTRYHI_REG]=g_cp0_regs[CP0_BADVADDR_REG]&0xFFFFE000;
  return get_addr_ht(0x80000000);
}

static void *kill_pointer(void *stub)
{
  int *ptr=(int *)((intptr_t)stub+8);
  assert((*ptr&0x9f000000)==0x10000000); //adr
  int offset=(((signed int)(*ptr<<8)>>13)<<2)|((*ptr>>29)&0x3);
  int *i_ptr=(int*)((intptr_t)ptr+offset);
  assert((*i_ptr&0xfc000000)==0x14000000); //b
  set_jump_target((intptr_t)i_ptr,(intptr_t)stub);
  return i_ptr;
}

static intptr_t get_pointer(void *stub)
{
  int *ptr=(int *)((intptr_t)stub+8);
  assert((*ptr&0x9f000000)==0x10000000); //adr
  int offset=(((signed int)(*ptr<<8)>>13)<<2)|((*ptr>>29)&0x3);
  int *i_ptr=(int*)((intptr_t)ptr+offset);
  assert((*i_ptr&0xfc000000)==0x14000000); //b
  return (intptr_t)i_ptr+(((signed int)(*i_ptr<<6)>>6)<<2);
}

// Find the "clean" entry point from a "dirty" entry point
// by skipping past the call to verify_code
static uintptr_t get_clean_addr(intptr_t addr)
{
  int *ptr=(int *)addr;
  while((*ptr&0xfc000000)!=0x94000000){ //bl
    ptr++;
    assert(((uintptr_t)ptr-(uintptr_t)addr)<=0x1C);
  }
  ptr++;
  if((*ptr&0xfc000000)==0x14000000) { //b
    return (intptr_t)ptr+(((signed int)(*ptr<<6)>>6)<<2); // follow branch
  }
  return (uintptr_t)ptr;
}

static int verify_dirty(void *addr)
{
  u_int *ptr=(u_int *)addr;

  uintptr_t source=0;
  if((*ptr&0xffe00000)==0x52a00000){ //movz
    assert((ptr[1]&0xffe00000)==0x72800000); //movk
    source=(((ptr[0]>>5)&0xffff)<<16)|((ptr[1]>>5)&0xffff);
    ptr+=2;
  }
  else if((*ptr&0x9f000000)==0x10000000){ //adr
    int offset=(((signed int)(*ptr<<8)>>13)<<2)|((*ptr>>29)&0x3);
    source=(intptr_t)ptr+offset;
    ptr++;
  }
  else if((*ptr&0x9f000000)==0x90000000){ //adrp
    int offset=(((signed int)(*ptr<<8)>>13)<<2)|((*ptr>>29)&0x3);
    source=((intptr_t)ptr&(intptr_t)(~0xfff))+((intptr_t)offset<<12);
    ptr++;
    if((*ptr&0xff000000)==0x91000000){//add
      source|=(*ptr>>10)&0xfff;
      ptr++;
    }
  }
  else
    assert(0); /*Should not happen*/

  uintptr_t copy=0;
  if((*ptr&0x9f000000)==0x10000000){ //adr
    int offset=(((signed int)(*ptr<<8)>>13)<<2)|((*ptr>>29)&0x3);
    copy=(intptr_t)ptr+offset;
    ptr++;
  }
  else if((*ptr&0x9f000000)==0x90000000){ //adrp
    int offset=(((signed int)(*ptr<<8)>>13)<<2)|((*ptr>>29)&0x3);
    copy=((intptr_t)ptr&(intptr_t)(~0xfff))+((intptr_t)offset<<12);
    ptr++;
    if((*ptr&0xff000000)==0x91000000){//add
      copy|=(*ptr>>10)&0xfff;
      ptr++;
    }
  }
  else
    assert(0); /*Should not happen*/

  assert((*ptr&0xffe00000)==0x52800000); //movz
  u_int len=(*ptr>>5)&0xffff;
  ptr+=2;

  if((*ptr&0xfc000000)!=0x94000000) ptr++;
  assert((*ptr&0xfc000000)==0x94000000); // bl instruction

  uintptr_t verifier=((signed int)(*ptr<<6)>>4)+(intptr_t)ptr;
  assert(verifier==(uintptr_t)verify_code||verifier==(uintptr_t)verify_code_vm||verifier==(uintptr_t)verify_code_ds);

  if(verifier==(uintptr_t)verify_code_vm||verifier==(uintptr_t)verify_code_ds) {
    unsigned int page=(u_int)source>>12;
    uint64_t map_value=memory_map[page];
    if(map_value>=((uintptr_t)1<<((sizeof(uintptr_t)<<3)-1))) { /*mapping=-1?*/
      return 0;
    }
    while(page<(((u_int)source+len-1)>>12)) {
      if((memory_map[++page]<<2)!=(map_value<<2)) return 0;
    }
    source = source+(map_value<<2);
  }
  //DebugMessage(M64MSG_VERBOSE, "verify_dirty: %x %x %x",source,copy,len);
  return !memcmp((void *)source,(void *)copy,len);
}

// This doesn't necessarily find all clean entry points, just
// guarantees that it's not dirty
static int isclean(intptr_t addr)
{
  int *ptr=(int *)addr;
  while((*ptr&0xfc000000)!=0x94000000){ //bl
    ptr++;
    if(((uintptr_t)ptr-(uintptr_t)addr)>0x1C)
      return 1;
  }
  uintptr_t verifier=(intptr_t)ptr+(((signed int)(*ptr<<6)>>6)<<2); // follow branch
  if(verifier==(uintptr_t)verify_code) return 0;
  if(verifier==(uintptr_t)verify_code_vm) return 0;
  if(verifier==(uintptr_t)verify_code_ds) return 0;
  return 1;
}

static void get_bounds(intptr_t addr,uintptr_t *start,uintptr_t *end)
{
  u_int *ptr=(u_int *)addr;

  uintptr_t source=0;
  if((*ptr&0xffe00000)==0x52a00000){ //movz
    assert((ptr[1]&0xffe00000)==0x72800000); //movk
    source=(((ptr[0]>>5)&0xffff)<<16)|((ptr[1]>>5)&0xffff);
    ptr+=2;
  }
  else if((*ptr&0x9f000000)==0x10000000){ //adr
    int offset=(((signed int)(*ptr<<8)>>13)<<2)|((*ptr>>29)&0x3);
    source=(intptr_t)ptr+offset;
    ptr++;
  }
  else if((*ptr&0x9f000000)==0x90000000){ //adrp
    int offset=(((signed int)(*ptr<<8)>>13)<<2)|((*ptr>>29)&0x3);
    source=((intptr_t)ptr&(intptr_t)(~0xfff))+((intptr_t)offset<<12);
    ptr++;
    if((*ptr&0xff000000)==0x91000000){//add
      source|=(*ptr>>10)&0xfff;
      ptr++;
    }
  }
  else
    assert(0); /*Should not happen*/

  ptr++;
  if((*ptr&0xffe00000)!=0x52800000) ptr++;
  assert((*ptr&0xffe00000)==0x52800000); //movz
  u_int len=(*ptr>>5)&0xffff;
  ptr+=2;

  if((*ptr&0xfc000000)!=0x94000000) ptr++;
  assert((*ptr&0xfc000000)==0x94000000); // bl instruction

  uintptr_t verifier=((signed int)(*ptr<<6)>>4)+(intptr_t)ptr;
  assert(verifier==(uintptr_t)verify_code||verifier==(uintptr_t)verify_code_vm||verifier==(uintptr_t)verify_code_ds);

  if(verifier==(uintptr_t)verify_code_vm||verifier==(uintptr_t)verify_code_ds) {
    if(memory_map[source>>12]>=((uintptr_t)1<<((sizeof(uintptr_t)<<3)-1))) { /*mapping=-1?*/
      source=0;
    }
    else source+=(memory_map[source>>12]<<2);
  }
  *start=source;
  *end=source+len;
}

/* Register allocation */

// Note: registers are allocated clean (unmodified state)
// if you intend to modify the register, you must call dirty_reg().
static void alloc_reg(struct regstat *cur,int i,signed char tr)
{
  int r,hr;
  int preferred_reg=19+(tr%10); // Try to allocate callee-save register first
  if(tr==CCREG) preferred_reg=HOST_CCREG;
  if(tr==BTREG) preferred_reg=HOST_BTREG;
  if(tr==PTEMP||tr==FTEMP) preferred_reg=18;
  
  // Don't allocate unused registers
  if((cur->u>>tr)&1) return;
  
  // see if it's already allocated
  for(hr=HOST_REGS-1;hr>=0;hr--)
  {
    if(cur->regmap[hr]==tr) return;
  }
  
  // Keep the same mapping if the register was already allocated in a loop
  preferred_reg = loop_reg(i,tr,preferred_reg);
  
  // Try to allocate the preferred register
  if(cur->regmap[preferred_reg]==-1) {
    cur->regmap[preferred_reg]=tr;
    cur->dirty&=~(1<<preferred_reg);
    cur->isconst&=~(1<<preferred_reg);
    return;
  }
  r=cur->regmap[preferred_reg];
  if(r<64&&((cur->u>>r)&1)) {
    cur->regmap[preferred_reg]=tr;
    cur->dirty&=~(1<<preferred_reg);
    cur->isconst&=~(1<<preferred_reg);
    return;
  }
  if(r>=64&&((cur->uu>>(r&63))&1)) {
    cur->regmap[preferred_reg]=tr;
    cur->dirty&=~(1<<preferred_reg);
    cur->isconst&=~(1<<preferred_reg);
    return;
  }
  
  // Clear any unneeded registers
  // We try to keep the mapping consistent, if possible, because it
  // makes branches easier (especially loops).  So we try to allocate
  // first (see above) before removing old mappings.  If this is not
  // possible then go ahead and clear out the registers that are no
  // longer needed.
  for(hr=HOST_REGS-1;hr>=0;hr--)
  {
    r=cur->regmap[hr];
    if(r>=0) {
      if(r<64) {
        if((cur->u>>r)&1) {cur->regmap[hr]=-1;break;}
      }
      else
      {
        if((cur->uu>>(r&63))&1) {cur->regmap[hr]=-1;break;}
      }
    }
  }
  // Try to allocate any available register, but prefer
  // registers that have not been used recently.
  if(i>0) {
    for(hr=HOST_REGS-1;hr>=0;hr--) {
      if(hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
        if(regs[i-1].regmap[hr]!=rs1[i-1]&&regs[i-1].regmap[hr]!=rs2[i-1]&&regs[i-1].regmap[hr]!=rt1[i-1]&&regs[i-1].regmap[hr]!=rt2[i-1]) {
          cur->regmap[hr]=tr;
          cur->dirty&=~(1<<hr);
          cur->isconst&=~(1<<hr);
          return;
        }
      }
    }
  }
  // Try to allocate any available register
  for(hr=HOST_REGS-1;hr>=0;hr--) {
    if(hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
      cur->regmap[hr]=tr;
      cur->dirty&=~(1<<hr);
      cur->isconst&=~(1<<hr);
      return;
    }
  }
  
  // Ok, now we have to evict someone
  // Pick a register we hopefully won't need soon
  u_char hsn[MAXREG+1];
  memset(hsn,10,sizeof(hsn));
  int j;
  lsn(hsn,i,&preferred_reg);
  //DebugMessage(M64MSG_VERBOSE, "eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d",cur->regmap[0],cur->regmap[1],cur->regmap[2],cur->regmap[3],cur->regmap[5],cur->regmap[6],cur->regmap[7]);
  //DebugMessage(M64MSG_VERBOSE, "hsn(%x): %d %d %d %d %d %d %d",start+i*4,hsn[cur->regmap[0]&63],hsn[cur->regmap[1]&63],hsn[cur->regmap[2]&63],hsn[cur->regmap[3]&63],hsn[cur->regmap[5]&63],hsn[cur->regmap[6]&63],hsn[cur->regmap[7]&63]);
  if(i>0) {
    // Don't evict the cycle count at entry points, otherwise the entry
    // stub will have to write it.
    if(bt[i]&&hsn[CCREG]>2) hsn[CCREG]=2;
    if(i>1&&hsn[CCREG]>2&&(itype[i-2]==RJUMP||itype[i-2]==UJUMP||itype[i-2]==CJUMP||itype[i-2]==SJUMP||itype[i-2]==FJUMP)) hsn[CCREG]=2;
    for(j=10;j>=3;j--)
    {
      // Alloc preferred register if available
      if(hsn[r=cur->regmap[preferred_reg]&63]==j) {
        for(hr=0;hr<HOST_REGS;hr++) {
          // Evict both parts of a 64-bit register
          if((cur->regmap[hr]&63)==r) {
            cur->regmap[hr]=-1;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
          }
        }
        cur->regmap[preferred_reg]=tr;
        return;
      }
      for(r=1;r<=MAXREG;r++)
      {
        if(hsn[r]==j&&r!=rs1[i-1]&&r!=rs2[i-1]&&r!=rt1[i-1]&&r!=rt2[i-1]) {
          for(hr=HOST_REGS-1;hr>=0;hr--) {
            if(hr!=HOST_CCREG||j<hsn[CCREG]) {
              if(cur->regmap[hr]==r+64) {
                cur->regmap[hr]=tr;
                cur->dirty&=~(1<<hr);
                cur->isconst&=~(1<<hr);
                return;
              }
            }
          }
          for(hr=HOST_REGS-1;hr>=0;hr--) {
            if(hr!=HOST_CCREG||j<hsn[CCREG]) {
              if(cur->regmap[hr]==r) {
                cur->regmap[hr]=tr;
                cur->dirty&=~(1<<hr);
                cur->isconst&=~(1<<hr);
                return;
              }
            }
          }
        }
      }
    }
  }
  for(j=10;j>=0;j--)
  {
    for(r=1;r<=MAXREG;r++)
    {
      if(hsn[r]==j) {
        for(hr=HOST_REGS-1;hr>=0;hr--) {
          if(cur->regmap[hr]==r+64) {
            cur->regmap[hr]=tr;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
        for(hr=HOST_REGS-1;hr>=0;hr--) {
          if(cur->regmap[hr]==r) {
            cur->regmap[hr]=tr;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
      }
    }
  }
  DebugMessage(M64MSG_ERROR, "This shouldn't happen (alloc_reg)");exit(1);
}

static void alloc_reg64(struct regstat *cur,int i,signed char tr)
{
  int preferred_reg=19+(tr%10); // Try to allocate callee-save register first
  int r,hr;
  
  // allocate the lower 32 bits
  alloc_reg(cur,i,tr);
  
  // Don't allocate unused registers
  if((cur->uu>>tr)&1) return;
  
  // see if the upper half is already allocated
  for(hr=HOST_REGS-1;hr>=0;hr--)
  {
    if(cur->regmap[hr]==tr+64) return;
  }
  
  // Keep the same mapping if the register was already allocated in a loop
  preferred_reg = loop_reg(i,tr,preferred_reg);
  
  // Try to allocate the preferred register
  if(cur->regmap[preferred_reg]==-1) {
    cur->regmap[preferred_reg]=tr|64;
    cur->dirty&=~(1<<preferred_reg);
    cur->isconst&=~(1<<preferred_reg);
    return;
  }
  r=cur->regmap[preferred_reg];
  if(r<64&&((cur->u>>r)&1)) {
    cur->regmap[preferred_reg]=tr|64;
    cur->dirty&=~(1<<preferred_reg);
    cur->isconst&=~(1<<preferred_reg);
    return;
  }
  if(r>=64&&((cur->uu>>(r&63))&1)) {
    cur->regmap[preferred_reg]=tr|64;
    cur->dirty&=~(1<<preferred_reg);
    cur->isconst&=~(1<<preferred_reg);
    return;
  }
  
  // Clear any unneeded registers
  // We try to keep the mapping consistent, if possible, because it
  // makes branches easier (especially loops).  So we try to allocate
  // first (see above) before removing old mappings.  If this is not
  // possible then go ahead and clear out the registers that are no
  // longer needed.
  for(hr=HOST_REGS-1;hr>=0;hr--)
  {
    r=cur->regmap[hr];
    if(r>=0) {
      if(r<64) {
        if((cur->u>>r)&1) {cur->regmap[hr]=-1;break;}
      }
      else
      {
        if((cur->uu>>(r&63))&1) {cur->regmap[hr]=-1;break;}
      }
    }
  }
  // Try to allocate any available register, but prefer
  // registers that have not been used recently.
  if(i>0) {
    for(hr=HOST_REGS-1;hr>=0;hr--) {
      if(hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
        if(regs[i-1].regmap[hr]!=rs1[i-1]&&regs[i-1].regmap[hr]!=rs2[i-1]&&regs[i-1].regmap[hr]!=rt1[i-1]&&regs[i-1].regmap[hr]!=rt2[i-1]) {
          cur->regmap[hr]=tr|64;
          cur->dirty&=~(1<<hr);
          cur->isconst&=~(1<<hr);
          return;
        }
      }
    }
  }
  // Try to allocate any available register
  for(hr=HOST_REGS-1;hr>=0;hr--) {
    if(hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
      cur->regmap[hr]=tr|64;
      cur->dirty&=~(1<<hr);
      cur->isconst&=~(1<<hr);
      return;
    }
  }
  
  // Ok, now we have to evict someone
  // Pick a register we hopefully won't need soon
  u_char hsn[MAXREG+1];
  memset(hsn,10,sizeof(hsn));
  int j;
  lsn(hsn,i,&preferred_reg);
  //DebugMessage(M64MSG_VERBOSE, "eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d",cur->regmap[0],cur->regmap[1],cur->regmap[2],cur->regmap[3],cur->regmap[5],cur->regmap[6],cur->regmap[7]);
  //DebugMessage(M64MSG_VERBOSE, "hsn(%x): %d %d %d %d %d %d %d",start+i*4,hsn[cur->regmap[0]&63],hsn[cur->regmap[1]&63],hsn[cur->regmap[2]&63],hsn[cur->regmap[3]&63],hsn[cur->regmap[5]&63],hsn[cur->regmap[6]&63],hsn[cur->regmap[7]&63]);
  if(i>0) {
    // Don't evict the cycle count at entry points, otherwise the entry
    // stub will have to write it.
    if(bt[i]&&hsn[CCREG]>2) hsn[CCREG]=2;
    if(i>1&&hsn[CCREG]>2&&(itype[i-2]==RJUMP||itype[i-2]==UJUMP||itype[i-2]==CJUMP||itype[i-2]==SJUMP||itype[i-2]==FJUMP)) hsn[CCREG]=2;
    for(j=10;j>=3;j--)
    {
      // Alloc preferred register if available
      if(hsn[r=cur->regmap[preferred_reg]&63]==j) {
        for(hr=0;hr<HOST_REGS;hr++) {
          // Evict both parts of a 64-bit register
          if((cur->regmap[hr]&63)==r) {
            cur->regmap[hr]=-1;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
          }
        }
        cur->regmap[preferred_reg]=tr|64;
        return;
      }
      for(r=1;r<=MAXREG;r++)
      {
        if(hsn[r]==j&&r!=rs1[i-1]&&r!=rs2[i-1]&&r!=rt1[i-1]&&r!=rt2[i-1]) {
          for(hr=HOST_REGS-1;hr>=0;hr--) {
            if(hr!=HOST_CCREG||j<hsn[CCREG]) {
              if(cur->regmap[hr]==r+64) {
                cur->regmap[hr]=tr|64;
                cur->dirty&=~(1<<hr);
                cur->isconst&=~(1<<hr);
                return;
              }
            }
          }
          for(hr=HOST_REGS-1;hr>=0;hr--) {
            if(hr!=HOST_CCREG||j<hsn[CCREG]) {
              if(cur->regmap[hr]==r) {
                cur->regmap[hr]=tr|64;
                cur->dirty&=~(1<<hr);
                cur->isconst&=~(1<<hr);
                return;
              }
            }
          }
        }
      }
    }
  }
  for(j=10;j>=0;j--)
  {
    for(r=1;r<=MAXREG;r++)
    {
      if(hsn[r]==j) {
        for(hr=HOST_REGS-1;hr>=0;hr--) {
          if(cur->regmap[hr]==r+64) {
            cur->regmap[hr]=tr|64;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
        for(hr=HOST_REGS-1;hr>=0;hr--) {
          if(cur->regmap[hr]==r) {
            cur->regmap[hr]=tr|64;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
      }
    }
  }
  DebugMessage(M64MSG_ERROR, "This shouldn't happen");exit(1);
}

// Allocate a temporary register.  This is done without regard to
// dirty status or whether the register we request is on the unneeded list
// Note: This will only allocate one register, even if called multiple times
static void alloc_reg_temp(struct regstat *cur,int i,signed char tr)
{
  int r,hr;
  int preferred_reg = -1;
  
  // see if it's already allocated
  for(hr=0;hr<HOST_REGS;hr++)
  {
    if(hr!=EXCLUDE_REG&&cur->regmap[hr]==tr) return;
  }
  
  // Try to allocate any available register
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
      cur->regmap[hr]=tr;
      cur->dirty&=~(1<<hr);
      cur->isconst&=~(1<<hr);
      return;
    }
  }
  
  // Find an unneeded register
  for(hr=0;hr<HOST_REGS;hr++)
  {
    r=cur->regmap[hr];
    if(r>=0) {
      if(r<64) {
        if((cur->u>>r)&1) {
          if(i==0||((unneeded_reg[i-1]>>r)&1)) {
            cur->regmap[hr]=tr;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
      }
      else
      {
        if((cur->uu>>(r&63))&1) {
          if(i==0||((unneeded_reg_upper[i-1]>>(r&63))&1)) {
            cur->regmap[hr]=tr;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
      }
    }
  }
  
  // Ok, now we have to evict someone
  // Pick a register we hopefully won't need soon
  // TODO: we might want to follow unconditional jumps here
  // TODO: get rid of dupe code and make this into a function
  u_char hsn[MAXREG+1];
  memset(hsn,10,sizeof(hsn));
  int j;
  lsn(hsn,i,&preferred_reg);
  //DebugMessage(M64MSG_VERBOSE, "hsn: %d %d %d %d %d %d %d",hsn[cur->regmap[0]&63],hsn[cur->regmap[1]&63],hsn[cur->regmap[2]&63],hsn[cur->regmap[3]&63],hsn[cur->regmap[5]&63],hsn[cur->regmap[6]&63],hsn[cur->regmap[7]&63]);
  if(i>0) {
    // Don't evict the cycle count at entry points, otherwise the entry
    // stub will have to write it.
    if(bt[i]&&hsn[CCREG]>2) hsn[CCREG]=2;
    if(i>1&&hsn[CCREG]>2&&(itype[i-2]==RJUMP||itype[i-2]==UJUMP||itype[i-2]==CJUMP||itype[i-2]==SJUMP||itype[i-2]==FJUMP)) hsn[CCREG]=2;
    for(j=10;j>=3;j--)
    {
      for(r=1;r<=MAXREG;r++)
      {
        if(hsn[r]==j&&r!=rs1[i-1]&&r!=rs2[i-1]&&r!=rt1[i-1]&&r!=rt2[i-1]) {
          for(hr=0;hr<HOST_REGS;hr++) {
            if(hr!=HOST_CCREG||hsn[CCREG]>2) {
              if(cur->regmap[hr]==r+64) {
                cur->regmap[hr]=tr;
                cur->dirty&=~(1<<hr);
                cur->isconst&=~(1<<hr);
                return;
              }
            }
          }
          for(hr=0;hr<HOST_REGS;hr++) {
            if(hr!=HOST_CCREG||hsn[CCREG]>2) {
              if(cur->regmap[hr]==r) {
                cur->regmap[hr]=tr;
                cur->dirty&=~(1<<hr);
                cur->isconst&=~(1<<hr);
                return;
              }
            }
          }
        }
      }
    }
  }
  for(j=10;j>=0;j--)
  {
    for(r=1;r<=MAXREG;r++)
    {
      if(hsn[r]==j) {
        for(hr=0;hr<HOST_REGS;hr++) {
          if(cur->regmap[hr]==r+64) {
            cur->regmap[hr]=tr;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
        for(hr=0;hr<HOST_REGS;hr++) {
          if(cur->regmap[hr]==r) {
            cur->regmap[hr]=tr;
            cur->dirty&=~(1<<hr);
            cur->isconst&=~(1<<hr);
            return;
          }
        }
      }
    }
  }
  DebugMessage(M64MSG_ERROR, "This shouldn't happen");exit(1);
}
// Allocate a specific ARM64 register.
static void alloc_arm64_reg(struct regstat *cur,int i,signed char tr,int hr)
{
  int n;
  int dirty=0;
  
  // see if it's already allocated (and dealloc it)
  for(n=0;n<HOST_REGS;n++)
  {
    if(n!=EXCLUDE_REG&&cur->regmap[n]==tr) {
      dirty=(cur->dirty>>n)&1;
      cur->regmap[n]=-1;
    }
  }
  
  cur->regmap[hr]=tr;
  cur->dirty&=~(1<<hr);
  cur->dirty|=dirty<<hr;
  cur->isconst&=~(1<<hr);
}

// Alloc cycle count into dedicated register
static void alloc_cc(struct regstat *cur,int i)
{
  alloc_arm64_reg(cur,i,CCREG,HOST_CCREG);
}

/* Special alloc */

void multdiv_alloc_arm64(struct regstat *current,int i)
{
  //  case 0x18: MULT
  //  case 0x19: MULTU
  //  case 0x1A: DIV
  //  case 0x1B: DIVU
  //  case 0x1C: DMULT
  //  case 0x1D: DMULTU
  //  case 0x1E: DDIV
  //  case 0x1F: DDIVU
  clear_const(current,rs1[i]);
  clear_const(current,rs2[i]);
  if(rs1[i]&&rs2[i])
  {
    if((opcode2[i]&4)==0) // 32-bit
    {
      current->u&=~(1LL<<HIREG);
      current->u&=~(1LL<<LOREG);
      alloc_reg(current,i,HIREG);
      alloc_reg(current,i,LOREG);
      alloc_reg(current,i,rs1[i]);
      alloc_reg(current,i,rs2[i]);
      current->is32|=1LL<<HIREG;
      current->is32|=1LL<<LOREG;
      dirty_reg(current,HIREG);
      dirty_reg(current,LOREG);
    }
    else // 64-bit
    {
      current->u&=~(1LL<<HIREG);
      current->u&=~(1LL<<LOREG);
      current->uu&=~(1LL<<HIREG);
      current->uu&=~(1LL<<LOREG);
      alloc_reg64(current,i,HIREG);
      alloc_reg64(current,i,LOREG);
      alloc_reg64(current,i,rs1[i]);
      alloc_reg64(current,i,rs2[i]);
      current->is32&=~(1LL<<HIREG);
      current->is32&=~(1LL<<LOREG);
      dirty_reg(current,HIREG);
      dirty_reg(current,LOREG);
    }
  }
  else
  {
    // Multiply by zero is zero.
    // MIPS does not have a divide by zero exception.
    // The result is undefined, we return zero.
    alloc_reg(current,i,HIREG);
    alloc_reg(current,i,LOREG);
    current->is32|=1LL<<HIREG;
    current->is32|=1LL<<LOREG;
    dirty_reg(current,HIREG);
    dirty_reg(current,LOREG);
  }
}
#define multdiv_alloc multdiv_alloc_arm64


/* Assembler */

static char regname[32][4] = {
 "w0",
 "w1",
 "w2",
 "w3",
 "w4",
 "w5",
 "w6",
 "w7",
 "w8",
 "w9",
 "w10",
 "w11",
 "w12",
 "w13",
 "w14",
 "w15",
 "w16",
 "w17",
 "w18",
 "w19",
 "w20",
 "w21",
 "w22",
 "w23",
 "w24",
 "w25",
 "w26",
 "w27",
 "w28",
 "w29",
 "w30",
 "wzr"};

static char regname64[32][4] = {
 "x0",
 "x1",
 "x2",
 "x3",
 "x4",
 "x5",
 "x6",
 "x7",
 "x8",
 "x9",
 "x10",
 "x11",
 "x12",
 "x13",
 "x14",
 "x15",
 "x16",
 "x17",
 "x18",
 "x19",
 "x20",
 "x21",
 "x22",
 "x23",
 "x24",
 "x25",
 "x26",
 "x27",
 "x28",
 "fp",
 "lr",
 "sp"};

static void output_byte(u_char byte)
{
  *(out++)=byte;
}

static void output_w32(u_int word)
{
  *((u_int *)out)=word;
  out+=4;
}

static u_int genjmp(uintptr_t addr)
{
  if(addr<4) return 0;
  int offset=addr-(intptr_t)out;
  assert(offset>=-134217728&&offset<134217728);
  return ((u_int)offset>>2)&0x3ffffff;
}

static u_int gencondjmp(uintptr_t addr)
{
  if(addr<4) return 0;
  int offset=addr-(intptr_t)out;
  assert(offset>=-1048576&&offset<1048576);
  return ((u_int)offset>>2)&0x7ffff;
}

uint32_t count_trailing_zeros(uint64_t value)
{
#ifdef _MSC_VER
  uint32_t trailing_zero_low = 0;
  uint32_t trailing_zero_high = 0;
  if(!_BitScanForward(&trailing_zero_low, (uint32_t)value))
    trailing_zero_low = 32;

  if(!_BitScanForward(&trailing_zero_high, (uint32_t)(value>>32)))
    trailing_zero_high = 32;

  if(trailing_zero_low == 32)
    return trailing_zero_low + trailing_zero_high;
  else
    return trailing_zero_low;
#else /* ARM64 */
  return __builtin_ctzll(value);
#endif
}

uint32_t count_leading_zeros(uint64_t value)
{
#ifdef _MSC_VER
  uint32_t leading_zero_low = 0;
  uint32_t leading_zero_high = 0;
  if(!_BitScanReverse(&leading_zero_low, (uint32_t)value))
    leading_zero_low = 32;
  else
    leading_zero_low = 31 - leading_zero_low;

  if(!_BitScanReverse(&leading_zero_high, (uint32_t)(value>>32)))
    leading_zero_high = 32;
  else
    leading_zero_high = 31 - leading_zero_high;

  if(leading_zero_high == 32)
    return leading_zero_low + leading_zero_high;
  else
    return leading_zero_high;
#else /* ARM64 */
  return __builtin_clzll(value);
#endif
}

// This function returns true if the argument is a non-empty
// sequence of ones starting at the least significant bit with the remainder
// zero.
static uint32_t is_mask(uint64_t value) {
  return value && ((value + 1) & value) == 0;
}

// This function returns true if the argument contains a
// non-empty sequence of ones with the remainder zero.
static uint32_t is_shifted_mask(uint64_t Value) {
  return Value && is_mask((Value - 1) | Value);
}

// Determine if an immediate value can be encoded
// as the immediate operand of a logical instruction for the given register
// size. If so, return 1 with "encoding" set to the encoded value in
// the form N:immr:imms.
static uint32_t genimm(uint64_t imm, uint32_t regsize, uint32_t * encoded) {
  // First, determine the element size.
  uint32_t size = regsize;
  do {
    size /= 2;
    uint64_t mask = (1ULL << size) - 1;

    if ((imm & mask) != ((imm >> size) & mask)) {
      size *= 2;
      break;
    }
  } while (size > 2);

  // Second, determine the rotation to make the element be: 0^m 1^n.
  uint32_t trailing_one, trailing_zero;
  uint64_t mask = ((uint64_t)-1LL) >> (64 - size);
  imm &= mask;

  if (is_shifted_mask(imm)) {
    trailing_zero = count_trailing_zeros(imm);
    assert(trailing_zero < 64);
    trailing_one = count_trailing_zeros(~(imm >> trailing_zero));
  } else {
    imm |= ~mask;
    if (!is_shifted_mask(~imm))
      return 0;
  
    uint32_t leading_one = count_leading_zeros(~imm);
    trailing_zero = 64 - leading_one;
    trailing_one = leading_one + count_trailing_zeros(~imm) - (64 - size);
  }

  // Encode in immr the number of RORs it would take to get *from* 0^m 1^n
  // to our target value, where trailing_zero is the number of RORs to go the opposite
  // direction.
  assert(size > trailing_zero);
  uint32_t immr = (size - trailing_zero) & (size - 1);

  // If size has a 1 in the n'th bit, create a value that has zeroes in
  // bits [0, n] and ones above that.
  uint64_t Nimms = ~(size-1) << 1;

  // Or the trailing_one value into the low bits, which must be below the Nth bit
  // bit mentioned above.
  Nimms |= (trailing_one-1);

  // Extract the seventh bit and toggle it to create the N field.
  uint32_t N = ((Nimms >> 6) & 1) ^ 1;

  *encoded = (N << 12) | (immr << 6) | (Nimms & 0x3f);
  return 1;
}

static void emit_mov(int rs,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("mov %s,%s",regname[rt],regname[rs]);
  output_w32(0x2a000000|rs<<16|WZR<<5|rt);
}

static void emit_mov64(int rs,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("mov %s,%s",regname64[rt],regname64[rs]);
  output_w32(0xaa000000|rs<<16|WZR<<5|rt);
}

static void emit_add(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("add %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x0b000000|rs2<<16|rs1<<5|rt);
}

static void emit_adds(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("adds %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x2b000000|rs2<<16|rs1<<5|rt);
}

static void emit_adc(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("adc %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x1a000000|rs2<<16|rs1<<5|rt);
}

static void emit_sub(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("sub %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x4b000000|rs2<<16|rs1<<5|rt);
}

static void emit_subs(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("subs %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x6b000000|rs2<<16|rs1<<5|rt);
}

static void emit_sbc(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("sbc %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x5a000000|rs2<<16|rs1<<5|rt);
}

static void emit_sbcs(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("sbcs %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x7a000000|rs2<<16|rs1<<5|rt);
}

static void emit_neg(int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("neg %s,%s",regname[rt],regname[rs]);
  output_w32(0x4b000000|rs<<16|WZR<<5|rt);
}

static void emit_negs(int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("negs %s,%s",regname[rt],regname[rs]);
  output_w32(0x6b000000|rs<<16|WZR<<5|rt);
}

static void emit_rscimm(int rs,int imm,u_int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm==0);
  assem_debug("ngc %s,%s",regname[rt],regname[rs]);
  output_w32(0x5a000000|rs<<16|WZR<<5|rt);
}

static void emit_zeroreg(int rt)
{
  assert(rt!=29);
  assem_debug("movz %s,#0",regname[rt]);
  output_w32(0x52800000|rt);
}

static void emit_zeroreg64(int rt)
{
  assert(rt!=29);
  assem_debug("movz %s,#0",regname64[rt]);
  output_w32(0xd2800000|rt);
}

static void emit_movz(u_int imm,u_int rt)
{
  assert(rt!=29);
  assert(imm<65536);
  assem_debug("movz %s,#%d",regname[rt],imm);
  output_w32(0x52800000|imm<<5|rt);
}

static void emit_movz_lsl16(u_int imm,u_int rt)
{
  assert(rt!=29);
  assert(imm<65536);
  assem_debug("movz %s, #%d, lsl #%d",regname[rt],imm,16);
  output_w32(0x52a00000|imm<<5|rt);
}

static void emit_movn(u_int imm,u_int rt)
{
  assert(rt!=29);
  assert(imm<65536);
  assem_debug("movn %s,#%d",regname[rt],imm);
  output_w32(0x12800000|imm<<5|rt);
}

static void emit_movn64(u_int imm,u_int rt)
{
  assert(rt!=29);
  assert(imm<65536);
  assem_debug("movn %s,#%d",regname64[rt],imm);
  output_w32(0x92800000|imm<<5|rt);
}

static void emit_movn_lsl16(u_int imm,u_int rt)
{
  assert(rt!=29);
  assert(imm<65536);
  assem_debug("movn %s, #%d, lsl #%d",regname[rt],imm,16);
  output_w32(0x12a00000|imm<<5|rt);
}

static void emit_movk(u_int imm,u_int rt)
{
  assert(rt!=29);
  assert(imm<65536);
  assem_debug("movk %s,#%d",regname[rt],imm);
  output_w32(0x72800000|imm<<5|rt);
}

static void emit_movk_lsl16(u_int imm,u_int rt)
{
  assert(rt!=29);
  assert(imm<65536);
  assem_debug("movk %s, #%d, lsl #%d",regname[rt],imm,16);
  output_w32(0x72a00000|imm<<5|rt);
}

static void emit_movimm(u_int imm,u_int rt)
{
  assert(rt!=29);
  uint32_t armval=0;
  if(imm<65536) {
    emit_movz(imm,rt);
  }else if((~imm)<65536) {
    emit_movn(~imm,rt);
  }else if((imm&0xffff)==0) {
    emit_movz_lsl16((imm>>16)&0xffff,rt);
  }else if(((~imm)&0xffff)==0) {
    emit_movn_lsl16((~imm>>16)&0xffff,rt);
  }else if(genimm((uint64_t)imm,32,&armval)) {
    assem_debug("orr %s, wzr, #%d (0x%x)",regname[rt],imm,imm);
    output_w32(0x32000000|armval<<10|WZR<<5|rt);
  }else{
    emit_movz_lsl16((imm>>16)&0xffff,rt);
    emit_movk(imm&0xffff,rt);
  }
}

static void emit_loadreg(int r, int hr)
{
  assert(hr!=29);
  if((r&63)==0)
    emit_zeroreg(hr);
  else if(r==MMREG)
    emit_movimm(((intptr_t)memory_map-(intptr_t)&dynarec_local)>>3,hr);
  else if(r==INVCP||r==ROREG){
    intptr_t addr=0;
    if(r==INVCP) addr=(intptr_t)&invc_ptr;
    if(r==ROREG) addr=(intptr_t)&ram_offset;
    u_int offset = addr-(uintptr_t)&dynarec_local;
    assert(offset<4096);
    assert(offset%8 == 0); /* 8 bytes aligned */
    assem_debug("ldr %s,fp+%d",regname[hr],offset);
    output_w32(0xf9400000|((offset>>3)<<10)|(FP<<5)|hr);
  }
  else {
    intptr_t addr=((intptr_t)reg)+((r&63)<<3)+((r&64)>>4);
    if((r&63)==HIREG) addr=(intptr_t)&hi+((r&64)>>4);
    if((r&63)==LOREG) addr=(intptr_t)&lo+((r&64)>>4);
    if(r==CCREG) addr=(intptr_t)&cycle_count;
    if(r==CSREG) addr=(intptr_t)&g_cp0_regs[CP0_STATUS_REG];
    if(r==FSREG) addr=(intptr_t)&FCR31;
    u_int offset = addr-(uintptr_t)&dynarec_local;
    assert(offset<4096);
    assert(offset%4 == 0); /* 4 bytes aligned */
    assem_debug("ldr %s,fp+%d",regname[hr],offset);
    output_w32(0xb9400000|((offset>>2)<<10)|(FP<<5)|hr);
  }
}

static void emit_storereg(int r, int hr)
{
  assert(hr!=29);
  intptr_t addr=((intptr_t)reg)+((r&63)<<3)+((r&64)>>4);
  if((r&63)==HIREG) addr=(intptr_t)&hi+((r&64)>>4);
  if((r&63)==LOREG) addr=(intptr_t)&lo+((r&64)>>4);
  if(r==CCREG) addr=(intptr_t)&cycle_count;
  if(r==FSREG) addr=(intptr_t)&FCR31;
  u_int offset = addr-(intptr_t)&dynarec_local;
  assert(offset<16380);
  assert(offset%4 == 0); /* 4 bytes aligned */
  assem_debug("str %s,fp+%d",regname[hr],offset);
  output_w32(0xb9000000|((offset>>2)<<10)|(FP<<5)|hr);
}

static void emit_storereg64(int r, int hr)
{
  assert(hr!=29);
  assert(r<FSREG);
  intptr_t addr=(intptr_t)&reg[r];
  if(r==HIREG) addr=(intptr_t)&hi;
  if(r==LOREG) addr=(intptr_t)&lo;
  u_int offset = addr-(intptr_t)&dynarec_local;
  assert(offset<32760);
  assert(offset%8 == 0); /* 8 bytes aligned */
  assem_debug("str %s,fp+%d",regname64[hr],offset);
  output_w32(0xf9000000|((offset>>3)<<10)|(FP<<5)|hr);
}

static void emit_test(int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("tst %s,%s",regname[rs],regname[rt]);
  output_w32(0x6a000000|rt<<16|rs<<5|WZR);
}

static void emit_test64(int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("tst %s,%s",regname64[rs],regname64[rt]);
  output_w32(0xea000000|rt<<16|rs<<5|WZR);
}

static void emit_testimm(int rs,int imm)
{
  assert(rs!=29);
  u_int armval, ret;
  assem_debug("tst %s,#%d",regname[rs],imm);
  ret=genimm(imm,32,&armval);
  assert(ret);
  output_w32(0x72000000|armval<<10|rs<<5|WZR);
}

static void emit_testimm64(int rs,int64_t imm)
{
  assert(rs!=29);
  u_int armval, ret;
  assem_debug("tst %s,#%d",regname64[rs],imm);
  ret=genimm(imm,64,&armval);
  assert(ret);
  output_w32(0xf2000000|armval<<10|rs<<5|WZR);
}

static void emit_not(int rs,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("mvn %s,%s",regname[rt],regname[rs]);
  output_w32(0x2a200000|rs<<16|WZR<<5|rt);
}

static void emit_and(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("and %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x0a000000|rs2<<16|rs1<<5|rt);
}

static void emit_or(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("orr %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x2a000000|rs2<<16|rs1<<5|rt);
}

static void emit_orr64(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("orr %s,%s,%s",regname64[rt],regname64[rs1],regname64[rs2]);
  output_w32(0xaa000000|rs2<<16|rs1<<5|rt);
}

static void emit_xor(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("eor %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x4a000000|rs2<<16|rs1<<5|rt);
}

static void emit_addimm64(u_int rs,int imm,u_int rt)
{
  assert(rt!=29);
  assert(imm>0&&imm<4096);
  assem_debug("add %s, %s, #%d",regname64[rt],regname64[rs],imm);
  output_w32(0x91000000|imm<<10|rs<<5|rt);
}

static void emit_addimm(u_int rs,int imm,u_int rt)
{
  assert(rt!=29);
  assert(rs!=29);

  if(imm!=0) {
    assert(imm>-65536&&imm<65536);
    //assert(imm>-16777216&&imm<16777216);
    if(imm<0&&imm>-4096) {
      assem_debug("sub %s, %s, #%d",regname[rt],regname[rs],-imm&0xfff);
      output_w32(0x51000000|((-imm)&0xfff)<<10|rs<<5|rt);
    }else if(imm>0&&imm<4096) {
      assem_debug("add %s, %s, #%d",regname[rt],regname[rs],imm&0xfff);
      output_w32(0x11000000|(imm&0xfff)<<10|rs<<5|rt);
    }else if(imm<0) {
      assem_debug("sub %s, %s, #%d lsl #%d",regname[rt],regname[rt],((-imm)>>12)&0xfff,12);
      output_w32(0x51400000|(((-imm)>>12)&0xfff)<<10|rs<<5|rt);
      if((-imm&0xfff)!=0) {
        assem_debug("sub %s, %s, #%d",regname[rt],regname[rs],(-imm&0xfff));
        output_w32(0x51000000|((-imm)&0xfff)<<10|rt<<5|rt);
      }
    }else {
      assem_debug("add %s, %s, #%d lsl #%d",regname[rt],regname[rt],(imm>>12)&0xfff,12);
      output_w32(0x11400000|((imm>>12)&0xfff)<<10|rs<<5|rt);
      if((imm&0xfff)!=0) {
        assem_debug("add %s, %s, #%d",regname[rt],regname[rs],imm&0xfff);
        output_w32(0x11000000|(imm&0xfff)<<10|rt<<5|rt);
      }
    }
  }
  else if(rs!=rt) emit_mov(rs,rt);
}

static void emit_addimm_and_set_flags(int imm,int rt)
{
  assert(rt!=29);
  assert(imm>-65536&&imm<65536);
  //assert(imm>-16777216&&imm<16777216);
  if(imm<0&&imm>-4096) {
    assem_debug("subs %s, %s, #%d",regname[rt],regname[rt],-imm&0xfff);
    output_w32(0x71000000|((-imm)&0xfff)<<10|rt<<5|rt);
  }else if(imm>0&&imm<4096) {
    assem_debug("adds %s, %s, #%d",regname[rt],regname[rt],imm&0xfff);
    output_w32(0x31000000|(imm&0xfff)<<10|rt<<5|rt);
  }else if(imm<0) {
    if((-imm&0xfff)!=0) {
      assem_debug("sub %s, %s, #%d lsl #%d",regname[rt],regname[rt],((-imm)>>12)&0xfff,12);
      output_w32(0x51400000|(((-imm)>>12)&0xfff)<<10|rt<<5|rt);
      assem_debug("subs %s, %s, #%d",regname[rt],regname[rt],(-imm&0xfff));
      output_w32(0x71000000|((-imm)&0xfff)<<10|rt<<5|rt);
    }else{
      assem_debug("subs %s, %s, #%d lsl #%d",regname[rt],regname[rt],((-imm)>>12)&0xfff,12);
      output_w32(0x71400000|(((-imm)>>12)&0xfff)<<10|rt<<5|rt);
    }
  }else {
    if((imm&0xfff)!=0) {
      assem_debug("add %s, %s, #%d lsl #%d",regname[rt],regname[rt],(imm>>12)&0xfff,12);
      output_w32(0x11400000|((imm>>12)&0xfff)<<10|rt<<5|rt);
      assem_debug("adds %s, %s, #%d",regname[rt],regname[rt],imm&0xfff);
      output_w32(0x31000000|(imm&0xfff)<<10|rt<<5|rt);
    }else{
      assem_debug("adds %s, %s, #%d lsl #%d",regname[rt],regname[rt],(imm>>12)&0xfff,12);
      output_w32(0x31400000|((imm>>12)&0xfff)<<10|rt<<5|rt);
    }
  }
}

#ifndef RAM_OFFSET
static void emit_addimm_no_flags(u_int imm,u_int rt)
{
  assert(0);
}
#endif

static void emit_addnop(u_int r)
{
  assert(r!=29);
  assem_debug("nop");
  output_w32(0xd503201f);
  /*assem_debug("add %s,%s,#0 (nop)",regname[r],regname[r]);
  output_w32(0x11000000|r<<5|r);*/
}

static void emit_addimm64_32(int rsh,int rsl,int imm,int rth,int rtl)
{
  assert(rsh!=29);
  assert(rsl!=29);
  assert(rth!=29);
  assert(rtl!=29);
  emit_movimm(imm,HOST_TEMPREG);
  emit_adds(HOST_TEMPREG,rsl,rtl);
  emit_adc(rsh,WZR,rth);
}

#ifdef INVERTED_CARRY
static void emit_sbb(int rs1,int rs2)
{
  assert(0);
}
static void emit_adcimm(u_int rs,int imm,u_int rt)
{
  assert(0);
}
#endif

static void emit_andimm(int rs,int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  u_int armval;
  if(imm==0) {
    emit_zeroreg(rt);
  }else if(genimm((uint64_t)imm,32,&armval)) {
    assem_debug("and %s,%s,#%d",regname[rt],regname[rs],imm);
    output_w32(0x12000000|armval<<10|rs<<5|rt);
  }else{
    assert(imm>0&&imm<65535);
    emit_movz(imm,HOST_TEMPREG);
    assem_debug("and %s,%s,%s",regname[rt],regname[rs],regname[HOST_TEMPREG]);
    output_w32(0x0a000000|HOST_TEMPREG<<16|rs<<5|rt);
  }
}

static void emit_andimm64(int rs,int64_t imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  u_int armval;
  assert(genimm((uint64_t)imm,64,&armval));
  assem_debug("and %s,%s,#%d",regname64[rt],regname64[rs],imm);
  output_w32(0x92000000|armval<<10|rs<<5|rt);
}

static void emit_orimm(int rs,int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  u_int armval;
  if(imm==0) {
    if(rs!=rt) emit_mov(rs,rt);
  }else if(genimm(imm,32,&armval)) {
    assem_debug("orr %s,%s,#%d",regname[rt],regname[rs],imm);
    output_w32(0x32000000|armval<<10|rs<<5|rt);
  }else{
    assert(imm>0&&imm<65536);
    emit_movz(imm,HOST_TEMPREG);
    assem_debug("orr %s,%s,%s",regname[rt],regname[rs],regname[HOST_TEMPREG]);
    output_w32(0x2a000000|HOST_TEMPREG<<16|rs<<5|rt);
  }
}

static void emit_xorimm(int rs,int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  u_int armval;
  if(imm==0) {
    if(rs!=rt) emit_mov(rs,rt);
  }else if(genimm((uint64_t)imm,32,&armval)) {
    assem_debug("eor %s,%s,#%d",regname[rt],regname[rs],imm);
    output_w32(0x52000000|armval<<10|rs<<5|rt);
  }else{
    assert(imm>0&&imm<65536);
    emit_movz(imm,HOST_TEMPREG);
    assem_debug("eor %s,%s,%s",regname[rt],regname[rs],regname[HOST_TEMPREG]);
    output_w32(0x4a000000|HOST_TEMPREG<<16|rs<<5|rt);
  }
}

static void emit_shlimm(int rs,u_int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm>0);
  assert(imm<32);
  //if(imm==1) ...
  assem_debug("lsl %s,%s,#%d",regname[rt],regname[rs],imm);
  output_w32(0x53000000|((31-imm)+1)<<16|(31-imm)<<10|rs<<5|rt);
}

static void emit_shlimm64(int rs,u_int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm>0);
  assert(imm<64);
  assem_debug("lsl %s,%s,#%d",regname64[rt],regname64[rs],imm);
  output_w32(0xd3400000|((63-imm)+1)<<16|(63-imm)<<10|rs<<5|rt);
}

static void emit_shrimm(int rs,u_int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm>0);
  assert(imm<32);
  assem_debug("lsr %s,%s,#%d",regname[rt],regname[rs],imm);
  output_w32(0x53000000|imm<<16|0x1f<<10|rs<<5|rt);
}

static void emit_shrimm64(int rs,u_int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm>0);
  assert(imm<64);
  assem_debug("lsr %s,%s,#%d",regname64[rt],regname64[rs],imm);
  output_w32(0xd3400000|imm<<16|0x3f<<10|rs<<5|rt);
}

static void emit_sarimm(int rs,u_int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm>0);
  assert(imm<32);
  assem_debug("asr %s,%s,#%d",regname[rt],regname[rs],imm);
  output_w32(0x13000000|imm<<16|0x1f<<10|rs<<5|rt);
}

static void emit_rorimm(int rs,u_int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm>0);
  assert(imm<32);
  assem_debug("ror %s,%s,#%d",regname[rt],regname[rs],imm);
  output_w32(0x13800000|rs<<16|imm<<10|rs<<5|rt);
}

static void emit_shl(u_int rs,u_int shift,u_int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(shift!=29);
  //if(imm==1) ...
  assem_debug("lsl %s,%s,%s",regname[rt],regname[rs],regname[shift]);
  output_w32(0x1ac02000|shift<<16|rs<<5|rt);
}

static void emit_shl64(u_int rs,u_int shift,u_int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(shift!=29);
  //if(imm==1) ...
  assem_debug("lsl %s,%s,%s",regname64[rt],regname64[rs],regname64[shift]);
  output_w32(0x9ac02000|shift<<16|rs<<5|rt);
}

static void emit_shr(u_int rs,u_int shift,u_int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(shift!=29);
  assem_debug("lsr %s,%s,%s",regname[rt],regname[rs],regname[shift]);
  output_w32(0x1ac02400|shift<<16|rs<<5|rt);
}

static void emit_shr64(u_int rs,u_int shift,u_int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(shift!=29);
  assem_debug("lsr %s,%s,%s",regname64[rt],regname64[rs],regname64[shift]);
  output_w32(0x9ac02400|shift<<16|rs<<5|rt);
}

static void emit_sar(u_int rs,u_int shift,u_int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(shift!=29);
  assem_debug("asr %s,%s,%s",regname[rt],regname[rs],regname[shift]);
  output_w32(0x1ac02800|shift<<16|rs<<5|rt);
}

static void emit_orrshlimm(u_int rs,int imm,u_int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm<32);
  assem_debug("orr %s,%s,%s,lsl #%d",regname[rt],regname[rt],regname[rs],imm);
  output_w32(0x2a000000|rs<<16|imm<<10|rt<<5|rt);
}

static void emit_orrshlimm64(u_int rs,int imm,u_int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm<64);
  assem_debug("orr %s,%s,%s,lsl #%d",regname64[rt],regname64[rt],regname64[rs],imm);
  output_w32(0xaa000000|rs<<16|imm<<10|rt<<5|rt);
}

static void emit_orrshrimm(u_int rs,int imm,u_int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(imm<32);
  assem_debug("orr %s,%s,%s,lsr #%d",regname[rt],regname[rt],regname[rs],imm);
  output_w32(0x2a400000|rs<<16|imm<<10|rt<<5|rt);
}

static void emit_shldimm(int rs,int rs2,u_int imm,int rt)
{
  assert(rs!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("shld %%%s,%%%s,%d",regname[rt],regname[rs2],imm);
  assert(imm>0);
  assert(imm<32);
  //if(imm==1) ...
  emit_shlimm(rs,imm,rt);
  emit_orrshrimm(rs2,32-imm,rt);
}

static void emit_shrdimm(int rs,int rs2,u_int imm,int rt)
{
  assert(rs!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("shrd %%%s,%%%s,%d",regname[rt],regname[rs2],imm);
  assert(imm>0);
  assert(imm<32);
  //if(imm==1) ...
  emit_shrimm(rs,imm,rt);
  emit_orrshlimm(rs2,32-imm,rt);
}

static void emit_cmpimm(int rs,int imm)
{
  assert(rs!=29);
  if(imm<0&&imm>-4096) {
    assem_debug("cmn %s,#%d",regname[rs],-imm&0xfff);
    output_w32(0x31000000|((-imm)&0xfff)<<10|rs<<5|WZR);
  }else if(imm>0&&imm<4096) {
    assem_debug("cmp %s,#%d",regname[rs],imm&0xfff);
    output_w32(0x71000000|(imm&0xfff)<<10|rs<<5|WZR);
  }else if(imm<0) {
    if((-imm&0xfff)==0) {
      assem_debug("cmn %s,#%d,lsl #12",regname[rs],-imm&0xfff);
      output_w32(0x31400000|((-imm>>12)&0xfff)<<10|rs<<5|WZR);
    }else{
      assert(imm>-65536);
      emit_movz(-imm,HOST_TEMPREG);
      assem_debug("cmn %s,%s",regname[rs],regname[HOST_TEMPREG]);
      output_w32(0x2b000000|HOST_TEMPREG<<16|rs<<5|WZR);
    }
  }else {                                                                                                                                  
    if((imm&0xfff)==0) {
      assem_debug("cmp %s,#%d,lsl #12",regname[rs],imm&0xfff);
      output_w32(0x71400000|((imm>>12)&0xfff)<<10|rs<<5|WZR);
    }else{
      assert(imm<65536);
      emit_movz(imm,HOST_TEMPREG);
      assem_debug("cmp %s,%s",regname[rs],regname[HOST_TEMPREG]);
      output_w32(0x6b000000|HOST_TEMPREG<<16|rs<<5|WZR);
    }
  }
}

static void emit_cmovne_imm(int imm,int rt)
{
  assert(imm==0||imm==1);
  assert(rt!=29);
  if(imm){
    assem_debug("csinc %s,%s,%s,eq",regname[rt],regname[rt],regname[WZR]);
    output_w32(0x1a800400|WZR<<16|EQ<<12|rt<<5|rt);
  }else{
    assem_debug("csel %s,%s,%s,ne",regname[rt],regname[WZR],regname[rt]);
    output_w32(0x1a800000|rt<<16|NE<<12|WZR<<5|rt);
  }
}

static void emit_cmovl_imm(int imm,int rt)
{
  assert(imm==0||imm==1);
  assert(rt!=29);
  if(imm){
    assem_debug("csinc %s,%s,%s,ge",regname[rt],regname[rt],regname[WZR]);
    output_w32(0x1a800400|WZR<<16|GE<<12|rt<<5|rt);
  }else{
    assem_debug("csel %s,%s,%s,lt",regname[rt],regname[WZR],regname[rt]);
    output_w32(0x1a800000|rt<<16|LT<<12|WZR<<5|rt);
  }
}

static void emit_cmovb_imm(int imm,int rt)
{
  assert(imm==0||imm==1);
  assert(rt!=29);
  if(imm){
    assem_debug("csinc %s,%s,%s,cs",regname[rt],regname[rt],regname[WZR]);
    output_w32(0x1a800400|WZR<<16|CS<<12|rt<<5|rt);
  }else{
    assem_debug("csel %s,%s,%s,cc",regname[rt],regname[WZR],regname[rt]);
    output_w32(0x1a800000|rt<<16|CC<<12|WZR<<5|rt);
  }
}

static void emit_cmovs_imm(int imm,int rt)
{
  assert(imm==0||imm==1);
  assert(rt!=29);
  if(imm){
    assem_debug("csinc %s,%s,%s,pl",regname[rt],regname[rt],regname[WZR]);
    output_w32(0x1a800400|WZR<<16|PL<<12|rt<<5|rt);
  }else{
    assem_debug("csel %s,%s,%s,mi",regname[rt],regname[WZR],regname[rt]);
    output_w32(0x1a800000|rt<<16|MI<<12|WZR<<5|rt);
  }
}

static void emit_cmove_reg(int rs,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("csel %s,%s,%s,eq",regname[rt],regname[rs],regname[rt]);
  output_w32(0x1a800000|rt<<16|EQ<<12|rs<<5|rt);
}

static void emit_cmovne_reg(int rs,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("csel %s,%s,%s,ne",regname[rt],regname[rs],regname[rt]);
  output_w32(0x1a800000|rt<<16|NE<<12|rs<<5|rt);
}

static void emit_cmovl_reg(int rs,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("csel %s,%s,%s,lt",regname[rt],regname[rs],regname[rt]);
  output_w32(0x1a800000|rt<<16|LT<<12|rs<<5|rt);
}

static void emit_cmovs_reg(int rs,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("csel %s,%s,%s,mi",regname[rt],regname[rs],regname[rt]);
  output_w32(0x1a800000|rt<<16|MI<<12|rs<<5|rt);
}

static void emit_csel_vs(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("csel %s,%s,%s,vs",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x1a800000|rs2<<16|VS<<12|rs1<<5|rt);
}

static void emit_csel_eq(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("csel %s,%s,%s,eq",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x1a800000|rs2<<16|EQ<<12|rs1<<5|rt);
}

static void emit_csel_cc(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("csel %s,%s,%s,cc",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x1a800000|rs2<<16|CC<<12|rs1<<5|rt);
}

static void emit_csel_ls(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("csel %s,%s,%s,ls",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x1a800000|rs2<<16|LS<<12|rs1<<5|rt);
}

static void emit_slti32(int rs,int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  if(rs!=rt) emit_zeroreg(rt);
  emit_cmpimm(rs,imm);
  if(rs==rt) emit_movimm(0,rt);
  emit_cmovl_imm(1,rt);
}

static void emit_sltiu32(int rs,int imm,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  if(rs!=rt) emit_zeroreg(rt);
  emit_cmpimm(rs,imm);
  if(rs==rt) emit_movimm(0,rt);
  emit_cmovb_imm(1,rt);
}

static void emit_slti64_32(int rsh,int rsl,int imm,int rt)
{
  assert(rsh!=29);
  assert(rsl!=29);
  assert(rt!=29);
  assert(rsh!=rt);
  emit_slti32(rsl,imm,rt);
  if(imm>=0)
  {
    emit_test(rsh,rsh);
    emit_cmovne_imm(0,rt);
    emit_cmovs_imm(1,rt);
  }
  else
  {
    emit_cmpimm(rsh,-1);
    emit_cmovne_imm(0,rt);
    emit_cmovl_imm(1,rt);
  }
}

static void emit_sltiu64_32(int rsh,int rsl,int imm,int rt)
{
  assert(rsh!=29);
  assert(rsl!=29);
  assert(rt!=29);
  assert(rsh!=rt);
  emit_sltiu32(rsl,imm,rt);
  if(imm>=0)
  {
    emit_test(rsh,rsh);
    emit_cmovne_imm(0,rt);
  }
  else
  {
    emit_cmpimm(rsh,-1);
    emit_cmovne_imm(1,rt);
  }
}

static void emit_cmp(int rs,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("cmp %s,%s",regname[rs],regname[rt]);
  output_w32(0x6b000000|rt<<16|rs<<5|WZR);
}

static void emit_set_gz32(int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  //assem_debug("set_gz32");
  emit_cmpimm(rs,1);
  emit_movimm(1,rt);
  emit_cmovl_imm(0,rt);
}

static void emit_set_nz32(int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  //assem_debug("set_nz32");
  if(rs!=rt) emit_mov(rs,rt);
  emit_test(rs,rs);
  emit_cmovne_imm(1,rt);
}

static void emit_set_gz64_32(int rsh, int rsl, int rt)
{
  assert(rsh!=29);
  assert(rsl!=29);
  assert(rt!=29);
  //assem_debug("set_gz64");
  emit_set_gz32(rsl,rt);
  emit_test(rsh,rsh);
  emit_cmovne_imm(1,rt);
  emit_cmovs_imm(0,rt);
}

static void emit_set_nz64_32(int rsh, int rsl, int rt)
{
  assert(rsh!=29);
  assert(rsl!=29);
  assert(rt!=29);
  //assem_debug("set_nz64");
  emit_or(rsh,rsl,rt);
  emit_test(rt,rt);
  emit_cmovne_imm(1,rt);
}

static void emit_set_if_less32(int rs1, int rs2, int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  //assem_debug("set if less (%%%s,%%%s),%%%s",regname[rs1],regname[rs2],regname[rt]);
  if(rs1!=rt&&rs2!=rt) emit_zeroreg(rt);
  emit_cmp(rs1,rs2);
  if(rs1==rt||rs2==rt) emit_movimm(0,rt);
  emit_cmovl_imm(1,rt);
}

static void emit_set_if_carry32(int rs1, int rs2, int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  //assem_debug("set if carry (%%%s,%%%s),%%%s",regname[rs1],regname[rs2],regname[rt]);
  if(rs1!=rt&&rs2!=rt) emit_zeroreg(rt);
  emit_cmp(rs1,rs2);
  if(rs1==rt||rs2==rt) emit_movimm(0,rt);
  emit_cmovb_imm(1,rt);
}

static void emit_set_if_less64_32(int u1, int l1, int u2, int l2, int rt)
{
  assert(u1!=29);
  assert(l1!=29);
  assert(u2!=29);
  assert(l2!=29);
  assert(rt!=29);
  //assem_debug("set if less64 (%%%s,%%%s,%%%s,%%%s),%%%s",regname[u1],regname[l1],regname[u2],regname[l2],regname[rt]);
  assert(u1!=rt);
  assert(u2!=rt);
  emit_cmp(l1,l2);
  emit_movimm(0,rt);
  emit_sbcs(u1,u2,HOST_TEMPREG);
  emit_cmovl_imm(1,rt);
}

static void emit_set_if_carry64_32(int u1, int l1, int u2, int l2, int rt)
{
  assert(u1!=29);
  assert(l1!=29);
  assert(u2!=29);
  assert(l2!=29);
  assert(rt!=29);
  //assem_debug("set if carry64 (%%%s,%%%s,%%%s,%%%s),%%%s",regname[u1],regname[l1],regname[u2],regname[l2],regname[rt]);
  assert(u1!=rt);
  assert(u2!=rt);
  emit_cmp(l1,l2);
  emit_movimm(0,rt);
  emit_sbcs(u1,u2,HOST_TEMPREG);
  emit_cmovb_imm(1,rt);
}

static void emit_call(intptr_t a)
{
  assem_debug("bl %x (%x+%x)",a,(intptr_t)out,a-(intptr_t)out);
  u_int offset=genjmp(a);
  output_w32(0x94000000|offset);
}

static void emit_jmp(intptr_t a)
{
  assem_debug("b %x (%x+%x)",a,(intptr_t)out,a-(intptr_t)out);
  u_int offset=genjmp(a);
  output_w32(0x14000000|offset);
}

static void emit_jne(intptr_t a)
{
  assem_debug("bne %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|NE);
}

static void emit_jeq(intptr_t a)
{
  assem_debug("beq %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|EQ);
}

static void emit_js(intptr_t a)
{
  assem_debug("bmi %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|MI);
}

static void emit_jns(intptr_t a)
{
  assem_debug("bpl %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|PL);
}

static void emit_jl(intptr_t a)
{
  assem_debug("blt %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|LT);
}

static void emit_jge(intptr_t a)
{
  assem_debug("bge %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|GE);
}

static void emit_jno(intptr_t a)
{
  assem_debug("bvc %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|VC);
}

static void emit_jcc(intptr_t a)
{
  assem_debug("bcc %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|CC);
}

static void emit_jae(intptr_t a)
{
  assem_debug("bcs %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|CS);
}

static void emit_jb(intptr_t a)
{
  assem_debug("bcc %x",a);
  u_int offset=gencondjmp(a);
  output_w32(0x54000000|offset<<5|CC);
}

static void emit_pushreg(u_int r)
{
  assert(0);
  assem_debug("push %%%s",regname[r]);
}

static void emit_popreg(u_int r)
{
  assert(0);
  assem_debug("pop %%%s",regname[r]);
}

static void emit_jmpreg(u_int r)
{
  assem_debug("br %s",regname64[r]);
  output_w32(0xd61f0000|r<<5);
}

static void emit_readword_indexed(int offset, int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(offset>-256&&offset<256);
  assem_debug("ldur %s,%s+%d",regname[rt],regname64[rs],offset);
  output_w32(0xb8400000|((u_int)offset&0x1ff)<<12|rs<<5|rt);
}

static void emit_readword_indexed64(int offset, int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(offset>-256&&offset<256);
  assem_debug("ldur %s,%s+%d",regname64[rt],regname64[rs],offset);
  output_w32(0xf8400000|((u_int)offset&0x1ff)<<12|rs<<5|rt);
}

static void emit_readword_dualindexedx4(int rs1, int rs2, int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("ldr %s, [%s,%s lsl #2]",regname[rt],regname64[rs1],regname64[rs2]);
  output_w32(0xb8607800|rs2<<16|rs1<<5|rt);
}

static void emit_readword64_dualindexedx8(int rs1, int rs2, int rt)
{
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("ldr %s, [%s,%s lsl #3]",regname64[rt],regname64[rs1],regname64[rs2]);
  output_w32(0xf8607800|rs2<<16|rs1<<5|rt);
}

static void emit_readword_indexed_tlb(int addr, int rs, int map, int rt)
{
  assert(map>=0);
  assert(rs!=29);
  assert(map!=29);
  assert(rt!=29);
  if(map<0) emit_readword_indexed(addr, rs, rt);
  else {
    assert(addr==0);
    emit_readword_dualindexedx4(rs, map, rt);
  }
}

static void emit_readdword_indexed_tlb(int addr, int rs, int map, int rh, int rl)
{
  assert(map>=0);
  assert(rs!=29);
  assert(map!=29);
  assert(rh!=29);
  assert(rl!=29);
  if(map<0) {
    if(rh>=0) emit_readword_indexed(addr, rs, rh);
    emit_readword_indexed(addr+4, rs, rl);
  }else{
    assert(rh!=rs);
    if(rh>=0) emit_readword_indexed_tlb(addr, rs, map, rh);
    emit_addimm64(map,1,HOST_TEMPREG);
    emit_readword_indexed_tlb(addr, rs, HOST_TEMPREG, rl);
  }
}

static void emit_movsbl_indexed(int offset, int rs, int rt)
{
  assert(0); /*Should not happen*/
}

static void emit_movsbl_indexed_tlb(int addr, int rs, int map, int rt)
{
  assert(map>=0);
  assert(rt!=29&&rt!=HOST_TEMPREG);
  assert(rs!=29&&rt!=HOST_TEMPREG);
  assert(map!=29&&rt!=HOST_TEMPREG);
  if(map<0) emit_movsbl_indexed(addr, rs, rt);
  else {
    if(addr==0) {
      emit_shlimm64(map,2,HOST_TEMPREG);
      assem_debug("ldrsb %s,[%s,%s]",regname[rt],regname64[rs],regname64[HOST_TEMPREG]);
      output_w32(0x38a06800|HOST_TEMPREG<<16|rs<<5|rt);
    }else{
      emit_addimm(rs,addr,rt);
      emit_shlimm64(map,2,HOST_TEMPREG);
      assem_debug("ldrsb %s,[%s,%s]",regname[rt],regname64[rt],regname64[HOST_TEMPREG]);
      output_w32(0x38a06800|HOST_TEMPREG<<16|rt<<5|rt);
    }
  }
}

static void emit_movswl_indexed(int offset, int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(offset>-256&&offset<256);
  assem_debug("ldursh %s,%s+%d",regname[rt],regname64[rs],offset);
  output_w32(0x78800000|((u_int)offset&0x1ff)<<12|rs<<5|rt);
}

static void emit_movzbl_indexed(int offset, int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(offset>-256&&offset<256);
  assem_debug("ldurb %s,%s+%d",regname[rt],regname64[rs],offset);
  output_w32(0x38400000|((u_int)offset&0x1ff)<<12|rs<<5|rt);
}

static void emit_movzbl_indexed_tlb(int addr, int rs, int map, int rt)
{
  assert(map>=0);
  assert(rt!=29&&rt!=HOST_TEMPREG);
  assert(rs!=29&&rt!=HOST_TEMPREG);
  assert(map!=29&&rt!=HOST_TEMPREG);
  if(map<0) emit_movzbl_indexed(addr, rs, rt);
  else {
    if(addr==0) {
      emit_shlimm64(map,2,HOST_TEMPREG);
      assem_debug("ldrb %s,[%s,%s]",regname[rt],regname64[rs],regname64[HOST_TEMPREG]);
      output_w32(0x38606800|HOST_TEMPREG<<16|rs<<5|rt);
    }else{
      emit_addimm(rs,addr,rt);
      emit_shlimm64(map,2,HOST_TEMPREG);
      assem_debug("ldrb %s,[%s,%s]",regname[rt],regname64[rt],regname64[HOST_TEMPREG]);
      output_w32(0x38606800|HOST_TEMPREG<<16|rt<<5|rt);
    }
  }
}

static void emit_movzwl_indexed(int offset, int rs, int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(offset>-256&&offset<256);
  assem_debug("ldurh %s,%s+%d",regname[rt],regname64[rs],offset);
  output_w32(0x78400000|((u_int)offset&0x1ff)<<12|rs<<5|rt);
}

static void emit_readword(intptr_t addr, int rt)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<16380);
  assert(offset%4 == 0); /* 4 bytes aligned */
  assem_debug("ldr %s,fp+%d",regname[rt],offset);
  output_w32(0xb9400000|((offset>>2)<<10)|(FP<<5)|rt);
}

static void emit_readword64(intptr_t addr, int rt)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<32760);
  assert(offset%8 == 0); /* 8 bytes aligned */
  assem_debug("ldr %s,fp+%d",regname64[rt],offset);
  output_w32(0xf9400000|((offset>>3)<<10)|(FP<<5)|rt);
}

static void emit_movsbl(int addr, int rt)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<4096);
  assem_debug("ldrsb %s,fp+%d",regname[rt],offset);
  output_w32(0x39800000|offset<<10|FP<<5|rt);
}

static void emit_movswl(int addr, int rt)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<8190);
  assert(offset%2 == 0); /* 2 bytes aligned */
  assem_debug("ldrsh %s,fp+%d",regname[rt],offset);
  output_w32(0x79800000|((offset>>1)<<10)|(FP<<5)|rt);
}

static void emit_movzbl(intptr_t addr, int rt)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<4096);
  assem_debug("ldrb %s,fp+%d",regname[rt],offset);
  output_w32(0x39400000|offset<<10|FP<<5|rt);
}

static void emit_movzwl(int addr, int rt)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<8190);
  assert(offset%2 == 0); /* 2 bytes aligned */
  assem_debug("ldrh %s,fp+%d",regname[rt],offset);
  output_w32(0x79400000|((offset>>1)<<10)|(FP<<5)|rt);
}

static void emit_writeword_indexed(int rt, int offset, int rs)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(offset>-256&&offset<256);
  assem_debug("stur %s,%s+%d",regname[rt],regname64[rs],offset);
  output_w32(0xb8000000|(((u_int)offset)&0x1ff)<<12|rs<<5|rt);
}

static void emit_writeword_indexed64(int rt, int offset, int rs)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(offset>-256&&offset<256);
  assem_debug("stur %s,%s+%d",regname64[rt],regname64[rs],offset);
  output_w32(0xf8000000|(((u_int)offset)&0x1ff)<<12|rs<<5|rt);
}

static void emit_writeword_dualindexedx4(int rt, int rs1, int rs2)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("str %s,[%s,%s lsl #2]",regname[rt],regname64[rs1],regname64[rs2]);
  output_w32(0xb8207800|rs2<<16|rs1<<5|rt);
}

static void emit_writeword_indexed_tlb(int rt, int addr, int rs, int map, int temp)
{
  assert(map>=0);
  assert(rs!=29);
  assert(map!=29);
  assert(rt!=29);
  if(map<0) emit_writeword_indexed(rt, addr, rs);
  else {
    assert(addr==0);
    emit_writeword_dualindexedx4(rt, rs, map);
  }
}

static void emit_writedword_indexed_tlb(int rh, int rl, int addr, int rs, int map, int temp)
{
  assert(map>=0);
  assert(rh!=29);
  assert(rl!=29);
  assert(map!=29);
  assert(rs!=29);
  assert(temp!=29);
  if(map<0) {
    if(rh>=0) emit_writeword_indexed(rh, addr, rs);
    emit_writeword_indexed(rl, addr+4, rs);
  }else{
    assert(rh>=0);
    if(temp!=rs) emit_addimm64(map,1,temp);
    emit_writeword_indexed_tlb(rh, addr, rs, map, temp);
    if(temp!=rs) emit_writeword_indexed_tlb(rl, addr, rs, temp, temp);
    else {
      emit_addimm(rs,4,rs);
      emit_writeword_indexed_tlb(rl, addr, rs, map, temp);
    }
  }
}

static void emit_writehword_indexed(int rt, int offset, int rs)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(offset>-256&&offset<256);
  assem_debug("sturh %s,%s+%d",regname[rt],regname64[rs],offset);
  output_w32(0x78000000|(((u_int)offset)&0x1ff)<<12|rs<<5|rt);
}

static void emit_writebyte_indexed(int rt, int offset, int rs)
{
  assert(rs!=29);
  assert(rt!=29);
  assert(offset>-256&&offset<256);
  assem_debug("sturb %s,%s+%d",regname[rt],regname64[rs],offset);
  output_w32(0x38000000|(((u_int)offset)&0x1ff)<<12|rs<<5|rt);
}

static void emit_writebyte_indexed_tlb(int rt, int addr, int rs, int map, int temp)
{
  assert(map>=0);
  assert(rt!=29&&rt!=HOST_TEMPREG);
  assert(rs!=29&&rt!=HOST_TEMPREG);
  assert(map!=29&&rt!=HOST_TEMPREG);
  assert(temp!=29&&rt!=HOST_TEMPREG);
  if(map<0) emit_writebyte_indexed(rt, addr, rs);
  else {
    if(addr==0) {
      emit_shlimm64(map,2,HOST_TEMPREG);
      assem_debug("strb %s,[%s,%s]",regname[rt],regname64[rs],regname64[HOST_TEMPREG]);
      output_w32(0x38206800|HOST_TEMPREG<<16|rs<<5|rt);
    }else{
      emit_addimm(rs,addr,temp);
      emit_shlimm64(map,2,HOST_TEMPREG);
      assem_debug("strb %s,[%s,%s]",regname[rt],regname64[temp],regname64[HOST_TEMPREG]);
      output_w32(0x38206800|HOST_TEMPREG<<16|temp<<5|rt);
    }
  }
}

static void emit_writeword(int rt, intptr_t addr)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<16380);
  assert(offset%4 == 0); /* 4 bytes aligned */
  assem_debug("str %s,fp+%d",regname[rt],offset);
  output_w32(0xb9000000|((offset>>2)<<10)|(FP<<5)|rt);
}

static void emit_writeword64(int rt, intptr_t addr)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<32760);
  assert(offset%8 == 0); /* 8 bytes aligned */
  assem_debug("str %s,fp+%d",regname64[rt],offset);
  output_w32(0xf9000000|((offset>>3)<<10)|(FP<<5)|rt);
}

static void emit_writehword(int rt, int addr)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<8190);
  assert(offset%2 == 0); /* 2 bytes aligned */
  assem_debug("strh %s,fp+%d",regname[rt],offset);
  output_w32(0x79000000|((offset>>1)<<10)|(FP<<5)|rt);
}

static void emit_writebyte(int rt, intptr_t addr)
{
  assert(rt!=29);
  u_int offset = addr-(uintptr_t)&dynarec_local;
  assert(offset<4096);
  assem_debug("strb %s,fp+%d",regname[rt],offset);
  output_w32(0x39000000|offset<<10|(FP<<5)|rt);
}

static void emit_msub(u_int rs1,u_int rs2,u_int rs3,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("msub %s,%s,%s,%s",regname[rt],regname[rs1],regname[rs2],regname[rs3]);
  output_w32(0x1b008000|(rs2<<16)|(rs3<<10)|(rs1<<5)|rt);
}

static void emit_mul64(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("mul %s,%s,%s",regname64[rt],regname64[rs1],regname64[rs2]);
  output_w32(0x9b000000|(rs2<<16)|(WZR<<10)|(rs1<<5)|rt);
}

static void emit_msub64(u_int rs1,u_int rs2,u_int rs3,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("msub %s,%s,%s,%s",regname64[rt],regname64[rs1],regname64[rs2],regname64[rs3]);
  output_w32(0x9b008000|(rs2<<16)|(rs3<<10)|(rs1<<5)|rt);
}

static void emit_umull(u_int rs1, u_int rs2, u_int rt)
{
  assem_debug("umull %s, %s, %s",regname64[rt],regname[rs1],regname[rs2]);
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  output_w32(0x9ba00000|(rs2<<16)|(WZR<<10)|(rs1<<5)|rt);
}

static void emit_umulh(u_int rs1, u_int rs2, u_int rt)
{
  assem_debug("umulh %s, %s, %s",regname64[rt],regname64[rs1],regname64[rs2]);
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  output_w32(0x9bc00000|(rs2<<16)|(WZR<<10)|(rs1<<5)|rt);
}

static void emit_smull(u_int rs1, u_int rs2, u_int rt)
{
  assem_debug("smull %s, %s, %s",regname64[rt],regname[rs1],regname[rs2]);
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  output_w32(0x9b200000|(rs2<<16)|(WZR<<10)|(rs1<<5)|rt);
}

static void emit_smulh(u_int rs1, u_int rs2, u_int rt)
{
  assem_debug("smulh %s, %s, %s",regname64[rt],regname64[rs1],regname64[rs2]);
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  output_w32(0x9b400000|(rs2<<16)|(WZR<<10)|(rs1<<5)|rt);
}

static void emit_sdiv(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("sdiv %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x1ac00c00|(rs2<<16)|(rs1<<5)|rt);
}

static void emit_udiv(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("udiv %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x1ac00800|(rs2<<16)|(rs1<<5)|rt);
}

static void emit_sdiv64(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("sdiv %s,%s,%s",regname64[rt],regname64[rs1],regname64[rs2]);
  output_w32(0x9ac00c00|(rs2<<16)|(rs1<<5)|rt);
}

static void emit_udiv64(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("udiv %s,%s,%s",regname64[rt],regname64[rs1],regname64[rs2]);
  output_w32(0x9ac00800|(rs2<<16)|(rs1<<5)|rt);
}

static void emit_bic(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("bic %s,%s,%s",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x0a200000|rs2<<16|rs1<<5|rt);
}

static void emit_bic64(u_int rs1,u_int rs2,u_int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("bic %s,%s,%s",regname64[rt],regname64[rs1],regname64[rs2]);
  output_w32(0x8a200000|rs2<<16|rs1<<5|rt);
}

// Load 2 immediates optimizing for small code size
static void emit_mov2imm_compact(int imm1,u_int rt1,int imm2,u_int rt2)
{
  assert(rt1!=29);
  assert(rt2!=29);
  emit_movimm(imm1,rt1);
  int imm=imm2-imm1;
  if(imm<0&&imm>-4096) {
    assem_debug("sub %s, %s, #%d",regname[rt2],regname[rt1],-imm&0xfff);
    output_w32(0x51000000|((-imm)&0xfff)<<10|rt1<<5|rt2);
  }else if(imm>=0&&imm<4096) {
    assem_debug("add %s, %s, #%d",regname[rt2],regname[rt1],imm&0xfff);
    output_w32(0x11000000|(imm&0xfff)<<10|rt1<<5|rt2);
  }else if(imm<0&&(-imm&0xfff)==0) {
    assem_debug("sub %s, %s, #%d lsl #%d",regname[rt2],regname[rt1],((-imm)>>12)&0xfff,12);
    output_w32(0x51400000|(((-imm)>>12)&0xfff)<<10|rt1<<5|rt2);
  }else if(imm>=0&&(imm&0xfff)==0) {
    assem_debug("add %s, %s, #%d lsl #%d",regname[rt2],regname[rt1],(imm>>12)&0xfff,12);
    output_w32(0x11400000|((imm>>12)&0xfff)<<10|rt1<<5|rt2);
  }
  else emit_movimm(imm2,rt2);
}

#ifdef HAVE_CMOV_IMM
// Conditionally select one of two immediates, optimizing for small code size
// This will only be called if HAVE_CMOV_IMM is defined
static void emit_cmov2imm_e_ne_compact(int imm1,int imm2,u_int rt)
{
  assert(0);
}
#endif

#ifndef HOST_IMM8
// special case for checking invalid_code
static void emit_cmpmem_indexedsr12_imm(int addr,int r,int imm)
{
  assert(0);
}
#endif

// special case for checking invalid_code
static void emit_cmpmem_indexedsr12_reg(int base,int r,int imm)
{
  assert(imm<128&&imm>=0);
  assert(r>=0&&r<29);
  emit_shrimm(r,12,HOST_TEMPREG);
  assem_debug("ldrb %s,[%s,%s]",regname[HOST_TEMPREG],regname64[base],regname64[HOST_TEMPREG]);
  output_w32(0x38606800|HOST_TEMPREG<<16|base<<5|HOST_TEMPREG);
  emit_cmpimm(HOST_TEMPREG,imm);
}

// special case for tlb mapping
static void emit_addsr12(int rs1,int rs2,int rt)
{
  assert(rs1!=29);
  assert(rs2!=29);
  assert(rt!=29);
  assem_debug("add %s,%s,%s lsr #12",regname[rt],regname[rs1],regname[rs2]);
  output_w32(0x0b400000|rs2<<16|12<<10|rs1<<5|rt);
}

#ifdef HAVE_CONDITIONAL_CALL
static void emit_callne(intptr_t a)
{
  assert(0);
}
#endif

#ifdef IMM_PREFETCH
// Used to preload hash table entries
static void emit_prefetch(void *addr)
{
  assert(0);
}
#endif

#ifdef REG_PREFETCH
static void emit_prefetchreg(int r)
{
  assert(0);
}
#endif

static void emit_flds(int r,int sr)
{
  assert(r!=29);
  assert((sr==30)||(sr==31));
  assem_debug("ldr s%d,[%s]",sr,regname[r]);
  output_w32(0xbd400000|r<<5|sr);
} 

static void emit_fldd(int r,int dr)
{
  assert(r!=29);
  assert((dr==30)||(dr==31));
  assem_debug("ldr d%d,[%s]",dr,regname[r]);
  output_w32(0xfd400000|r<<5|dr);
} 

static void emit_fsts(int sr,int r)
{
  assert(r!=29);
  assert((sr==30)||(sr==31));
  assem_debug("str s%d,[%s]",sr,regname[r]);
  output_w32(0xbd000000|r<<5|sr);
} 

static void emit_fstd(int dr,int r)
{
  assert(r!=29);
  assert((dr==30)||(dr==31));
  assem_debug("str d%d,[%s]",dr,regname[r]);
  output_w32(0xfd000000|r<<5|dr);
} 

static void emit_fsqrts(int s,int d)
{
  assert(s==31);
  assert(d==31);
  assem_debug("fsqrts s%d,s%d",d,s);
  output_w32(0x1e21c000|s<<5|d);
} 

static void emit_fsqrtd(int s,int d)
{
  assert(s==31);
  assert(d==31);
  assem_debug("fsqrtd d%d,d%d",d,s);
  output_w32(0x1e61c000|s<<5|d);
} 

static void emit_fabss(int s,int d)
{
  assert(s==31);
  assert(d==31);
  assem_debug("fabss s%d,s%d",d,s);
  output_w32(0x1e20c000|s<<5|d);
} 

static void emit_fabsd(int s,int d)
{
  assert(s==31);
  assert(d==31);
  assem_debug("fabsd d%d,d%d",d,s);
  output_w32(0x1e60c000|s<<5|d);
} 

static void emit_fnegs(int s,int d)
{
  assert(s==31);
  assert(d==31);
  assem_debug("fnegs s%d,s%d",d,s);
  output_w32(0x1e214000|s<<5|d);
} 

static void emit_fnegd(int s,int d)
{
  assert(s==31);
  assert(d==31);
  assem_debug("fnegd d%d,d%d",d,s);
  output_w32(0x1e614000|s<<5|d);
} 

static void emit_fadds(int s1,int s2,int d)
{
  assert(s1==31);
  assert((s2==30)||(s2==31));
  assert(d==31);
  assem_debug("fadds s%d,s%d,s%d",d,s1,s2);
  output_w32(0x1e202800|s2<<16|s1<<5|d);
} 

static void emit_faddd(int s1,int s2,int d)
{
  assert(s1==31);
  assert((s2==30)||(s2==31));
  assert(d==31);
  assem_debug("faddd d%d,d%d,d%d",d,s1,s2);
  output_w32(0x1e602800|s2<<16|s1<<5|d);
} 

static void emit_fsubs(int s1,int s2,int d)
{
  assert(s1==31);
  assert((s2==30)||(s2==31));
  assert(d==31);
  assem_debug("fsubs s%d,s%d,s%d",d,s1,s2);
  output_w32(0x1e203800|s2<<16|s1<<5|d);
} 

static void emit_fsubd(int s1,int s2,int d)
{
  assert(s1==31);
  assert((s2==30)||(s2==31));
  assert(d==31);
  assem_debug("fsubd d%d,d%d,d%d",d,s1,s2);
  output_w32(0x1e603800|s2<<16|s1<<5|d);
} 

static void emit_fmuls(int s1,int s2,int d)
{
  assert(s1==31);
  assert((s2==30)||(s2==31));
  assert(d==31);
  assem_debug("fmuls s%d,s%d,s%d",d,s1,s2);
  output_w32(0x1e200800|s2<<16|s1<<5|d);
} 

static void emit_fmuld(int s1,int s2,int d)
{
  assert(s1==31);
  assert((s2==30)||(s2==31));
  assert(d==31);
  assem_debug("fmuld d%d,d%d,d%d",d,s1,s2);
  output_w32(0x1e600800|s2<<16|s1<<5|d);
} 

static void emit_fdivs(int s1,int s2,int d)
{
  assert(s1==31);
  assert((s2==30)||(s2==31));
  assert(d==31);
  assem_debug("fdivs s%d,s%d,s%d",d,s1,s2);
  output_w32(0x1e201800|s2<<16|s1<<5|d);
} 

static void emit_fdivd(int s1,int s2,int d)
{
  assert(s1==31);
  assert((s2==30)||(s2==31));
  assert(d==31);
  assem_debug("fdivd d%d,d%d,d%d",d,s1,s2);
  output_w32(0x1e601800|s2<<16|s1<<5|d);
}

static void emit_scvtf_s_w(int rs,int rd)
{
  //int32_t -> float
  assert(rs!=29);
  assert(rd==31);
  assem_debug("scvtf s%d,%s",rd,regname[rs]);
  output_w32(0x1e220000|rs<<5|rd);
}

static void emit_scvtf_d_w(int rs,int rd)
{
  //int32_t -> double
  assert(rs!=29);
  assert(rd==31);
  assem_debug("scvtf d%d,%s",rd,regname[rs]);
  output_w32(0x1e620000|rs<<5|rd);
}

static void emit_scvtf_s_l(int rs,int rd)
{
  //int64_t -> float
  assert(rs!=29);
  assert(rd==31);
  assem_debug("scvtf s%d,%s",rd,regname64[rs]);
  output_w32(0x9e220000|rs<<5|rd);
}

static void emit_scvtf_d_l(int rs,int rd)
{
  //int64_t -> double
  assert(rs!=29);
  assert(rd==31);
  assem_debug("scvtf d%d,%s",rd,regname64[rs]);
  output_w32(0x9e620000|rs<<5|rd);
}

static void emit_fcvt_d_s(int rs,int rd)
{
  //float -> double
  assert(rs==31);
  assert(rd==31);
  assem_debug("fcvt d%d,s%d",rd,rs);
  output_w32(0x1e22c000|rs<<5|rd);
}

static void emit_fcvt_s_d(int rs,int rd)
{
  //double -> float
  assert(rs==31);
  assert(rd==31);
  assem_debug("fcvt s%d,d%d",rd,rs);
  output_w32(0x1e624000|rs<<5|rd);
}

static void emit_fcvtns_l_s(int rs,int rd)
{
  //float -> int64_t round toward nearest
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtns %s,s%d,",regname64[rd],rs);
  output_w32(0x9e200000|rs<<5|rd);
}

static void emit_fcvtns_w_s(int rs,int rd)
{
  //float -> int32_t round toward nearest
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtns %s,s%d,",regname[rd],rs);
  output_w32(0x1e200000|rs<<5|rd);
}

static void emit_fcvtns_l_d(int rs,int rd)
{
  //double -> int64_t round toward nearest
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtns %s,d%d,",regname64[rd],rs);
  output_w32(0x9e600000|rs<<5|rd);
}

static void emit_fcvtns_w_d(int rs,int rd)
{
  //double -> int32_t round toward nearest
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtns %s,d%d,",regname[rd],rs);
  output_w32(0x1e600000|rs<<5|rd);
}

static void emit_fcvtzs_l_s(int rs,int rd)
{
  //float -> int64_t round toward zero
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtzs %s,s%d,",regname64[rd],rs);
  output_w32(0x9e380000|rs<<5|rd);
}

static void emit_fcvtzs_w_s(int rs,int rd)
{
  //float -> int32_t round toward zero
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtzs %s,s%d,",regname[rd],rs);
  output_w32(0x1e380000|rs<<5|rd);
}

static void emit_fcvtzs_l_d(int rs,int rd)
{
  //double -> int64_t round toward zero
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtzs %s,d%d,",regname64[rd],rs);
  output_w32(0x9e780000|rs<<5|rd);
}

static void emit_fcvtzs_w_d(int rs,int rd)
{
  //double -> int32_t round toward zero
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtzs %s,d%d,",regname[rd],rs);
  output_w32(0x1e780000|rs<<5|rd);
}

static void emit_fcvtps_l_s(int rs,int rd)
{
  //float -> int64_t round toward +inf
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtps %s,s%d,",regname64[rd],rs);
  output_w32(0x9e280000|rs<<5|rd);
}

static void emit_fcvtps_w_s(int rs,int rd)
{
  //float -> int32_t round toward +inf
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtps %s,s%d,",regname[rd],rs);
  output_w32(0x1e280000|rs<<5|rd);
}

static void emit_fcvtps_l_d(int rs,int rd)
{
  //double -> int64_t round toward +inf
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtps %s,d%d,",regname64[rd],rs);
  output_w32(0x9e680000|rs<<5|rd);
}

static void emit_fcvtps_w_d(int rs,int rd)
{
  //double -> int32_t round toward +inf
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtps %s,d%d,",regname[rd],rs);
  output_w32(0x1e680000|rs<<5|rd);
}

static void emit_fcvtms_l_s(int rs,int rd)
{
  //float -> int64_t round toward -inf
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtms %s,s%d,",regname64[rd],rs);
  output_w32(0x9e300000|rs<<5|rd);
}

static void emit_fcvtms_w_s(int rs,int rd)
{
  //float -> int32_t round toward -inf
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtms %s,s%d,",regname[rd],rs);
  output_w32(0x1e300000|rs<<5|rd);
}

static void emit_fcvtms_l_d(int rs,int rd)
{
  //double -> int64_t round toward -inf
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtms %s,d%d,",regname64[rd],rs);
  output_w32(0x9e700000|rs<<5|rd);
}

static void emit_fcvtms_w_d(int rs,int rd)
{
  //double -> int32_t round toward -inf
  assert(rs==31);
  assert(rd!=29);
  assem_debug("fcvtms %s,d%d,",regname[rd],rs);
  output_w32(0x1e700000|rs<<5|rd);
}

static void emit_fcmps(int x,int y)
{
  assert(x==30);
  assert(y==31);
  assem_debug("fcmp s%d, s%d",x,y);
  output_w32(0x1e202000|y<<16|x<<5);
} 

static void emit_fcmpd(int x,int y)
{
  assert(x==30);
  assert(y==31);
  assem_debug("fcmp d%d, d%d",x,y);
  output_w32(0x1e602000|y<<16|x<<5);
}

static void emit_jno_unlikely(int a)
{
  emit_jno(a);
  //assem_debug("addvc pc,pc,#? (%x)",/*a-(int)out-8,*/a);
  //output_w32(0x72800000|rd_rn_rm(15,15,0));
}

static void emit_breakpoint(u_int imm)
{
  assem_debug("brk #%d",imm);
  output_w32(0xd4200000|imm<<5);
}

static void emit_adr(intptr_t addr, int rt)
{
  int offset=addr-(intptr_t)out;
  assert(offset>=-1048576&&offset<1048576);
  assem_debug("adr %d,#%d",regname64[rt],offset);
  output_w32(0x10000000|((u_int)offset&0x3)<<29|(((u_int)offset>>2)&0x7ffff)<<5|rt);
}

static void emit_read_ptr(intptr_t addr, int rt)
{
  int offset=addr-(intptr_t)out;
  if(offset>=-1048576&&offset<1048576){
    assem_debug("adr %d,#%d",regname64[rt],offset);
    output_w32(0x10000000|((u_int)offset&0x3)<<29|(((u_int)offset>>2)&0x7ffff)<<5|rt);
  }
  else{
    offset=((addr&(intptr_t)~0xfff)-((intptr_t)out&(intptr_t)~0xfff))>>12;
    assert((((intptr_t)out&(intptr_t)~0xfff)+(offset<<12))==(addr&(intptr_t)~0xfff));
    assem_debug("adrp %d,#%d",regname64[rt],offset);
    output_w32(0x90000000|((u_int)offset&0x3)<<29|(((u_int)offset>>2)&0x7ffff)<<5|rt);
    if((addr&(intptr_t)0xfff)!=0)
      assem_debug("add %s, %s, #%d",regname64[rt],regname64[rt],addr&0xfff);
      output_w32(0x91000000|(addr&0xfff)<<10|rt<<5|rt);
  }
}

static void emit_sxtw(int rs,int rt)
{
  assert(rs!=29);
  assert(rt!=29);
  assem_debug("sxtw %s,%s",regname64[rt],regname[rs]);
  output_w32(0x93407c00|rs<<5|rt);
}

// Save registers before function call
static void save_regs(u_int reglist)
{
  signed char rt[2];
  int index=0;
  int offset=0;

  reglist&=0x7ffff; // only save the caller-save registers, x0-x18
  if(!reglist) return;

  int i;
  for(i=0; reglist!=0; i++){
    if(reglist&1){
      rt[index]=i;
      index++;
    }
    if(index>1){
      assert(offset>=0&&offset<=136);
      assem_debug("stp %s,%s,[fp+#%d]",regname64[rt[0]],regname64[rt[1]],offset);
      output_w32(0xa9000000|(offset>>3)<<15|rt[1]<<10|FP<<5|rt[0]);
      offset+=16;
      index=0;
    }
    reglist>>=1;
  }

  if(index!=0) {
    assert(index==1);
    assert(offset>=0&&offset<=144);
    assem_debug("str %s,[fp+#%d]",regname64[rt[0]],offset);
    output_w32(0xf9000000|(offset>>3)<<10|FP<<5|rt[0]);
  }
}
// Restore registers after function call
static void restore_regs(u_int reglist)
{
  signed char rt[2];
  int index=0;
  int offset=0;

  reglist&=0x7ffff; // only restore the caller-save registers, x0-x18
  if(!reglist) return;

  int i;
  for(i=0; reglist!=0; i++){
    if(reglist&1){
      rt[index]=i;
      index++;
    }
    if(index>1){
      assert(offset>=0&&offset<=136);
      assem_debug("ldp %s,%s,[fp+#%d]",regname[rt[0]],regname[rt[1]],offset);
      output_w32(0xa9400000|(offset>>3)<<15|rt[1]<<10|FP<<5|rt[0]);
      offset+=16;
      index=0;
    }
    reglist>>=1;
  }

  if(index!=0) {
    assert(index==1);
    assert(offset>=0&&offset<=144);
    assem_debug("ldr %s,[fp+#%d]",regname[rt[0]],offset);
    output_w32(0xf9400000|(offset>>3)<<10|FP<<5|rt[0]);
  }
}

// Write out a single register
static void wb_register_arm64(signed char r,signed char regmap[],uint64_t dirty,uint64_t is32)
{
  int hr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if((regmap[hr]&63)==r) {
        if((dirty>>hr)&1) {
          if(regmap[hr]<64) {
            if((is32>>regmap[hr])&1) {
              emit_sxtw(hr,hr);
              emit_storereg64(r,hr);
            }
            else
              emit_storereg(r,hr);
          }else{
            emit_storereg(r|64,hr);
          }
        }
      }
    }
  }
}
#define wb_register wb_register_arm64

// Write out all dirty registers (except cycle count)
static void wb_dirtys_arm64(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty)
{
  int hr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(i_regmap[hr]>0) {
        if(i_regmap[hr]!=CCREG) {
          if((i_dirty>>hr)&1) {
            if(i_regmap[hr]<64) {
              if( ((i_is32>>i_regmap[hr])&1) ) {
                emit_sxtw(hr,HOST_TEMPREG);
                emit_storereg64(i_regmap[hr],HOST_TEMPREG);
              }
              else
                emit_storereg(i_regmap[hr],hr);
            }else{
              if( !((i_is32>>(i_regmap[hr]&63))&1) ) {
                emit_storereg(i_regmap[hr],hr);
              }
            }
          }
        }
      }
    }
  }
}
#define wb_dirtys wb_dirtys_arm64

// Write out dirty registers that we need to reload (pair with load_needed_regs)
// This writes the registers not written by store_regs_bt
static void wb_needed_dirtys_arm64(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty,int addr)
{
  int hr;
  int t=(addr-start)>>2;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(i_regmap[hr]>0) {
        if(i_regmap[hr]!=CCREG) {
          if(i_regmap[hr]==regs[t].regmap_entry[hr] && ((regs[t].dirty>>hr)&1) && !(((i_is32&~regs[t].was32&~unneeded_reg_upper[t])>>(i_regmap[hr]&63))&1)) {
            if((i_dirty>>hr)&1) {
              if(i_regmap[hr]<64) {
                if( ((i_is32>>i_regmap[hr])&1) ) {
                  emit_sxtw(hr,HOST_TEMPREG);
                  emit_storereg64(i_regmap[hr],HOST_TEMPREG);
                }
                else
                  emit_storereg(i_regmap[hr],hr);
              }else{
                if( !((i_is32>>(i_regmap[hr]&63))&1) ) {
                  emit_storereg(i_regmap[hr],hr);
                }
              }
            }
          }
        }
      }
    }
  }
}
#define wb_needed_dirtys wb_needed_dirtys_arm64

// Store dirty registers prior to branch
static void store_regs_bt_arm64(signed char i_regmap[],uint64_t i_is32,uint64_t i_dirty,int addr)
{
  if(internal_branch(i_is32,addr))
  {
    int t=(addr-start)>>2;
    int hr;
    for(hr=0;hr<HOST_REGS;hr++) {
      if(hr!=EXCLUDE_REG) {
        if(i_regmap[hr]>0 && i_regmap[hr]!=CCREG) {
          if(i_regmap[hr]!=regs[t].regmap_entry[hr] || !((regs[t].dirty>>hr)&1) || (((i_is32&~regs[t].was32&~unneeded_reg_upper[t])>>(i_regmap[hr]&63))&1)) {
            if((i_dirty>>hr)&1) {
              if(i_regmap[hr]<64) {
                if(!((unneeded_reg[t]>>i_regmap[hr])&1)) {
                  if( ((i_is32>>i_regmap[hr])&1) && !((unneeded_reg_upper[t]>>i_regmap[hr])&1) ) {
                    emit_sxtw(hr,HOST_TEMPREG);
                    emit_storereg64(i_regmap[hr],HOST_TEMPREG);
                  }
                  else
                    emit_storereg(i_regmap[hr],hr);
                }
              }else{
                if( !((i_is32>>(i_regmap[hr]&63))&1) && !((unneeded_reg_upper[t]>>(i_regmap[hr]&63))&1) ) {
                  emit_storereg(i_regmap[hr],hr);
                }
              }
            }
          }
        }
      }
    }
  }
  else
  {
    // Branch out of this block, write out all dirty regs
    wb_dirtys(i_regmap,i_is32,i_dirty);
  }
}
#define store_regs_bt store_regs_bt_arm64

// Sign-extend to 64 bits and write out upper half of a register
// This is useful where we have a 32-bit value in a register, and want to
// keep it in a 32-bit register, but can't guarantee that it won't be read
// as a 64-bit value later.
static void wb_sx(signed char pre[],signed char entry[],uint64_t dirty,uint64_t is32_pre,uint64_t is32,uint64_t u,uint64_t uu)
{
  if(is32_pre==is32) return;
  int hr,tr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      //if(pre[hr]==entry[hr]) {
        if((tr=pre[hr])>=0) {
          if((dirty>>hr)&1) {
            if( ((is32_pre&~is32&~uu)>>tr)&1 ) {
              emit_sarimm(hr,31,HOST_TEMPREG);
              emit_storereg(tr|64,HOST_TEMPREG);
            }
          }
        }
      //}
    }
  }
}

static void wb_valid(signed char pre[],signed char entry[],u_int dirty_pre,u_int dirty,uint64_t is32_pre,uint64_t u,uint64_t uu)
{
  //if(dirty_pre==dirty) return;
  int hr,tr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      tr=pre[hr];
      if(((~u)>>(tr&63))&1) {
        if(tr>0) {
          if(((dirty_pre&~dirty)>>hr)&1) {
            if(tr>0&&tr<36) {
              if(((is32_pre&~uu)>>tr)&1) {
                emit_sxtw(hr,HOST_TEMPREG);
                emit_storereg64(tr,HOST_TEMPREG);
              }
              else
                emit_storereg(tr,hr);
            }
            else if(tr>=64) {
              emit_storereg(tr,hr);
            }
          }
        }
      }
    }
  }
}

// Write back consts using LR so we don't disturb the other registers
static void wb_consts(signed char i_regmap[],uint64_t i_is32,u_int i_dirty,int i)
{
  int hr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG&&i_regmap[hr]>=0&&((i_dirty>>hr)&1)) {
      if(((regs[i].isconst>>hr)&1)&&i_regmap[hr]>0) {
        if(i_regmap[hr]<64 || !((i_is32>>(i_regmap[hr]&63))&1) ) {
          int value=constmap[i][hr];
          if(value==0) {
            emit_zeroreg(HOST_TEMPREG);
          }
          else {
            emit_movimm(value,HOST_TEMPREG);
          }
          if((i_is32>>i_regmap[hr])&1) {
            if(value!=-1&&value!=0) emit_sxtw(HOST_TEMPREG,HOST_TEMPREG);
            emit_storereg64(i_regmap[hr],HOST_TEMPREG);
          }
          else
            emit_storereg(i_regmap[hr],HOST_TEMPREG);
        }
      }
    }
  }
}

static void wb_invalidate_arm64(signed char pre[],signed char entry[],uint64_t dirty,uint64_t is32,
  uint64_t u,uint64_t uu)
{
  int hr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(pre[hr]!=entry[hr]) {
        if(pre[hr]>=0) {
          if((dirty>>hr)&1) {
            if(get_reg(entry,pre[hr])<0) {
              if(pre[hr]<64) {
                if(!((u>>pre[hr])&1)) {
                  if( ((is32>>pre[hr])&1) && !((uu>>pre[hr])&1) ) {
                    emit_sxtw(hr,hr);
                    emit_storereg64(pre[hr],hr);
                  }
                  else
                    emit_storereg(pre[hr],hr);
                }
              }else{
                if(!((uu>>(pre[hr]&63))&1) && !((is32>>(pre[hr]&63))&1)) {
                  emit_storereg(pre[hr],hr);
                }
              }
            }
          }
        }
      }
    }
  }
  // Move from one register to another (no writeback)
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(pre[hr]!=entry[hr]) {
        if(pre[hr]>=0&&(pre[hr]&63)<TEMPREG) {
          int nr;
          if((nr=get_reg(entry,pre[hr]))>=0) {
            if((pre[hr]!=INVCP)&&(pre[hr]!=ROREG))
              emit_mov(hr,nr);
            else
              emit_mov64(hr,nr);
          }
        }
      }
    }
  }
}
#define wb_invalidate wb_invalidate_arm64

/* Stubs/epilogue */

static void literal_pool(int n)
{
  if(!literalcount) return;
  assert(0); /* Should not happen */
}

static void literal_pool_jumpover(int n)
{
  if(!literalcount) return;
  assert(0); /* Should not happen */
}

static void emit_extjump2(intptr_t addr, int target, intptr_t linker)
{
  u_char *ptr=(u_char *)addr;
  assert(((ptr[3]&0xfc)==0x14)||((ptr[3]&0xff)==0x54)); //b or b.cond

  emit_movz_lsl16(((u_int)target>>16)&0xffff,1);
  emit_movk((u_int)target&0xffff,1);

  //addr is in the current recompiled block (max 256k)
  //offset shouldn't exceed +/-1MB 
  emit_adr(addr,0);

#ifdef DEBUG_CYCLE_COUNT
  emit_readword((intptr_t)&last_count,HOST_TEMPREG);
  emit_add(HOST_CCREG,HOST_TEMPREG,HOST_CCREG);
  emit_readword((intptr_t)&next_interrupt,HOST_TEMPREG);
  emit_writeword(HOST_CCREG,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
  emit_sub(HOST_CCREG,HOST_TEMPREG,HOST_CCREG);
  emit_writeword(HOST_TEMPREG,(intptr_t)&last_count);
#endif
  emit_call(linker);
  emit_jmpreg(0);
}

static void emit_extjump(intptr_t addr, int target)
{
  emit_extjump2(addr, target, (intptr_t)dynamic_linker);
}
static void emit_extjump_ds(intptr_t addr, int target)
{
  emit_extjump2(addr, target, (intptr_t)dynamic_linker_ds);
}

static void do_readstub(int n)
{
  assem_debug("do_readstub %x",start+stubs[n][3]*4);
  literal_pool(256);
  set_jump_target(stubs[n][1],(intptr_t)out);
  int type=stubs[n][0];
  int i=stubs[n][3];
  int rs=stubs[n][4];
  struct regstat *i_regs=(struct regstat *)stubs[n][5];
  u_int reglist=stubs[n][7];
  signed char *i_regmap=i_regs->regmap;
  int addr=get_reg(i_regmap,AGEN1+(i&1));
  int rth,rt;
  int ds;
  if(itype[i]==C1LS||itype[i]==LOADLR) {
    rth=get_reg(i_regmap,FTEMP|64);
    rt=get_reg(i_regmap,FTEMP);
  }else{
    rth=get_reg(i_regmap,rt1[i]|64);
    rt=get_reg(i_regmap,rt1[i]);
  }
  assert(rs>=0);
  if(addr<0) addr=rt;
  if(addr<0&&itype[i]!=C1LS&&itype[i]!=LOADLR) addr=get_reg(i_regmap,-1);
  assert(addr>=0);
  intptr_t ftable=0;
  if(type==LOADB_STUB||type==LOADBU_STUB)
    ftable=(intptr_t)readmemb;
  if(type==LOADH_STUB||type==LOADHU_STUB)
    ftable=(intptr_t)readmemh;
  if(type==LOADW_STUB)
    ftable=(intptr_t)readmem;
  if(type==LOADD_STUB)
    ftable=(intptr_t)readmemd;
  emit_writeword(rs,(intptr_t)&address);
  //emit_pusha();
  save_regs(reglist);
  ds=i_regs!=&regs[i];
  int real_rs=(itype[i]==LOADLR)?-1:get_reg(i_regmap,rs1[i]);
  u_int cmask=ds?-1:(0x7ffff|~i_regs->wasconst);
  if(!ds) load_all_consts(regs[i].regmap_entry,regs[i].was32,regs[i].wasdirty&~(1<<addr)&(real_rs<0?-1:~(1<<real_rs))&0x7ffff,i);
  wb_dirtys(i_regs->regmap_entry,i_regs->was32,i_regs->wasdirty&cmask&~(1<<addr)&(real_rs<0?-1:~(1<<real_rs)));
  if(!ds) wb_consts(regs[i].regmap_entry,regs[i].was32,regs[i].wasdirty&~(1<<addr)&(real_rs<0?-1:~(1<<real_rs))&~0x7ffff,i);
  emit_shrimm(rs,16,1);
  int cc=get_reg(i_regmap,CCREG);
  if(cc<0) {
    emit_loadreg(CCREG,2);
  }
  emit_read_ptr(ftable,0);
  emit_addimm(cc<0?2:cc,2*stubs[n][6]+2,2);
  emit_movimm(start+stubs[n][3]*4+(((regs[i].was32>>rs1[i])&1)<<1)+ds,3);
  //emit_readword((int)&last_count,temp);
  //emit_add(cc,temp,cc);
  //emit_writeword(cc,(int)&g_cp0_regs[CP0_COUNT_REG]);
  //emit_mov(15,14);
  emit_call((intptr_t)&indirect_jump_indexed);
  //emit_callreg(rs);
  //emit_readword_dualindexedx4(rs,HOST_TEMPREG,15);
  // We really shouldn't need to update the count here,
  // but not doing so causes random crashes...
  emit_readword((intptr_t)&g_cp0_regs[CP0_COUNT_REG],HOST_TEMPREG);
  emit_readword((intptr_t)&next_interrupt,2);
  emit_addimm(HOST_TEMPREG,-2*stubs[n][6]-2,HOST_TEMPREG);
  emit_writeword(2,(intptr_t)&last_count);
  emit_sub(HOST_TEMPREG,2,cc<0?HOST_TEMPREG:cc);
  if(cc<0) {
    emit_storereg(CCREG,HOST_TEMPREG);
  }
  //emit_popa();
  restore_regs(reglist);
  //if((cc=get_reg(regmap,CCREG))>=0) {
  //  emit_loadreg(CCREG,cc);
  //}
  if(rt>=0) {
    if(type==LOADB_STUB)
      emit_movsbl((intptr_t)&readmem_dword,rt);
    if(type==LOADBU_STUB)
      emit_movzbl((intptr_t)&readmem_dword,rt);
    if(type==LOADH_STUB)
      emit_movswl((intptr_t)&readmem_dword,rt);
    if(type==LOADHU_STUB)
      emit_movzwl((intptr_t)&readmem_dword,rt);
    if(type==LOADW_STUB)
      emit_readword((intptr_t)&readmem_dword,rt);
    if(type==LOADD_STUB) {
      emit_readword((intptr_t)&readmem_dword,rt);
      if(rth>=0) emit_readword(((intptr_t)&readmem_dword)+4,rth);
    }
  }
  emit_jmp(stubs[n][2]); // return address
}

static void inline_readstub(int type, int i, u_int addr, signed char regmap[], int target, int adj, u_int reglist)
{
  int rs=get_reg(regmap,target);
  int rth=get_reg(regmap,target|64);
  int rt=get_reg(regmap,target);
  if(rs<0) rs=get_reg(regmap,-1);
  assert(rs>=0);
  intptr_t ftable=0;
  if(type==LOADB_STUB||type==LOADBU_STUB)
    ftable=(intptr_t)readmemb;
  if(type==LOADH_STUB||type==LOADHU_STUB)
    ftable=(intptr_t)readmemh;
  if(type==LOADW_STUB)
    ftable=(intptr_t)readmem;
  if(type==LOADD_STUB)
    ftable=(intptr_t)readmemd;
  emit_writeword(rs,(intptr_t)&address);
  //emit_pusha();
  save_regs(reglist);
  if((signed int)addr>=(signed int)0xC0000000) {
    // Theoretically we can have a pagefault here, if the TLB has never
    // been enabled and the address is outside the range 80000000..BFFFFFFF
    // Write out the registers so the pagefault can be handled.  This is
    // a very rare case and likely represents a bug.
    int ds=regmap!=regs[i].regmap;
    if(!ds) load_all_consts(regs[i].regmap_entry,regs[i].was32,regs[i].wasdirty,i);
    if(!ds) wb_dirtys(regs[i].regmap_entry,regs[i].was32,regs[i].wasdirty);
    else wb_dirtys(branch_regs[i-1].regmap_entry,branch_regs[i-1].was32,branch_regs[i-1].wasdirty);
  }
  //emit_shrimm(rs,16,1);
  int cc=get_reg(regmap,CCREG);
  if(cc<0) {
    emit_loadreg(CCREG,2);
  }
  //emit_movimm(ftable,0);
  emit_read_ptr(((uintptr_t *)ftable)[addr>>16],0);
  //emit_readword((int)&last_count,12);
  emit_addimm(cc<0?2:cc,CLOCK_DIVIDER*(adj+1),2);
  if((signed int)addr>=(signed int)0xC0000000) {
    // Pagefault address
    int ds=regmap!=regs[i].regmap;
    emit_movimm(start+i*4+(((regs[i].was32>>rs1[i])&1)<<1)+ds,3);
  }
  //emit_add(12,2,2);
  //emit_writeword(2,(int)&g_cp0_regs[CP0_COUNT_REG]);
  //emit_call(((u_int *)ftable)[addr>>16]);
  emit_call((intptr_t)&indirect_jump);
  // We really shouldn't need to update the count here,
  // but not doing so causes random crashes...
  emit_readword((intptr_t)&g_cp0_regs[CP0_COUNT_REG],HOST_TEMPREG);
  emit_readword((intptr_t)&next_interrupt,2);
  emit_addimm(HOST_TEMPREG,-(int)CLOCK_DIVIDER*(adj+1),HOST_TEMPREG);
  emit_writeword(2,(intptr_t)&last_count);
  emit_sub(HOST_TEMPREG,2,cc<0?HOST_TEMPREG:cc);
  if(cc<0) {
    emit_storereg(CCREG,HOST_TEMPREG);
  }
  //emit_popa();
  restore_regs(reglist);
  if(rt>=0) {
    if(type==LOADB_STUB)
      emit_movsbl((intptr_t)&readmem_dword,rt);
    if(type==LOADBU_STUB)
      emit_movzbl((intptr_t)&readmem_dword,rt);
    if(type==LOADH_STUB)
      emit_movswl((intptr_t)&readmem_dword,rt);
    if(type==LOADHU_STUB)
      emit_movzwl((intptr_t)&readmem_dword,rt);
    if(type==LOADW_STUB)
      emit_readword((intptr_t)&readmem_dword,rt);
    if(type==LOADD_STUB) {
      emit_readword((intptr_t)&readmem_dword,rt);
      if(rth>=0) emit_readword(((intptr_t)&readmem_dword)+4,rth);
    }
  }
}

static void do_writestub(int n)
{
  assem_debug("do_writestub %x",start+stubs[n][3]*4);
  literal_pool(256);
  set_jump_target(stubs[n][1],(intptr_t)out);
  int type=stubs[n][0];
  int i=stubs[n][3];
  int rs=stubs[n][4];
  struct regstat *i_regs=(struct regstat *)stubs[n][5];
  u_int reglist=stubs[n][7];
  signed char *i_regmap=i_regs->regmap;
  int addr=get_reg(i_regmap,AGEN1+(i&1));
  int rth,rt,r;
  int ds;
  if(itype[i]==C1LS) {
    rth=get_reg(i_regmap,FTEMP|64);
    rt=get_reg(i_regmap,r=FTEMP);
  }else{
    rth=get_reg(i_regmap,rs2[i]|64);
    rt=get_reg(i_regmap,r=rs2[i]);
  }
  assert(rs>=0);
  assert(rt>=0);
  if(addr<0) addr=get_reg(i_regmap,-1);
  assert(addr>=0);
  intptr_t ftable=0;
  if(type==STOREB_STUB)
    ftable=(intptr_t)writememb;
  if(type==STOREH_STUB)
    ftable=(intptr_t)writememh;
  if(type==STOREW_STUB)
    ftable=(intptr_t)writemem;
  if(type==STORED_STUB)
    ftable=(intptr_t)writememd;
  emit_writeword(rs,(intptr_t)&address);
  //emit_shrimm(rs,16,rs);
  //emit_movmem_indexedx4(ftable,rs,rs);
  if(type==STOREB_STUB)
    emit_writebyte(rt,(intptr_t)&cpu_byte);
  if(type==STOREH_STUB)
    emit_writehword(rt,(intptr_t)&cpu_hword);
  if(type==STOREW_STUB)
    emit_writeword(rt,(intptr_t)&cpu_word);
  if(type==STORED_STUB) {
    emit_writeword(rt,(intptr_t)&cpu_dword);
    emit_writeword(r?rth:rt,(intptr_t)&cpu_dword+4);
  }
  //emit_pusha();
  save_regs(reglist);
  ds=i_regs!=&regs[i];
  int real_rs=get_reg(i_regmap,rs1[i]);
  u_int cmask=ds?-1:(0x7ffff|~i_regs->wasconst);
  if(!ds) load_all_consts(regs[i].regmap_entry,regs[i].was32,regs[i].wasdirty&~(1<<addr)&(real_rs<0?-1:~(1<<real_rs))&0x7ffff,i);
  wb_dirtys(i_regs->regmap_entry,i_regs->was32,i_regs->wasdirty&cmask&~(1<<addr)&(real_rs<0?-1:~(1<<real_rs)));
  if(!ds) wb_consts(regs[i].regmap_entry,regs[i].was32,regs[i].wasdirty&~(1<<addr)&(real_rs<0?-1:~(1<<real_rs))&~0x7ffff,i);
  emit_shrimm(rs,16,1);
  int cc=get_reg(i_regmap,CCREG);
  if(cc<0) {
    emit_loadreg(CCREG,2);
  }
  emit_read_ptr(ftable,0);
  emit_addimm(cc<0?2:cc,2*stubs[n][6]+2,2);
  emit_movimm(start+stubs[n][3]*4+(((regs[i].was32>>rs1[i])&1)<<1)+ds,3);
  //emit_readword((int)&last_count,temp);
  //emit_addimm(cc,2*stubs[n][5]+2,cc);
  //emit_add(cc,temp,cc);
  //emit_writeword(cc,(int)&g_cp0_regs[CP0_COUNT_REG]);
  emit_call((intptr_t)&indirect_jump_indexed);
  //emit_callreg(rs);
  emit_readword((intptr_t)&g_cp0_regs[CP0_COUNT_REG],HOST_TEMPREG);
  emit_readword((intptr_t)&next_interrupt,2);
  emit_addimm(HOST_TEMPREG,-2*stubs[n][6]-2,HOST_TEMPREG);
  emit_writeword(2,(intptr_t)&last_count);
  emit_sub(HOST_TEMPREG,2,cc<0?HOST_TEMPREG:cc);
  if(cc<0) {
    emit_storereg(CCREG,HOST_TEMPREG);
  }
  //emit_popa();
  restore_regs(reglist);
  //if((cc=get_reg(regmap,CCREG))>=0) {
  //  emit_loadreg(CCREG,cc);
  //}
  emit_jmp(stubs[n][2]); // return address
}

static void inline_writestub(int type, int i, u_int addr, signed char regmap[], int target, int adj, u_int reglist)
{
  int rs=get_reg(regmap,-1);
  int rth=get_reg(regmap,target|64);
  int rt=get_reg(regmap,target);
  assert(rs>=0);
  assert(rt>=0);
  intptr_t ftable=0;
  if(type==STOREB_STUB)
    ftable=(intptr_t)writememb;
  if(type==STOREH_STUB)
    ftable=(intptr_t)writememh;
  if(type==STOREW_STUB)
    ftable=(intptr_t)writemem;
  if(type==STORED_STUB)
    ftable=(intptr_t)writememd;
  emit_writeword(rs,(intptr_t)&address);
  //emit_shrimm(rs,16,rs);
  //emit_movmem_indexedx4(ftable,rs,rs);
  if(type==STOREB_STUB)
    emit_writebyte(rt,(intptr_t)&cpu_byte);
  if(type==STOREH_STUB)
    emit_writehword(rt,(intptr_t)&cpu_hword);
  if(type==STOREW_STUB)
    emit_writeword(rt,(intptr_t)&cpu_word);
  if(type==STORED_STUB) {
    emit_writeword(rt,(intptr_t)&cpu_dword);
    emit_writeword(target?rth:rt,(intptr_t)&cpu_dword+4);
  }
  //emit_pusha();
  save_regs(reglist);
  if((signed int)addr>=(signed int)0xC0000000) {
    // Theoretically we can have a pagefault here, if the TLB has never
    // been enabled and the address is outside the range 80000000..BFFFFFFF
    // Write out the registers so the pagefault can be handled.  This is
    // a very rare case and likely represents a bug.
    int ds=regmap!=regs[i].regmap;
    if(!ds) load_all_consts(regs[i].regmap_entry,regs[i].was32,regs[i].wasdirty,i);
    if(!ds) wb_dirtys(regs[i].regmap_entry,regs[i].was32,regs[i].wasdirty);
    else wb_dirtys(branch_regs[i-1].regmap_entry,branch_regs[i-1].was32,branch_regs[i-1].wasdirty);
  }
  //emit_shrimm(rs,16,1);
  int cc=get_reg(regmap,CCREG);
  if(cc<0) {
    emit_loadreg(CCREG,2);
  }
  //emit_movimm(ftable,0);
  emit_read_ptr(((uintptr_t *)ftable)[addr>>16],0);
  //emit_readword((int)&last_count,12);
  emit_addimm(cc<0?2:cc,CLOCK_DIVIDER*(adj+1),2);
  if((signed int)addr>=(signed int)0xC0000000) {
    // Pagefault address
    int ds=regmap!=regs[i].regmap;
    emit_movimm(start+i*4+(((regs[i].was32>>rs1[i])&1)<<1)+ds,3);
  }
  //emit_add(12,2,2);
  //emit_writeword(2,(int)&g_cp0_regs[CP0_COUNT_REG]);
  //emit_call(((u_int *)ftable)[addr>>16]);
  emit_call((intptr_t)&indirect_jump);
  emit_readword((intptr_t)&g_cp0_regs[CP0_COUNT_REG],HOST_TEMPREG);
  emit_readword((intptr_t)&next_interrupt,2);
  emit_addimm(HOST_TEMPREG,-(int)CLOCK_DIVIDER*(adj+1),HOST_TEMPREG);
  emit_writeword(2,(intptr_t)&last_count);
  emit_sub(HOST_TEMPREG,2,cc<0?HOST_TEMPREG:cc);
  if(cc<0) {
    emit_storereg(CCREG,HOST_TEMPREG);
  }
  //emit_popa();
  restore_regs(reglist);
}

static void do_unalignedwritestub(int n)
{
  set_jump_target(stubs[n][1],(intptr_t)out);
  emit_breakpoint(0);
  emit_jmp(stubs[n][2]); // return address
}

static void do_invstub(int n)
{
  literal_pool(20);
  u_int reglist=stubs[n][3];
  set_jump_target(stubs[n][1],(intptr_t)out);
  save_regs(reglist);
  if(stubs[n][4]!=0) emit_mov(stubs[n][4],0);
  emit_call((intptr_t)&invalidate_addr);
  restore_regs(reglist);
  emit_jmp(stubs[n][2]); // return address
}

static intptr_t do_dirty_stub(int i)
{
  assem_debug("do_dirty_stub %x",start+i*4);

  // Careful about the code output here, verify_dirty and get_bounds needs to parse it.
  if((int)start<(int)0xC0000000){
    emit_read_ptr((intptr_t)source,1);
  }else{
    emit_movz_lsl16(((u_int)start>>16)&0xffff,1);
    emit_movk(((u_int)start)&0xffff,1);
  }

  emit_read_ptr((intptr_t)copy,2);

  emit_movz(slen*4,3);
  emit_movimm(start+i*4,0);
  emit_call((int)start<(int)0xC0000000?(intptr_t)&verify_code:(intptr_t)&verify_code_vm);
  intptr_t entry=(intptr_t)out;
  load_regs_entry(i);
  if(entry==(intptr_t)out) entry=instr_addr[i];
  emit_jmp(instr_addr[i]);
  return entry;
}

static void do_dirty_stub_ds(void)
{
  // Careful about the code output here, verify_dirty and get_bounds needs to parse it.
  if((int)start<(int)0xC0000000){
    emit_read_ptr((intptr_t)source,1);
  }else{
    emit_movz_lsl16(((u_int)start>>16)&0xffff,1);
    emit_movk(((u_int)start)&0xffff,1);
  }

  emit_read_ptr((intptr_t)copy,2);

  emit_movz(slen*4,3);
  emit_movimm(start+1,0);
  emit_call((intptr_t)&verify_code_ds);
}

static void do_cop1stub(int n)
{
  literal_pool(256);
  assem_debug("do_cop1stub %x",start+stubs[n][3]*4);
  set_jump_target(stubs[n][1],(intptr_t)out);
  int i=stubs[n][3];
  //int rs=stubs[n][4];
  struct regstat *i_regs=(struct regstat *)stubs[n][5];
  int ds=stubs[n][6];
  if(!ds) {
    load_all_consts(regs[i].regmap_entry,regs[i].was32,regs[i].wasdirty,i);
    //if(i_regs!=&regs[i]) DebugMessage(M64MSG_VERBOSE, "oops: regs[i]=%x i_regs=%x",(int)&regs[i],(int)i_regs);
  }
  //else {DebugMessage(M64MSG_ERROR, "fp exception in delay slot");}
  wb_dirtys(i_regs->regmap_entry,i_regs->was32,i_regs->wasdirty);
  if(regs[i].regmap_entry[HOST_CCREG]!=CCREG) emit_loadreg(CCREG,HOST_CCREG);
  emit_movimm(start+(i-ds)*4,EAX); // Get PC
  emit_addimm(HOST_CCREG,CLOCK_DIVIDER*ccadj[i],HOST_CCREG); // CHECK: is this right?  There should probably be an extra cycle...
  emit_jmp(ds?(intptr_t)fp_exception_ds:(intptr_t)fp_exception);
}

/* TLB */

static int do_tlb_r(int s,int ar,int map,int cache,int x,int a,int shift,int c,u_int addr)
{
  if(c) {
    if((signed int)addr>=(signed int)0xC0000000) {
      // address_generation already loaded the const
      emit_readword64_dualindexedx8(FP,map,map);
    }
    else
      return -1; // No mapping
  }
  else {
    assert(s!=map);
    if(cache>=0) {
      // Use cached offset to memory map
      emit_addsr12(cache,s,map);
    }else{
      emit_loadreg(MMREG,map);
      emit_addsr12(map,s,map);
    }
    // Schedule this while we wait on the load
    //if(x) emit_xorimm(s,x,ar);
    if(shift>=0) emit_shlimm(s,3,shift);
    if(~a) emit_andimm(s,a,ar);
    emit_readword64_dualindexedx8(FP,map,map);
  }
  return map;
}

static int do_tlb_r_branch(int map, int c, u_int addr, intptr_t *jaddr)
{
  if(!c||(signed int)addr>=(signed int)0xC0000000) {
    emit_test64(map,map);
    *jaddr=(intptr_t)out;
    emit_js(0);
  }
  return map;
}

static void gen_tlb_addr_r(int ar, int map) {
  if(map>=0) {
    assem_debug("add %s,%s,%s lsl #2",regname64[ar],regname64[ar],regname64[map]);
    output_w32(0x8b000000|map<<16|2<<10|ar<<5|ar);
  }
}

static int do_tlb_w(int s,int ar,int map,int cache,int x,int c,u_int addr)
{
  if(c) {
    if(addr<0x80800000||addr>=0xC0000000) {
      // address_generation already loaded the const
      emit_readword64_dualindexedx8(FP,map,map);
    }
    else
      return -1; // No mapping
  }
  else {
    assert(s!=map);
    if(cache>=0) {
      // Use cached offset to memory map
      emit_addsr12(cache,s,map);
    }else{
      emit_loadreg(MMREG,map);
      emit_addsr12(map,s,map);
    }
    // Schedule this while we wait on the load
    //if(x) emit_xorimm(s,x,ar);
    emit_readword64_dualindexedx8(FP,map,map);
  }
  return map;
}

static void do_tlb_w_branch(int map, int c, u_int addr, intptr_t *jaddr)
{
  if(!c||addr<0x80800000||addr>=0xC0000000) {
    emit_testimm64(map,WRITE_PROTECT);
    *jaddr=(intptr_t)out;
    emit_jne(0);
  }
}

static void gen_tlb_addr_w(int ar, int map) {
  if(map>=0) {
    assem_debug("add %s,%s,%s lsl #2",regname64[ar],regname64[ar],regname64[map]);
    output_w32(0x8b000000|map<<16|2<<10|ar<<5|ar);
  }
}

// This reverses the above operation
static void gen_orig_addr_w(int ar, int map) {
  if(map>=0) {
    assem_debug("sub %s,%s,%s lsl #2",regname[ar],regname[ar],regname[map]);
    output_w32(0xcb000000|map<<16|2<<10|ar<<5|ar);
  }
}

// Generate the address of the memory_map entry, relative to dynarec_local
static void generate_map_const(u_int addr,int tr) {
  //DebugMessage(M64MSG_VERBOSE, "generate_map_const(%x,%s)",addr,regname[tr]);
  emit_movimm((addr>>12)+(((uintptr_t)memory_map-(uintptr_t)&dynarec_local)>>3),tr);
}

#ifdef NEW_DYNAREC_DEBUG
static void do_print_pc(int prog_cpt) {
  save_regs(0x7ffff);
  emit_movimm(prog_cpt,0);
  emit_call((intptr_t)print_pc);
  restore_regs(0x7ffff);
}
#endif

/* Special assem */
static void shift_assemble_arm64(int i,struct regstat *i_regs)
{
  if(rt1[i]) {
    if(opcode2[i]<=0x07) // SLLV/SRLV/SRAV
    {
      signed char s,t,shift;
      t=get_reg(i_regs->regmap,rt1[i]);
      s=get_reg(i_regs->regmap,rs1[i]);
      shift=get_reg(i_regs->regmap,rs2[i]);
      if(t>=0){
        if(rs1[i]==0)
        {
          emit_zeroreg(t);
        }
        else if(rs2[i]==0)
        {
          assert(s>=0);
          if(s!=t) emit_mov(s,t);
        }
        else
        {
          emit_andimm(shift,31,HOST_TEMPREG);
          if(opcode2[i]==4) // SLLV
          {
            emit_shl(s,HOST_TEMPREG,t);
          }
          if(opcode2[i]==6) // SRLV
          {
            emit_shr(s,HOST_TEMPREG,t);
          }
          if(opcode2[i]==7) // SRAV
          {
            emit_sar(s,HOST_TEMPREG,t);
          }
        }
      }
    } else { // DSLLV/DSRLV/DSRAV
      signed char sh,sl,th,tl,shift;
      th=get_reg(i_regs->regmap,rt1[i]|64);
      tl=get_reg(i_regs->regmap,rt1[i]);
      sh=get_reg(i_regs->regmap,rs1[i]|64);
      sl=get_reg(i_regs->regmap,rs1[i]);
      shift=get_reg(i_regs->regmap,rs2[i]);
      if(tl>=0){
        if(rs1[i]==0)
        {
          emit_zeroreg(tl);
          if(th>=0) emit_zeroreg(th);
        }
        else if(rs2[i]==0)
        {
          assert(sl>=0);
          if(sl!=tl) emit_mov(sl,tl);
          if(th>=0&&sh!=th) emit_mov(sh,th);
        }
        else
        {
          // FIXME: What if shift==tl ?
          assert(shift!=tl);
          int temp=get_reg(i_regs->regmap,-1);
          int real_th=th;
          if(th<0&&opcode2[i]!=0x14) {th=temp;} // DSLLV doesn't need a temporary register
          assert(sl>=0);
          assert(sh>=0);
          emit_andimm(shift,31,HOST_TEMPREG);
          if(opcode2[i]==0x14) // DSLLV
          {
            if(th>=0) emit_shl(sh,HOST_TEMPREG,th);
            emit_neg(HOST_TEMPREG,HOST_TEMPREG);
            emit_addimm(HOST_TEMPREG,32,HOST_TEMPREG);
            emit_shr(sl,HOST_TEMPREG,HOST_TEMPREG);
            emit_or(HOST_TEMPREG,th,th);
            emit_andimm(shift,31,HOST_TEMPREG);
            emit_testimm(shift,32);
            emit_shl(sl,HOST_TEMPREG,tl);
            if(th>=0) emit_cmovne_reg(tl,th);
            emit_cmovne_imm(0,tl);
          }
          if(opcode2[i]==0x16) // DSRLV
          {
            assert(th>=0);
            emit_shr(sl,HOST_TEMPREG,tl);
            emit_neg(HOST_TEMPREG,HOST_TEMPREG);
            emit_addimm(HOST_TEMPREG,32,HOST_TEMPREG);
            emit_shl(sh,HOST_TEMPREG,HOST_TEMPREG);
            emit_or(HOST_TEMPREG,tl,tl);
            emit_andimm(shift,31,HOST_TEMPREG);
            emit_testimm(shift,32);
            emit_shr(sh,HOST_TEMPREG,th);
            emit_cmovne_reg(th,tl);
            if(real_th>=0) emit_cmovne_imm(0,th);
          }
          if(opcode2[i]==0x17) // DSRAV
          {
            assert(th>=0);
            emit_shr(sl,HOST_TEMPREG,tl);
            emit_neg(HOST_TEMPREG,HOST_TEMPREG);
            emit_addimm(HOST_TEMPREG,32,HOST_TEMPREG);
            if(real_th>=0) {
              assert(temp>=0);
              emit_sarimm(th,31,temp);
            }
            emit_shl(sh,HOST_TEMPREG,HOST_TEMPREG);
            emit_or(HOST_TEMPREG,tl,tl);
            emit_andimm(shift,31,HOST_TEMPREG);
            emit_testimm(shift,32);
            emit_sar(sh,HOST_TEMPREG,th);
            emit_cmovne_reg(th,tl);
            if(real_th>=0) emit_cmovne_reg(temp,th);
          }
        }
      }
    }
  }
}
#define shift_assemble shift_assemble_arm64

static void load_assemble_arm64(int i,struct regstat *i_regs)
{
  int s,th,tl,addr,map=-1,cache=-1;
  int offset;
  intptr_t jaddr=0;
  int memtarget=0;
  int c=0;
  u_int hr,reglist=0;
  th=get_reg(i_regs->regmap,rt1[i]|64);
  tl=get_reg(i_regs->regmap,rt1[i]);
  s=get_reg(i_regs->regmap,rs1[i]);
  offset=imm[i];
  for(hr=0;hr<HOST_REGS;hr++) {
    if(i_regs->regmap[hr]>=0) reglist|=1<<hr;
  }
  if(i_regs->regmap[HOST_CCREG]==CCREG) reglist&=~(1<<HOST_CCREG);
  if(s>=0) {
    c=(i_regs->wasconst>>s)&1;
    memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80800000;
    if(using_tlb&&((signed int)(constmap[i][s]+offset))>=(signed int)0xC0000000) memtarget=1;
  }
  if(tl<0) tl=get_reg(i_regs->regmap,-1);
  if(offset||s<0||c) addr=tl;
  else addr=s;
  //DebugMessage(M64MSG_VERBOSE, "load_assemble: c=%d",c);
  //if(c) DebugMessage(M64MSG_VERBOSE, "load_assemble: const=%x",(int)constmap[i][s]+offset);
  assert(tl>=0); // Even if the load is a NOP, we must check for pagefaults and I/O
  reglist&=~(1<<tl);
  if(th>=0) reglist&=~(1<<th);
  if(!using_tlb) {
    if(!c) {
//#define R29_HACK 1
      #ifdef R29_HACK
      // Strmnnrmn's speed hack
      if(rs1[i]!=29||start<0x80001000||start>=0x80800000)
      #endif
      {
        emit_cmpimm(addr,0x800000);
        jaddr=(intptr_t)out;
        #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
        // Hint to branch predictor that the branch is unlikely to be taken
        if(rs1[i]>=28)
          emit_jno_unlikely(0);
        else
        #endif
        emit_jno(0);
      }
    }
  }else{ // using tlb
    int x=0;
    if (opcode[i]==0x20||opcode[i]==0x24) x=3; // LB/LBU
    if (opcode[i]==0x21||opcode[i]==0x25) x=2; // LH/LHU
    map=get_reg(i_regs->regmap,TLREG);
    cache=get_reg(i_regs->regmap,MMREG);
    assert(map>=0);
    reglist&=~(1<<map);
    map=do_tlb_r(addr,tl,map,cache,x,-1,-1,c,constmap[i][s]+offset);
    do_tlb_r_branch(map,c,constmap[i][s]+offset,&jaddr);
  }
  #ifdef RAM_OFFSET
  if(map<0) {
    map=get_reg(i_regs->regmap,ROREG);
    if(map<0) emit_loadreg(ROREG,map=HOST_TEMPREG);
  }
  #endif

  int dummy=(rt1[i]==0)||(tl!=get_reg(i_regs->regmap,rt1[i])); // ignore loads to r0 and unneeded reg
  if (opcode[i]==0x20) { // LB
    if(!c||memtarget) {
      if(!dummy) {
        #ifdef HOST_IMM_ADDR32
        if(c)
          emit_movsbl_tlb((constmap[i][s]+offset)^3,map,tl);
        else
        #endif
        {
          //emit_xorimm(addr,3,tl);
          //gen_tlb_addr_r(tl,map);
          //emit_movsbl_indexed((intptr_t)g_rdram-0x80000000,tl,tl);
          int x=0;
          if(!c) emit_xorimm(addr,3,tl);
          else x=((constmap[i][s]+offset)^3)-(constmap[i][s]+offset);
          emit_movsbl_indexed_tlb(x,tl,map,tl);
        }
      }
      if(jaddr)
        add_stub(LOADB_STUB,jaddr,(intptr_t)out,i,addr,(intptr_t)i_regs,ccadj[i],reglist);
    }
    else
      inline_readstub(LOADB_STUB,i,constmap[i][s]+offset,i_regs->regmap,rt1[i],ccadj[i],reglist);
  }
  if (opcode[i]==0x21) { // LH
    if(!c||memtarget) {
      if(!dummy) {
        #ifdef HOST_IMM_ADDR32
        if(c)
          emit_movswl_tlb((constmap[i][s]+offset)^2,map,tl);
        else
        #endif
        {
          int x=0;
          if(!c) emit_xorimm(addr,2,tl);
          else x=((constmap[i][s]+offset)^2)-(constmap[i][s]+offset);
          //#ifdef
          //emit_movswl_indexed_tlb(x,tl,map,tl);
          //else
          if(map>=0) {
            gen_tlb_addr_r(tl,map);
            emit_movswl_indexed(x,tl,tl);
          }else{
            assert(0); /* Should not happen */
            #ifdef RAM_OFFSET
            emit_movswl_indexed(x,tl,tl);
            #else
            emit_movswl_indexed((intptr_t)g_rdram-0x80000000+x,tl,tl);
            #endif
          }
        }
      }
      if(jaddr)
        add_stub(LOADH_STUB,jaddr,(intptr_t)out,i,addr,(intptr_t)i_regs,ccadj[i],reglist);
    }
    else
      inline_readstub(LOADH_STUB,i,constmap[i][s]+offset,i_regs->regmap,rt1[i],ccadj[i],reglist);
  }
  if (opcode[i]==0x23) { // LW
    if(!c||memtarget) {
      if(!dummy) {
        //emit_readword_indexed((intptr_t)g_rdram-0x80000000,addr,tl);
        #ifdef HOST_IMM_ADDR32
        if(c)
          emit_readword_tlb(constmap[i][s]+offset,map,tl);
        else
        #endif
        emit_readword_indexed_tlb(0,addr,map,tl);
      }
      if(jaddr)
        add_stub(LOADW_STUB,jaddr,(intptr_t)out,i,addr,(intptr_t)i_regs,ccadj[i],reglist);
    }
    else
      inline_readstub(LOADW_STUB,i,constmap[i][s]+offset,i_regs->regmap,rt1[i],ccadj[i],reglist);
  }
  if (opcode[i]==0x24) { // LBU
    if(!c||memtarget) {
      if(!dummy) {
        #ifdef HOST_IMM_ADDR32
        if(c)
          emit_movzbl_tlb((constmap[i][s]+offset)^3,map,tl);
        else
        #endif
        {
          //emit_xorimm(addr,3,tl);
          //gen_tlb_addr_r(tl,map);
          //emit_movzbl_indexed((intptr_t)g_rdram-0x80000000,tl,tl);
          int x=0;
          if(!c) emit_xorimm(addr,3,tl);
          else x=((constmap[i][s]+offset)^3)-(constmap[i][s]+offset);
          emit_movzbl_indexed_tlb(x,tl,map,tl);
        }
      }
      if(jaddr)
        add_stub(LOADBU_STUB,jaddr,(intptr_t)out,i,addr,(intptr_t)i_regs,ccadj[i],reglist);
    }
    else
      inline_readstub(LOADBU_STUB,i,constmap[i][s]+offset,i_regs->regmap,rt1[i],ccadj[i],reglist);
  }
  if (opcode[i]==0x25) { // LHU
    if(!c||memtarget) {
      if(!dummy) {
        #ifdef HOST_IMM_ADDR32
        if(c)
          emit_movzwl_tlb((constmap[i][s]+offset)^2,map,tl);
        else
        #endif
        {
          int x=0;
          if(!c) emit_xorimm(addr,2,tl);
          else x=((constmap[i][s]+offset)^2)-(constmap[i][s]+offset);
          //#ifdef
          //emit_movzwl_indexed_tlb(x,tl,map,tl);
          //#else
          if(map>=0) {
            gen_tlb_addr_r(tl,map);
            emit_movzwl_indexed(x,tl,tl);
          }else{
            assert(0); /* Should not happen */
            #ifdef RAM_OFFSET
            emit_movzwl_indexed(x,tl,tl);
            #else
            emit_movzwl_indexed((intptr_t)g_rdram-0x80000000+x,tl,tl);
            #endif
          }
        }
      }
      if(jaddr)
        add_stub(LOADHU_STUB,jaddr,(intptr_t)out,i,addr,(intptr_t)i_regs,ccadj[i],reglist);
    }
    else
      inline_readstub(LOADHU_STUB,i,constmap[i][s]+offset,i_regs->regmap,rt1[i],ccadj[i],reglist);
  }
  if (opcode[i]==0x27) { // LWU
    assert(th>=0);
    if(!c||memtarget) {
      if(!dummy) {
        //emit_readword_indexed((intptr_t)g_rdram-0x80000000,addr,tl);
        #ifdef HOST_IMM_ADDR32
        if(c)
          emit_readword_tlb(constmap[i][s]+offset,map,tl);
        else
        #endif
        emit_readword_indexed_tlb(0,addr,map,tl);
      }
      if(jaddr)
        add_stub(LOADW_STUB,jaddr,(intptr_t)out,i,addr,(intptr_t)i_regs,ccadj[i],reglist);
    }
    else {
      inline_readstub(LOADW_STUB,i,constmap[i][s]+offset,i_regs->regmap,rt1[i],ccadj[i],reglist);
    }
    emit_zeroreg(th);
  }
  if (opcode[i]==0x37) { // LD
    if(!c||memtarget) {
      if(!dummy) {
        //gen_tlb_addr_r(tl,map);
        //if(th>=0) emit_readword_indexed((intptr_t)g_rdram-0x80000000,addr,th);
        //emit_readword_indexed((intptr_t)g_rdram-0x7FFFFFFC,addr,tl);
        #ifdef HOST_IMM_ADDR32
        if(c)
          emit_readdword_tlb(constmap[i][s]+offset,map,th,tl);
        else
        #endif
        emit_readdword_indexed_tlb(0,addr,map,th,tl);
      }
      if(jaddr)
        add_stub(LOADD_STUB,jaddr,(intptr_t)out,i,addr,(intptr_t)i_regs,ccadj[i],reglist);
    }
    else
      inline_readstub(LOADD_STUB,i,constmap[i][s]+offset,i_regs->regmap,rt1[i],ccadj[i],reglist);
  }
  //emit_storereg(rt1[i],tl); // DEBUG
  //if(opcode[i]==0x23)
  //if(opcode[i]==0x24)
  //if(opcode[i]==0x23||opcode[i]==0x24)
  /*if(opcode[i]==0x21||opcode[i]==0x23||opcode[i]==0x24)
  {
    //emit_pusha();
    save_regs(0x100f);
        emit_readword((intptr_t)&last_count,ECX);
        #if NEW_DYNAREC == NEW_DYNAREC_X86
        if(get_reg(i_regs->regmap,CCREG)<0)
          emit_loadreg(CCREG,HOST_CCREG);
        emit_add(HOST_CCREG,ECX,HOST_CCREG);
        emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG);
        emit_writeword(HOST_CCREG,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
        #endif
        #if NEW_DYNAREC == NEW_DYNAREC_ARM
        if(get_reg(i_regs->regmap,CCREG)<0)
          emit_loadreg(CCREG,0);
        else
          emit_mov(HOST_CCREG,0);
        emit_add(0,ECX,0);
        emit_addimm(0,2*ccadj[i],0);
        emit_writeword(0,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
        #endif
    emit_call((intptr_t)memdebug);
    //emit_popa();
    restore_regs(0x100f);
  }*/
}
#define load_assemble load_assemble_arm64

static void loadlr_assemble_arm64(int i,struct regstat *i_regs)
{
  int s,th,tl,temp,temp2,addr,map=-1,cache=-1;
  int offset;
  intptr_t jaddr=0;
  int memtarget=0;
  int c=0;
  u_int hr,reglist=0;
  th=get_reg(i_regs->regmap,rt1[i]|64);
  tl=get_reg(i_regs->regmap,rt1[i]);
  s=get_reg(i_regs->regmap,rs1[i]);
  temp=get_reg(i_regs->regmap,-1);
  temp2=get_reg(i_regs->regmap,FTEMP);
  addr=get_reg(i_regs->regmap,AGEN1+(i&1));
  assert(addr<0);
  offset=imm[i];
  for(hr=0;hr<HOST_REGS;hr++) {
    if(i_regs->regmap[hr]>=0) reglist|=1<<hr;
  }
  reglist|=1<<temp;
  if(offset||s<0||c) addr=temp2;
  else addr=s;
  if(s>=0) {
    c=(i_regs->wasconst>>s)&1;
    memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80800000;
    if(using_tlb&&((signed int)(constmap[i][s]+offset))>=(signed int)0xC0000000) memtarget=1;
  }
  if(!using_tlb) {
    if(!c) {
      emit_shlimm(addr,3,temp);
      if (opcode[i]==0x22||opcode[i]==0x26) {
        emit_andimm(addr,0xFFFFFFFC,temp2); // LWL/LWR
      }else{
        emit_andimm(addr,0xFFFFFFF8,temp2); // LDL/LDR
      }
      emit_cmpimm(addr,0x800000);
      jaddr=(intptr_t)out;
      emit_jno(0);
    }
    else {
      if (opcode[i]==0x22||opcode[i]==0x26) {
        emit_movimm(((constmap[i][s]+offset)<<3)&24,temp); // LWL/LWR
      }else{
        emit_movimm(((constmap[i][s]+offset)<<3)&56,temp); // LDL/LDR
      }
    }
  }else{ // using tlb
    int a;
    if(c) {
      a=-1;
    }else if (opcode[i]==0x22||opcode[i]==0x26) {
      a=0xFFFFFFFC; // LWL/LWR
    }else{
      a=0xFFFFFFF8; // LDL/LDR
    }
    map=get_reg(i_regs->regmap,TLREG);
    cache=get_reg(i_regs->regmap,MMREG); // Get cached offset to memory_map
    assert(map>=0);
    reglist&=~(1<<map);
    map=do_tlb_r(addr,temp2,map,cache,0,a,c?-1:temp,c,constmap[i][s]+offset);
    if(c) {
      if (opcode[i]==0x22||opcode[i]==0x26) {
        emit_movimm(((constmap[i][s]+offset)<<3)&24,temp); // LWL/LWR
      }else{
        emit_movimm(((constmap[i][s]+offset)<<3)&56,temp); // LDL/LDR
      }
    }
    do_tlb_r_branch(map,c,constmap[i][s]+offset,&jaddr);
  }
  #ifdef RAM_OFFSET
  if(map<0) {
    map=get_reg(i_regs->regmap,ROREG);
    if(map<0) emit_loadreg(ROREG,map=HOST_TEMPREG);
  }
  #endif

  if (opcode[i]==0x22||opcode[i]==0x26) { // LWL/LWR
    if(!c||memtarget) {
      //emit_readword_indexed((intptr_t)g_rdram-0x80000000,temp2,temp2);
      emit_readword_indexed_tlb(0,temp2,map,temp2);
      if(jaddr) add_stub(LOADW_STUB,jaddr,(intptr_t)out,i,temp2,(intptr_t)i_regs,ccadj[i],reglist);
    }
    else
      inline_readstub(LOADW_STUB,i,(constmap[i][s]+offset)&0xFFFFFFFC,i_regs->regmap,FTEMP,ccadj[i],reglist);
    if(rt1[i]) {
      assert(tl>=0);
      emit_andimm(temp,24,temp);
      if (opcode[i]==0x26) emit_xorimm(temp,24,temp); // LWR
      emit_movimm(-1,HOST_TEMPREG);
      if (opcode[i]==0x26) {
        emit_shr(temp2,temp,temp2);
        emit_shr(HOST_TEMPREG,temp,HOST_TEMPREG);
        emit_bic(tl,HOST_TEMPREG,tl);
      }else{
        emit_shl(temp2,temp,temp2);
        emit_shl(HOST_TEMPREG,temp,HOST_TEMPREG);
        emit_bic(tl,HOST_TEMPREG,tl);
      }
      emit_or(temp2,tl,tl);
    }
    //emit_storereg(rt1[i],tl); // DEBUG
  }
  if (opcode[i]==0x1A||opcode[i]==0x1B) { // LDL/LDR
    int temp2h=get_reg(i_regs->regmap,FTEMP|64);
    if(!c||memtarget) {
      //if(th>=0) emit_readword_indexed((intptr_t)g_rdram-0x80000000,temp2,temp2h);
      //emit_readword_indexed((intptr_t)g_rdram-0x7FFFFFFC,temp2,temp2);
      emit_readdword_indexed_tlb(0,temp2,map,temp2h,temp2);
      if(jaddr) add_stub(LOADD_STUB,jaddr,(intptr_t)out,i,temp2,(intptr_t)i_regs,ccadj[i],reglist);
    }
    else
      inline_readstub(LOADD_STUB,i,(constmap[i][s]+offset)&0xFFFFFFF8,i_regs->regmap,FTEMP,ccadj[i],reglist);
    if(rt1[i]) {
      assert(th>=0);
      assert(tl>=0);
      emit_andimm(temp,56,temp);
      if (opcode[i]==0x1A) { // LDL
        emit_mov(temp2,HOST_TEMPREG);
        emit_orrshlimm64(temp2h,32,HOST_TEMPREG);
        emit_mov(tl,temp2);
        emit_orrshlimm64(th,32,temp2);
        emit_movn64(0,temp2h);
        emit_shl64(temp2h,temp,temp2h);
        emit_bic64(temp2,temp2h,temp2); 
        emit_shl64(HOST_TEMPREG,temp,HOST_TEMPREG);
        emit_orr64(temp2,HOST_TEMPREG,temp2);
        emit_mov(temp2,tl);
        emit_shrimm64(temp2,32,th);
      }
      if (opcode[i]==0x1B) { // LDR
        emit_xorimm(temp,56,temp);
        emit_mov(temp2,HOST_TEMPREG);
        emit_orrshlimm64(temp2h,32,HOST_TEMPREG);
        emit_mov(tl,temp2);
        emit_orrshlimm64(th,32,temp2);
        emit_movn64(0,temp2h);
        emit_shr64(temp2h,temp,temp2h);
        emit_bic64(temp2,temp2h,temp2); 
        emit_shr64(HOST_TEMPREG,temp,HOST_TEMPREG);
        emit_orr64(temp2,HOST_TEMPREG,temp2);
        emit_mov(temp2,tl);
        emit_shrimm64(temp2,32,th);
      }
    }
  }
}
#define loadlr_assemble loadlr_assemble_arm64

static void store_assemble_arm64(int i,struct regstat *i_regs)
{
  int s,th,tl,map=-1,cache=-1;
  int addr,temp;
  int offset;
  intptr_t jaddr=0;
  intptr_t jaddr2=0;
  int type=0;
  int memtarget=0;
  int c=0;
  int agr=AGEN1+(i&1);
  u_int hr,reglist=0;
  th=get_reg(i_regs->regmap,rs2[i]|64);
  tl=get_reg(i_regs->regmap,rs2[i]);
  s=get_reg(i_regs->regmap,rs1[i]);
  temp=get_reg(i_regs->regmap,agr);
  if(temp<0) temp=get_reg(i_regs->regmap,-1);
  offset=imm[i];
  if(s>=0) {
    c=(i_regs->wasconst>>s)&1;
    memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80800000;
    if(using_tlb&&((signed int)(constmap[i][s]+offset))>=(signed int)0xC0000000) memtarget=1;
  }
  assert(tl>=0);
  assert(temp>=0);
  for(hr=0;hr<HOST_REGS;hr++) {
    if(i_regs->regmap[hr]>=0) reglist|=1<<hr;
  }
  if(i_regs->regmap[HOST_CCREG]==CCREG) reglist&=~(1<<HOST_CCREG);
  if(offset||s<0||c) addr=temp;
  else addr=s;
  if(!using_tlb) {
    if(!c) {
      #ifdef R29_HACK
      // Strmnnrmn's speed hack
      memtarget=1;
      if(rs1[i]!=29||start<0x80001000||start>=0x80800000)
      #endif
      emit_cmpimm(addr,0x800000);
      #ifdef DESTRUCTIVE_SHIFT
      if(s==addr) emit_mov(s,temp);
      #endif
      #ifdef R29_HACK
      if(rs1[i]!=29||start<0x80001000||start>=0x80800000)
      #endif
      {
        jaddr=(intptr_t)out;
        #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
        // Hint to branch predictor that the branch is unlikely to be taken
        if(rs1[i]>=28)
          emit_jno_unlikely(0);
        else
        #endif
        emit_jno(0);
      }
    }
  }else{ // using tlb
    int x=0;
    if (opcode[i]==0x28) x=3; // SB
    if (opcode[i]==0x29) x=2; // SH
    map=get_reg(i_regs->regmap,TLREG);
    cache=get_reg(i_regs->regmap,MMREG);
    assert(map>=0);
    reglist&=~(1<<map);
    map=do_tlb_w(addr,temp,map,cache,x,c,constmap[i][s]+offset);
    do_tlb_w_branch(map,c,constmap[i][s]+offset,&jaddr);
  }
  #ifdef RAM_OFFSET
  if(map<0) {
    map=get_reg(i_regs->regmap,ROREG);
    if(map<0) emit_loadreg(ROREG,map=HOST_TEMPREG);
  }
  #endif

  if (opcode[i]==0x28) { // SB
    if(!c||memtarget) {
      int x=0;
      if(!c) emit_xorimm(addr,3,temp);
      else x=((constmap[i][s]+offset)^3)-(constmap[i][s]+offset);
      //gen_tlb_addr_w(temp,map);
      //emit_writebyte_indexed(tl,(intptr_t)g_rdram-0x80000000,temp);
      emit_writebyte_indexed_tlb(tl,x,temp,map,temp);
    }
    type=STOREB_STUB;
  }
  if (opcode[i]==0x29) { // SH
    if(!c||memtarget) {
      int x=0;
      if(!c) emit_xorimm(addr,2,temp);
      else x=((constmap[i][s]+offset)^2)-(constmap[i][s]+offset);
      //#ifdef
      //emit_writehword_indexed_tlb(tl,x,temp,map,temp);
      //#else
      if(map>=0) {
        gen_tlb_addr_w(temp,map);
        emit_writehword_indexed(tl,x,temp);
      }else {
        assert(0); /* Should not happen */
        emit_writehword_indexed(tl,(intptr_t)g_rdram-0x80000000+x,temp);
      }
    }
    type=STOREH_STUB;
  }
  if (opcode[i]==0x2B) { // SW
    if(!c||memtarget)
      //emit_writeword_indexed(tl,(intptr_t)g_rdram-0x80000000,addr);
      emit_writeword_indexed_tlb(tl,0,addr,map,temp);
    type=STOREW_STUB;
  }
  if (opcode[i]==0x3F) { // SD
    if(!c||memtarget) {
      if(rs2[i]) {
        assert(th>=0);
        //emit_writeword_indexed(th,(intptr_t)g_rdram-0x80000000,addr);
        //emit_writeword_indexed(tl,(intptr_t)g_rdram-0x7FFFFFFC,addr);
        emit_writedword_indexed_tlb(th,tl,0,addr,map,temp);
      }else{
        // Store zero
        //emit_writeword_indexed(tl,(intptr_t)g_rdram-0x80000000,temp);
        //emit_writeword_indexed(tl,(intptr_t)g_rdram-0x7FFFFFFC,temp);
        emit_writedword_indexed_tlb(tl,tl,0,addr,map,temp);
      }
    }
    type=STORED_STUB;
  }
  if(!using_tlb) {
    if(!c||memtarget) {
      #ifdef DESTRUCTIVE_SHIFT
      // The x86 shift operation is 'destructive'; it overwrites the
      // source register, so we need to make a copy first and use that.
      addr=temp;
      #endif
      #if defined(HOST_IMM8)
      int ir=get_reg(i_regs->regmap,INVCP);
      assert(ir>=0);
      emit_cmpmem_indexedsr12_reg(ir,addr,1);
      #else
      emit_cmpmem_indexedsr12_imm((intptr_t)invalid_code,addr,1);
      #endif
      #if defined(HAVE_CONDITIONAL_CALL) && !defined(DESTRUCTIVE_SHIFT)
      emit_callne(invalidate_addr_reg[addr]);
      #else
      jaddr2=(intptr_t)out;
      emit_jne(0);
      add_stub(INVCODE_STUB,jaddr2,(intptr_t)out,reglist|(1<<HOST_CCREG),addr,0,0,0);
      #endif
    }
  }
  if(jaddr) {
    add_stub(type,jaddr,(intptr_t)out,i,addr,(intptr_t)i_regs,ccadj[i],reglist);
  } else if(c&&!memtarget) {
    inline_writestub(type,i,constmap[i][s]+offset,i_regs->regmap,rs2[i],ccadj[i],reglist);
  }
  //if(opcode[i]==0x2B || opcode[i]==0x3F)
  //if(opcode[i]==0x2B || opcode[i]==0x28)
  //if(opcode[i]==0x2B || opcode[i]==0x29)
  //if(opcode[i]==0x2B)

// Uncomment for extra debug output:
/*
  if(opcode[i]==0x2B || opcode[i]==0x28 || opcode[i]==0x29 || opcode[i]==0x3F)
  {
    #if NEW_DYNAREC == NEW_DYNAREC_X86
    emit_pusha();
    #endif
    #if NEW_DYNAREC == NEW_DYNAREC_ARM
    save_regs(0x100f);
    #endif
        emit_readword((intptr_t)&last_count,ECX);
        #if NEW_DYNAREC == NEW_DYNAREC_X86
        if(get_reg(i_regs->regmap,CCREG)<0)
          emit_loadreg(CCREG,HOST_CCREG);
        emit_add(HOST_CCREG,ECX,HOST_CCREG);
        emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG);
        emit_writeword(HOST_CCREG,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
        #endif
        #if NEW_DYNAREC == NEW_DYNAREC_ARM
        if(get_reg(i_regs->regmap,CCREG)<0)
          emit_loadreg(CCREG,0);
        else
          emit_mov(HOST_CCREG,0);
        emit_add(0,ECX,0);
        emit_addimm(0,2*ccadj[i],0);
        emit_writeword(0,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
        #endif
    emit_call((intptr_t)memdebug);
    #if NEW_DYNAREC == NEW_DYNAREC_X86
    emit_popa();
    #endif
    #if NEW_DYNAREC == NEW_DYNAREC_ARM
    restore_regs(0x100f);
    #endif
  }
*/
}
#define store_assemble store_assemble_arm64

static void storelr_assemble_arm64(int i,struct regstat *i_regs)
{
  int s,th,tl;
  int temp;
  int map=-1;
  int temp2=0;
  int offset;
  intptr_t jaddr=0;
  intptr_t jaddr2=0;
  intptr_t case1,case2,case3;
  intptr_t done0,done1,done2;
  int memtarget=0;
  int c=0;
  int agr=AGEN1+(i&1);
  u_int hr,reglist=0;
  th=get_reg(i_regs->regmap,rs2[i]|64);
  tl=get_reg(i_regs->regmap,rs2[i]);
  s=get_reg(i_regs->regmap,rs1[i]);
  temp=get_reg(i_regs->regmap,agr);
  if(temp<0) temp=get_reg(i_regs->regmap,-1);
  offset=imm[i];
  if(s>=0) {
    c=(i_regs->isconst>>s)&1;
    memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80800000;
    if(using_tlb&&((signed int)(constmap[i][s]+offset))>=(signed int)0xC0000000) memtarget=1;
  }
  assert(tl>=0);
  for(hr=0;hr<HOST_REGS;hr++) {
    if(i_regs->regmap[hr]>=0) reglist|=1<<hr;
  }
  assert(temp>=0);
  if(!using_tlb) {
    if(!c) {
      emit_cmpimm(s<0||offset?temp:s,0x800000);
      if(!offset&&s!=temp) emit_mov(s,temp);
      jaddr=(intptr_t)out;
      emit_jno(0);
    }
    else
    {
      if(!memtarget||!rs1[i]) {
        jaddr=(intptr_t)out;
        emit_jmp(0);
      }
    }
  }else{ // using tlb
    map=get_reg(i_regs->regmap,TLREG);
    int cache=get_reg(i_regs->regmap,MMREG);
    assert(map>=0);
    reglist&=~(1<<map);
    map=do_tlb_w(c||s<0||offset?temp:s,temp,map,cache,0,c,constmap[i][s]+offset);
    if(!c&&!offset&&s>=0) emit_mov(s,temp);
    do_tlb_w_branch(map,c,constmap[i][s]+offset,&jaddr);
    if(!jaddr&&!memtarget) {
      jaddr=(intptr_t)out;
      emit_jmp(0);
    }
  }

  #ifdef RAM_OFFSET
  if(map<0) {
    map=get_reg(i_regs->regmap,ROREG);
    if(map<0) emit_loadreg(ROREG,map=HOST_TEMPREG);
  }
  #endif
  gen_tlb_addr_w(temp,map);

  if (opcode[i]==0x2C||opcode[i]==0x2D) { // SDL/SDR
    temp2=get_reg(i_regs->regmap,FTEMP);
    if(!rs2[i]) temp2=th=tl;
  }

  emit_testimm64(temp,2);
  case2=(intptr_t)out;
  emit_jne(0);
  emit_testimm64(temp,1);
  case1=(intptr_t)out;
  emit_jne(0);
  // 0
  if (opcode[i]==0x2A) { // SWL
    emit_writeword_indexed(tl,0,temp);
  }
  if (opcode[i]==0x2E) { // SWR
    emit_writebyte_indexed(tl,3,temp);
  }
  if (opcode[i]==0x2C) { // SDL
    emit_writeword_indexed(th,0,temp);
    if(rs2[i]) emit_mov(tl,temp2);
  }
  if (opcode[i]==0x2D) { // SDR
    emit_writebyte_indexed(tl,3,temp);
    if(rs2[i]) emit_shldimm(th,tl,24,temp2);
  }
  done0=(intptr_t)out;
  emit_jmp(0);
  // 1
  set_jump_target(case1,(intptr_t)out);
  if (opcode[i]==0x2A) { // SWL
    // Write 3 msb into three least significant bytes
    if(rs2[i]) emit_rorimm(tl,8,tl);
    emit_writehword_indexed(tl,-1,temp);
    if(rs2[i]) emit_rorimm(tl,16,tl);
    emit_writebyte_indexed(tl,1,temp);
    if(rs2[i]) emit_rorimm(tl,8,tl);
  }
  if (opcode[i]==0x2E) { // SWR
    // Write two lsb into two most significant bytes
    emit_writehword_indexed(tl,1,temp);
  }
  if (opcode[i]==0x2C) { // SDL
    if(rs2[i]) emit_shrdimm(tl,th,8,temp2);
    // Write 3 msb into three least significant bytes
    if(rs2[i]) emit_rorimm(th,8,th);
    emit_writehword_indexed(th,-1,temp);
    if(rs2[i]) emit_rorimm(th,16,th);
    emit_writebyte_indexed(th,1,temp);
    if(rs2[i]) emit_rorimm(th,8,th);
  }
  if (opcode[i]==0x2D) { // SDR
    if(rs2[i]) emit_shldimm(th,tl,16,temp2);
    // Write two lsb into two most significant bytes
    emit_writehword_indexed(tl,1,temp);
  }
  done1=(intptr_t)out;
  emit_jmp(0);
  // 2
  set_jump_target(case2,(intptr_t)out);
  emit_testimm64(temp,1);
  case3=(intptr_t)out;
  emit_jne(0);
  if (opcode[i]==0x2A) { // SWL
    // Write two msb into two least significant bytes
    if(rs2[i]) emit_rorimm(tl,16,tl);
    emit_writehword_indexed(tl,-2,temp);
    if(rs2[i]) emit_rorimm(tl,16,tl);
  }
  if (opcode[i]==0x2E) { // SWR
    // Write 3 lsb into three most significant bytes
    emit_writebyte_indexed(tl,-1,temp);
    if(rs2[i]) emit_rorimm(tl,8,tl);
    emit_writehword_indexed(tl,0,temp);
    if(rs2[i]) emit_rorimm(tl,24,tl);
  }
  if (opcode[i]==0x2C) { // SDL
    if(rs2[i]) emit_shrdimm(tl,th,16,temp2);
    // Write two msb into two least significant bytes
    if(rs2[i]) emit_rorimm(th,16,th);
    emit_writehword_indexed(th,-2,temp);
    if(rs2[i]) emit_rorimm(th,16,th);
  }
  if (opcode[i]==0x2D) { // SDR
    if(rs2[i]) emit_shldimm(th,tl,8,temp2);
    // Write 3 lsb into three most significant bytes
    emit_writebyte_indexed(tl,-1,temp);
    if(rs2[i]) emit_rorimm(tl,8,tl);
    emit_writehword_indexed(tl,0,temp);
    if(rs2[i]) emit_rorimm(tl,24,tl);
  }
  done2=(intptr_t)out;
  emit_jmp(0);
  // 3
  set_jump_target(case3,(intptr_t)out);
  if (opcode[i]==0x2A) { // SWL
    // Write msb into least significant byte
    if(rs2[i]) emit_rorimm(tl,24,tl);
    emit_writebyte_indexed(tl,-3,temp);
    if(rs2[i]) emit_rorimm(tl,8,tl);
  }
  if (opcode[i]==0x2E) { // SWR
    // Write entire word
    emit_writeword_indexed(tl,-3,temp);
  }
  if (opcode[i]==0x2C) { // SDL
    if(rs2[i]) emit_shrdimm(tl,th,24,temp2);
    // Write msb into least significant byte
    if(rs2[i]) emit_rorimm(th,24,th);
    emit_writebyte_indexed(th,-3,temp);
    if(rs2[i]) emit_rorimm(th,8,th);
  }
  if (opcode[i]==0x2D) { // SDR
    if(rs2[i]) emit_mov(th,temp2);
    // Write entire word
    emit_writeword_indexed(tl,-3,temp);
  }
  set_jump_target(done0,(intptr_t)out);
  set_jump_target(done1,(intptr_t)out);
  set_jump_target(done2,(intptr_t)out);
  if (opcode[i]==0x2C) { // SDL
    emit_testimm64(temp,4);
    done0=(intptr_t)out;
    emit_jne(0);
    emit_andimm64(temp,~3,temp);
    emit_writeword_indexed(temp2,4,temp);
    set_jump_target(done0,(intptr_t)out);
  }
  if (opcode[i]==0x2D) { // SDR
    emit_testimm64(temp,4);
    done0=(intptr_t)out;
    emit_jeq(0);
    emit_andimm64(temp,~3,temp);
    emit_writeword_indexed(temp2,-4,temp);
    set_jump_target(done0,(intptr_t)out);
  }
  if(!c||!memtarget)
    add_stub(STORELR_STUB,jaddr,(intptr_t)out,0,(intptr_t)i_regs,rs2[i],ccadj[i],reglist);
  if(!using_tlb) {
    #ifdef RAM_OFFSET
    int map=get_reg(i_regs->regmap,ROREG);
    if(map<0) map=HOST_TEMPREG;
    gen_orig_addr_w(temp,map);
    #else
    emit_addimm_no_flags((uintptr_t)0x80000000-(uintptr_t)g_rdram,temp);
    #endif
    #if defined(HOST_IMM8)
    int ir=get_reg(i_regs->regmap,INVCP);
    assert(ir>=0);
    emit_cmpmem_indexedsr12_reg(ir,temp,1);
    #else
    emit_cmpmem_indexedsr12_imm((intptr_t)invalid_code,temp,1);
    #endif
    #if defined(HAVE_CONDITIONAL_CALL) && !defined(DESTRUCTIVE_SHIFT)
    emit_callne(invalidate_addr_reg[temp]);
    #else
    jaddr2=(intptr_t)out;
    emit_jne(0);
    add_stub(INVCODE_STUB,jaddr2,(intptr_t)out,reglist|(1<<HOST_CCREG),temp,0,0,0);
    #endif
  }
  /*
    emit_pusha();
    //save_regs(0x100f);
        emit_readword((intptr_t)&last_count,ECX);
        if(get_reg(i_regs->regmap,CCREG)<0)
          emit_loadreg(CCREG,HOST_CCREG);
        emit_add(HOST_CCREG,ECX,HOST_CCREG);
        emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG);
        emit_writeword(HOST_CCREG,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
    emit_call((intptr_t)memdebug);
    emit_popa();
    //restore_regs(0x100f);
  */
}
#define storelr_assemble storelr_assemble_arm64

static void cop0_assemble(int i,struct regstat *i_regs)
{
  if(opcode2[i]==0) // MFC0
  {
    if(rt1[i]) {
      signed char t=get_reg(i_regs->regmap,rt1[i]);
      char copr=(source[i]>>11)&0x1f;
      if(t>=0) {
        emit_addimm64(FP,(intptr_t)&fake_pc-(intptr_t)&dynarec_local,0);
        emit_movimm((source[i]>>11)&0x1f,1);
        emit_writeword64(0,(intptr_t)&PC);
        emit_writebyte(1,(intptr_t)&(fake_pc.f.r.nrd));
        if(copr==9) {
          emit_readword((intptr_t)&last_count,ECX);
          emit_loadreg(CCREG,HOST_CCREG); // TODO: do proper reg alloc
          emit_add(HOST_CCREG,ECX,HOST_CCREG);
          emit_addimm(HOST_CCREG,CLOCK_DIVIDER*ccadj[i],HOST_CCREG);
          emit_writeword(HOST_CCREG,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
        }
        emit_call((intptr_t)cached_interpreter_table.MFC0);
        emit_readword((intptr_t)&readmem_dword,t);
      }
    }
  }
  else if(opcode2[i]==4) // MTC0
  {
    signed char s=get_reg(i_regs->regmap,rs1[i]);
    char copr=(source[i]>>11)&0x1f;
    assert(s>=0);
    emit_writeword(s,(intptr_t)&readmem_dword);
    wb_register(rs1[i],i_regs->regmap,i_regs->dirty,i_regs->is32);
    emit_addimm64(FP,(intptr_t)&fake_pc-(intptr_t)&dynarec_local,0);
    emit_movimm((source[i]>>11)&0x1f,1);
    emit_writeword64(0,(intptr_t)&PC);
    emit_writebyte(1,(intptr_t)&(fake_pc.f.r.nrd));
    if(copr==9||copr==11||copr==12) {
      emit_readword((intptr_t)&last_count,ECX);
      emit_loadreg(CCREG,HOST_CCREG); // TODO: do proper reg alloc
      emit_add(HOST_CCREG,ECX,HOST_CCREG);
      emit_addimm(HOST_CCREG,CLOCK_DIVIDER*ccadj[i],HOST_CCREG);
      emit_writeword(HOST_CCREG,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
    }
    // What a mess.  The status register (12) can enable interrupts,
    // so needs a special case to handle a pending interrupt.
    // The interrupt must be taken immediately, because a subsequent
    // instruction might disable interrupts again.
    if(copr==12&&!is_delayslot) {
      emit_movimm(start+i*4+4,0);
      emit_movimm(0,1);
      emit_writeword(0,(intptr_t)&pcaddr);
      emit_writeword(1,(intptr_t)&pending_exception);
    }
    //else if(copr==12&&is_delayslot) emit_call((int)MTC0_R12);
    //else
    emit_call((intptr_t)cached_interpreter_table.MTC0);
    if(copr==9||copr==11||copr==12) {
      emit_readword((intptr_t)&g_cp0_regs[CP0_COUNT_REG],HOST_CCREG);
      emit_readword((intptr_t)&next_interrupt,ECX);
      emit_addimm(HOST_CCREG,-(int)CLOCK_DIVIDER*ccadj[i],HOST_CCREG);
      emit_sub(HOST_CCREG,ECX,HOST_CCREG);
      emit_writeword(ECX,(intptr_t)&last_count);
      emit_storereg(CCREG,HOST_CCREG);
    }
    if(copr==12) {
      assert(!is_delayslot);
      emit_readword((intptr_t)&pending_exception,HOST_TEMPREG);
    }
    emit_loadreg(rs1[i],s);
    if(get_reg(i_regs->regmap,rs1[i]|64)>=0)
      emit_loadreg(rs1[i]|64,get_reg(i_regs->regmap,rs1[i]|64));
    if(copr==12) {
      emit_test(HOST_TEMPREG,HOST_TEMPREG);
      emit_jeq((intptr_t)out+8);
      emit_jmp((intptr_t)&do_interrupt);
    }
    cop1_usable=0;
  }
  else
  {
    assert(opcode2[i]==0x10);
    if((source[i]&0x3f)==0x01) // TLBR
      emit_call((intptr_t)cached_interpreter_table.TLBR);
    if((source[i]&0x3f)==0x02) // TLBWI
      emit_call((intptr_t)TLBWI_new);
    if((source[i]&0x3f)==0x06) { // TLBWR
      // The TLB entry written by TLBWR is dependent on the count,
      // so update the cycle count
      emit_readword((intptr_t)&last_count,ECX);
      if(i_regs->regmap[HOST_CCREG]!=CCREG) emit_loadreg(CCREG,HOST_CCREG);
      emit_add(HOST_CCREG,ECX,HOST_CCREG);
      emit_addimm(HOST_CCREG,CLOCK_DIVIDER*ccadj[i],HOST_CCREG);
      emit_writeword(HOST_CCREG,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
      emit_call((intptr_t)TLBWR_new);
    }
    if((source[i]&0x3f)==0x08) // TLBP
      emit_call((intptr_t)cached_interpreter_table.TLBP);
    if((source[i]&0x3f)==0x18) // ERET
    {
      int count=ccadj[i];
      if(i_regs->regmap[HOST_CCREG]!=CCREG) emit_loadreg(CCREG,HOST_CCREG);
      emit_addimm(HOST_CCREG,CLOCK_DIVIDER*count,HOST_CCREG); // TODO: Should there be an extra cycle here?
      emit_jmp((intptr_t)jump_eret);
    }
  }
}

static void cop1_assemble(int i,struct regstat *i_regs)
{
  // Check cop1 unusable
  if(!cop1_usable) {
    signed char rs=get_reg(i_regs->regmap,CSREG);
    assert(rs>=0);
    emit_testimm(rs,0x20000000);
    intptr_t jaddr=(intptr_t)out;
    emit_jeq(0);
    add_stub(FP_STUB,jaddr,(intptr_t)out,i,rs,(intptr_t)i_regs,is_delayslot,0);
    cop1_usable=1;
  }
  if (opcode2[i]==0) { // MFC1
    signed char tl=get_reg(i_regs->regmap,rt1[i]);
    if(tl>=0) {
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],tl);
      emit_readword_indexed(0,tl,tl);
    }
  }
  else if (opcode2[i]==1) { // DMFC1
    signed char tl=get_reg(i_regs->regmap,rt1[i]);
    signed char th=get_reg(i_regs->regmap,rt1[i]|64);
    if(tl>=0) {
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],tl);
      if(th>=0) emit_readword_indexed(4,tl,th);
      emit_readword_indexed(0,tl,tl);
    }
  }
  else if (opcode2[i]==4) { // MTC1
    signed char sl=get_reg(i_regs->regmap,rs1[i]);
    signed char temp=get_reg(i_regs->regmap,-1);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_writeword_indexed(sl,0,temp);
  }
  else if (opcode2[i]==5) { // DMTC1
    signed char sl=get_reg(i_regs->regmap,rs1[i]);
    signed char sh=rs1[i]>0?get_reg(i_regs->regmap,rs1[i]|64):sl;
    signed char temp=get_reg(i_regs->regmap,-1);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_writeword_indexed(sh,4,temp);
    emit_writeword_indexed(sl,0,temp);
  }
  else if (opcode2[i]==2) // CFC1
  {
    signed char tl=get_reg(i_regs->regmap,rt1[i]);
    if(tl>=0) {
      u_int copr=(source[i]>>11)&0x1f;
      if(copr==0) emit_readword((intptr_t)&FCR0,tl);
      if(copr==31) emit_readword((intptr_t)&FCR31,tl);
    }
  }
  else if (opcode2[i]==6) // CTC1
  {
    signed char sl=get_reg(i_regs->regmap,rs1[i]);
    u_int copr=(source[i]>>11)&0x1f;
    assert(sl>=0);
    if(copr==31)
    {
      emit_writeword(sl,(intptr_t)&FCR31);

      // Set the rounding mode
      signed char temp=get_reg(i_regs->regmap,-1);
      assert(temp>=0);
      emit_andimm(sl,3,temp);
      emit_addimm64(FP,(intptr_t)&rounding_modes-(intptr_t)&dynarec_local,HOST_TEMPREG);
      emit_readword_dualindexedx4(HOST_TEMPREG,temp,temp);
      output_w32(0xd53b4400|HOST_TEMPREG); /*Read FPCR*/
      emit_andimm(HOST_TEMPREG,~0xc00000,HOST_TEMPREG); /*Clear RMode*/
      emit_or(temp,HOST_TEMPREG,HOST_TEMPREG); /*Set RMode*/
      output_w32(0xd51b4400|HOST_TEMPREG); /*Write FPCR*/
    }
  }
}

static void c1ls_assemble_arm64(int i,struct regstat *i_regs)
{
  int s,th,tl;
  int temp,ar;
  int map=-1;
  int offset;
  int c=0;
  intptr_t jaddr=0;
  intptr_t jaddr2=0;
  intptr_t jaddr3=0;
  int type=0;
  int agr=AGEN1+(i&1);
  u_int hr,reglist=0;
  th=get_reg(i_regs->regmap,FTEMP|64);
  tl=get_reg(i_regs->regmap,FTEMP);
  s=get_reg(i_regs->regmap,rs1[i]);
  temp=get_reg(i_regs->regmap,agr);
  if(temp<0) temp=get_reg(i_regs->regmap,-1);
  offset=imm[i];
  assert(tl>=0);
  assert(rs1[i]>0);
  assert(temp>=0);
  for(hr=0;hr<HOST_REGS;hr++) {
    if(i_regs->regmap[hr]>=0) reglist|=1<<hr;
  }
  if(i_regs->regmap[HOST_CCREG]==CCREG) reglist&=~(1<<HOST_CCREG);
  if (opcode[i]==0x31||opcode[i]==0x35) // LWC1/LDC1
  {
    // Loads use a temporary register which we need to save
    reglist|=1<<temp;
  }
  if (opcode[i]==0x39||opcode[i]==0x3D) // SWC1/SDC1
    ar=temp;
  else // LWC1/LDC1
    ar=tl;
  //if(s<0) emit_loadreg(rs1[i],ar); //address_generation does this now
  //else c=(i_regs->wasconst>>s)&1;
  if(s>=0) c=(i_regs->wasconst>>s)&1;
  // Check cop1 unusable
  if(!cop1_usable) {
    signed char rs=get_reg(i_regs->regmap,CSREG);
    assert(rs>=0);
    emit_testimm(rs,0x20000000);
    jaddr=(intptr_t)out;
    emit_jeq(0);
    add_stub(FP_STUB,jaddr,(intptr_t)out,i,rs,(intptr_t)i_regs,is_delayslot,0);
    cop1_usable=1;
  }
  if (opcode[i]==0x39) { // SWC1 (get float address)
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>16)&0x1f],tl);
  }
  if (opcode[i]==0x3D) { // SDC1 (get double address)
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>16)&0x1f],tl);
  }
  // Generate address + offset
  if(!using_tlb) {
    if(!c) 
      emit_cmpimm(offset||c||s<0?ar:s,0x800000);
  }
  else
  {
    map=get_reg(i_regs->regmap,TLREG);
    int cache=get_reg(i_regs->regmap,MMREG);
    assert(map>=0);
    reglist&=~(1<<map);
    if (opcode[i]==0x31||opcode[i]==0x35) { // LWC1/LDC1
      map=do_tlb_r(offset||c||s<0?ar:s,ar,map,cache,0,-1,-1,c,constmap[i][s]+offset);
    }
    if (opcode[i]==0x39||opcode[i]==0x3D) { // SWC1/SDC1
      map=do_tlb_w(offset||c||s<0?ar:s,ar,map,cache,0,c,constmap[i][s]+offset);
    }
  }
  #ifdef RAM_OFFSET
  if(map<0) {
    map=get_reg(i_regs->regmap,ROREG);
    if(map<0) emit_loadreg(ROREG,map=HOST_TEMPREG);
  }
  #endif

  if (opcode[i]==0x39) { // SWC1 (read float)
    emit_readword_indexed(0,tl,tl);
  }
  if (opcode[i]==0x3D) { // SDC1 (read double)
    emit_readword_indexed(4,tl,th);
    emit_readword_indexed(0,tl,tl);
  }
  if (opcode[i]==0x31) { // LWC1 (get target address)
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>16)&0x1f],temp);
  }
  if (opcode[i]==0x35) { // LDC1 (get target address)
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>16)&0x1f],temp);
  }
  if(!using_tlb) {
    if(!c) {
      jaddr2=(intptr_t)out;
      emit_jno(0);
    }
    else if(((signed int)(constmap[i][s]+offset))>=(signed int)0x80800000) {
      jaddr2=(intptr_t)out;
      emit_jmp(0); // inline_readstub/inline_writestub?  Very rare case
    }
    #ifdef DESTRUCTIVE_SHIFT
    if (opcode[i]==0x39||opcode[i]==0x3D) { // SWC1/SDC1
      if(!offset&&!c&&s>=0) emit_mov(s,ar);
    }
    #endif
  }else{
    if (opcode[i]==0x31||opcode[i]==0x35) { // LWC1/LDC1
      do_tlb_r_branch(map,c,constmap[i][s]+offset,&jaddr2);
    }
    if (opcode[i]==0x39||opcode[i]==0x3D) { // SWC1/SDC1
      do_tlb_w_branch(map,c,constmap[i][s]+offset,&jaddr2);
    }
  }
  if (opcode[i]==0x31) { // LWC1
    //if(s>=0&&!c&&!offset) emit_mov(s,tl);
    //gen_tlb_addr_r(ar,map);
    //emit_readword_indexed((intptr_t)g_rdram-0x80000000,tl,tl);
    #ifdef HOST_IMM_ADDR32
    if(c) emit_readword_tlb(constmap[i][s]+offset,map,tl);
    else
    #endif
    emit_readword_indexed_tlb(0,offset||c||s<0?tl:s,map,tl);
    type=LOADW_STUB;
  }
  if (opcode[i]==0x35) { // LDC1
    assert(th>=0);
    //if(s>=0&&!c&&!offset) emit_mov(s,tl);
    //gen_tlb_addr_r(ar,map);
    //emit_readword_indexed((intptr_t)g_rdram-0x80000000,tl,th);
    //emit_readword_indexed((intptr_t)g_rdram-0x7FFFFFFC,tl,tl);
    #ifdef HOST_IMM_ADDR32
    if(c) emit_readdword_tlb(constmap[i][s]+offset,map,th,tl);
    else
    #endif
    emit_readdword_indexed_tlb(0,offset||c||s<0?tl:s,map,th,tl);
    type=LOADD_STUB;
  }
  if (opcode[i]==0x39) { // SWC1
    //emit_writeword_indexed(tl,(intptr_t)g_rdram-0x80000000,temp);
    emit_writeword_indexed_tlb(tl,0,offset||c||s<0?temp:s,map,temp);
    type=STOREW_STUB;
  }
  if (opcode[i]==0x3D) { // SDC1
    assert(th>=0);
    //emit_writeword_indexed(th,(intptr_t)g_rdram-0x80000000,temp);
    //emit_writeword_indexed(tl,(intptr_t)g_rdram-0x7FFFFFFC,temp);
    emit_writedword_indexed_tlb(th,tl,0,offset||c||s<0?temp:s,map,temp);
    type=STORED_STUB;
  }
  if(!using_tlb) {
    if (opcode[i]==0x39||opcode[i]==0x3D) { // SWC1/SDC1
      #ifndef DESTRUCTIVE_SHIFT
      temp=offset||c||s<0?ar:s;
      #endif
      #if defined(HOST_IMM8)
      int ir=get_reg(i_regs->regmap,INVCP);
      assert(ir>=0);
      emit_cmpmem_indexedsr12_reg(ir,temp,1);
      #else
      emit_cmpmem_indexedsr12_imm((intptr_t)invalid_code,temp,1);
      #endif
      #if defined(HAVE_CONDITIONAL_CALL) && !defined(DESTRUCTIVE_SHIFT)
      emit_callne(invalidate_addr_reg[temp]);
      #else
      jaddr3=(intptr_t)out;
      emit_jne(0);
      add_stub(INVCODE_STUB,jaddr3,(intptr_t)out,reglist|(1<<HOST_CCREG),temp,0,0,0);
      #endif
    }
  }
  if(jaddr2) add_stub(type,jaddr2,(intptr_t)out,i,offset||c||s<0?ar:s,(intptr_t)i_regs,ccadj[i],reglist);
  if (opcode[i]==0x31) { // LWC1 (write float)
    emit_writeword_indexed(tl,0,temp);
  }
  if (opcode[i]==0x35) { // LDC1 (write double)
    emit_writeword_indexed(th,4,temp);
    emit_writeword_indexed(tl,0,temp);
  }
  //if(opcode[i]==0x39)
  /*if(opcode[i]==0x39||opcode[i]==0x31)
  {
    emit_pusha();
        emit_readword((intptr_t)&last_count,ECX);
        if(get_reg(i_regs->regmap,CCREG)<0)
          emit_loadreg(CCREG,HOST_CCREG);
        emit_add(HOST_CCREG,ECX,HOST_CCREG);
        emit_addimm(HOST_CCREG,2*ccadj[i],HOST_CCREG);
        emit_writeword(HOST_CCREG,(intptr_t)&g_cp0_regs[CP0_COUNT_REG]);
    emit_call((intptr_t)memdebug);
    emit_popa();
  }*/
}
#define c1ls_assemble c1ls_assemble_arm64

static void fconv_assemble_arm64(int i,struct regstat *i_regs)
{
  signed char temp=get_reg(i_regs->regmap,-1);
  assert(temp>=0);
  // Check cop1 unusable
  if(!cop1_usable) {
    signed char rs=get_reg(i_regs->regmap,CSREG);
    assert(rs>=0);
    emit_testimm(rs,0x20000000);
    intptr_t jaddr=(intptr_t)out;
    emit_jeq(0);
    add_stub(FP_STUB,jaddr,(intptr_t)out,i,rs,(intptr_t)i_regs,is_delayslot,0);
    cop1_usable=1;
  }

  /*Single-precision to Integer*/

  //TOBEDONE
  //if(opcode2[i]==0x10&&(source[i]&0x3f)==0x24) { //cvt_w_s
  //}

  //if(opcode2[i]==0x10&&(source[i]&0x3f)==0x25) { //cvt_l_s
  //}

  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x08) { //round_l_s
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_flds(temp,31);
    emit_fcvtns_l_s(31,HOST_TEMPREG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed64(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x09) { //trunc_l_s
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_flds(temp,31);
    emit_fcvtzs_l_s(31,HOST_TEMPREG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed64(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0a) { //ceil_l_s
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_flds(temp,31);
    emit_fcvtps_l_s(31,HOST_TEMPREG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed64(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0b) { //floor_l_s
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_flds(temp,31);
    emit_fcvtms_l_s(31,HOST_TEMPREG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed64(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0c) { //round_w_s
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_flds(temp,31);
    emit_fcvtns_w_s(31,HOST_TEMPREG);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0d) { //trunc_w_s
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_flds(temp,31);
    emit_fcvtzs_w_s(31,HOST_TEMPREG);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0e) { //ceil_w_s
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_flds(temp,31);
    emit_fcvtps_w_s(31,HOST_TEMPREG);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0f) { //floor_w_s
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_flds(temp,31);
    emit_fcvtms_w_s(31,HOST_TEMPREG);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed(HOST_TEMPREG,0,temp);
    return;
  }

  /*Double-precision to Integer*/

  //TOBEDONE
  //if(opcode2[i]==0x11&&(source[i]&0x3f)==0x24) { //cvt_w_d
  //}

  //if(opcode2[i]==0x11&&(source[i]&0x3f)==0x25) { //cvt_l_d
  //}

  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x08) { //round_l_d
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_fldd(temp,31);
    emit_fcvtns_l_d(31,HOST_TEMPREG);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed64(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x09) { //trunc_l_d
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_fldd(temp,31);
    emit_fcvtzs_l_d(31,HOST_TEMPREG);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed64(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0a) { //ceil_l_d
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_fldd(temp,31);
    emit_fcvtps_l_d(31,HOST_TEMPREG);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed64(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0b) { //floor_l_d
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_fldd(temp,31);
    emit_fcvtms_l_d(31,HOST_TEMPREG);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed64(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0c) { //round_w_d
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_fldd(temp,31);
    emit_fcvtns_w_d(31,HOST_TEMPREG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0d) { //trunc_w_d
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_fldd(temp,31);
    emit_fcvtzs_w_d(31,HOST_TEMPREG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0e) { //ceil_w_d
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_fldd(temp,31);
    emit_fcvtps_w_d(31,HOST_TEMPREG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed(HOST_TEMPREG,0,temp);
    return;
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0f) { //floor_w_d
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_fldd(temp,31);
    emit_fcvtms_w_d(31,HOST_TEMPREG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_writeword_indexed(HOST_TEMPREG,0,temp);
    return;
  }

  /*Single-precision to Double-precision*/
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x21) { //cvt_d_s
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_flds(temp,31);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_fcvt_d_s(31,31);
    emit_fstd(31,temp);
    return;
  }

  /*Double-precision to Single-precision*/
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x20) { //cvt_s_d
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_fldd(temp,31);
    emit_fcvt_s_d(31,31);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_fsts(31,temp);
    return;
  }

  /*Integer to Single-precision*/
  if(opcode2[i]==0x14&&(source[i]&0x3f)==0x20) { //cvt_s_w
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_readword_indexed(0,temp,HOST_TEMPREG);
    emit_scvtf_s_w(HOST_TEMPREG,31);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_fsts(31,temp);
    return;
  }

  if(opcode2[i]==0x15&&(source[i]&0x3f)==0x20) { //cvt_s_l
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_readword_indexed64(0,temp,HOST_TEMPREG);
    emit_scvtf_s_l(HOST_TEMPREG,31);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
    emit_fsts(31,temp);
    return;
  }

  /*Integer Double-precision*/
  if(opcode2[i]==0x14&&(source[i]&0x3f)==0x21) { //cvt_d_w
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_readword_indexed(0,temp,HOST_TEMPREG);
    emit_scvtf_d_w(HOST_TEMPREG,31);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_fstd(31,temp);
    return;
  }

  if(opcode2[i]==0x15&&(source[i]&0x3f)==0x21) { //cvt_d_l
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_readword_indexed64(0,temp,HOST_TEMPREG);
    emit_scvtf_d_l(HOST_TEMPREG,31);
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f))
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
    emit_fstd(31,temp);
    return;
  }
  
  // C emulation code
  
  u_int hr,reglist=0;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(i_regs->regmap[hr]>=0) reglist|=1<<hr;
  }
  save_regs(reglist);
  
  if(opcode2[i]==0x14&&(source[i]&0x3f)==0x20) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_s_w);
  }
  if(opcode2[i]==0x14&&(source[i]&0x3f)==0x21) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_d_w);
  }
  if(opcode2[i]==0x15&&(source[i]&0x3f)==0x20) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_s_l);
  }
  if(opcode2[i]==0x15&&(source[i]&0x3f)==0x21) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_d_l);
  }
  
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x21) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_d_s);
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x24) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_w_s);
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x25) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_l_s);
  }
  
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x20) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_s_d);
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x24) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_w_d);
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x25) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)cvt_l_d);
  }
  
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x08) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)round_l_s);
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x09) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)trunc_l_s);
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0a) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)ceil_l_s);
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0b) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)floor_l_s);
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0c) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)round_w_s);
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0d) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)trunc_w_s);
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0e) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)ceil_w_s);
  }
  if(opcode2[i]==0x10&&(source[i]&0x3f)==0x0f) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)floor_w_s);
  }
  
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x08) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)round_l_d);
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x09) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)trunc_l_d);
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0a) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)ceil_l_d);
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0b) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)floor_l_d);
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0c) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)round_w_d);
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0d) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)trunc_w_d);
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0e) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)ceil_w_d);
  }
  if(opcode2[i]==0x11&&(source[i]&0x3f)==0x0f) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    emit_call((intptr_t)floor_w_d);
  }
  
  restore_regs(reglist);
}
#define fconv_assemble fconv_assemble_arm64

static void fcomp_assemble(int i,struct regstat *i_regs)
{
  signed char fs=get_reg(i_regs->regmap,FSREG);
  signed char temp=get_reg(i_regs->regmap,-1);
  assert(temp>=0);
  // Check cop1 unusable
  if(!cop1_usable) {
    signed char cs=get_reg(i_regs->regmap,CSREG);
    assert(cs>=0);
    emit_testimm(cs,0x20000000);
    intptr_t jaddr=(intptr_t)out;
    emit_jeq(0);
    add_stub(FP_STUB,jaddr,(intptr_t)out,i,cs,(intptr_t)i_regs,is_delayslot,0);
    cop1_usable=1;
  }
  
  if((source[i]&0x3f)==0x30) {
    emit_andimm(fs,~0x800000,fs);
    return;
  }
  
  if((source[i]&0x3e)==0x38) {
    // sf/ngle - these should throw exceptions for NaNs
    emit_andimm(fs,~0x800000,fs);
    return;
  }
  
  if(opcode2[i]==0x10) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>16)&0x1f],HOST_TEMPREG);
    emit_flds(temp,30);
    emit_flds(HOST_TEMPREG,31);
    emit_andimm(fs,~0x800000,fs);
    emit_orimm(fs,0x800000,temp);
    emit_fcmps(30,31);
    if((source[i]&0x3f)==0x31) emit_csel_vs(temp,fs,fs); // c_un_s
    if((source[i]&0x3f)==0x32) emit_csel_eq(temp,fs,fs); // c_eq_s
    if((source[i]&0x3f)==0x33) {emit_csel_eq(temp,fs,fs);emit_csel_vs(temp,fs,fs);} // c_ueq_s
    if((source[i]&0x3f)==0x34) emit_csel_cc(temp,fs,fs); // c_olt_s
    if((source[i]&0x3f)==0x35) {emit_csel_cc(temp,fs,fs);emit_csel_vs(temp,fs,fs);} // c_ult_s
    if((source[i]&0x3f)==0x36) emit_csel_ls(temp,fs,fs); // c_ole_s
    if((source[i]&0x3f)==0x37) {emit_csel_ls(temp,fs,fs);emit_csel_vs(temp,fs,fs);} // c_ule_s
    if((source[i]&0x3f)==0x3a) emit_csel_eq(temp,fs,fs); // c_seq_s
    if((source[i]&0x3f)==0x3b) emit_csel_eq(temp,fs,fs); // c_ngl_s
    if((source[i]&0x3f)==0x3c) emit_csel_cc(temp,fs,fs); // c_lt_s
    if((source[i]&0x3f)==0x3d) emit_csel_cc(temp,fs,fs); // c_nge_s
    if((source[i]&0x3f)==0x3e) emit_csel_ls(temp,fs,fs); // c_le_s
    if((source[i]&0x3f)==0x3f) emit_csel_ls(temp,fs,fs); // c_ngt_s
    return;
  }
  if(opcode2[i]==0x11) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>16)&0x1f],HOST_TEMPREG);
    emit_fldd(temp,30);
    emit_fldd(HOST_TEMPREG,31);
    emit_andimm(fs,~0x800000,fs);
    emit_orimm(fs,0x800000,temp);
    emit_fcmpd(30,31);
    if((source[i]&0x3f)==0x31) emit_csel_vs(temp,fs,fs); // c_un_d
    if((source[i]&0x3f)==0x32) emit_csel_eq(temp,fs,fs); // c_eq_d
    if((source[i]&0x3f)==0x33) {emit_csel_eq(temp,fs,fs);emit_csel_vs(temp,fs,fs);} // c_ueq_d
    if((source[i]&0x3f)==0x34) emit_csel_cc(temp,fs,fs); // c_olt_d
    if((source[i]&0x3f)==0x35) {emit_csel_cc(temp,fs,fs);emit_csel_vs(temp,fs,fs);} // c_ult_d
    if((source[i]&0x3f)==0x36) emit_csel_ls(temp,fs,fs); // c_ole_d
    if((source[i]&0x3f)==0x37) {emit_csel_ls(temp,fs,fs);emit_csel_vs(temp,fs,fs);} // c_ule_d
    if((source[i]&0x3f)==0x3a) emit_csel_eq(temp,fs,fs); // c_seq_d
    if((source[i]&0x3f)==0x3b) emit_csel_eq(temp,fs,fs); // c_ngl_d
    if((source[i]&0x3f)==0x3c) emit_csel_cc(temp,fs,fs); // c_lt_d
    if((source[i]&0x3f)==0x3d) emit_csel_cc(temp,fs,fs); // c_nge_d
    if((source[i]&0x3f)==0x3e) emit_csel_ls(temp,fs,fs); // c_le_d
    if((source[i]&0x3f)==0x3f) emit_csel_ls(temp,fs,fs); // c_ngt_d
    return;
  }
  
  // C only
  
  u_int hr,reglist=0;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(i_regs->regmap[hr]>=0) reglist|=1<<hr;
  }
  reglist&=~(1<<fs);
  save_regs(reglist);
  if(opcode2[i]==0x10) {
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>16)&0x1f],ARG2_REG);
    if((source[i]&0x3f)==0x30) emit_call((intptr_t)c_f_s);
    if((source[i]&0x3f)==0x31) emit_call((intptr_t)c_un_s);
    if((source[i]&0x3f)==0x32) emit_call((intptr_t)c_eq_s);
    if((source[i]&0x3f)==0x33) emit_call((intptr_t)c_ueq_s);
    if((source[i]&0x3f)==0x34) emit_call((intptr_t)c_olt_s);
    if((source[i]&0x3f)==0x35) emit_call((intptr_t)c_ult_s);
    if((source[i]&0x3f)==0x36) emit_call((intptr_t)c_ole_s);
    if((source[i]&0x3f)==0x37) emit_call((intptr_t)c_ule_s);
    if((source[i]&0x3f)==0x38) emit_call((intptr_t)c_sf_s);
    if((source[i]&0x3f)==0x39) emit_call((intptr_t)c_ngle_s);
    if((source[i]&0x3f)==0x3a) emit_call((intptr_t)c_seq_s);
    if((source[i]&0x3f)==0x3b) emit_call((intptr_t)c_ngl_s);
    if((source[i]&0x3f)==0x3c) emit_call((intptr_t)c_lt_s);
    if((source[i]&0x3f)==0x3d) emit_call((intptr_t)c_nge_s);
    if((source[i]&0x3f)==0x3e) emit_call((intptr_t)c_le_s);
    if((source[i]&0x3f)==0x3f) emit_call((intptr_t)c_ngt_s);
  }
  if(opcode2[i]==0x11) {
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>16)&0x1f],ARG2_REG);
    if((source[i]&0x3f)==0x30) emit_call((intptr_t)c_f_d);
    if((source[i]&0x3f)==0x31) emit_call((intptr_t)c_un_d);
    if((source[i]&0x3f)==0x32) emit_call((intptr_t)c_eq_d);
    if((source[i]&0x3f)==0x33) emit_call((intptr_t)c_ueq_d);
    if((source[i]&0x3f)==0x34) emit_call((intptr_t)c_olt_d);
    if((source[i]&0x3f)==0x35) emit_call((intptr_t)c_ult_d);
    if((source[i]&0x3f)==0x36) emit_call((intptr_t)c_ole_d);
    if((source[i]&0x3f)==0x37) emit_call((intptr_t)c_ule_d);
    if((source[i]&0x3f)==0x38) emit_call((intptr_t)c_sf_d);
    if((source[i]&0x3f)==0x39) emit_call((intptr_t)c_ngle_d);
    if((source[i]&0x3f)==0x3a) emit_call((intptr_t)c_seq_d);
    if((source[i]&0x3f)==0x3b) emit_call((intptr_t)c_ngl_d);
    if((source[i]&0x3f)==0x3c) emit_call((intptr_t)c_lt_d);
    if((source[i]&0x3f)==0x3d) emit_call((intptr_t)c_nge_d);
    if((source[i]&0x3f)==0x3e) emit_call((intptr_t)c_le_d);
    if((source[i]&0x3f)==0x3f) emit_call((intptr_t)c_ngt_d);
  }
  restore_regs(reglist);
  emit_loadreg(FSREG,fs);
}

static void float_assemble(int i,struct regstat *i_regs)
{
  signed char temp=get_reg(i_regs->regmap,-1);
  assert(temp>=0);
  // Check cop1 unusable
  if(!cop1_usable) {
    signed char cs=get_reg(i_regs->regmap,CSREG);
    assert(cs>=0);
    emit_testimm(cs,0x20000000);
    intptr_t jaddr=(intptr_t)out;
    emit_jeq(0);
    add_stub(FP_STUB,jaddr,(intptr_t)out,i,cs,(intptr_t)i_regs,is_delayslot,0);
    cop1_usable=1;
  }
  
  if((source[i]&0x3f)==6) // mov
  {
    if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f)) {
      if(opcode2[i]==0x10) {
        emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
        emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],HOST_TEMPREG);
        emit_flds(temp,31);
        emit_fsts(31,HOST_TEMPREG);
      }
      if(opcode2[i]==0x11) {
        emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
        emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],HOST_TEMPREG);
        emit_fldd(temp,31);
        emit_fstd(31,HOST_TEMPREG);
      }
    }
    return;
  }
  
  if((source[i]&0x3f)>3)
  {
    if(opcode2[i]==0x10) {
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
      emit_flds(temp,31);
      if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f)) {
        emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
      }
      if((source[i]&0x3f)==4) // sqrt
        emit_fsqrts(31,31);
      if((source[i]&0x3f)==5) // abs
        emit_fabss(31,31);
      if((source[i]&0x3f)==7) // neg
        emit_fnegs(31,31);
      emit_fsts(31,temp);
    }
    if(opcode2[i]==0x11) {
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
      emit_fldd(temp,31);
      if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f)) {
        emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
      }
      if((source[i]&0x3f)==4) // sqrt
        emit_fsqrtd(31,31);
      if((source[i]&0x3f)==5) // abs
        emit_fabsd(31,31);
      if((source[i]&0x3f)==7) // neg
        emit_fnegd(31,31);
      emit_fstd(31,temp);
    }
    return;
  }
  if((source[i]&0x3f)<4)
  {
    if(opcode2[i]==0x10) {
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],temp);
    }
    if(opcode2[i]==0x11) {
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],temp);
    }
    if(((source[i]>>11)&0x1f)!=((source[i]>>16)&0x1f)) {
      if(opcode2[i]==0x10) {
        emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>16)&0x1f],HOST_TEMPREG);
        emit_flds(temp,31);
        emit_flds(HOST_TEMPREG,30);
        if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f)) {
          if(((source[i]>>16)&0x1f)!=((source[i]>>6)&0x1f)) {
            emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
          }
        }
        if((source[i]&0x3f)==0) emit_fadds(31,30,31);
        if((source[i]&0x3f)==1) emit_fsubs(31,30,31);
        if((source[i]&0x3f)==2) emit_fmuls(31,30,31);
        if((source[i]&0x3f)==3) emit_fdivs(31,30,31);
        if(((source[i]>>16)&0x1f)==((source[i]>>6)&0x1f)) {
          emit_fsts(31,HOST_TEMPREG);
        }else{
          emit_fsts(31,temp);
        }
      }
      else if(opcode2[i]==0x11) {
        emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>16)&0x1f],HOST_TEMPREG);
        emit_fldd(temp,31);
        emit_fldd(HOST_TEMPREG,30);
        if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f)) {
          if(((source[i]>>16)&0x1f)!=((source[i]>>6)&0x1f)) {
            emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
          }
        }
        if((source[i]&0x3f)==0) emit_faddd(31,30,31);
        if((source[i]&0x3f)==1) emit_fsubd(31,30,31);
        if((source[i]&0x3f)==2) emit_fmuld(31,30,31);
        if((source[i]&0x3f)==3) emit_fdivd(31,30,31);
        if(((source[i]>>16)&0x1f)==((source[i]>>6)&0x1f)) {
          emit_fstd(31,HOST_TEMPREG);
        }else{
          emit_fstd(31,temp);
        }
      }
    }
    else {
      if(opcode2[i]==0x10) {
        emit_flds(temp,31);
        if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f)) {
          emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>6)&0x1f],temp);
        }
        if((source[i]&0x3f)==0) emit_fadds(31,31,31);
        if((source[i]&0x3f)==1) emit_fsubs(31,31,31);
        if((source[i]&0x3f)==2) emit_fmuls(31,31,31);
        if((source[i]&0x3f)==3) emit_fdivs(31,31,31);
        emit_fsts(31,temp);
      }
      else if(opcode2[i]==0x11) {
        emit_fldd(temp,31);
        if(((source[i]>>11)&0x1f)!=((source[i]>>6)&0x1f)) {
          emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>6)&0x1f],temp);
        }
        if((source[i]&0x3f)==0) emit_faddd(31,31,31);
        if((source[i]&0x3f)==1) emit_fsubd(31,31,31);
        if((source[i]&0x3f)==2) emit_fmuld(31,31,31);
        if((source[i]&0x3f)==3) emit_fdivd(31,31,31);
        emit_fstd(31,temp);
      }
    }
    return;
  }
  
  u_int hr,reglist=0;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(i_regs->regmap[hr]>=0) reglist|=1<<hr;
  }
  if(opcode2[i]==0x10) { // Single precision
    save_regs(reglist);
    emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>11)&0x1f],ARG1_REG);
    if((source[i]&0x3f)<4) {
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>>16)&0x1f],ARG2_REG);
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG3_REG);
    }else{
      emit_readword64((intptr_t)&reg_cop1_simple[(source[i]>> 6)&0x1f],ARG2_REG);
    }
    switch(source[i]&0x3f)
    {
      case 0x00: emit_call((intptr_t)add_s);break;
      case 0x01: emit_call((intptr_t)sub_s);break;
      case 0x02: emit_call((intptr_t)mul_s);break;
      case 0x03: emit_call((intptr_t)div_s);break;
      case 0x04: emit_call((intptr_t)sqrt_s);break;
      case 0x05: emit_call((intptr_t)abs_s);break;
      case 0x06: emit_call((intptr_t)mov_s);break;
      case 0x07: emit_call((intptr_t)neg_s);break;
    }
    restore_regs(reglist);
  }
  if(opcode2[i]==0x11) { // Double precision
    save_regs(reglist);
    emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>11)&0x1f],ARG1_REG);
    if((source[i]&0x3f)<4) {
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>>16)&0x1f],ARG2_REG);
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG3_REG);
    }else{
      emit_readword64((intptr_t)&reg_cop1_double[(source[i]>> 6)&0x1f],ARG2_REG);
    }
    switch(source[i]&0x3f)
    {
      case 0x00: emit_call((intptr_t)add_d);break;
      case 0x01: emit_call((intptr_t)sub_d);break;
      case 0x02: emit_call((intptr_t)mul_d);break;
      case 0x03: emit_call((intptr_t)div_d);break;
      case 0x04: emit_call((intptr_t)sqrt_d);break;
      case 0x05: emit_call((intptr_t)abs_d);break;
      case 0x06: emit_call((intptr_t)mov_d);break;
      case 0x07: emit_call((intptr_t)neg_d);break;
    }
    restore_regs(reglist);
  }
}

static void multdiv_assemble_arm64(int i,struct regstat *i_regs)
{
  //  case 0x18: MULT
  //  case 0x19: MULTU
  //  case 0x1A: DIV
  //  case 0x1B: DIVU
  //  case 0x1C: DMULT
  //  case 0x1D: DMULTU
  //  case 0x1E: DDIV
  //  case 0x1F: DDIVU
  if(rs1[i]&&rs2[i])
  {
    if((opcode2[i]&4)==0) // 32-bit
    {
      if(opcode2[i]==0x18) // MULT
      {
        signed char m1=get_reg(i_regs->regmap,rs1[i]);
        signed char m2=get_reg(i_regs->regmap,rs2[i]);
        signed char high=get_reg(i_regs->regmap,HIREG);
        signed char low=get_reg(i_regs->regmap,LOREG);
        assert(m1>=0);
        assert(m2>=0);
        assert(high>=0);
        assert(low>=0);
        emit_smull(m1,m2,high);
        emit_mov(high,low);
        emit_shrimm64(high,32,high);
      }
      if(opcode2[i]==0x19) // MULTU
      {
        signed char m1=get_reg(i_regs->regmap,rs1[i]);
        signed char m2=get_reg(i_regs->regmap,rs2[i]);
        signed char high=get_reg(i_regs->regmap,HIREG);
        signed char low=get_reg(i_regs->regmap,LOREG);
        assert(m1>=0);
        assert(m2>=0);
        assert(high>=0);
        assert(low>=0);
        emit_umull(m1,m2,high);
        emit_mov(high,low);
        emit_shrimm64(high,32,high);
      }
      if(opcode2[i]==0x1A) // DIV
      {
        signed char numerator=get_reg(i_regs->regmap,rs1[i]);
        signed char denominator=get_reg(i_regs->regmap,rs2[i]);
        assert(numerator>=0);
        assert(denominator>=0);
        signed char quotient=get_reg(i_regs->regmap,LOREG);
        signed char remainder=get_reg(i_regs->regmap,HIREG);
        assert(quotient>=0);
        assert(remainder>=0);
        emit_test(denominator,denominator);
        emit_jeq((intptr_t)out+12); // Division by zero
        emit_sdiv(numerator,denominator,quotient);
        emit_msub(quotient,denominator,numerator,remainder);
      }
      if(opcode2[i]==0x1B) // DIVU
      {
        signed char numerator=get_reg(i_regs->regmap,rs1[i]);
        signed char denominator=get_reg(i_regs->regmap,rs2[i]);
        assert(numerator>=0);
        assert(denominator>=0);
        signed char quotient=get_reg(i_regs->regmap,LOREG);
        signed char remainder=get_reg(i_regs->regmap,HIREG);
        assert(quotient>=0);
        assert(remainder>=0);
        emit_test(denominator,denominator);
        emit_jeq((intptr_t)out+12); // Division by zero
        emit_udiv(numerator,denominator,quotient);
        emit_msub(quotient,denominator,numerator,remainder);
      }
    }
    else // 64-bit
    {
      if(opcode2[i]==0x1C) // DMULT
      {
        signed char m1h=get_reg(i_regs->regmap,rs1[i]|64);
        signed char m1l=get_reg(i_regs->regmap,rs1[i]);
        signed char m2h=get_reg(i_regs->regmap,rs2[i]|64);
        signed char m2l=get_reg(i_regs->regmap,rs2[i]);
        assert(m1h>=0);
        assert(m2h>=0);
        assert(m1l>=0);
        assert(m2l>=0);
        signed char hih=get_reg(i_regs->regmap,HIREG|64);
        signed char hil=get_reg(i_regs->regmap,HIREG);
        signed char loh=get_reg(i_regs->regmap,LOREG|64);
        signed char lol=get_reg(i_regs->regmap,LOREG);
        assert(hih>=0);
        assert(hil>=0);
        assert(loh>=0);
        assert(lol>=0);
        emit_mov(m1l,lol);
        emit_orrshlimm64(m1h,32,lol);
        emit_mov(m2l,loh);
        emit_orrshlimm64(m2h,32,loh);
        emit_mul64(lol,loh,hil);
        emit_smulh(lol,loh,hih);
        emit_mov(hil,lol);
        emit_shrimm64(hil,32,loh);
        emit_mov(hih,hil);
        emit_shrimm64(hih,32,hih);
      }
      if(opcode2[i]==0x1D) // DMULTU
      {
        signed char m1h=get_reg(i_regs->regmap,rs1[i]|64);
        signed char m1l=get_reg(i_regs->regmap,rs1[i]);
        signed char m2h=get_reg(i_regs->regmap,rs2[i]|64);
        signed char m2l=get_reg(i_regs->regmap,rs2[i]);
        assert(m1h>=0);
        assert(m2h>=0);
        assert(m1l>=0);
        assert(m2l>=0);
        signed char hih=get_reg(i_regs->regmap,HIREG|64);
        signed char hil=get_reg(i_regs->regmap,HIREG);
        signed char loh=get_reg(i_regs->regmap,LOREG|64);
        signed char lol=get_reg(i_regs->regmap,LOREG);
        assert(hih>=0);
        assert(hil>=0);
        assert(loh>=0);
        assert(lol>=0);
        emit_mov(m1l,lol);
        emit_orrshlimm64(m1h,32,lol);
        emit_mov(m2l,loh);
        emit_orrshlimm64(m2h,32,loh);
        emit_mul64(lol,loh,hil);
        emit_umulh(lol,loh,hih);
        emit_mov(hil,lol);
        emit_shrimm64(hil,32,loh);
        emit_mov(hih,hil);
        emit_shrimm64(hih,32,hih);
      }
      if(opcode2[i]==0x1E) // DDIV
      {
        signed char numh=get_reg(i_regs->regmap,rs1[i]|64);
        signed char numl=get_reg(i_regs->regmap,rs1[i]);
        signed char denomh=get_reg(i_regs->regmap,rs2[i]|64);
        signed char denoml=get_reg(i_regs->regmap,rs2[i]);
        assert(numh>=0);
        assert(numl>=0);
        assert(denomh>=0);
        assert(denoml>=0);
        signed char remh=get_reg(i_regs->regmap,HIREG|64);
        signed char reml=get_reg(i_regs->regmap,HIREG);
        signed char quoh=get_reg(i_regs->regmap,LOREG|64);
        signed char quol=get_reg(i_regs->regmap,LOREG);
        assert(remh>=0);
        assert(reml>=0);
        assert(quoh>=0);
        assert(quol>=0);
        emit_mov(numl,quol);
        emit_orrshlimm64(numh,32,quol);
        emit_mov(denoml,quoh);
        emit_orrshlimm64(denomh,32,quoh);
        emit_sdiv64(quol,quoh,reml);
        emit_msub64(reml,quoh,quol,remh);
        emit_mov(reml,quol);
        emit_shrimm64(reml,32,quoh);
        emit_mov(remh,reml);
        emit_shrimm64(remh,32,remh);
      }
      if(opcode2[i]==0x1F) // DDIVU
      {
        signed char numh=get_reg(i_regs->regmap,rs1[i]|64);
        signed char numl=get_reg(i_regs->regmap,rs1[i]);
        signed char denomh=get_reg(i_regs->regmap,rs2[i]|64);
        signed char denoml=get_reg(i_regs->regmap,rs2[i]);
        assert(numh>=0);
        assert(numl>=0);
        assert(denomh>=0);
        assert(denoml>=0);
        signed char remh=get_reg(i_regs->regmap,HIREG|64);
        signed char reml=get_reg(i_regs->regmap,HIREG);
        signed char quoh=get_reg(i_regs->regmap,LOREG|64);
        signed char quol=get_reg(i_regs->regmap,LOREG);
        assert(remh>=0);
        assert(reml>=0);
        assert(quoh>=0);
        assert(quol>=0);
        emit_mov(numl,quol);
        emit_orrshlimm64(numh,32,quol);
        emit_mov(denoml,quoh);
        emit_orrshlimm64(denomh,32,quoh);
        emit_udiv64(quol,quoh,reml);
        emit_msub64(reml,quoh,quol,remh);
        emit_mov(reml,quol);
        emit_shrimm64(reml,32,quoh);
        emit_mov(remh,reml);
        emit_shrimm64(remh,32,remh);
      }
    }
  }
  else
  {
    // Multiply by zero is zero.
    // MIPS does not have a divide by zero exception.
    // The result is undefined, we return zero.
    signed char hr=get_reg(i_regs->regmap,HIREG);
    signed char lr=get_reg(i_regs->regmap,LOREG);
    if(hr>=0) emit_zeroreg(hr);
    if(lr>=0) emit_zeroreg(lr);
  }
}
#define multdiv_assemble multdiv_assemble_arm64

static void do_preload_rhash(int r) {
  // Don't need this for ARM64.  On x86, this puts the value 0xf8 into the
  // register. On ARM64 the hash can be done with a single instruction (below)
}

static void do_preload_rhtbl(int ht) {
  emit_addimm64(FP,(intptr_t)&mini_ht-(intptr_t)&dynarec_local,ht);
}

static void do_rhash(int rs,int rh) {
  emit_andimm(rs,0xf8,rh);
  emit_shlimm(rh,1,rh);
}

static void do_miniht_load(int ht,int rh) {
  assem_debug("add %s,%s,%s",regname64[ht],regname64[ht],regname64[rh]);
  output_w32(0x8b000000|rh<<16|ht<<5|ht);
  assem_debug("ldr %s,[%s]",regname[rh],regname64[ht]);
  output_w32(0xb9400000|ht<<5|rh);
}

static void do_miniht_jump(int rs,int rh,int ht) {
  emit_cmp(rh,rs);
  #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
  emit_jeq((intptr_t)out+12);
  emit_mov(rs,7);
  emit_jmp(jump_vaddr_reg[7]);
  #else
  emit_jeq((intptr_t)out+8);
  emit_jmp(jump_vaddr_reg[rs]);
  #endif
  assem_debug("ldr %s,[%s,#8]",regname64[ht],regname64[ht]);
  output_w32(0xf9400000|(8>>3)<<10|ht<<5|ht);
  emit_jmpreg(ht);
}

static void do_miniht_insert(u_int return_address,int rt,int temp) {
  emit_movz_lsl16((return_address>>16)&0xffff,rt);
  emit_movk(return_address&0xffff,rt);
  add_to_linker((intptr_t)out,return_address,1);
  emit_adr((intptr_t)out,temp);
  emit_writeword64(temp,(intptr_t)&mini_ht[(return_address&0xFF)>>3][1]);
  emit_writeword(rt,(intptr_t)&mini_ht[(return_address&0xFF)>>3][0]);
}

// Clearing the cache is rather slow on ARM Linux, so mark the areas
// that need to be cleared, and then only clear these areas once.
static void do_clear_cache(void)
{
  int i,j;
  for (i=0;i<(1<<(TARGET_SIZE_2-17));i++)
  {
    u_int bitmap=needs_clear_cache[i];
    if(bitmap) {
      uintptr_t start,end;
      for(j=0;j<32;j++) 
      {
        if(bitmap&(1<<j)) {
          start=BASE_ADDR+i*131072+j*4096;
          end=start+4095;
          j++;
          while(j<32) {
            if(bitmap&(1<<j)) {
              end+=4096;
              j++;
            }else{
              __clear_cache((char *)start,(char *)end);
              //cacheflush((void *)start,(void *)end,0);
              break;
            }
          }
        }
      }
      needs_clear_cache[i]=0;
    }
  }
}

// CPU-architecture-specific initialization
static void arch_init(void) {
  rounding_modes[0]=0x0<<22; // round
  rounding_modes[1]=0x3<<22; // trunc
  rounding_modes[2]=0x1<<22; // ceil
  rounding_modes[3]=0x2<<22; // floor

  jump_table_symbols[15] = (intptr_t) cached_interpreter_table.MFC0;
  jump_table_symbols[16] = (intptr_t) cached_interpreter_table.MTC0;
  jump_table_symbols[17] = (intptr_t) cached_interpreter_table.TLBR;
  jump_table_symbols[18] = (intptr_t) cached_interpreter_table.TLBP;

  #ifdef RAM_OFFSET
  ram_offset=((intptr_t)g_rdram-(intptr_t)0x80000000)>>2;
  #endif
}
