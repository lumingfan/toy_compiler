#pragma once

#include <memory>

#include <any>

#include "antlr4-runtime.h"
#include "l24BaseVisitor.h"

#include "frontend/ast.h"

namespace l24 {

class ASTBuilder : public l24BaseVisitor {
public:
    std::shared_ptr<ProgNode> build(l24Parser::ProgramContext *ctx);

    virtual std::any visitProgram(l24Parser::ProgramContext *ctx) override;
    virtual std::any visitFunc(l24Parser::FuncContext *ctx) override;
    virtual std::any visitBlock(l24Parser::BlockContext *ctx) override;
    virtual std::any visitStmt(l24Parser::StmtContext *ctx) override;
    virtual std::any visitNumber(l24Parser::NumberContext *ctx) override;
};

} // namespace l24
