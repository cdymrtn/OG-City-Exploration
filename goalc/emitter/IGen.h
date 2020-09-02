#ifndef JAK_IGEN_H
#define JAK_IGEN_H

#include <cassert>
#include "Register.h"
#include "Instruction.h"

namespace emitter {
class IGen {
 public:
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   MOVES
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  /*!
   * Move data from src to dst. Moves all 64-bits of the GPR.
   */
  static Instruction mov_gpr64_gpr64(Register dst, Register src) {
    assert(dst.is_gpr());
    assert(src.is_gpr());
    Instruction instr(0x89);
    instr.set_modrm_and_rex(src.hw_id(), dst.hw_id(), 3, true);
    return instr;
  }

  /*!
   * Move a 64-bit constant into a register.
   */
  static Instruction mov_gpr64_u64(Register dst, uint64_t val) {
    assert(dst.is_gpr());
    bool rex_b = false;
    auto dst_hw_id = dst.hw_id();
    if (dst_hw_id >= 8) {
      dst_hw_id -= 8;
      rex_b = true;
    }
    Instruction instr(0xb8 + dst_hw_id);
    instr.set(REX(true, false, false, rex_b));
    instr.set(Imm(8, val));
    return instr;
  }

  /*!
   * Move a 32-bit constant into a register. Zeros the upper 32 bits.
   */
  static Instruction mov_gpr64_u32(Register dst, uint64_t val) {
    assert(val <= UINT32_MAX);
    assert(dst.is_gpr());
    auto dst_hw_id = dst.hw_id();
    bool rex_b = false;
    if (dst_hw_id >= 8) {
      dst_hw_id -= 8;
      rex_b = true;
    }

    Instruction instr(0xb8 + dst_hw_id);
    if (rex_b) {
      instr.set(REX(false, false, false, rex_b));
    }
    instr.set(Imm(4, val));
    return instr;
  }

  /*!
   * Move a signed 32-bit constant into a register. Sign extends for the upper 32 bits.
   * When possible prefer mov_gpr64_u32. (use this only for negative values...)
   * This is always bigger than mov_gpr64_u32, but smaller than a mov_gpr_u64.
   */
  static Instruction mov_gpr64_s32(Register dst, int64_t val) {
    assert(val >= INT32_MIN && val <= INT32_MAX);
    assert(dst.is_gpr());
    Instruction instr(0xc7);
    instr.set_modrm_and_rex(0, dst.hw_id(), 3, true);
    instr.set(Imm(4, val));
    return instr;
  }

  /*!
   * Move 32-bits of xmm to 32 bits of gpr (no sign extension).
   */
  static Instruction movd_gpr32_xmm32(Register dst, Register src) {
    assert(dst.is_gpr());
    assert(src.is_xmm());
    Instruction instr(0x66);
    instr.set_op2(0x0f);
    instr.set_op3(0x7e);
    instr.set_modrm_and_rex(src.hw_id(), dst.hw_id(), 3, false);
    instr.swap_op0_rex();
    return instr;
  }

  /*!
   * Move 32-bits of gpr to 32-bits of xmm (no sign extension)
   */
  static Instruction movd_xmm32_gpr32(Register dst, Register src) {
    assert(dst.is_xmm());
    assert(src.is_gpr());
    Instruction instr(0x66);
    instr.set_op2(0x0f);
    instr.set_op3(0x6e);
    instr.set_modrm_and_rex(dst.hw_id(), src.hw_id(), 3, false);
    instr.swap_op0_rex();
    return instr;
  }

  /*!
   * Move 32-bits between xmm's
   */
  static Instruction mov_xmm32_xmm32(Register dst, Register src) {
    assert(dst.is_xmm());
    assert(src.is_xmm());
    Instruction instr(0xf3);
    instr.set_op2(0x0f);
    instr.set_op3(0x10);
    instr.set_modrm_and_rex(dst.hw_id(), src.hw_id(), 3, false);
    return instr;
  }

