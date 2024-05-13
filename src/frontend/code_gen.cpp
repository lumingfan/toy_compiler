#include "frontend/code_gen.h"

namespace l24 {
void CodeGenBase::asmGen() const {
    // Initialize the target registry etc.
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    (_ctx._module)->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
        llvm::errs() << Error;
        return ;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto TheTargetMachine = Target->createTargetMachine(
        TargetTriple, CPU, Features, opt, llvm::Reloc::PIC_);

    (this->_ctx._module)->setDataLayout(TheTargetMachine->createDataLayout());

    auto Filename = "output.S";
    std::error_code EC;
    llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
        llvm::errs() << "Could not open file: " << EC.message();
    }

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CodeGenFileType::AssemblyFile;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        llvm::errs() << "TheTargetMachine can't emit a file of this type";
        return ;
    }

    pass.run(*(this->_ctx._module));
    dest.flush();
}

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
llvm::Value *CodeGenBase::codeGenLVal(std::shared_ptr<ASTNode> node) {
    auto l_val_node = std::dynamic_pointer_cast<LValNode>(node);
    llvm::Value *val = this->_ctx.getValue(l_val_node->_ident, CodeGenContext::ValType::ANY);
    if (val == nullptr) {
        CodeGenContext::LogError("Ident: " + l_val_node->_ident + " hasn't been declared");
    }
    return val;
}

