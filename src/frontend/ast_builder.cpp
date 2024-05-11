#include <any>
#include <iostream>

#include "frontend/ast_builder.h"
#include "frontend/ast.h"

namespace l24 {

std::shared_ptr<ASTNode> ASTBuilder::build(l24Parser::ProgramContext *ctx) {
    return std::any_cast<std::shared_ptr<ProgNode>>(visitProgram(ctx));
}

std::any ASTBuilder::visitProgram(l24Parser::ProgramContext *ctx) {
    auto program = std::make_shared<ProgNode>();
    program->_func = std::any_cast<std::shared_ptr<FuncNode>>(visitFunc(ctx->func()));
    return program;
}

std::any ASTBuilder::visitFunc(l24Parser::FuncContext *ctx) {
    auto func = std::make_shared<FuncNode>();
    func->_type = ctx->Int()->getText();
    func->_ident = ctx->Ident()->getText();
    func->_block = std::move(std::any_cast<std::shared_ptr<BlockNode>>(visitBlock(ctx->block())));

    return func;
}

std::any ASTBuilder::visitBlock(l24Parser::BlockContext *ctx) {
    auto block = std::make_shared<BlockNode>();
    block->_stmt = std::move(std::any_cast<std::shared_ptr<StmtNode>>(visitStmt(ctx->stmt())));
    return block;
}

std::any ASTBuilder::visitStmt(l24Parser::StmtContext *ctx) {
    auto stmt = std::make_shared<StmtNode>();
    stmt->_expr = std::move(std::any_cast<std::shared_ptr<ExprNode>>(visitExp(ctx->exp())));
    return stmt;
}

std::any ASTBuilder::visitNumber(l24Parser::NumberContext *ctx) {
    return std::make_shared<NumberNode>(std::stoll(ctx->IntLiteral()->getText()));
}
std::any ASTBuilder::visitExp(l24Parser::ExpContext *ctx) {
    auto expr = std::make_shared<ExprNode>();
    expr->_unary_expr = std::move(std::any_cast<std::shared_ptr<UnaryExprNode>>(visitUnaryExp(ctx->unaryExp())));
    return expr;
}
std::any ASTBuilder::visitUnaryExp(l24Parser::UnaryExpContext *ctx) {
    auto unary_expr = std::make_shared<UnaryExprNode>();
    if (ctx->primaryExp()) {
        unary_expr->_primary_expr = std::move(std::any_cast<std::shared_ptr<PrimExprNode>>(visitPrimaryExp(ctx->primaryExp())));
    } else if (ctx->unaryExp() && ctx->unaryOp()){
        unary_expr->_unary_op = std::move(std::any_cast<std::shared_ptr<UnaryOpNode>>(visitUnaryOp(ctx->unaryOp())));
        unary_expr->_unary_expr = std::move(std::any_cast<std::shared_ptr<UnaryExprNode>>(visitUnaryExp(ctx->unaryExp())));
    } else {
        ASTBuilder::BuildError("UnaryExp build failed");
    }
    return unary_expr;
}
std::any ASTBuilder::visitUnaryOp(l24Parser::UnaryOpContext *ctx) {
    std::shared_ptr<UnaryOpNode> unary_op;
    if (ctx->Minus()) {
        unary_op = std::make_shared<UnaryOpNode>(ctx->Minus()->getText());
    } else if (ctx->Plus()) {
        unary_op = std::make_shared<UnaryOpNode>(ctx->Plus()->getText());
    } else if (ctx->Not()) {
        unary_op = std::make_shared<UnaryOpNode>(ctx->Not()->getText());
    } else {
        ASTBuilder::BuildError("UnaryOp build failed");
    }
    return unary_op;
}

std::any ASTBuilder::visitPrimaryExp(l24Parser::PrimaryExpContext *ctx) {
    auto prim_exp = std::make_shared<PrimExprNode>();
    if (ctx->exp()) {
        prim_exp->_expr = std::move(std::any_cast<std::shared_ptr<ExprNode>>(visitExp(ctx->exp())));
    } else if (ctx->number()){
        prim_exp->_number = std::move(std::any_cast<std::shared_ptr<NumberNode>>(visitNumber(ctx->number())));
    } else {
        ASTBuilder::BuildError("PrimaryExp build failed");
    }
    return prim_exp;
}

} // namespace l24
