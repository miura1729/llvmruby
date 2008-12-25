#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/ModuleProvider.h"
#include "llvm/PassManager.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/IRBuilder.h"
#include <iostream>
using namespace llvm;

#include "ruby.h"

extern VALUE cLLVMRuby;
extern VALUE cLLVMValue;
extern VALUE cLLVMUser;
extern VALUE cLLVMUse;
extern VALUE cLLVMModule;
extern VALUE cLLVMFunction;
extern VALUE cLLVMBasicBlock;
extern VALUE cLLVMBuilder;
extern VALUE cLLVMType;
extern VALUE cLLVMPointerType;
extern VALUE cLLVMStructType;
extern VALUE cLLVMArrayType;
extern VALUE cLLVMVectorType;
extern VALUE cLLVMFunctionType;
extern VALUE cLLVMInstruction;
extern VALUE cLLVMBinaryOperator;
extern VALUE cLLVMAllocationInst;

#define HANDLE_TERM_INST(Num, Opcode, Klass) extern VALUE cLLVM##Klass;
#define HANDLE_MEMORY_INST(Num, Opcode, Klass) extern VALUE cLLVM##Klass;
#define HANDLE_OTHER_INST(Num, Opcode, Klass) extern VALUE cLLVM##Klass;
#include "llvm/Instruction.def"

extern VALUE cLLVMBinaryOps;
extern VALUE cLLVMPhi;
extern VALUE cLLVMPassManager;

#define LLVM_VAL(obj) ((Value*)DATA_PTR(obj))
#define LLVM_USER(obj) ((User*)DATA_PTR(obj))
#define LLVM_USE(obj) ((Use*)DATA_PTR(obj))
#define LLVM_TYPE(obj) ((Type*)DATA_PTR(obj))
#define LLVM_FUNC_TYPE(obj) ((FunctionType*)DATA_PTR(obj))
#define LLVM_MODULE(obj) ((Module*)DATA_PTR(obj))
#define LLVM_FUNCTION(obj) ((Function*)DATA_PTR(obj))
#define LLVM_BASIC_BLOCK(obj) ((BasicBlock*)DATA_PTR(obj))
#define LLVM_INSTRUCTION(obj) ((Instruction*)DATA_PTR(obj))
#define LLVM_PHI(obj) ((PHINode*)DATA_PTR(obj))

#define CHECK_TYPE(val, klass)\
  if(!rb_obj_is_kind_of(val, klass)) {\
    rb_raise(rb_eTypeError, "wrong argument type: %s given, expected %s", rb_obj_classname(val), rb_class2name(klass));\
  }


#define USE_ASSERT_CHECK

extern "C" {
VALUE llvm_value_wrap(Value*);
VALUE llvm_user_wrap(User*);
VALUE llvm_use_wrap(Use*);
VALUE llvm_function_wrap(Function*);
VALUE llvm_basic_block_wrap(BasicBlock*);
VALUE llvm_function_create_block(VALUE);
VALUE llvm_instruction_wrap(Instruction*);
}
