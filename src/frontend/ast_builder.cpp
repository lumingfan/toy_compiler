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
    for (auto blk_item_ctx : ctx->blockItem()) {
        block->_block_items.push_back(std::move(std::any_cast<std::shared_ptr<BlockItemNode>>(visitBlockItem(blk_item_ctx))));
    }
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
    expr->_lor_expr = std::move(std::any_cast<std::shared_ptr<LorExprNode>>(visitLOrExp(ctx->lOrExp())));
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
        prim_exp->_l_val = std::move(std::any_cast<std::shared_ptr<LValNode>>(visitLVal(ctx->lVal())));
    }
    return prim_exp;
}
std::any ASTBuilder::visitAddExp(l24Parser::AddExpContext *ctx) {
    auto add_exp = std::make_shared<AddExprNode>();
    if (ctx->Plus() || ctx->Minus()) {
        if (ctx->Plus()) {
            add_exp->_op = ctx->Plus()->getText()[0];
        } else {
            add_exp->_op = ctx->Minus()->getText()[0];
        }
        add_exp->_add_expr = std::move(std::any_cast<std::shared_ptr<AddExprNode>>(visitAddExp(ctx->addExp())));
        add_exp->_mul_expr = std::move(std::any_cast<std::shared_ptr<MulExprNode>>(visitMulExp(ctx->mulExp())));
    } else {
        add_exp->_mul_expr = std::move(std::any_cast<std::shared_ptr<MulExprNode>>(visitMulExp(ctx->mulExp())));
    }
    return add_exp;
}
std::any ASTBuilder::visitMulExp(l24Parser::MulExpContext *ctx) {
    auto mul_exp = std::make_shared<MulExprNode>();
    if (ctx->Slash() || ctx->Star() || ctx->Percentage()) {
        if (ctx->Slash()) {
            mul_exp->_op = ctx->Slash()->getText()[0];
        } else if (ctx->Star()) {
            mul_exp->_op = ctx->Star()->getText()[0];
        } else {
            mul_exp->_op = ctx->Percentage()->getText()[0];
        }
        mul_exp->_unary_expr = std::move(std::any_cast<std::shared_ptr<UnaryExprNode>>(visitUnaryExp(ctx->unaryExp())));
        mul_exp->_mul_expr = std::move(std::any_cast<std::shared_ptr<MulExprNode>>(visitMulExp(ctx->mulExp())));
    } else {
        mul_exp->_unary_expr = std::move(std::any_cast<std::shared_ptr<UnaryExprNode>>(visitUnaryExp(ctx->unaryExp())));
    }
    return mul_exp;
}
std::any ASTBuilder::visitLOrExp(l24Parser::LOrExpContext *ctx) {
    auto lor_expr = std::make_shared<LorExprNode>();
    if (ctx->lAndExp() && ctx->lOrExp()) {
        lor_expr->_land_expr = std::move(std::any_cast<std::shared_ptr<LandExprNode>>(visitLAndExp(ctx->lAndExp())));
        lor_expr->_lor_expr = std::move(std::any_cast<std::shared_ptr<LorExprNode>>(visitLOrExp(ctx->lOrExp())));
    } else {
        lor_expr->_land_expr = std::move(std::any_cast<std::shared_ptr<LandExprNode>>(visitLAndExp(ctx->lAndExp())));
    }
    return lor_expr;
}
std::any ASTBuilder::visitLAndExp(l24Parser::LAndExpContext *ctx) {
    auto land_expr = std::make_shared<LandExprNode>();
    if (ctx->lAndExp() && ctx->eqExp()) {
        land_expr->_land_expr = std::move(std::any_cast<std::shared_ptr<LandExprNode>>(visitLAndExp(ctx->lAndExp())));
        land_expr->_eq_expr = std::move(std::any_cast<std::shared_ptr<EqExprNode>>(visitEqExp(ctx->eqExp())));
    } else {
        land_expr->_eq_expr = std::move(std::any_cast<std::shared_ptr<EqExprNode>>(visitEqExp(ctx->eqExp())));
    }
    return land_expr;
}
std::any ASTBuilder::visitEqExp(l24Parser::EqExpContext *ctx) {
    auto eq_expr = std::make_shared<EqExprNode>();
    if (ctx->eqExp() && ctx->relExp()) {
        if (ctx->Eq()) {
            eq_expr->op = ctx->Eq()->getText();
        } else {
            eq_expr->op = ctx->NotEq()->getText();
        }
        eq_expr->_rel_expr = std::move(std::any_cast<std::shared_ptr<RelExprNode>>(visitRelExp(ctx->relExp())));
        eq_expr->_eq_expr = std::move(std::any_cast<std::shared_ptr<EqExprNode>>(visitEqExp(ctx->eqExp())));
    } else {
        eq_expr->_rel_expr = std::move(std::any_cast<std::shared_ptr<RelExprNode>>(visitRelExp(ctx->relExp())));
    }
    return eq_expr;
}
std::any ASTBuilder::visitRelExp(l24Parser::RelExpContext *ctx) {
    auto rel_expr = std::make_shared<RelExprNode>();
    if (ctx->relExp() && ctx->addExp()) {
        if (ctx->Less()) {
            rel_expr->op = ctx->Less()->getText();
        } else if (ctx->Greater()){
            rel_expr->op = ctx->Greater()->getText();
        } else if (ctx->LessEq()) {
            rel_expr->op = ctx->LessEq()->getText();
        } else {
            rel_expr->op = ctx->GreaterEq()->getText();
        }
        rel_expr->_add_expr = std::move(std::any_cast<std::shared_ptr<AddExprNode>>(visitAddExp(ctx->addExp())));
        rel_expr->_rel_expr = std::move(std::any_cast<std::shared_ptr<RelExprNode>>(visitRelExp(ctx->relExp())));
    } else {
        rel_expr->_add_expr = std::move(std::any_cast<std::shared_ptr<AddExprNode>>(visitAddExp(ctx->addExp())));
    }
    return rel_expr;
}
std::any ASTBuilder::visitBlockItem(l24Parser::BlockItemContext *ctx) {
    auto blk_item_node = std::make_shared<BlockItemNode>();
    if (ctx->decl()) {
        blk_item_node->_decl = std::move(std::any_cast<std::shared_ptr<DeclNode>>(visitDecl(ctx->decl())));
    } else {
        blk_item_node->_stmt = std::move(std::any_cast<std::shared_ptr<StmtNode>>(visitStmt(ctx->stmt())));
    }
    return blk_item_node;
}

