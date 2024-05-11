#pragma once

#include <utility>
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
};

class ProgNode : public ASTNode  {
public:
    std::shared_ptr<ASTNode> _func;
};

class FuncNode : public ASTNode {
public:
    std::string _type;
    std::string _ident;
    std::shared_ptr<ASTNode> _block; 
};

class IdentNode : public ASTNode {
public:
    std::string _ident;
    explicit IdentNode(std::string ident) : _ident(std::move(ident)) {}
};

class BlockNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _stmt;
};

class StmtNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _number;
};

class NumberNode : public ASTNode {
public:
    long long _int_literal;
    explicit NumberNode(int literal): _int_literal(literal) {}
};


    
} // namespace l24
