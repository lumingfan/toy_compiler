#include <any>
#include <iostream>

#include "frontend/ast_builder.h"
#include "frontend/ast.h"

namespace l24 {

std::shared_ptr<ProgNode> ASTBuilder::build(l24Parser::ProgramContext *ctx) {
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
    stmt->_number = std::move(std::any_cast<std::shared_ptr<NumberNode>>(visitNumber(ctx->number())));
    return stmt;
}

std::any ASTBuilder::visitNumber(l24Parser::NumberContext *ctx) {
    auto number = std::make_shared<NumberNode>();
    number->_int_literal = std::stoll(ctx->IntLiteral()->getText());
    return number;
}

} // namespace l24
