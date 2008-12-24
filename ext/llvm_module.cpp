#include "llvmruby.h"
#include "llvm/Assembly/Parser.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/System/DynamicLibrary.h"
#include <fstream>
#include <sstream>

extern "C" {

VALUE
llvm_module_allocate(VALUE klass) {
  return Data_Wrap_Struct(klass, NULL, NULL, NULL);
}

VALUE
llvm_module_initialize(VALUE self, VALUE rname) {
  Check_Type(rname, T_STRING);
  DATA_PTR(self) = new Module(StringValuePtr(rname));
  return self;
}

VALUE
llvm_module_get_or_insert_function(VALUE self, VALUE name, VALUE rtype) {
  Check_Type(name, T_STRING);
  CHECK_TYPE(rtype, cLLVMFunctionType);

  Module *m = LLVM_MODULE(self);
  FunctionType *type = LLVM_FUNC_TYPE(rtype);
  Constant *fn = m->getOrInsertFunction(StringValuePtr(name), type);

#if defined(USE_ASSERT_CHECK)
  if (isa<Function>(fn) == 0) {
    rb_raise(rb_eRuntimeError, 
	     "cast<Function>(fn) argument of incompatible type !");
  }
#endif

  Function *f = cast<Function>(fn);
  return llvm_function_wrap(f); 
}

VALUE
llvm_module_get_function(VALUE self, VALUE name) {
  Check_Type(name, T_STRING);
  Module *m = LLVM_MODULE(self);
  Function *f = m->getFunction(StringValuePtr(name));
  return llvm_function_wrap(f);  
}

VALUE
llvm_module_global_constant(VALUE self, VALUE rtype, VALUE rinitializer) {
  Module *m = LLVM_MODULE(self);
  Type *type = LLVM_TYPE(rtype);
  Constant *initializer = (Constant*)DATA_PTR(rinitializer);
  GlobalVariable *gv = new GlobalVariable(type, true, GlobalValue::InternalLinkage, initializer, "", m);
  return llvm_value_wrap(gv);
}

VALUE
llvm_module_global_variable(VALUE self, VALUE rtype, VALUE rinitializer) {
  Module *m = LLVM_MODULE(self);
  Type *type = LLVM_TYPE(rtype);
  Constant *initializer = (Constant*)DATA_PTR(rinitializer);
  GlobalVariable *gv = new GlobalVariable(type, false, GlobalValue::InternalLinkage, initializer, "", m);
  return llvm_value_wrap(gv);
}


VALUE
llvm_module_inspect(VALUE self) {
  Module *m = LLVM_MODULE(self);
  std::ostringstream strstrm;
  strstrm << *m;
  return rb_str_new2(strstrm.str().c_str());
}

VALUE
llvm_pass_manager_allocate(VALUE klass) {
  return Data_Wrap_Struct(klass, NULL, NULL, NULL);
}

VALUE
llvm_pass_manager_initialize(VALUE self) {
  PassManager *pm = new PassManager;
  DATA_PTR(self) = pm;
  return self;
}

VALUE
llvm_pass_manager_run(VALUE self, VALUE module) {
  PassManager *pm = (PassManager*) DATA_PTR(self);
  Module *m = LLVM_MODULE(module);
  
  pm->add(new TargetData(m));
  pm->add(createVerifierPass());
  pm->add(createLowerSetJmpPass());
  pm->add(createRaiseAllocationsPass());
  pm->add(createCFGSimplificationPass());
  pm->add(createPromoteMemoryToRegisterPass());
  pm->add(createGlobalOptimizerPass());
  pm->add(createGlobalDCEPass());
  pm->add(createFunctionInliningPass());
  
  pm->run(*m);
  return Qtrue;
}

static ExecutionEngine *EE = NULL;

VALUE
llvm_execution_engine_get(VALUE klass, VALUE module) {
  CHECK_TYPE(module, cLLVMModule);

#if defined(__CYGWIN__)

  // Load dll Modules for ruby
  sys::DynamicLibrary::LoadLibraryPermanently("cygwin1.dll");
  sys::DynamicLibrary::LoadLibraryPermanently("cygruby190.dll");

#endif

  Module *m = LLVM_MODULE(module);
  ExistingModuleProvider *MP = new ExistingModuleProvider(m);

  if(EE == NULL) {
    EE = ExecutionEngine::create(MP, false);
  } else {
    EE->addModuleProvider(MP);
  }

  return Qtrue;
}

VALUE
llvm_module_external_function(VALUE self, VALUE name, VALUE type) {
  Check_Type(name, T_STRING);
  CHECK_TYPE(type, cLLVMFunctionType);

  Module *module = LLVM_MODULE(self);
  Function *f = Function::Create(
    LLVM_FUNC_TYPE(type), 
    Function::ExternalLinkage, 
    StringValuePtr(name),
    module
  );
  return Data_Wrap_Struct(cLLVMFunction, NULL, NULL, f);
}

VALUE
llvm_module_read_assembly(VALUE self, VALUE assembly) {
  Check_Type(assembly, T_STRING);

  ParseError e;
  Module *module = ParseAssemblyString(
    StringValuePtr(assembly),
    LLVM_MODULE(self),
    &e
  );
  //TODO How do we handle errors?
  return Data_Wrap_Struct(cLLVMModule, NULL, NULL, module);
}

VALUE
llvm_module_read_bitcode(VALUE self, VALUE bitcode) {
  Check_Type(bitcode, T_STRING);

#if defined(RSTRING_PTR)
  MemoryBuffer *buf = MemoryBuffer::getMemBufferCopy(RSTRING_PTR(bitcode),RSTRING_PTR(bitcode)+RSTRING_LEN(bitcode));  
#else
  MemoryBuffer *buf = MemoryBuffer::getMemBufferCopy(RSTRING(bitcode)->ptr,RSTRING(bitcode)->ptr+RSTRING(bitcode)->len);
#endif

  Module *module = ParseBitcodeFile(buf);
  delete buf;
  return Data_Wrap_Struct(cLLVMModule, NULL, NULL, module);
}


VALUE
llvm_module_write_bitcode(VALUE self, VALUE file_name) {
  Check_Type(file_name, T_STRING);

  // Don't really know how to handle c++ streams well, 
  // dumping all into string buffer and then saving
  std::ofstream file;
  file.open(StringValuePtr(file_name)); 
  WriteBitcodeToFile(LLVM_MODULE(self), file);   // Convert value into a string.
  return Qtrue;
}

VALUE
llvm_execution_engine_run_function(int argc, VALUE *argv, VALUE klass) {
  if(argc < 1) { rb_raise(rb_eArgError, "Expected at least one argument"); }
  CHECK_TYPE(argv[0], cLLVMFunction);
  Function *func = LLVM_FUNCTION(argv[0]);

  // Using run function is much slower than getting C function pointer
  // and calling that, but it lets us pass in arbitrary numbers of
  // arguments easily for now, which is nice
  std::vector<GenericValue> arg_values;
  for(int i = 1; i < argc; ++i) {
    GenericValue arg_val;
    arg_val.IntVal = APInt(sizeof(long)*8, argv[i]);
    arg_values.push_back(arg_val);
  }

  GenericValue v = EE->runFunction(func, arg_values);
  VALUE val = v.IntVal.getZExtValue();
  return val;
}

/* For tests: assume no args, return uncoverted int and turn it into fixnum */
VALUE llvm_execution_engine_run_autoconvert(VALUE klass, VALUE func) {
  std::vector<GenericValue> args;
  GenericValue v = EE->runFunction(LLVM_FUNCTION(func), args);
  VALUE val = INT2NUM(v.IntVal.getZExtValue());
  return val;
}
}
