#pragma once

#include <memory>

#include <any>

#include "antlr4-runtime.h"
#include "l24BaseVisitor.h"

#include "frontend/ast.h"

namespace l24 {

class ASTBuilder : public l24BaseVisitor {
public:
    std::shared_ptr<ASTNode> build(l24Parser::EntryContext *ctx);

    static void BuildError(const char *str) {
        std::cerr << str << std::endl;
    }
    std::any visitEntry(l24Parser::EntryContext *ctx) override;
    std::any visitProgram(l24Parser::ProgramContext *ctx) override;
    std::any visitFunc(l24Parser::FuncContext *ctx) override;
    std::any visitFuncFParams(l24Parser::FuncFParamsContext *ctx) override;
    std::any visitFuncFParam(l24Parser::FuncFParamContext *ctx) override;
    std::any visitFuncRParams(l24Parser::FuncRParamsContext *ctx) override;
    std::any visitBlock(l24Parser::BlockContext *ctx) override;
    std::any visitBlockItem(l24Parser::BlockItemContext *ctx) override;
    std::any visitDecl(l24Parser::DeclContext *ctx) override;
    std::any visitConstDecl(l24Parser::ConstDeclContext *ctx) override;
    std::any visitVarDecl(l24Parser::VarDeclContext *ctx) override;
    std::any visitConstDef(l24Parser::ConstDefContext *ctx) override;
    std::any visitVarDef(l24Parser::VarDefContext *ctx) override;
    std::any visitInitVal(l24Parser::InitValContext *ctx) override;

    std::any visitStmt(l24Parser::StmtContext *ctx) override;
    std::any visitNumber(l24Parser::NumberContext *ctx) override;
    std::any visitExp(l24Parser::ExpContext *ctx) override;
    std::any visitAddExp(l24Parser::AddExpContext *ctx) override;
    std::any visitMulExp(l24Parser::MulExpContext *ctx) override;
    std::any visitUnaryExp(l24Parser::UnaryExpContext *ctx) override;
    std::any visitUnaryOp(l24Parser::UnaryOpContext *ctx) override;
    std::any visitPrimaryExp(l24Parser::PrimaryExpContext *ctx) override;
    std::any visitLOrExp(l24Parser::LOrExpContext *ctx) override;
    std::any visitLAndExp(l24Parser::LAndExpContext *ctx) override;
    std::any visitEqExp(l24Parser::EqExpContext *ctx) override;
    std::any visitRelExp(l24Parser::RelExpContext *ctx) override;
    std::any visitLVal(l24Parser::LValContext *ctx) override;


};

} // namespace l24
