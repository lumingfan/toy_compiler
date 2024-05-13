#pragma once

#include <map>
#include <iostream>
#include <variant>

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"

#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"


#include "frontend/ast.h"
#include "frontend/code_gen_ctx.h"

namespace l24 {

class CodeGen {
public:
    virtual llvm::Value *codeGenProgram(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenFunc(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenBlock(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenStmt(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenAddExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenMulExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenUnaryExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenPrimaryExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenNumber(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenLorExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenLandExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenEqExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenRelExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenBlockItem(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenDecl(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenConstDecl(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenVarDecl(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenConstDef(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenVarDef(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenConstInitVal(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenInitVal(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenConstExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenLVal(std::shared_ptr<ASTNode> node) = 0;
};

class CodeGenBase : public CodeGen {
private:
    CodeGenContext _ctx;
    llvm::Value *intToBoolean(llvm::Value *val) const {
        llvm::Value *zero = llvm::ConstantInt::get(*(this->_ctx._context), llvm::APInt(64, 0, false));
        return (this->_ctx._builder)->CreateICmpNE(zero, val);
    }
    llvm::Value *booleanToInt(llvm::Value *val) const {
        return (this->_ctx._builder)->CreateIntCast(val, llvm::Type::getInt64Ty(*(this->_ctx._context)), false);
    }

    llvm::Value *getInitInt() const {
        return llvm::ConstantInt::get(*(this->_ctx._context), llvm::APInt(64, 0, false));
    }

public:
    void asmGen() const;

    llvm::Value *codeGenExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenLorExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenLandExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenEqExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenRelExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenUnaryExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenAddExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenBlockItem(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenDecl(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenConstDecl(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenConstDef(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenConstInitVal(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenConstExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenLVal(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenMulExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenPrimaryExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenProgram(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenFunc(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenBlock(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenStmt(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenNumber(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenVarDecl(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenVarDef(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenInitVal(std::shared_ptr<ASTNode> node) override;

};


} //namespace l24