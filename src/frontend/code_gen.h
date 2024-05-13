#pragma once

#include <map>
#include <iostream>
#include <variant>

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"

#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"


#include "frontend/ast.h"

namespace l24 {

class CodeGenContext {
public:
    enum ValType {
        CONST, VAR, ANY
    };
    struct ConstVal {
        llvm::Value *_val;
        ConstVal(): _val(nullptr) {}
        ConstVal(llvm::Value *v): _val(v) {}
    };

    struct VarVal {
        llvm::Value *_val;
        VarVal(): _val(nullptr) {}
        VarVal(llvm::Value *v): _val(v) {}
    };

    std::unique_ptr<llvm::LLVMContext> _context;
    std::unique_ptr<llvm::Module> _module;
    std::unique_ptr<llvm::IRBuilder<>> _builder;
    std::map<std::string, std::variant<ConstVal, VarVal>> _named_values;

    CodeGenContext() {
        _context = std::make_unique<llvm::LLVMContext>();
        _module = std::make_unique<llvm::Module>("l24_module", *_context);
        _builder = std::make_unique<llvm::IRBuilder<>>(*_context);
    }

    static void LogError(const std::string &str) {
        std::cerr << "Error: " << str << std::endl;
        exit(1);
    }

    void setValue(const std::string &ident, ValType ty, llvm::Value *val) {
        assert(ty == ValType::CONST || ty == ValType::VAR);
        if (this->_named_values.count(ident) == 0) {
            switch(ty) {
            case ValType::CONST:
               this->_named_values[ident] = ConstVal(val);
               break;
            case ValType::VAR:
                this->_named_values[ident] = VarVal(val);
                break;
            default:
                LogError("you must specify var/const of this ident");
            }
            return ;
        }

        assert(ty == ValType::VAR);
        try {
            this->_named_values[ident] = VarVal(val);
        } catch (std::bad_variant_access const& ex) {
            LogError(ident + " is a const");
        }
    }

    llvm::Value *getValue(const std::string& ident, ValType ty) {
        if (this->_named_values.count(ident) == 0) {
            return nullptr;
        }
        auto var_val = this->_named_values[ident];
        try {
            switch (ty) {
            case ValType::VAR:
                return std::get<VarVal>(var_val)._val;
            default:
                return std::get<ConstVal>(var_val)._val;
            }
        } catch (std::bad_variant_access const& ex) {
            switch (ty) {
            case ValType::CONST:
                LogError(std::string(ex.what()) + ": contained var , not const");
                break;
            case ValType::VAR:
                LogError(std::string(ex.what()) + ": contained const, not var");
                break;
            case ValType::ANY:
                return std::get<VarVal>(var_val)._val;
            }
        }

        return nullptr;
    }
};


class CodeGen {
public:
    virtual llvm::Value *codeGenProgram(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenFunc(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenBlock(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenStmt(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenAddExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenMulExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenUnaryExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenPrimaryExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenNumber(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenLorExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenLandExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenEqExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenRelExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenBlockItem(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenDecl(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenConstDecl(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenVarDecl(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenConstDef(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenVarDef(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenConstInitVal(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenInitVal(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenConstExp(std::shared_ptr<ASTNode> node) = 0;
    virtual llvm::Value *codeGenLVal(std::shared_ptr<ASTNode> node) = 0;
};

class CodeGenBase : public CodeGen {
private:
    CodeGenContext _ctx;
    llvm::Value *intToBoolean(llvm::Value *val) const {
        llvm::Value *zero = llvm::ConstantInt::get(*(this->_ctx._context), llvm::APInt(64, 0, false));
        return (this->_ctx._builder)->CreateICmpNE(zero, val);
    }
    llvm::Value *booleanToInt(llvm::Value *val) const {
        return (this->_ctx._builder)->CreateIntCast(val, llvm::Type::getInt64Ty(*(this->_ctx._context)), false);
    }

    llvm::Value *getInitInt() const {
        return llvm::ConstantInt::get(*(this->_ctx._context), llvm::APInt(64, 0, false));
    }

public:
    void asmGen() const;

    llvm::Value *codeGenExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenLorExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenLandExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenEqExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenRelExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenUnaryExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenAddExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenBlockItem(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenDecl(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenConstDecl(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenConstDef(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenConstInitVal(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenConstExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenLVal(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenMulExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenPrimaryExp(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenProgram(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenFunc(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenBlock(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenStmt(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenNumber(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenVarDecl(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenVarDef(std::shared_ptr<ASTNode> node) override;
    llvm::Value *codeGenInitVal(std::shared_ptr<ASTNode> node) override;

};


} //namespace l24