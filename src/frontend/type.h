#pragma once

#include "llvm/IR/Value.h"

namespace l24 {

class L24Type {
public:
    enum ValType {
        CONST,
        VAR,
        ANY
    };

    struct ConstVal {
        llvm::Value *_val;
        ConstVal(): _val(nullptr) {}
        ConstVal(llvm::Value *v): _val(v) {}
    };

    struct VarVal {
        llvm::Value *_val;
        VarVal(): _val(nullptr) {}
        VarVal(llvm::Value *v): _val(v) {}
    };

};
} // namespace l24
