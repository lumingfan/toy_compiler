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
        llvm::AllocaInst *_val;
        ConstVal(): _val(nullptr) {}
        ConstVal(llvm::AllocaInst *v): _val(v) {}
    };

    struct VarVal {
        llvm::AllocaInst *_val;
        VarVal(): _val(nullptr) {}
        VarVal(llvm::AllocaInst *v): _val(v) {}
    };

};
} // namespace l24
