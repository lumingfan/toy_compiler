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

    llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *func, const std::string &var_name, llvm::Value *array_size = nullptr, bool is_ptr = false) {
        llvm::IRBuilder<> TmpB(&func->getEntryBlock(),
                               func->getEntryBlock().begin());
        llvm::Type *ty;
        if (is_ptr) {
            ty = llvm::PointerType::get(llvm::IntegerType::get(*(this->_context), 64), 0);
        } else {
            ty = llvm::Type::getInt64Ty(*(this->_context));
        }
        if (array_size == nullptr) {
            return TmpB.CreateAlloca(ty, nullptr, var_name);
        }

        auto size = llvm::dyn_cast<llvm::ConstantInt>(array_size)->getSExtValue();
        return TmpB.CreateAlloca(llvm::ArrayType::get(ty, size), array_size,var_name);
    }

    llvm::AllocaInst *createDefineValueInst(std::vector<llvm::Value *>vals, const std::string &ident, llvm::Value *array_size = nullptr, bool is_ptr = false) {
        llvm::AllocaInst *alloca = this->CreateEntryBlockAlloca((this->_builder)->GetInsertBlock()->getParent(), ident, array_size, is_ptr);
        // scalar
        if (array_size == nullptr) {
            this->_builder->CreateStore(vals[0], alloca);
            return alloca;
        }

        auto ty = llvm::Type::getInt64Ty(*(this->_context));
        // we don't support struct, so the first value of indexList always be 0
        auto first_val = llvm::ConstantInt::get(ty, 0);

        auto alloca_ty = llvm::ArrayType::get(ty, llvm::dyn_cast<llvm::ConstantInt>(alloca->getArraySize())->getSExtValue());

        int64_t idx = 0;
        for (llvm::Value *val : vals) {
            llvm::Value* indexList[2] = {first_val, llvm::ConstantInt::get(ty, idx)};
            auto ptr = this->_builder->CreateGEP(alloca_ty, alloca, indexList);
            this->_builder->CreateStore(val, ptr);
            ++idx;
        }
        return alloca;
    }

    void createSetValueInst(llvm::AllocaInst *alloca, llvm::Value *val, llvm::Value *sub_idx) const {
        // set a scalar value
        if (sub_idx == nullptr) {
            this->_builder->CreateStore(val, alloca);
            return ;
        }

        // get value from an array
        auto ty = alloca->getAllocatedType();
        auto alloca_ty = llvm::ArrayType::get(ty, llvm::dyn_cast<llvm::ConstantInt>(alloca->getArraySize())->getSExtValue());

        // we don't support struct, so the first value of indexList always be 0
        llvm::Value* indexList[2] = {llvm::ConstantInt::get(ty, 0), sub_idx};
        auto ptr = this->_builder->CreateGEP(alloca_ty, alloca, indexList);
        this->_builder->CreateStore(val, ptr);
    }

    llvm::Value *createGetValueInst(llvm::AllocaInst *alloca, const std::string &ident, llvm::Value *sub_idx) const {
        // get a scalar value
        if (sub_idx == nullptr) {
            return this->_builder->CreateLoad(alloca->getAllocatedType(), alloca, ident.c_str());
        }

        // get value from an array
        auto ty = alloca->getAllocatedType();
        auto alloca_ty = llvm::ArrayType::get(ty, llvm::dyn_cast<llvm::ConstantInt>(alloca->getArraySize())->getSExtValue());
        // we don't support struct, so the first value of indexList always be 0
        llvm::Value* indexList[2] = {llvm::ConstantInt::get(ty, 0), sub_idx};

        auto ptr = this->_builder->CreateGEP(alloca_ty, alloca, indexList);
        return this->_builder->CreateLoad(ty, ptr, ident.c_str());
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
    void defineValue(const std::string &ident, L24Type::ValType ty, std::vector<llvm::Value*> vals, llvm::Value *array_size = nullptr);
    void setValue(const std::string &ident, L24Type::ValType ty, llvm::Value *val, llvm::Value *sub_idx = nullptr);
    llvm::Value *getValue(const std::string &ident, L24Type::ValType ty, llvm::Value *sub_idx = nullptr);
    bool inCurrentLayer(const std::string &ident);
    void defineGlobalValue(const std::string &ident, llvm::Type *ty, std::vector<llvm::Value*> vals, llvm::Value *array_size = nullptr);
    void setGlobalValue(const std::string &ident, llvm::Value* val, llvm::Value *sub_idx = nullptr);
    llvm::Value *getGlobalValue(const std::string &ident, llvm::Value *sub_idx = nullptr);
};

} // namespace l24