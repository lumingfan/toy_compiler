#include "frontend/code_gen.h"
#include "frontend/type.h"

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

llvm::Value *CodeGenBase::codeGenEntry(std::shared_ptr<ASTNode> node) {
    auto entry_node = std::dynamic_pointer_cast<EntryNode>(node);
    // generate function declaration for standard library
    this->_ctx.codeGenStandardLibrary();
    this->codeGenProgram(entry_node->_prog);
    this->_ctx._module->print(llvm::errs(), nullptr);
    return nullptr;
}

llvm::Value *CodeGenBase::codeGenProgram(std::shared_ptr<ASTNode> node) {
    auto prog_node = std::dynamic_pointer_cast<ProgNode>(node);

    if (prog_node->_prog) {
        this->codeGenProgram(prog_node->_prog);
    }

    if (prog_node->_decl) {
        this->codeGenDecl(prog_node->_decl);
    }

    if (prog_node->_func) {
        this->codeGenFunc(prog_node->_func);
    }

    return nullptr;
}
llvm::Value *CodeGenBase::codeGenFunc(std::shared_ptr<ASTNode> node) {
    auto func_node = std::dynamic_pointer_cast<FuncNode>(node);

    if ((this->_ctx._module)->getFunction(func_node->_ident) != nullptr) {
        CodeGenContext::LogError("function can't be redefined");
    }

    // args type:  (int,int) etc.
    auto func_params_node  = std::dynamic_pointer_cast<FuncFParamsNode>(func_node->_param);
    int params_size = func_params_node->_params.size();
    std::vector<llvm::Type *> types;
    std::vector<bool> is_ptr_vec;
    for (int i = 0; i < params_size; ++i) {
        auto func_param_node = std::dynamic_pointer_cast<FuncFParamNode>(func_params_node->_params[i]);
        if (func_param_node->_type == "int") {
            types.emplace_back(llvm::Type::getInt64Ty(*(this->_ctx._context)));
            is_ptr_vec.push_back(false);
        } else {
            // int array
            types.emplace_back(llvm::PointerType::get(llvm::IntegerType::get(*(this->_ctx._context), 64), 0));
            is_ptr_vec.push_back(true);
        }
    }

    llvm::FunctionType *ft;
    if (func_node->_type == "int") {
        ft = llvm::FunctionType::get(llvm::Type::getInt64Ty(*(this->_ctx._context)), types, false);
    } else {
        ft = llvm::FunctionType::get(llvm::Type::getVoidTy(*(this->_ctx._context)), types, false);
    }
    llvm::Function *func =
        llvm::Function::Create(ft, llvm::Function::ExternalLinkage, func_node->_ident, (this->_ctx._module).get());

    // set args ident
    int idx = 0;
    for (auto &arg : func->args()) {
        auto func_param_node = std::dynamic_pointer_cast<FuncFParamNode>(func_params_node->_params[idx++]);
        arg.setName(func_param_node->_ident);
    }

    llvm::BasicBlock *BB = llvm::BasicBlock::Create(*(this->_ctx._context), "entry", func);
    (this->_ctx._builder)->SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    this->_ctx.pushNamedValuesLayer();
    idx = 0;
    for (auto &arg : func->args()) {
        (this->_ctx).defineValue(std::string(arg.getName()), L24Type::ValType::VAR, {&arg}, nullptr, is_ptr_vec[idx++]);
    }

    this->codeGenBlock(func_node->_block);

    // if the last instruction is not a return inst, append one
    if (!llvm::isa<llvm::ReturnInst>(this->_ctx._builder->GetInsertBlock()->back())) {
        this->_ctx._builder->CreateRetVoid();
    }
    // Validate the generated code, checking for consistency.
    llvm::verifyFunction(*func);

    this->_ctx.popNamedValuesLayer();
    return func;
}
llvm::Value *CodeGenBase::codeGenLVal(std::shared_ptr<ASTNode> node) {
    auto l_val_node = std::dynamic_pointer_cast<LValNode>(node);
    llvm::Value *sub_idx = nullptr;
    if (l_val_node->_exp) {
        sub_idx = this->codeGenExp(l_val_node->_exp);
    }

    llvm::Value *val = this->_ctx.getValue(l_val_node->_ident, L24Type::ValType::ANY, sub_idx);
    if (val == nullptr) {
        CodeGenContext::LogError("Ident: " + l_val_node->_ident + " hasn't been declared");
    }
    return val;
}

