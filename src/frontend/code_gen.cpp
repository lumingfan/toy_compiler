#include "frontend/code_gen.h"

namespace l24 {
void CodeGenBase::asmGen() {
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
} //namespace l24
