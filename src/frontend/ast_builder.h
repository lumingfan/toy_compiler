#pragma once

#include <memory>

#include <any>

#include "antlr4-runtime.h"
#include "l24BaseVisitor.h"

#include "frontend/ast.h"

namespace l24 {

class ASTBuilder : public l24BaseVisitor {
public:
    std::shared_ptr<ASTNode> build(l24Parser::ProgramContext *ctx);

    std::any visitProgram(l24Parser::ProgramContext *ctx) override;
    std::any visitFunc(l24Parser::FuncContext *ctx) override;
    std::any visitBlock(l24Parser::BlockContext *ctx) override;
    std::any visitStmt(l24Parser::StmtContext *ctx) override;
    std::any visitNumber(l24Parser::NumberContext *ctx) override;
};

} // namespace l24
