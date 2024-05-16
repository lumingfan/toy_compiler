#pragma once

#include <map>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "frontend/type.h"

namespace l24 {
class CodeGenContext {
private:
    std::map<std::string, std::variant<L24Type::ConstVal, L24Type::VarVal>> &getNamedValues(int layer) {
        return _nested_named_values[layer];
    }

    int getCurrentLayer() const {
        return static_cast<int>(_nested_named_values.size()) - 1;
    }

    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *func, const std::string &var_name) {
        llvm::IRBuilder<> TmpB(&func->getEntryBlock(),
                               func->getEntryBlock().begin());
        return TmpB.CreateAlloca(llvm::Type::getInt64Ty(*(this->_context)), nullptr, var_name);
    }

    llvm::AllocaInst *createDefineValueInst(llvm::Value *val, const std::string &ident) {
        llvm::AllocaInst *alloca = this->CreateEntryBlockAlloca((this->_builder)->GetInsertBlock()->getParent(), ident);
        this->_builder->CreateStore(val, alloca);
        return alloca;
    }

    void createSetValueInst(llvm::AllocaInst *alloca, llvm::Value *val) const {
        this->_builder->CreateStore(val, alloca);
    }

    llvm::Value *createGetValueInst(llvm::AllocaInst *alloca, const std::string &ident) const {
        return this->_builder->CreateLoad(alloca->getAllocatedType(), alloca, ident.c_str());
    }


public:
    std::unique_ptr<llvm::LLVMContext> _context;
    std::unique_ptr<llvm::Module> _module;
    std::unique_ptr<llvm::IRBuilder<>> _builder;

    // used by find var/const
    std::vector<std::map<std::string, std::variant<L24Type::ConstVal, L24Type::VarVal>>> _nested_named_values;

    // used by continue/break to generate unconditional branch instruction
    // first is loop block (continue)
    // second is after loop block (break)
    std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> _nested_blocks;


    CodeGenContext();
    static void LogError(const std::string &str);
    void codeGenStandardLibrary();
    void pushNamedValuesLayer();
    void popNamedValuesLayer();
    void defineValue(const std::string &ident, L24Type::ValType ty, llvm::Value *val);
    void setValue(const std::string &ident, L24Type::ValType ty, llvm::Value *val);
    llvm::Value *getValue(const std::string &ident, L24Type::ValType ty);
    bool inCurrentLayer(const std::string &ident);
    void defineGlobalValue(const std::string &ident, llvm::Type *ty, llvm::Value *val);
    void setGlobalValue(const std::string &ident, llvm::Type *ty, llvm::Value *val);
    llvm::Value *getGlobalValue(const std::string &ident, llvm::Type *ty);
};

} // namespace l24