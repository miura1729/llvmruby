#include "llvmruby.h"

extern VALUE cLLVMBasicBlock;
extern VALUE cLLVMBuilder;

extern "C" {
VALUE 
llvm_basic_block_wrap(BasicBlock* bb) { 
  return Data_Wrap_Struct(cLLVMBasicBlock, NULL, NULL, bb); 
}

VALUE
llvm_basic_block_size(VALUE self) {
  BasicBlock *bb = LLVM_BASIC_BLOCK(self);
  return INT2NUM(bb->size());
}

VALUE
llvm_basic_block_get_instruction_list(VALUE self) {
  BasicBlock *bb = LLVM_BASIC_BLOCK(self);
  VALUE ins_array = rb_ary_new();
  BasicBlock::iterator ins = bb->begin();
  while(ins != bb->end()) {
    Instruction *i = ins++;
    rb_ary_push(ins_array, llvm_instruction_wrap(i));
  }
  return ins_array;
}

VALUE 
llvm_basic_block_builder(VALUE self) {
  BasicBlock* bb;
  Data_Get_Struct(self, BasicBlock, bb);
  IRBuilder<> *builder = new IRBuilder<>(bb);
  return Data_Wrap_Struct(cLLVMBuilder, NULL, NULL, builder);
}

#define DATA_GET_BUILDER IRBuilder<> *builder; Data_Get_Struct(self, IRBuilder<>, builder);
#define DATA_GET_BLOCK   BasicBlock *bb; CHECK_TYPE(rbb, cLLVMBasicBlock); Data_Get_Struct(rbb, BasicBlock, bb);

VALUE
llvm_builder_set_insert_point(VALUE self, VALUE rbb) {
  DATA_GET_BUILDER
  DATA_GET_BLOCK 
  builder->SetInsertPoint(bb);
  return self;
}

VALUE 
llvm_builder_bin_op(VALUE self, VALUE rbin_op, VALUE rv1, VALUE rv2) {
  Check_Type(rbin_op, T_FIXNUM);
  DATA_GET_BUILDER

  Instruction::BinaryOps bin_op = (Instruction::BinaryOps)FIX2INT(rbin_op);
  
  Value *v1, *v2; 
  Data_Get_Struct(rv1, Value, v1);
  Data_Get_Struct(rv2, Value, v2);
  Value *res = builder->CreateBinOp(bin_op, v1, v2);
  return Data_Wrap_Struct(cLLVMBinaryOperator, NULL, NULL, res);
}

VALUE
llvm_builder_phi(VALUE self, VALUE type) {
  CHECK_TYPE(type, cLLVMType);
  DATA_GET_BUILDER
  PHINode *v = builder->CreatePHI(LLVM_TYPE(type)); 
  return Data_Wrap_Struct(cLLVMPhi, NULL, NULL, v);
}

VALUE
llvm_phi_add_incoming(VALUE self, VALUE val, VALUE rbb) {
  CHECK_TYPE(val, cLLVMValue);

  DATA_GET_BLOCK
  PHINode *phi = LLVM_PHI(self);

  phi->addIncoming(LLVM_VAL(val), bb);
  return self;
}

VALUE 
llvm_builder_return(VALUE self, VALUE rv) {
  CHECK_TYPE(rv, cLLVMValue);
  DATA_GET_BUILDER
  return Data_Wrap_Struct(cLLVMReturnInst, NULL, NULL, builder->CreateRet(LLVM_VAL(rv)));
}

VALUE 
llvm_builder_br(VALUE self, VALUE rblock) {
  DATA_GET_BUILDER

  BasicBlock *bb;
  Data_Get_Struct(rblock, BasicBlock, bb);

  Value *branch_instr = builder->CreateBr(bb);
  return Data_Wrap_Struct(cLLVMBranchInst, NULL, NULL, branch_instr); 
}  

VALUE 
llvm_builder_cond_br(VALUE self, VALUE rcond, VALUE rtrue_block, VALUE rfalse_block) {
  DATA_GET_BUILDER

  Value *cond;
  Data_Get_Struct(rcond, Value, cond);

  BasicBlock *true_block, *false_block;
  Data_Get_Struct(rtrue_block, BasicBlock, true_block);
  Data_Get_Struct(rfalse_block, BasicBlock, false_block);
#if defined(USE_ASSERT_CHECK)
  if (cond->getType() != Type::Int1Ty) {
    rb_raise(rb_eRuntimeError, "May only branch on boolean predicates!");
  }
#endif

  Value *branch_instr = builder->CreateCondBr(cond, true_block, false_block);
  return Data_Wrap_Struct(cLLVMBranchInst, NULL, NULL, branch_instr);
}

VALUE
llvm_builder_switch(VALUE self, VALUE rv, VALUE rdefault) {
  DATA_GET_BUILDER

  BasicBlock *deflt;
  Data_Get_Struct(rdefault, BasicBlock, deflt);

  Value *v;
  Data_Get_Struct(rv, Value, v);

  Instruction *switch_instr = builder->CreateSwitch(v, deflt);
  return llvm_instruction_wrap(switch_instr);
}

VALUE
llvm_builder_invoke(int argc, VALUE *argv, VALUE self) {
  DATA_GET_BUILDER
  if(argc < 3) { rb_raise(rb_eArgError, "Expected at least three argument"); }
  int num_args = argc - 3;

  Value *callee = LLVM_VAL(argv[0]);
  BasicBlock *ndest;
  BasicBlock *udest;
  std::vector<Value *> vecarg(num_args);

  Data_Get_Struct(argv[1], BasicBlock, ndest);
  Data_Get_Struct(argv[2], BasicBlock, udest);
  for (int i = 0; i < num_args; i++) {
    vecarg[i] = LLVM_VAL(argv[i + 3]);
  }
  
  return llvm_value_wrap(builder->CreateInvoke(callee, ndest, udest, vecarg.begin(), vecarg.end()));
}

VALUE
llvm_builder_unwind(VALUE self) {
  DATA_GET_BUILDER
  return llvm_value_wrap(builder->CreateUnwind());
}

VALUE
llvm_builder_malloc(VALUE self, VALUE rtype, VALUE rsize) {
  DATA_GET_BUILDER

  const Type *type;
  Data_Get_Struct(rtype, Type, type);

  Value *size = ConstantInt::get(Type::Int32Ty, FIX2INT(rsize));
  Instruction *v = builder->CreateMalloc(type, size);
  return llvm_instruction_wrap(v);
}

VALUE
llvm_builder_free(VALUE self, VALUE rptr) {
   DATA_GET_BUILDER
   Value *v = LLVM_VAL(rptr);
   Instruction *free_inst = builder->CreateFree(v);
   return llvm_instruction_wrap(free_inst);
}
  
VALUE 
llvm_builder_alloca(VALUE self, VALUE rtype, VALUE rsize) {
  DATA_GET_BUILDER

  const Type* type;
  Data_Get_Struct(rtype, Type, type);

  Value *size = ConstantInt::get(Type::Int32Ty, FIX2INT(rsize));
  Instruction *v = builder->CreateAlloca(type, size);
  return Data_Wrap_Struct(cLLVMAllocationInst, NULL, NULL, v);
}

VALUE
llvm_builder_load(int argc, VALUE *argv, VALUE self) {
  DATA_GET_BUILDER
  VALUE rptr;
  VALUE isVolatile;
  Value *ptr;

  rb_scan_args(argc, argv, "11", &rptr, &isVolatile);
  Data_Get_Struct(rptr, Value, ptr);
  return Data_Wrap_Struct(cLLVMLoadInst, NULL, NULL, builder->CreateLoad(ptr, RTEST(isVolatile)));
}

VALUE
llvm_builder_store(int argc, VALUE *argv, VALUE self) {
  DATA_GET_BUILDER

  Value *v, *ptr;
  VALUE rv;
  VALUE rptr;
  VALUE isVolatile;

  rb_scan_args(argc, argv, "21", &rv, &rptr, &isVolatile);
  Data_Get_Struct(rv, Value, v);
  Data_Get_Struct(rptr, Value, ptr);
  return Data_Wrap_Struct(cLLVMStoreInst, NULL, NULL, builder->CreateStore(v, ptr, RTEST(isVolatile)));
}

VALUE
llvm_builder_icmp(VALUE self, VALUE pred, VALUE lhs, VALUE rhs) {
  DATA_GET_BUILDER

  CmpInst::Predicate p = (CmpInst::Predicate)FIX2INT(pred);
  Value *v = builder->CreateICmp(p, LLVM_VAL(lhs), LLVM_VAL(rhs));
  return Data_Wrap_Struct(cLLVMICmpInst, NULL, NULL, v);
}

VALUE
llvm_builder_fcmp(VALUE self, VALUE pred, VALUE lhs, VALUE rhs) {
  DATA_GET_BUILDER

  CmpInst::Predicate p = (CmpInst::Predicate)FIX2INT(pred);
  Value *v = builder->CreateFCmp(p, LLVM_VAL(lhs), LLVM_VAL(rhs));
  return Data_Wrap_Struct(cLLVMFCmpInst, NULL, NULL, v);
}

VALUE
llvm_builder_gep(VALUE self, VALUE rptr, VALUE ridx) {
  DATA_GET_BUILDER

  Value *ptr, *idx;
  Data_Get_Struct(rptr, Value, ptr);
  Data_Get_Struct(ridx, Value, idx);
  return llvm_value_wrap(builder->CreateGEP(ptr, idx));
}

VALUE
llvm_builder_struct_gep(VALUE self, VALUE rptr, VALUE ridx) {
  DATA_GET_BUILDER 

  Value *ptr; 
  Data_Get_Struct(rptr, Value, ptr);
  return llvm_value_wrap(builder->CreateStructGEP(ptr, FIX2INT(ridx)));
}

VALUE
llvm_builder_cast(VALUE self, VALUE rop, VALUE rv, VALUE rdest_ty) {
   DATA_GET_BUILDER

   Instruction::CastOps op = (Instruction::CastOps)FIX2INT(rop); 

   Value *v;
   Data_Get_Struct(rv, Value, v);

   Type *dest_ty;
   Data_Get_Struct(rdest_ty, Type, dest_ty);

   return llvm_value_wrap(builder->CreateCast(op, v, dest_ty));
}

VALUE llvm_builder_int_cast(VALUE self, VALUE i, VALUE type, VALUE sign) {
  DATA_GET_BUILDER
  bool isSigned = (sign != Qnil && sign != Qfalse);
  return llvm_value_wrap(builder->CreateIntCast(LLVM_VAL(i), LLVM_TYPE(type), isSigned));
}

VALUE
llvm_builder_call(int argc, VALUE* argv, VALUE self) {
  DATA_GET_BUILDER

  Function *callee = LLVM_FUNCTION(argv[0]);
  int num_args = argc-1;
  Value** args = (Value**)alloca(num_args*sizeof(Value*));

#if defined(USE_ASSERT_CHECK)
  const FunctionType *FTy =
    cast<FunctionType>(cast<PointerType>(callee->getType())->getElementType());
  char message[255];
  if (!((unsigned)num_args == FTy->getNumParams() ||
	(FTy->isVarArg() && (unsigned) num_args > FTy->getNumParams()))) {
    snprintf(message, 255, 
	     "Calling a function with bad signature number of argument %d expect %d", 
	     num_args, FTy->getNumParams());
    rb_raise(rb_eRuntimeError, message);
  }
#endif

  for(int i = 0; i < num_args; ++i) {
    args[i] = LLVM_VAL(argv[i+1]); 

#if defined(USE_ASSERT_CHECK)
    if (FTy->getParamType(i) != args[i]->getType()) {
      snprintf(message, 255, 
	       "Calling a function with a bad signature in %d argument", 
	       i);
      rb_raise(rb_eRuntimeError, message);
    }
#endif
  }
  return llvm_value_wrap(builder->CreateCall(callee, args, args+num_args));
}

VALUE
llvm_builder_insert_element(VALUE self, VALUE rv, VALUE rnv, VALUE ridx) {
  DATA_GET_BUILDER

  Value *v, *nv, *idx;
  Data_Get_Struct(rv, Value, v); 
  Data_Get_Struct(rnv, Value, nv);
  Data_Get_Struct(ridx, Value, idx);

  return llvm_value_wrap(builder->CreateInsertElement(v, nv, idx));
}

VALUE
llvm_builder_extract_element(VALUE self, VALUE rv, VALUE ridx) {
  DATA_GET_BUILDER

  Value *v, *idx;
  Data_Get_Struct(rv, Value, v);
  Data_Get_Struct(ridx, Value, idx);

  return llvm_value_wrap(builder->CreateExtractElement(v, idx));
}

VALUE
llvm_builder_get_global(VALUE self) {
  GlobalVariable *g = new GlobalVariable(Type::Int64Ty, false, GlobalValue::ExternalLinkage, 0, "shakalaka");
  return llvm_value_wrap(g);
}

VALUE
llvm_builder_create_global_string_ptr(VALUE self, VALUE str) {
  DATA_GET_BUILDER
  Value *v = builder->CreateGlobalStringPtr(StringValuePtr(str));
  return llvm_value_wrap(v);
}
}