llvm::Value *CodeGenBase::codeGenBlock(std::shared_ptr<ASTNode> node) {
    auto block_node = std::dynamic_pointer_cast<BlockNode>(node);

    this->_ctx.pushNamedValuesLayer();
    for (const auto& blk_item_node : block_node->_block_items) {
        this->codeGenBlockItem(blk_item_node);
    }
    this->_ctx.popNamedValuesLayer();
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenStmt(std::shared_ptr<ASTNode> node) {
    auto stmt_node = std::dynamic_pointer_cast<StmtNode>(node);
    if (stmt_node->_block != nullptr) {
        return this->codeGenBlock(stmt_node->_block);
    }
    if (stmt_node->_if_stmt != nullptr) {
        return this->codeGenIfStmt(node);
    }
    if (stmt_node->_while_stmt != nullptr) {
        return this->codeGenWhileStmt(node);
    }
    if (stmt_node->_is_break_stmt || stmt_node->_is_continue_stmt) {
        if ((this->_ctx._nested_blocks).empty()) {
            l24::CodeGenContext::LogError("syntax error: continue/break must exist in a loop");
            return nullptr;
        }
        if (stmt_node->_is_continue_stmt) {
            this->_ctx._builder->CreateBr(this->_ctx._nested_blocks.back().first);
        } else {
            this->_ctx._builder->CreateBr(this->_ctx._nested_blocks.back().second);
        }
        return nullptr;
    }

    if (stmt_node->_expr == nullptr) {
        if (stmt_node->_is_ret_stmt) {
            (this->_ctx._builder)->CreateRetVoid();
        }
        return nullptr;
    }
    llvm::Value *new_val = this->codeGenExp(stmt_node->_expr);

    if (stmt_node->_is_ret_stmt) {
        (this->_ctx._builder)->CreateRet(new_val);
        return nullptr;
    }

    if (stmt_node->_l_val.empty()) {
        return new_val;
    }

    llvm::Value *sub_idx = nullptr;
    if (stmt_node->_sub_idx) {
        sub_idx = this->codeGenExp(stmt_node->_sub_idx);
    }
    this->_ctx.setValue(stmt_node->_l_val, L24Type::ValType::VAR, new_val, sub_idx);
    return new_val;
}
llvm::Value *CodeGenBase::codeGenIfStmt(std::shared_ptr<ASTNode> node) {
    auto stmt_node = std::dynamic_pointer_cast<StmtNode>(node);
    llvm::Value *cond = this->codeGenExp(stmt_node->_expr);
    if (cond == nullptr) {
        return nullptr;
    }
    cond = this->intToBoolean(cond);
    llvm::Function *func = (this->_ctx._builder)->GetInsertBlock()->getParent();

    // Create blocks for the then and else cases.  Insert the 'then' block at the
    // end of the function.
    llvm::BasicBlock *thenBB =
        llvm::BasicBlock::Create(*(this->_ctx._context), "then", func);
    llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(*(this->_ctx._context), "else");
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*(this->_ctx._context), "ifcont");

    (this->_ctx._builder)->CreateCondBr(cond, thenBB, elseBB);

    // Emit then value.
    (this->_ctx._builder)->SetInsertPoint(thenBB);

    // generate then stmts code
    this->codeGenStmt(stmt_node->_if_stmt);

    // prevent two branch insts case
    // this may happen with continue/break
    if (!llvm::isa<llvm::BranchInst>((this->_ctx._builder)->GetInsertBlock()->back())) {
        (this->_ctx._builder)->CreateBr(mergeBB);
    }

    // Codegen of 'Then' can change the current block, update ThenBB for the PHI.

    // Emit else block.
    func->insert(func->end(), elseBB);
    (this->_ctx._builder)->SetInsertPoint(elseBB);

    // generate else stmts code
    if (stmt_node->_else_stmt) {
        this->codeGenStmt(stmt_node->_else_stmt);
    }

    // prevent two branch insts case
    // this may happen with continue/break
    if (!llvm::isa<llvm::BranchInst>((this->_ctx._builder)->GetInsertBlock()->back())) {
        (this->_ctx._builder)->CreateBr(mergeBB);
    }
    // Codegen of 'Else' can change the current block, update ElseBB for the PHI.

    // Emit merge block.
    func->insert(func->end(), mergeBB);
    (this->_ctx._builder)->SetInsertPoint(mergeBB);

    return nullptr;
}