std::any ASTBuilder::visitDecl(l24Parser::DeclContext *ctx) {
    auto decl_node = std::make_shared<DeclNode>();
    decl_node->_const_decl = std::move(std::any_cast<std::shared_ptr<ConstDeclNode>>(visitConstDecl(ctx->constDecl())));
    return decl_node;
}

std::any ASTBuilder::visitConstDecl(l24Parser::ConstDeclContext *ctx) {
    auto const_decl_node = std::make_shared<ConstDeclNode>();
    const_decl_node->_b_type = ctx->bType()->Int()->getText();
    for (auto const_def_ctx : ctx->constDef()) {
        const_decl_node->_const_defs.push_back(std::move(std::any_cast<std::shared_ptr<ConstDefNode>>(visitConstDef(const_def_ctx))));
    }
    return const_decl_node;
}

std::any ASTBuilder::visitConstDef(l24Parser::ConstDefContext *ctx) {
    auto const_def_node = std::make_shared<ConstDefNode>();
    const_def_node->_ident = ctx->Ident()->getText();
    const_def_node->_const_init_val = std::move(std::any_cast<std::shared_ptr<ConstInitValNode>>(visitConstInitVal(ctx->constInitVal())));
    return const_def_node;
}

std::any ASTBuilder::visitConstInitVal(l24Parser::ConstInitValContext *ctx) {
    auto const_init_val_node = std::make_shared<ConstInitValNode>();
    const_init_val_node->_const_exp = std::move(std::any_cast<std::shared_ptr<ConstExpNode>>(visitConstExp(ctx->constExp())));
    return const_init_val_node;
}

std::any ASTBuilder::visitConstExp(l24Parser::ConstExpContext *ctx) {
    auto const_exp_node = std::make_shared<ConstExpNode>();
    const_exp_node->_exp = std::move(std::any_cast<std::shared_ptr<ExprNode>>(visitExp(ctx->exp())));
    return const_exp_node;
}
std::any ASTBuilder::visitLVal(l24Parser::LValContext *ctx) {
    auto l_val_node = std::make_shared<LValNode>();
    l_val_node->_ident = std::make_shared<IdentNode>(ctx->Ident()->getText());
    return l_val_node;
}

} // namespace l24




