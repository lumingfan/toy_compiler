#include "frontend/code_gen.h"

namespace l24 {

llvm::Value *CodeGenBase::codeGenProgram(std::shared_ptr<ASTNode> node) {
    auto prog_node = std::dynamic_pointer_cast<ProgNode>(node);
    this->codeGenFunc(prog_node->_func);
    this->_ctx._module->print(llvm::outs(), nullptr);
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
    return this->codeGenExp(stmt_node->_expr);
}
llvm::Value *CodeGenBase::codeGenNumber(std::shared_ptr<ASTNode> node) {
    auto number_node = std::dynamic_pointer_cast<NumberNode>(node);
    return llvm::ConstantInt::get(*(this->_ctx._context), llvm::APInt(64, number_node->_int_literal));
}
llvm::Value *CodeGenBase::codeGenExp(std::shared_ptr<ASTNode> node) {
    auto expr_node = std::dynamic_pointer_cast<ExprNode>(node);
    return this->codeGenUnaryExp(expr_node->_unary_expr);
}
llvm::Value *CodeGenBase::codeGenUnaryExp(std::shared_ptr<ASTNode> node) {
    auto unary_node = std::dynamic_pointer_cast<UnaryExprNode>(node);
    if (unary_node->_primary_expr) {
        return this->codeGenPrimaryExp(unary_node->_primary_expr);
    } else if (unary_node->_unary_expr && unary_node->_unary_op) {
        std::string op =
            std::dynamic_pointer_cast<UnaryOpNode>(unary_node->_unary_op)->_op;
        llvm::Value *val = this->codeGenUnaryExp(unary_node->_unary_expr);
        if (op == "+") {
            return val;
        } else if (op == "-") {
            return (this->_ctx._builder)->CreateNeg(val, "sub_tmp");
        } else if (op == "!") {
            llvm::Value *zero = llvm::ConstantInt::get(*(this->_ctx._context), llvm::APInt(64, 0, false));
            llvm::Value *bool_val =  (this->_ctx._builder)->CreateICmpEQ(zero, val);
            return (this->_ctx._builder)->CreateIntCast(bool_val, llvm::Type::getInt64Ty(*(this->_ctx._context)), false);
        }
    }
    return nullptr;
}

llvm::Value *CodeGenBase::codeGenPrimaryExp(std::shared_ptr<ASTNode> node) {
    auto prim_exp_node = std::dynamic_pointer_cast<PrimExprNode>(node);
    if (prim_exp_node->_expr) {
        return this->codeGenExp(prim_exp_node->_expr);
    } else if (prim_exp_node->_number){
        return this->codeGenNumber(prim_exp_node->_number);
    }
    return nullptr;
}
} //namespace l24