  // todo - GPR64 -> XMM64 (zext)
  // todo - XMM -> GPR64
  // todo - XMM128 - XMM128

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   GOAL Loads and Stores
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  /*!
   * movsx dst, BYTE PTR [addr1 + addr2]
   * addr1 and addr2 have to be different registers.
   * Cannot use rsp.
   */
  static Instruction load8s_gpr64_gpr64_plus_gpr64(Register dst, Register addr1, Register addr2) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0xf);
    instr.set_op2(0xbe);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(dst.hw_id(), addr1.hw_id(), addr2.hw_id(), true,
                                                  false);
    return instr;
  }

  static Instruction store8_gpr64_gpr64_plus_gpr64(Register addr1, Register addr2, Register value) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0x88);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(value.hw_id(), addr1.hw_id(), addr2.hw_id());
    if (value.id() > RBX) {
      instr.add_rex();
    }
    return instr;
  }

  static Instruction load8s_gpr64_gpr64_plus_gpr64_plus_s8(Register dst,
                                                           Register addr1,
                                                           Register addr2,
                                                           s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xbe);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, true);
    return instr;
  }

  static Instruction store8_gpr64_gpr64_plus_gpr64_plus_s8(Register addr1,
                                                           Register addr2,
                                                           Register value,
                                                           s64 offset) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0x88);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(value.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, false);
    if (value.id() > RBX) {
      instr.add_rex();
    }
    return instr;
  }

  static Instruction load8s_gpr64_gpr64_plus_gpr64_plus_s32(Register dst,
                                                            Register addr1,
                                                            Register addr2,
                                                            s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xbe);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, true);
    return instr;
  }

  static Instruction store8_gpr64_gpr64_plus_gpr64_plus_s32(Register addr1,
                                                            Register addr2,
                                                            Register value,
                                                            s64 offset) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x88);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(value.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, false);
    if (value.id() > RBX) {
      instr.add_rex();
    }
    return instr;
  }

  /*!
   * movzx dst, BYTE PTR [addr1 + addr2]
   * addr1 and addr2 have to be different registers.
   * Cannot use rsp.
   */
  static Instruction load8u_gpr64_gpr64_plus_gpr64(Register dst, Register addr1, Register addr2) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0xf);
    instr.set_op2(0xb6);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(dst.hw_id(), addr1.hw_id(), addr2.hw_id(), true,
                                                  false);
    return instr;
  }

  static Instruction load8u_gpr64_gpr64_plus_gpr64_plus_s8(Register dst,
                                                           Register addr1,
                                                           Register addr2,
                                                           s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xb6);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, true);
    return instr;
  }

  static Instruction load8u_gpr64_gpr64_plus_gpr64_plus_s32(Register dst,
                                                            Register addr1,
                                                            Register addr2,
                                                            s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xb6);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, true);
    return instr;
  }

  /*!
   * movsx dst, WORD PTR [addr1 + addr2]
   * addr1 and addr2 have to be different registers.
   * Cannot use rsp.
   */
  static Instruction load16s_gpr64_gpr64_plus_gpr64(Register dst, Register addr1, Register addr2) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0xf);
    instr.set_op2(0xbf);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(dst.hw_id(), addr1.hw_id(), addr2.hw_id(), true,
                                                  false);
    return instr;
  }

  static Instruction store16_gpr64_gpr64_plus_gpr64(Register addr1,
                                                    Register addr2,
                                                    Register value) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0x66);
    instr.set_op2(0x89);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(value.hw_id(), addr1.hw_id(), addr2.hw_id());
    instr.swap_op0_rex();  // why?????
    return instr;
  }

  static Instruction store16_gpr64_gpr64_plus_gpr64_plus_s8(Register addr1,
                                                            Register addr2,
                                                            Register value,
                                                            s64 offset) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0x66);
    instr.set_op2(0x89);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(value.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, false);
    instr.swap_op0_rex();  // why?????
    return instr;
  }

  static Instruction store16_gpr64_gpr64_plus_gpr64_plus_s32(Register addr1,
                                                             Register addr2,
                                                             Register value,
                                                             s64 offset) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x66);
    instr.set_op2(0x89);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(value.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, false);
    instr.swap_op0_rex();  // why?????
    return instr;
  }

  static Instruction load16s_gpr64_gpr64_plus_gpr64_plus_s8(Register dst,
                                                            Register addr1,
                                                            Register addr2,
                                                            s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xbf);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, true);
    return instr;
  }

  static Instruction load16s_gpr64_gpr64_plus_gpr64_plus_s32(Register dst,
                                                             Register addr1,
                                                             Register addr2,
                                                             s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xbf);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, true);
    return instr;
  }

  /*!
   * movzx dst, WORD PTR [addr1 + addr2]
   * addr1 and addr2 have to be different registers.
   * Cannot use rsp.
   */
  static Instruction load16u_gpr64_gpr64_plus_gpr64(Register dst, Register addr1, Register addr2) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0xf);
    instr.set_op2(0xb7);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(dst.hw_id(), addr1.hw_id(), addr2.hw_id(), true,
                                                  false);
    return instr;
  }

  static Instruction load16u_gpr64_gpr64_plus_gpr64_plus_s8(Register dst,
                                                            Register addr1,
                                                            Register addr2,
                                                            s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xb7);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, true);
    return instr;
  }

  static Instruction load16u_gpr64_gpr64_plus_gpr64_plus_s32(Register dst,
                                                             Register addr1,
                                                             Register addr2,
                                                             s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xb7);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, true);
    return instr;
  }

  /*!
   * movsxd dst, DWORD PTR [addr1 + addr2]
   * addr1 and addr2 have to be different registers.
   * Cannot use rsp.
   */
  static Instruction load32s_gpr64_gpr64_plus_gpr64(Register dst, Register addr1, Register addr2) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0x63);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(dst.hw_id(), addr1.hw_id(), addr2.hw_id(), true);
    return instr;
  }

  static Instruction store32_gpr64_gpr64_plus_gpr64(Register addr1,
                                                    Register addr2,
                                                    Register value) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0x89);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(value.hw_id(), addr1.hw_id(), addr2.hw_id());
    return instr;
  }

  static Instruction load32s_gpr64_gpr64_plus_gpr64_plus_s8(Register dst,
                                                            Register addr1,
                                                            Register addr2,
                                                            s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0x63);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, true);
    return instr;
  }

  static Instruction store32_gpr64_gpr64_plus_gpr64_plus_s8(Register addr1,
                                                            Register addr2,
                                                            Register value,
                                                            s64 offset) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0x89);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(value.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, false);
    return instr;
  }

  static Instruction load32s_gpr64_gpr64_plus_gpr64_plus_s32(Register dst,
                                                             Register addr1,
                                                             Register addr2,
                                                             s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x63);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, true);
    return instr;
  }

  static Instruction store32_gpr64_gpr64_plus_gpr64_plus_s32(Register addr1,
                                                             Register addr2,
                                                             Register value,
                                                             s64 offset) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x89);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(value.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, false);
    return instr;
  }

  /*!
   * movzxd dst, DWORD PTR [addr1 + addr2]
   * addr1 and addr2 have to be different registers.
   * Cannot use rsp.
   */
  static Instruction load32u_gpr64_gpr64_plus_gpr64(Register dst, Register addr1, Register addr2) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0x8b);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(dst.hw_id(), addr1.hw_id(), addr2.hw_id());
    return instr;
  }

  static Instruction load32u_gpr64_gpr64_plus_gpr64_plus_s8(Register dst,
                                                            Register addr1,
                                                            Register addr2,
                                                            s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0x8b);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, false);
    return instr;
  }

  static Instruction load32u_gpr64_gpr64_plus_gpr64_plus_s32(Register dst,
                                                             Register addr1,
                                                             Register addr2,
                                                             s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x8b);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, false);
    return instr;
  }

  /*!
   * mov dst, QWORD PTR [addr1 + addr2]
   * addr1 and addr2 have to be different registers.
   * Cannot use rsp.
   */
  static Instruction load64_gpr64_gpr64_plus_gpr64(Register dst, Register addr1, Register addr2) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0x8b);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(dst.hw_id(), addr1.hw_id(), addr2.hw_id(), true);
    return instr;
  }

  static Instruction store64_gpr64_gpr64_plus_gpr64(Register addr1,
                                                    Register addr2,
                                                    Register value) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    Instruction instr(0x89);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(value.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                  true);
    return instr;
  }

  static Instruction load64_gpr64_gpr64_plus_gpr64_plus_s8(Register dst,
                                                           Register addr1,
                                                           Register addr2,
                                                           s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0x8b);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, true);
    return instr;
  }

  static Instruction store64_gpr64_gpr64_plus_gpr64_plus_s8(Register addr1,
                                                            Register addr2,
                                                            Register value,
                                                            s64 offset) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT8_MIN && offset <= INT8_MAX);
    Instruction instr(0x89);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(value.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, true);
    return instr;
  }

  static Instruction load64_gpr64_gpr64_plus_gpr64_plus_s32(Register dst,
                                                            Register addr1,
                                                            Register addr2,
                                                            s64 offset) {
    assert(dst.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x8b);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(dst.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, true);
    return instr;
  }

  static Instruction store64_gpr64_gpr64_plus_gpr64_plus_s32(Register addr1,
                                                             Register addr2,
                                                             Register value,
                                                             s64 offset) {
    assert(value.is_gpr());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(addr1 != addr2);
    assert(addr1 != RSP);
    assert(addr2 != RSP);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x89);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(value.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                      offset, true);
    return instr;
  }

  static Instruction store_goal_gpr(Register addr,
                                    Register value,
                                    Register off,
                                    int offset,
                                    int size) {
    switch (size) {
      case 1:
        if (offset == 0) {
          return store8_gpr64_gpr64_plus_gpr64(addr, off, value);
        } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
          return store8_gpr64_gpr64_plus_gpr64_plus_s8(addr, off, value, offset);
        } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
          return store8_gpr64_gpr64_plus_gpr64_plus_s32(addr, off, value, offset);
        } else {
          assert(false);
        }
      case 2:
        if (offset == 0) {
          return store16_gpr64_gpr64_plus_gpr64(addr, off, value);
        } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
          return store16_gpr64_gpr64_plus_gpr64_plus_s8(addr, off, value, offset);
        } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
          return store16_gpr64_gpr64_plus_gpr64_plus_s32(addr, off, value, offset);
        } else {
          assert(false);
        }
      case 4:
        if (offset == 0) {
          return store32_gpr64_gpr64_plus_gpr64(addr, off, value);
        } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
          return store32_gpr64_gpr64_plus_gpr64_plus_s8(addr, off, value, offset);
        } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
          return store32_gpr64_gpr64_plus_gpr64_plus_s32(addr, off, value, offset);
        } else {
          assert(false);
        }
      case 8:
        if (offset == 0) {
          return store64_gpr64_gpr64_plus_gpr64(addr, off, value);
        } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
          return store64_gpr64_gpr64_plus_gpr64_plus_s8(addr, off, value, offset);
        } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
          return store64_gpr64_gpr64_plus_gpr64_plus_s32(addr, off, value, offset);
        } else {
          assert(false);
        }
      default:
        assert(false);
    }
  }

  /*!
   * Load memory at addr + offset, where addr is a GOAL pointer and off is the offset register.
   * This will pick the appropriate fancy addressing mode instruction.
   */
  static Instruction load_goal_gpr(Register dst,
                                   Register addr,
                                   Register off,
                                   int offset,
                                   int size,
                                   bool sign_extend) {
    switch (size) {
      case 1:
        if (offset == 0) {
          if (sign_extend) {
            return load8s_gpr64_gpr64_plus_gpr64(dst, addr, off);
          } else {
            return load8u_gpr64_gpr64_plus_gpr64(dst, addr, off);
          }
        } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
          if (sign_extend) {
            return load8s_gpr64_gpr64_plus_gpr64_plus_s8(dst, addr, off, offset);
          } else {
            return load8u_gpr64_gpr64_plus_gpr64_plus_s8(dst, addr, off, offset);
          }
        } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
          if (sign_extend) {
            return load8s_gpr64_gpr64_plus_gpr64_plus_s32(dst, addr, off, offset);
          } else {
            return load8u_gpr64_gpr64_plus_gpr64_plus_s32(dst, addr, off, offset);
          }
        } else {
          assert(false);
        }
      case 2:
        if (offset == 0) {
          if (sign_extend) {
            return load16s_gpr64_gpr64_plus_gpr64(dst, addr, off);
          } else {
            return load16u_gpr64_gpr64_plus_gpr64(dst, addr, off);
          }
        } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
          if (sign_extend) {
            return load16s_gpr64_gpr64_plus_gpr64_plus_s8(dst, addr, off, offset);
          } else {
            return load16u_gpr64_gpr64_plus_gpr64_plus_s8(dst, addr, off, offset);
          }
        } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
          if (sign_extend) {
            return load16s_gpr64_gpr64_plus_gpr64_plus_s32(dst, addr, off, offset);
          } else {
            return load16u_gpr64_gpr64_plus_gpr64_plus_s32(dst, addr, off, offset);
          }
        } else {
          assert(false);
        }
      case 4:
        if (offset == 0) {
          if (sign_extend) {
            return load32s_gpr64_gpr64_plus_gpr64(dst, addr, off);
          } else {
            return load32u_gpr64_gpr64_plus_gpr64(dst, addr, off);
          }
        } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
          if (sign_extend) {
            return load32s_gpr64_gpr64_plus_gpr64_plus_s8(dst, addr, off, offset);
          } else {
            return load32u_gpr64_gpr64_plus_gpr64_plus_s8(dst, addr, off, offset);
          }
        } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
          if (sign_extend) {
            return load32s_gpr64_gpr64_plus_gpr64_plus_s32(dst, addr, off, offset);
          } else {
            return load32u_gpr64_gpr64_plus_gpr64_plus_s32(dst, addr, off, offset);
          }
        } else {
          assert(false);
        }
      case 8:
        if (offset == 0) {
          return load64_gpr64_gpr64_plus_gpr64(dst, addr, off);

        } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
          return load64_gpr64_gpr64_plus_gpr64_plus_s8(dst, addr, off, offset);

        } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
          return load64_gpr64_gpr64_plus_gpr64_plus_s32(dst, addr, off, offset);

        } else {
          assert(false);
        }
      default:
        assert(false);
    }
  }

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   LOADS n' STORES - XMM32
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  static Instruction store32_xmm32_gpr64_plus_gpr64(Register addr1,
                                                    Register addr2,
                                                    Register xmm_value) {
    assert(xmm_value.is_xmm());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());

    Instruction instr(0xf3);
    instr.set_op2(0x0f);
    instr.set_op3(0x11);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(xmm_value.hw_id(), addr1.hw_id(), addr2.hw_id());

    instr.swap_op0_rex();
    return instr;
  }

  static Instruction load32_xmm32_gpr64_plus_gpr64(Register xmm_dest,
                                                   Register addr1,
                                                   Register addr2) {
    assert(xmm_dest.is_xmm());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());

    Instruction instr(0xf3);
    instr.set_op2(0x0f);
    instr.set_op3(0x10);
    instr.set_modrm_and_rex_for_reg_plus_reg_addr(xmm_dest.hw_id(), addr1.hw_id(), addr2.hw_id());

    instr.swap_op0_rex();
    return instr;
  }

  static Instruction store32_xmm32_gpr64_plus_gpr64_plus_s8(Register addr1,
                                                            Register addr2,
                                                            Register xmm_value,
                                                            s64 offset) {
    assert(xmm_value.is_xmm());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(offset >= INT8_MIN && offset <= INT8_MAX);

    Instruction instr(0xf3);
    instr.set_op2(0x0f);
    instr.set_op3(0x11);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(xmm_value.hw_id(), addr1.hw_id(),
                                                     addr2.hw_id(), offset, false);

    instr.swap_op0_rex();
    return instr;
  }

  static Instruction load32_xmm32_gpr64_plus_gpr64_plus_s8(Register xmm_dest,
                                                           Register addr1,
                                                           Register addr2,
                                                           s64 offset) {
    assert(xmm_dest.is_xmm());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(offset >= INT8_MIN && offset <= INT8_MAX);

    Instruction instr(0xf3);
    instr.set_op2(0x0f);
    instr.set_op3(0x10);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s8(xmm_dest.hw_id(), addr1.hw_id(), addr2.hw_id(),
                                                     offset, false);

    instr.swap_op0_rex();
    return instr;
  }

  static Instruction store32_xmm32_gpr64_plus_gpr64_plus_s32(Register addr1,
                                                             Register addr2,
                                                             Register xmm_value,
                                                             s64 offset) {
    assert(xmm_value.is_xmm());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);

    Instruction instr(0xf3);
    instr.set_op2(0x0f);
    instr.set_op3(0x11);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(xmm_value.hw_id(), addr1.hw_id(),
                                                      addr2.hw_id(), offset, false);

    instr.swap_op0_rex();
    return instr;
  }

  static Instruction load32_xmm32_gpr64_plus_gpr64_plus_s32(Register xmm_dest,
                                                            Register addr1,
                                                            Register addr2,
                                                            s64 offset) {
    assert(xmm_dest.is_xmm());
    assert(addr1.is_gpr());
    assert(addr2.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);

    Instruction instr(0xf3);
    instr.set_op2(0x0f);
    instr.set_op3(0x10);
    instr.set_modrm_and_rex_for_reg_plus_reg_plus_s32(xmm_dest.hw_id(), addr1.hw_id(),
                                                      addr2.hw_id(), offset, false);

    instr.swap_op0_rex();
    return instr;
  }

  static Instruction load_goal_xmm32(Register xmm_dest, Register addr, Register off, s64 offset) {
    if (offset == 0) {
      return load32_xmm32_gpr64_plus_gpr64(xmm_dest, addr, off);
    } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
      return load32_xmm32_gpr64_plus_gpr64_plus_s8(xmm_dest, addr, off, offset);
    } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
      return load32_xmm32_gpr64_plus_gpr64_plus_s32(xmm_dest, addr, off, offset);
    } else {
      assert(false);
    }
  }

  static Instruction store_goal_xmm32(Register addr, Register xmm_value, Register off, s64 offset) {
    if (offset == 0) {
      return store32_xmm32_gpr64_plus_gpr64(addr, off, xmm_value);
    } else if (offset >= INT8_MIN && offset <= INT8_MAX) {
      return store32_xmm32_gpr64_plus_gpr64_plus_s8(addr, off, xmm_value, offset);
    } else if (offset >= INT32_MIN && offset <= INT32_MAX) {
      return store32_xmm32_gpr64_plus_gpr64_plus_s32(addr, off, xmm_value, offset);
    } else {
      assert(false);
    }
  }

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   LOADS n' STORES - XMM128
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  /*!
   * Store a 128-bit xmm into an address stored in a register, no offset
   */
  static Instruction store128_gpr64_xmm128(Register gpr_addr, Register xmm_value) {
    assert(gpr_addr.is_gpr());
    assert(xmm_value.is_xmm());
    Instruction instr(0x66);
    instr.set_op2(0x0f);
    instr.set_op3(0x7f);
    instr.set_modrm_and_rex_for_reg_addr(xmm_value.hw_id(), gpr_addr.hw_id(), false);
    instr.swap_op0_rex();
    return instr;
  }

  static Instruction load128_xmm128_gpr64(Register xmm_dest, Register gpr_addr) {
    assert(gpr_addr.is_gpr());
    assert(xmm_dest.is_xmm());
    Instruction instr(0x66);
    instr.set_op2(0x0f);
    instr.set_op3(0x6f);
    instr.set_modrm_and_rex_for_reg_addr(xmm_dest.hw_id(), gpr_addr.hw_id(), false);
    instr.swap_op0_rex();
    return instr;
  }

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   RIP loads and stores
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  static Instruction load64_rip_s32(Register dest, s64 offset) {
    assert(dest.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x8b);
    instr.set_modrm_and_rex_for_rip_plus_s32(dest.hw_id(), offset, true);
    return instr;
  }

  static Instruction load32s_rip_s32(Register dest, s64 offset) {
    assert(dest.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x63);
    instr.set_modrm_and_rex_for_rip_plus_s32(dest.hw_id(), offset, true);
    return instr;
  }

  static Instruction load32u_rip_s32(Register dest, s64 offset) {
    assert(dest.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x8b);
    instr.set_modrm_and_rex_for_rip_plus_s32(dest.hw_id(), offset, false);
    return instr;
  }

  static Instruction load16u_rip_s32(Register dest, s64 offset) {
    assert(dest.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xb7);
    instr.set_modrm_and_rex_for_rip_plus_s32(dest.hw_id(), offset, true);
    return instr;
  }

  static Instruction load16s_rip_s32(Register dest, s64 offset) {
    assert(dest.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xbf);
    instr.set_modrm_and_rex_for_rip_plus_s32(dest.hw_id(), offset, true);
    return instr;
  }

  static Instruction load8u_rip_s32(Register dest, s64 offset) {
    assert(dest.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xb6);
    instr.set_modrm_and_rex_for_rip_plus_s32(dest.hw_id(), offset, true);
    return instr;
  }

  static Instruction load8s_rip_s32(Register dest, s64 offset) {
    assert(dest.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0xf);
    instr.set_op2(0xbe);
    instr.set_modrm_and_rex_for_rip_plus_s32(dest.hw_id(), offset, true);
    return instr;
  }

  static Instruction static_load(Register dest, s64 offset, int size, bool sign_extend) {
    switch (size) {
      case 1:
        if (sign_extend) {
          return load8s_rip_s32(dest, offset);
        } else {
          return load8u_rip_s32(dest, offset);
        }
        break;
      case 2:
        if (sign_extend) {
          return load16s_rip_s32(dest, offset);
        } else {
          return load16u_rip_s32(dest, offset);
        }
        break;
      case 4:
        if (sign_extend) {
          return load32s_rip_s32(dest, offset);
        } else {
          return load32u_rip_s32(dest, offset);
        }
        break;
      case 8:
        return load64_rip_s32(dest, offset);
      default:
        assert(false);
    }
  }

  static Instruction store64_rip_s32(Register src, s64 offset) {
    assert(src.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x89);
    instr.set_modrm_and_rex_for_rip_plus_s32(src.hw_id(), offset, true);
    return instr;
  }

  static Instruction store32_rip_s32(Register src, s64 offset) {
    assert(src.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x89);
    instr.set_modrm_and_rex_for_rip_plus_s32(src.hw_id(), offset, false);
    return instr;
  }

  static Instruction store16_rip_s32(Register src, s64 offset) {
    assert(src.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x66);
    instr.set_op2(0x89);
    instr.set_modrm_and_rex_for_rip_plus_s32(src.hw_id(), offset, false);
    instr.swap_op0_rex();
    return instr;
  }

  static Instruction store8_rip_s32(Register src, s64 offset) {
    assert(src.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x88);
    instr.set_modrm_and_rex_for_rip_plus_s32(src.hw_id(), offset, false);
    if (src.id() > RBX) {
      instr.add_rex();
    }
    return instr;
  }

  static Instruction static_store(Register value, s64 offset, int size) {
    switch (size) {
      case 1:
        return store8_rip_s32(value, offset);
      case 2:
        return store16_rip_s32(value, offset);
      case 4:
        return store32_rip_s32(value, offset);
      case 8:
        return store64_rip_s32(value, offset);
      default:
        assert(false);
    }
  }

  static Instruction static_addr(Register dst, s64 offset) {
    assert(dst.is_gpr());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);
    Instruction instr(0x8d);
    instr.set_modrm_and_rex_for_rip_plus_s32(dst.hw_id(), offset, true);
    return instr;
  }

  static Instruction static_load_xmm32(Register xmm_dest, s64 offset) {
    assert(xmm_dest.is_xmm());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);

    Instruction instr(0xf3);
    instr.set_op2(0x0f);
    instr.set_op3(0x10);
    instr.set_modrm_and_rex_for_rip_plus_s32(xmm_dest.hw_id(), offset, false);

    instr.swap_op0_rex();
    return instr;
  }

  static Instruction static_store_xmm32(Register xmm_value, s64 offset) {
    assert(xmm_value.is_xmm());
    assert(offset >= INT32_MIN && offset <= INT32_MAX);

    Instruction instr(0xf3);
    instr.set_op2(0x0f);
    instr.set_op3(0x11);
    instr.set_modrm_and_rex_for_rip_plus_s32(xmm_value.hw_id(), offset, false);

    instr.swap_op0_rex();
    return instr;
  }

  // TODO, special load/stores of 128 bit values.

  // TODO, consider specialized stack loads and stores?

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   FUNCTION STUFF
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  /*!
   * Function return. Pops the 64-bit return address (real) off the stack and jumps to it.
   */
  static Instruction ret() { return Instruction(0xc3); }

  /*!
   * Instruction to push gpr (64-bits) onto the stack
   */
  static Instruction push_gpr64(Register reg) {
    assert(reg.is_gpr());
    if (reg.hw_id() >= 8) {
      auto i = Instruction(0x50 + reg.hw_id() - 8);
      i.set(REX(false, false, false, true));
      return i;
    }
    return Instruction(0x50 + reg.hw_id());
  }

  /*!
   * Instruction to pop 64 bit gpr from the stack
   */
  static Instruction pop_gpr64(Register reg) {
    if (reg.hw_id() >= 8) {
      auto i = Instruction(0x58 + reg.hw_id() - 8);
      i.set(REX(false, false, false, true));
      return i;
    }
    return Instruction(0x58 + reg.hw_id());
  }

  /*!
   * Call a function stored in a 64-bit gpr
   */
  static Instruction call_r64(uint8_t reg) {
    Instruction instr(0xff);
    if (reg >= 8) {
      instr.set(REX(false, false, false, true));
      reg -= 8;
    }
    assert(reg < 8);
    ModRM mrm;
    mrm.rm = reg;
    mrm.reg_op = 2;
    mrm.mod = 3;
    instr.set(mrm);
    return instr;
  }

  /*!
   * Call a function stored in a 64-bit gpr
   */
  static Instruction jmp_r64(uint8_t reg) {
    Instruction instr(0xff);
    if (reg >= 8) {
      instr.set(REX(false, false, false, true));
      reg -= 8;
    }
    assert(reg < 8);
    ModRM mrm;
    mrm.rm = reg;
    mrm.reg_op = 4;
    mrm.mod = 3;
    instr.set(mrm);
    return instr;
  }

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   INTEGER MATH
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  static Instruction sub_gpr64_imm8s(Register reg, int64_t imm) {
    assert(reg.is_gpr());
    assert(imm >= INT8_MIN && imm <= INT8_MAX);
    // SUB r/m64, imm8 : REX.W + 83 /5 ib
    Instruction instr(0x83);
    instr.set_modrm_and_rex(5, reg.hw_id(), 3, true);
    instr.set(Imm(1, imm));
    return instr;
  }

  static Instruction sub_gpr64_imm32s(Register reg, int64_t imm) {
    assert(reg.is_gpr());
    assert(imm >= INT32_MIN && imm <= INT32_MAX);
    Instruction instr(0x81);
    instr.set_modrm_and_rex(5, reg.hw_id(), 3, true);
    instr.set(Imm(4, imm));
    return instr;
  }

  static Instruction add_gpr64_imm8s(Register reg, int64_t v) {
    assert(v >= INT8_MIN && v <= INT8_MAX);
    Instruction instr(0x83);
    instr.set_modrm_and_rex(0, reg.hw_id(), 3, true);
    instr.set(Imm(1, v));
    return instr;
  }

  static Instruction add_gpr64_imm32s(Register reg, int64_t v) {
    assert(v >= INT32_MIN && v <= INT32_MAX);
    Instruction instr(0x81);
    instr.set_modrm_and_rex(0, reg.hw_id(), 3, true);
    instr.set(Imm(4, v));
    return instr;
  }

  static Instruction add_gpr64_imm(Register reg, int64_t imm) {
    if (imm >= INT8_MIN && imm <= INT8_MAX) {
      return add_gpr64_imm8s(reg, imm);
    } else if (imm >= INT32_MIN && imm <= INT32_MAX) {
      return add_gpr64_imm32s(reg, imm);
    } else {
      assert(false);
    }
  }

  static Instruction sub_gpr64_imm(Register reg, int64_t imm) {
    if (imm >= INT8_MIN && imm <= INT8_MAX) {
      return sub_gpr64_imm8s(reg, imm);
    } else if (imm >= INT32_MIN && imm <= INT32_MAX) {
      return sub_gpr64_imm32s(reg, imm);
    } else {
      assert(false);
    }
  }

  static Instruction add_gpr64_gpr64(Register dst, Register src) {
    Instruction instr(0x01);
    assert(dst.is_gpr());
    assert(src.is_gpr());
    instr.set_modrm_and_rex(src.hw_id(), dst.hw_id(), 3, true);
    return instr;
  }

  static Instruction sub_gpr64_gpr64(Register dst, Register src) {
    Instruction instr(0x29);
    assert(dst.is_gpr());
    assert(src.is_gpr());
    instr.set_modrm_and_rex(src.hw_id(), dst.hw_id(), 3, true);
    return instr;
  }

  /*!
   * Multiply gprs (32-bit, signed).
   * (Note - probably worth doing imul on gpr64's to implement the EE's unsigned multiply)
   */
  static Instruction imul_gpr32_gpr32(Register dst, Register src) {
    Instruction instr(0xf);
    instr.set_op2(0xaf);
    assert(dst.is_gpr());
    assert(src.is_gpr());
    instr.set_modrm_and_rex(dst.hw_id(), src.hw_id(), 3, false);
    return instr;
  }

  /*!
   * Divide (idiv, 32 bit)
   * todo UNTESTED
   */
  static Instruction idiv_gpr32(Register reg) {
    Instruction instr(0xf7);
    assert(reg.is_gpr());
    instr.set_modrm_and_rex(7, reg.hw_id(), 3, false);
    return instr;
  }

  /*!
   * Convert doubleword to quadword for division.
   * todo UNTESTED
   */
  static Instruction cdq() {
    Instruction instr(0x99);
    return instr;
  }

  /*!
   * Move from gpr32 to gpr64, with sign extension.
   * Needed for multiplication/divsion madness.
   */
  static Instruction movsx_r64_r32(Register dst, Register src) {
    Instruction instr(0x63);
    assert(dst.is_gpr());
    assert(src.is_gpr());
    instr.set_modrm_and_rex(dst.hw_id(), src.hw_id(), 3, true);
    return instr;
  }

  /*!
   * Compare gpr64.  This sets the flags for the jumps.
   * todo UNTESTED
   */
  static Instruction cmp_gpr64_gpr64(Register a, Register b) {
    Instruction instr(0x3b);
    assert(a.is_gpr());
    assert(b.is_gpr());
    instr.set_modrm_and_rex(a.hw_id(), b.hw_id(), 3, true);
    return instr;
  }

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   BIT STUFF
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  /*!
   * Or of two gprs
   */
  static Instruction or_gpr64_gpr64(Register dst, Register src) {
    Instruction instr(0x0b);
    assert(dst.is_gpr());
    assert(src.is_gpr());
    instr.set_modrm_and_rex(dst.hw_id(), src.hw_id(), 3, true);
    return instr;
  }

  /*!
   * And of two gprs
   */
  static Instruction and_gpr64_gpr64(Register dst, Register src) {
    Instruction instr(0x23);
    assert(dst.is_gpr());
    assert(src.is_gpr());
    instr.set_modrm_and_rex(dst.hw_id(), src.hw_id(), 3, true);
    return instr;
  }

  /*!
   * Xor of two gprs
   */
  static Instruction xor_gpr64_gpr64(Register dst, Register src) {
    Instruction instr(0x33);
    assert(dst.is_gpr());
    assert(src.is_gpr());
    instr.set_modrm_and_rex(dst.hw_id(), src.hw_id(), 3, true);
    return instr;
  }

  /*!
   * Bitwise not a gpr
   */
  static Instruction not_gpr64(Register reg) {
    Instruction instr(0xf7);
    assert(reg.is_gpr());
    instr.set_modrm_and_rex(2, reg.hw_id(), 3, true);
    return instr;
  }

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   SHIFTS
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  // sllv reg, imm
  // srlv reg, imm
  // srav reg, imm

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   CONTROL FLOW
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  // call, jump reg

  // jump imm8, imm32 ?? (is there an imm16?)

  // je, jne, jle, jge, jl, jg, jbe, jae, jb, ja

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   FLOAT MATH
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  // cmp_flt
  // mulss
  // divss
  // subss
  // addss
  // float to int
  // int to float

  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  //   UTILITIES
  //;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
};
}  // namespace emitter

#endif  // JAK_IGEN_H
