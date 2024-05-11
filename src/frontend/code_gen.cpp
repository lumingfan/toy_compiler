#include "frontend/code_gen.h"

namespace l24 {

llvm::Value *CodeGenBase::codeGenProgram(std::shared_ptr<ASTNode> node) {
    auto prog_node = std::dynamic_pointer_cast<ProgNode>(node);
    this->codeGenFunc(prog_node->_func);
    this->_ctx._module->print(llvm::errs(), nullptr);
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenFunc(std::shared_ptr<ASTNode> node) {
    auto func_node = std::dynamic_pointer_cast<FuncNode>(node);
    llvm::FunctionType *ft = llvm::FunctionType::get(llvm::Type::getInt64Ty(*(this->_ctx._context)), false);
    llvm::Function *func = llvm::Function::Create(ft, llvm::Function::ExternalLinkage,  func_node->_ident, *(this->_ctx._module));

    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*(this->_ctx._context), "entry", func);
    (this->_ctx._builder)->SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    (this->_ctx._named_values).clear();

    if (llvm::Value *RetVal = this->codeGenBlock(func_node->_block)) {
        // Finish off the function.
        (this->_ctx._builder)->CreateRet(RetVal);

        // Validate the generated code, checking for consistency.
        llvm::verifyFunction(*func);
        return func;
    }
    func->eraseFromParent();
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenIdent(std::shared_ptr<ASTNode> node) {
    auto ident_node = std::dynamic_pointer_cast<IdentNode>(node);
    llvm::Value *val = this->_ctx._named_values[ident_node->_ident];
    if (val == nullptr) {
        CodeGenContext::LogError("Unknown variable name");
        return nullptr;
    }
    return val;
}
llvm::Value *CodeGenBase::codeGenBlock(std::shared_ptr<ASTNode> node) {
    auto block_node = std::dynamic_pointer_cast<BlockNode>(node);
    return this->codeGenStmt(block_node->_stmt);
}
llvm::Value *CodeGenBase::codeGenStmt(std::shared_ptr<ASTNode> node) {
    auto stmt_node = std::dynamic_pointer_cast<StmtNode>(node);
    return this->codeGenNumber(stmt_node->_number);
}
llvm::Value *CodeGenBase::codeGenNumber(std::shared_ptr<ASTNode> node) {
    auto number_node = std::dynamic_pointer_cast<NumberNode>(node);
    return llvm::ConstantInt::get(*(this->_ctx._context), llvm::APInt(64, number_node->_int_literal));
}
} //namespace l24
