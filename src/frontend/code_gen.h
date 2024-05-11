#pragma once

#include <map>
#include <iostream>

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Function.h"


#include "frontend/ast.h"

namespace l24 {

class CodeGenContext {
public:
    std::unique_ptr<llvm::LLVMContext> _context;
    std::unique_ptr<llvm::Module> _module;
    std::unique_ptr<llvm::IRBuilder<>> _builder;
    std::map<std::string, llvm::Value *> _named_values;

    CodeGenContext() {
        _context = std::make_unique<llvm::LLVMContext>();
        _module = std::make_unique<llvm::Module>("l24_module", *_context);
        _builder = std::make_unique<llvm::IRBuilder<>>(*_context);
    }

    static void LogError(const char *str) {
        std::cerr << "Error: " << str << std::endl;
    }
};


class CodeGen {
public:
    virtual llvm::Value *codeGenProgram(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenFunc(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenIdent(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenBlock(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenStmt(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenUnaryExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenPrimaryExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenNumber(std::shared_ptr<ASTNode> node) = 0;
};

class CodeGenBase : public CodeGen {
private:
    CodeGenContext _ctx;
public:
    llvm::Value *codeGenExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenUnaryExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenPrimaryExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenProgram(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenFunc(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenIdent(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenBlock(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenStmt(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenNumber(std::shared_ptr<ASTNode> node) override;
};


} //namespace l24