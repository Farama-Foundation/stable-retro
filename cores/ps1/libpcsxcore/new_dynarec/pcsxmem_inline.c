/*
 * (C) Gražvydas "notaz" Ignotas, 2011
 *
 * This work is licensed under the terms of GNU GPL version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef DRC_DBG

static int pcsx_direct_read(int type, u_int addr, int cc_adj, int cc, int rs, int rt)
{
  if ((addr & 0xfffff000) == 0x1f801000) {
    u_int t;
    switch (addr & 0xffff) {
      case 0x1120: // rcnt2 count
        if (rt < 0) goto dont_care;
        if (cc < 0) return 0;
        host_tempreg_acquire();
        emit_readword(&rcnts[2].mode, HOST_TEMPREG);
        emit_readword(&rcnts[2].cycleStart, rt);
        emit_testimm(HOST_TEMPREG, 0x200);
        emit_readword(&last_count, HOST_TEMPREG);
        emit_sub(HOST_TEMPREG, rt, HOST_TEMPREG);
        emit_add(HOST_TEMPREG, cc, HOST_TEMPREG);
        if (cc_adj)
          emit_addimm(HOST_TEMPREG, cc_adj, rt);
        host_tempreg_release();
        emit_shrne_imm(rt, 3, rt);
        mov_loadtype_adj(type!=LOADW_STUB?type:LOADH_STUB, rt, rt);
        goto hit;
      case 0x1104:
      case 0x1114:
      case 0x1124: // rcnt mode
        if (rt < 0) return 0;
        t = (addr >> 4) & 3;
        emit_readword(&rcnts[t].mode, rt);
        host_tempreg_acquire();
        emit_andimm(rt, ~0x1800, HOST_TEMPREG);
        emit_writeword(HOST_TEMPREG, &rcnts[t].mode);
        host_tempreg_release();
        mov_loadtype_adj(type, rt, rt);
        goto hit;
    }
  }
  else {
    if (rt < 0)
      goto dont_care;
  }

  return 0;

hit:
  assem_debug("pcsx_direct_read %08x end\n", addr);
  return 1;

dont_care:
  assem_debug("pcsx_direct_read %08x dummy\n", addr);
  return 1;
}

#else

static int pcsx_direct_read(int type, u_int addr, int cc_adj, int cc, int rs, int rt)
{
  return 0;
}

#endif

// vim:shiftwidth=2:expandtab