llvm::Value *CodeGenBase::codeGenWhileStmt(std::shared_ptr<ASTNode> node) {
    auto stmt_node = std::dynamic_pointer_cast<StmtNode>(node);

    llvm::Function *func = (this->_ctx._builder)->GetInsertBlock()->getParent();
    llvm::BasicBlock *loop_bb = llvm::BasicBlock::Create(*(this->_ctx._context), "loop_cond", func);
    llvm::BasicBlock *body_bb = llvm::BasicBlock::Create(*(this->_ctx._context), "loop_body");
    llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(*(this->_ctx._context), "after_loop");

    // record block info for continue/break
    (this->_ctx._nested_blocks).emplace_back(loop_bb, after_bb);

    (this->_ctx._builder)->CreateBr(loop_bb);

    (this->_ctx._builder)->SetInsertPoint(loop_bb);

    // generate condition expression code
    llvm::Value *cond = this->codeGenExp(stmt_node->_expr);
    if (cond == nullptr) {
        // we exit this loop
        (this->_ctx._nested_blocks).pop_back();
        return nullptr;
    }

    cond = this->intToBoolean(cond);
    // Insert the conditional branch into the end
    (this->_ctx._builder)->CreateCondBr(cond, body_bb, after_bb);

    func->insert(func->end(), body_bb);
    (this->_ctx._builder)->SetInsertPoint(body_bb);

    // generate body code
    this->codeGenStmt(stmt_node->_while_stmt);

    // prevent two br instructions case
    // continue/break may cause this situation
    if (!llvm::isa<llvm::BranchInst> ((this->_ctx._builder)->GetInsertBlock()->back())) {
        (this->_ctx._builder)->CreateBr(loop_bb);
    }

    // Start emit AfterBB
    func->insert(func->end(), after_bb);
    (this->_ctx._builder)->SetInsertPoint(after_bb);

    (this->_ctx._nested_blocks).pop_back();
    return nullptr;
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
    } else {
        // function call
        llvm::Function *func = (this->_ctx._module)->getFunction(unary_node->_func_ident);
        if (func == nullptr) {
            CodeGenContext::LogError("unknown function " + unary_node->_func_ident);
        }

        auto func_params_node = std::dynamic_pointer_cast<FuncRParamsNode>(unary_node->_func_r_params);
        if (func->arg_size() != func_params_node->_exps.size()) {
            CodeGenContext::LogError("Incorrect arguments number, expect "
                                     + std::to_string(func->arg_size()) +
                                     " get " + std::to_string(func_params_node->_exps.size()));
        }

        std::vector<llvm::Value *> args_v;
        for (const auto & _exp : func_params_node->_exps) {
            args_v.push_back(this->codeGenExp(_exp));
        }
        return (this->_ctx._builder)->CreateCall(func, args_v, "call_" + unary_node->_func_ident);
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
//        llvm::Value *val = this->_ctx.getValue(const_def_node->_ident, L24Type::ValType::CONST);
//        if (val == nullptr || !val->getType()->isIntOrIntVectorTy(64)) {
//            CodeGenContext::LogError("const define: type violates");
//        }
    }
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenConstDef(std::shared_ptr<ASTNode> node) {
    auto const_def_node = std::dynamic_pointer_cast<ConstDefNode>(node);
    auto init_val_node = std::dynamic_pointer_cast<InitValNode>(const_def_node->_init_val);

    // array
    if (const_def_node->_exp != nullptr) {
        llvm::Value *array_size = this->codeGenExp(const_def_node->_exp);
        this->_ctx.defineValue(const_def_node->_ident, L24Type::ValType::CONST, this->getInitVals(init_val_node, array_size), array_size);
    } else {
        this->_ctx.defineValue(const_def_node->_ident, L24Type::ValType::CONST, this->getInitVals(init_val_node));
    }
    return nullptr;
}

llvm::Value *CodeGenBase::codeGenVarDecl(std::shared_ptr<ASTNode> node) {
    auto var_decl_node = std::dynamic_pointer_cast<VarDeclNode>(node);
    for (const auto& var_def_ast_node : var_decl_node->_var_defs) {
        this->codeGenVarDef(var_def_ast_node);
        auto var_def_node = std::dynamic_pointer_cast<VarDefNode>(var_def_ast_node);
//        llvm::Value *val = this->_ctx.getValue(var_def_node->_ident, L24Type::ValType::VAR);
//        if (val == nullptr || !val->getType()->isIntOrIntVectorTy(64)) {
//            CodeGenContext::LogError("var define: type violates");
//        }
    }
    return nullptr;
}
llvm::Value *CodeGenBase::codeGenVarDef(std::shared_ptr<ASTNode> node) {
    auto var_def_node = std::dynamic_pointer_cast<VarDefNode>(node);
    auto init_val_node = std::dynamic_pointer_cast<InitValNode>(var_def_node->_init_val);

    // array
    if (var_def_node->_exp != nullptr) {
        llvm::Value *array_size = this->codeGenExp(var_def_node->_exp);
        this->_ctx.defineValue(var_def_node->_ident, L24Type::ValType::VAR, this->getInitVals(init_val_node, array_size), array_size);
    } else {
        this->_ctx.defineValue(var_def_node->_ident, L24Type::ValType::VAR, this->getInitVals(init_val_node));
    }
    return nullptr;
}

std::vector<llvm::Value*> CodeGenBase::getInitVals(std::shared_ptr<InitValNode> node, llvm::Value *array_size) {
    int64_t size = 1;
    if (array_size != nullptr) {
        size = llvm::dyn_cast<llvm::ConstantInt>(array_size)->getSExtValue();
        if (size == 0) {
            CodeGenContext::LogError("You can't define a array with size 0");
        }
    }
    std::vector<llvm::Value*> val_vec;
    for (int64_t idx = 0; idx < size; ++idx) {
        // cases:
        //  1. int a;
        //  2. int a[2] = {1};
        if (node == nullptr || idx >= node->_exp.size()) {
            val_vec.emplace_back(this->getInitInt());
        } else {
            val_vec.emplace_back(this->codeGenExp(node->_exp[idx]));
        }
    }
    return val_vec;
}

} //namespace l24