llvm::Value *CodeGenBase::codeGenBlock(std::shared_ptr<ASTNode> node) {
    auto block_node = std::dynamic_pointer_cast<BlockNode>(node);
    llvm::Value *ret_val = nullptr;
    for (const auto& blk_item_node : block_node->_block_items) {
        ret_val = this->codeGenBlockItem(blk_item_node);
    }
    return ret_val;
}
llvm::Value *CodeGenBase::codeGenStmt(std::shared_ptr<ASTNode> node) {
    auto stmt_node = std::dynamic_pointer_cast<StmtNode>(node);
    llvm::Value *new_val = this->codeGenExp(stmt_node->_expr);
    if (stmt_node->_ident.empty()) {
        return new_val;
    }
    llvm::Value *val = this->_ctx.getValue(stmt_node->_ident, CodeGenContext::ValType::VAR);
    if (val == nullptr) {
        CodeGenContext::LogError("variable: " + stmt_node->_ident + " hasn't been declared");
    }

    this->_ctx.setValue(stmt_node->_ident, CodeGenContext::ValType::VAR, new_val);
    return new_val;
}
llvm::Value *CodeGenBase::codeGenNumber(std::shared_ptr<ASTNode> node) {
    auto number_node = std::dynamic_pointer_cast<NumberNode>(node);
    return llvm::ConstantInt::get(*(this->_ctx._context), llvm::APInt(64, number_node->_int_literal));
}
llvm::Value *CodeGenBase::codeGenExp(std::shared_ptr<ASTNode> node) {
    auto expr_node = std::dynamic_pointer_cast<ExprNode>(node);
    return this->codeGenLorExp(expr_node->_lor_expr);
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
            return (this->_ctx._builder)->CreateNeg(val, "unary_sub_tmp");
        } else if (op == "!") {
            llvm::Value *zero = llvm::ConstantInt::get(*(this->_ctx._context), llvm::APInt(64, 0, false));
            return this->booleanToInt((this->_ctx._builder)->CreateICmpEQ(zero, val));
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
    } else if (prim_exp_node->_l_val) {
        return this->codeGenLVal(prim_exp_node->_l_val);
    }
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenAddExp(std::shared_ptr<ASTNode> node) {
    auto add_exp_node = std::dynamic_pointer_cast<AddExprNode>(node);
    if (add_exp_node->_op != '\0') {
        llvm::Value *lv = this->codeGenAddExp(add_exp_node->_add_expr);
        llvm::Value *rv = this->codeGenMulExp(add_exp_node->_mul_expr) ;
        switch (add_exp_node->_op) {
        case '+':
            return (this->_ctx._builder)->CreateAdd(lv, rv, "add_temp");
        case '-':
            return (this->_ctx._builder)->CreateSub(lv, rv, "bin_sub_temp");
        }
    }
    return this->codeGenMulExp(add_exp_node->_mul_expr);
}
llvm::Value *CodeGenBase::codeGenMulExp(std::shared_ptr<ASTNode> node) {
    auto mul_exp_node = std::dynamic_pointer_cast<MulExprNode>(node);
    if (mul_exp_node->_op != '\0') {
        llvm::Value *lv = this->codeGenMulExp(mul_exp_node->_mul_expr);
        llvm::Value *rv = this->codeGenUnaryExp(mul_exp_node->_unary_expr);
        switch(mul_exp_node->_op) {
        case '*':
            return (this->_ctx._builder)->CreateMul(lv, rv, "mul_tmp");
        case '/':
            return (this->_ctx._builder)->CreateSDiv(lv, rv, "div_tmp");
        case '%':
            return (this->_ctx._builder)->CreateSRem(lv, rv, "rem_tmp");
        }
    }
    return this->codeGenUnaryExp(mul_exp_node->_unary_expr);
}
llvm::Value *CodeGenBase::codeGenLorExp(std::shared_ptr<ASTNode> node) {
    auto lor_exp_node = std::dynamic_pointer_cast<LorExprNode>(node);
    if (lor_exp_node->_land_expr && lor_exp_node->_lor_expr) {
        llvm::Value *lv = this->intToBoolean(this->codeGenLorExp(lor_exp_node->_lor_expr));
        llvm::Value *rv = this->intToBoolean(this->codeGenLandExp(lor_exp_node->_land_expr));
        return this->booleanToInt((this->_ctx._builder)->CreateLogicalOr(lv, rv));
    }
    return this->codeGenLandExp(lor_exp_node->_land_expr);
}
llvm::Value *CodeGenBase::codeGenLandExp(std::shared_ptr<ASTNode> node) {
    auto land_exp_node = std::dynamic_pointer_cast<LandExprNode>(node);
    if (land_exp_node->_land_expr && land_exp_node->_eq_expr) {
        llvm::Value *lv = this->intToBoolean(this->codeGenLandExp(land_exp_node->_land_expr));
        llvm::Value *rv = this->intToBoolean(this->codeGenEqExp(land_exp_node->_eq_expr));
        return this->booleanToInt((this->_ctx._builder)->CreateLogicalAnd(lv, rv));
    }
    return this->codeGenEqExp(land_exp_node->_eq_expr);
}
llvm::Value *CodeGenBase::codeGenEqExp(std::shared_ptr<ASTNode> node) {
    auto eq_exp_node = std::dynamic_pointer_cast<EqExprNode>(node);
    if (eq_exp_node->_eq_expr && eq_exp_node->_rel_expr) {
        llvm::Value *lv = this->codeGenEqExp(eq_exp_node->_eq_expr);
        llvm::Value *rv = this->codeGenRelExp(eq_exp_node->_rel_expr);
        if (eq_exp_node->op == "==") {
            return this->booleanToInt((this->_ctx._builder)->CreateICmpEQ(lv, rv));
        } else {
            return this->booleanToInt((this->_ctx._builder)->CreateICmpNE(lv, rv));
        }
    }
    return this->codeGenRelExp(eq_exp_node->_rel_expr);
}
llvm::Value *CodeGenBase::codeGenRelExp(std::shared_ptr<ASTNode> node) {
    auto rel_exp_node = std::dynamic_pointer_cast<RelExprNode>(node);
    if (rel_exp_node->_rel_expr && rel_exp_node->_add_expr) {
        llvm::Value *lv = this->codeGenRelExp(rel_exp_node->_rel_expr);
        llvm::Value *rv = this->codeGenAddExp(rel_exp_node->_add_expr);

        if (rel_exp_node->op == "<") {
            return this->booleanToInt((this->_ctx._builder)->CreateICmpSLT(lv, rv));
        } else if (rel_exp_node->op == ">") {
            return this->booleanToInt((this->_ctx._builder)->CreateICmpSGT(lv, rv));
        } else if (rel_exp_node->op == "<=") {
            return this->booleanToInt((this->_ctx._builder)->CreateICmpSLE(lv, rv));
        } else {
            return this->booleanToInt((this->_ctx._builder)->CreateICmpSGE(lv, rv));
        }
    }
    return this->codeGenAddExp(rel_exp_node->_add_expr);
}
llvm::Value *CodeGenBase::codeGenBlockItem(std::shared_ptr<ASTNode> node) {
    auto blk_item_node = std::dynamic_pointer_cast<BlockItemNode>(node);
    if (blk_item_node->_decl) {
        return this->codeGenDecl(blk_item_node->_decl);
    }
    return this->codeGenStmt(blk_item_node->_stmt);
}
llvm::Value *CodeGenBase::codeGenDecl(std::shared_ptr<ASTNode> node) {
    auto decl_node = std::dynamic_pointer_cast<DeclNode>(node);
    if (decl_node->_const_decl) {
        return this->codeGenConstDecl(decl_node->_const_decl);
    }
    return this->codeGenVarDecl(decl_node->_var_decl);
}
llvm::Value *CodeGenBase::codeGenConstDecl(std::shared_ptr<ASTNode> node) {
    auto const_decl_node = std::dynamic_pointer_cast<ConstDeclNode>(node);
    for (const auto& const_def_ast_node : const_decl_node->_const_defs) {
        this->codeGenConstDef(const_def_ast_node);
        auto const_def_node = std::dynamic_pointer_cast<ConstDefNode>(const_def_ast_node);
        llvm::Value *val = this->_ctx.getValue(const_def_node->_ident, CodeGenContext::ValType::CONST);
        if (val == nullptr || !val->getType()->isIntOrIntVectorTy(64)) {
            CodeGenContext::LogError("const define: type violates");
        }
    }
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenConstDef(std::shared_ptr<ASTNode> node) {
    auto const_def_node = std::dynamic_pointer_cast<ConstDefNode>(node);
    llvm::Value *val = this->_ctx.getValue(const_def_node->_ident, CodeGenContext::ValType::CONST);
    if (val != nullptr) {
        CodeGenContext::LogError("const define: const " + const_def_node->_ident + " has been defined");
    }
    this->_ctx.setValue(const_def_node->_ident, CodeGenContext::ValType::CONST, this->codeGenConstInitVal(const_def_node->_const_init_val));
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenConstInitVal(std::shared_ptr<ASTNode> node) {
    auto const_init_val_node = std::dynamic_pointer_cast<ConstInitValNode>(node);
    return this->codeGenConstExp(const_init_val_node->_const_exp);
}
llvm::Value *CodeGenBase::codeGenConstExp(std::shared_ptr<ASTNode> node) {
    auto const_exp_node = std::dynamic_pointer_cast<ConstExpNode>(node);
    return this->codeGenExp(const_exp_node->_exp);
}
llvm::Value *CodeGenBase::codeGenVarDecl(std::shared_ptr<ASTNode> node) {
    auto var_decl_node = std::dynamic_pointer_cast<VarDeclNode>(node);
    for (const auto& var_def_ast_node : var_decl_node->_var_defs) {
        this->codeGenVarDef(var_def_ast_node);
        auto var_def_node = std::dynamic_pointer_cast<VarDefNode>(var_def_ast_node);
        llvm::Value *val = this->_ctx.getValue(var_def_node->_ident, CodeGenContext::ValType::VAR);
        if (val == nullptr || !val->getType()->isIntOrIntVectorTy(64)) {
            CodeGenContext::LogError("var define: type violates");
        }
    }
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenVarDef(std::shared_ptr<ASTNode> node) {
    auto var_def_node = std::dynamic_pointer_cast<VarDefNode>(node);
    llvm::Value *val = this->_ctx.getValue(var_def_node->_ident, CodeGenContext::ValType::VAR);
    if (val != nullptr) {
        CodeGenContext::LogError("var define: var " + var_def_node->_ident + " has been defined");
    }
    if (var_def_node->_init_val) {
        this->_ctx.setValue(var_def_node->_ident, CodeGenContext::ValType::VAR, this->codeGenInitVal(var_def_node->_init_val));
    } else {
        this->_ctx.setValue(var_def_node->_ident, CodeGenContext::ValType::VAR, this->getInitInt());
    }
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenInitVal(std::shared_ptr<ASTNode> node) {
    auto init_val_node = std::dynamic_pointer_cast<InitValNode>(node);
    return this->codeGenExp(init_val_node->_exp);
}
} //namespace l24
