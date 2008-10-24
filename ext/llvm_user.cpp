#include "llvmruby.h"

extern "C" {
VALUE 
llvm_user_wrap(Function *f) {
  return Data_Wrap_Struct(cLLVMFunction, NULL, NULL, f);
}

//llvm_user_get_operand_list
//llvm_user_set_operand
//llvm_user_get_operand
//llvm_user_get_num_operands
//llvm_user_drop_all_references
//llvm_user_replace_all_uses_of_with

}
