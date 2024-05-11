#include <map>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"


#include "frontend/ast.h"
#include "frontend/code_gen.h"

namespace l24 {
static std::unique_ptr<llvm::LLVMContext> TheContext;
static std::unique_ptr<llvm::Module> TheModule;
static std::unique_ptr<llvm::IRBuilder<>> Builder;
static std::map<std::string, llvm::Value *> NamedValues;

static void Init() {
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("test", *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
}

void LogError(const char *str) {
    std::cerr << "Error: " <<  str << std::endl;
}

llvm::Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}

llvm::Value *ProgNode::codeGen() {
    Init();
    this->_func->codeGen();
    TheModule->print(llvm::errs(), nullptr);
    return nullptr;
}

llvm::Value *FuncNode::codeGen() {
    llvm::FunctionType *ft = llvm::FunctionType::get(llvm::Type::getInt64Ty(*TheContext), false);
    llvm::Function *func = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,  this->_ident, *TheModule);

    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*TheContext, "entry", func);
    Builder->SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    NamedValues.clear();

    if (llvm::Value *RetVal = this->_block->codeGen()) {
        // Finish off the function.
        Builder->CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        llvm::verifyFunction(*func);
        return func;
    }
    func->eraseFromParent();
    return nullptr;
}


llvm::Value *IdentNode::codeGen() {
    llvm::Value *val = NamedValues[this->_ident];
    if (val == nullptr) {
        LogErrorV("Unknown variable name");
        return nullptr; 
    }
    return val;
}

llvm::Value *BlockNode::codeGen() {
    return this->_stmt->codeGen();
}

llvm::Value *StmtNode::codeGen() {
    return this->_number->codeGen();
}

llvm::Value *NumberNode::codeGen() {
    return llvm::ConstantInt::get(*TheContext, llvm::APInt(64, this->_int_literal));
}


}  // namespace l24
