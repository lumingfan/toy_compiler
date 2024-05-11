#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include "llvm/IR/Value.h"

namespace l24 {
class CodeGenContext;

void Print();

class ASTNode {
public:
    virtual ~ASTNode() = default; 
    virtual llvm::Value* codeGen() = 0; 
};

class ProgNode : ASTNode  {
public:
    std::shared_ptr<ASTNode> _func;
    virtual llvm::Value* codeGen() override;

};

class FuncNode : public ASTNode {
public:
    std::string _type;
    std::string _ident;
    std::shared_ptr<ASTNode> _block; 
    virtual llvm::Value* codeGen() override;
};

class IdentNode : public ASTNode {
public:
    std::string _ident;
    IdentNode(std::string ident) : _ident(ident) {}
    virtual llvm::Value* codeGen() override; 
};

class BlockNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _stmt;
    virtual llvm::Value* codeGen() override; 
};

class StmtNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _number;
    virtual llvm::Value* codeGen() override; 
};

class NumberNode : public ASTNode {
public:
    long long _int_literal;
    virtual llvm::Value* codeGen() override; 
};


    
} // namespace l24
