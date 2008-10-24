#include "llvmruby.h"

extern "C" {
VALUE
llvm_use_wrap(Use *u) {
  return Data_Wrap_Struct(cLLVMUse, NULL, NULL, u);
}

}
