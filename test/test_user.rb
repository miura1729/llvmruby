require 'test/unit'
require 'llvm'

include LLVM

class UserTests < Test::Unit::TestCase

  def setup
    @assembly_byteswap=<<-EOF
    ; ModuleID = 'byteswap.bc'
    target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:128:128"
    target triple = "i386-apple-darwin9"

    define i32 @bswap(i32 %x) nounwind {
    entry:
    	%tmp3 = shl i32 %x, 24		; <i32> [#uses=1]
    	%tmp5 = shl i32 %x, 8		; <i32> [#uses=1]
    	%tmp6 = and i32 %tmp5, 16711680		; <i32> [#uses=1]
    	%tmp9 = lshr i32 %x, 8		; <i32> [#uses=1]
    	%tmp1018 = and i32 %tmp9, 65280		; <i32> [#uses=1]
    	%tmp7 = or i32 %tmp1018, %tmp3		; <i32> [#uses=1]
    	%tmp11 = or i32 %tmp7, %tmp6		; <i32> [#uses=1]
    	ret i32 %tmp11
    }
    EOF

    m = LLVM::Module.read_assembly(@assembly_byteswap)
    bswap = m.get_function("bswap")
    bbs = bswap.get_basic_block_list
    @ins = bbs[0].get_instruction_list
  end

  def test_get_operand_list
    olist = @ins[0].get_operand_list
    assert_equal(2, olist.size)
  end

  def test_number_of_operands
    assert_equal(2, @ins[0].get_num_operands)  # shl instruction
    assert_equal(1, @ins[7].get_num_operands)  # ret instruction
  end

  # TODO: find a way for testing this function
  def test_get_operand
    op0 = @ins[0].get_operand(0)
    op1 = @ins[1].get_operand(1)
    assert_equal("x",op0.name)
  end

  def test_set_operand
    ins_before = @ins[0].inspect
    assert_match(/\%tmp3 = shl i32 \%x, 24/, ins_before)
    @ins[0].set_operand(0,42.llvm)
    ins_after = @ins[0].inspect
    assert_match(/\%tmp3 = shl i32 42, 24/, ins_after)
  end

  # TODO find a way for testing this function
  def test_drop_all_references
    @ins[0].drop_all_references()
  end

  def test_replace_uses_of_with
    ins_before = @ins[0].inspect
    assert_match(/\%tmp3 = shl i32 \%x, 24/, ins_before)
    @ins[0].replace_uses_of_with(24.llvm,1234.llvm)
    ins_after = @ins[0].inspect
    assert_match(/\%tmp3 = shl i32 \%x, 1234/, ins_after)
  end

end

