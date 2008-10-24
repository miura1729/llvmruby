#include "llvmruby.h"

extern "C" {
VALUE
llvm_user_wrap(User *u) {
  return Data_Wrap_Struct(cLLVMUser, NULL, NULL, u);
}

//llvm_user_get_operand_list
VALUE
llvm_user_get_operand_list(VALUE self) {
  User *usr = LLVM_USER(self);
  VALUE op_array = rb_ary_new();
  User::op_iterator op = usr->op_begin();
  while(op != usr->op_end()) {
    Use *u = op++;
    rb_ary_push(op_array, llvm_use_wrap(op));
  }
  return op_array;
}


//llvm_user_set_operand
VALUE
llvm_user_set_operand(VALUE self, VALUE i, VALUE val) {
  User *u = LLVM_USER(self);
  Value *v;
  Data_Get_Struct(val, Value, v);
  u->setOperand(FIX2UINT(i),v);
  return Qtrue;
}


//llvm_user_get_operand
VALUE
llvm_user_get_operand(VALUE self, VALUE i) {
  User *u = LLVM_USER(self);
  return llvm_value_wrap(u->getOperand(FIX2UINT(i)));
}


//llvm_user_get_num_operands
VALUE
llvm_user_get_num_operands(VALUE self) {
  User *u = LLVM_USER(self);
  return INT2NUM(u->getNumOperands());
}


//llvm_user_drop_all_references
VALUE
llvm_user_drop_all_references(VALUE self) {
  User *u = LLVM_USER(self);
  u->dropAllReferences();
  return Qtrue;
}


//llvm_user_replace_uses_of_with
VALUE
llvm_user_replace_uses_of_with(VALUE self, VALUE from, VALUE to) {
  User *u = LLVM_USER(self);
  Value *v_from, *v_to;
  Data_Get_Struct(from, Value, v_from);
  Data_Get_Struct(to, Value, v_to);
  u->replaceUsesOfWith(v_from,v_to);
  return Qtrue;
}


}
