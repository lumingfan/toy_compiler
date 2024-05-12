#pragma once

#include <utility>
#include <vector>
#include <iostream>
#include <memory>
#include "llvm/IR/Value.h"

namespace l24 {
class CodeGenContext;


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
    std::shared_ptr<ASTNode> _expr;
};

class ExprNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _lor_expr;
};

class LorExprNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _land_expr;
    std::shared_ptr<ASTNode> _lor_expr;
};

class LandExprNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _eq_expr;
    std::shared_ptr<ASTNode> _land_expr;
};


class EqExprNode : public ASTNode {
public:
    std::string op;
    std::shared_ptr<ASTNode> _rel_expr;
    std::shared_ptr<ASTNode> _eq_expr;
};

class RelExprNode : public ASTNode {
public:
    std::string op;
    std::shared_ptr<ASTNode> _add_expr;
    std::shared_ptr<ASTNode> _rel_expr;
};

class AddExprNode : public ASTNode {
public:
    char _op{'\0'};
    std::shared_ptr<ASTNode> _mul_expr;
    std::shared_ptr<ASTNode> _add_expr;
};

class MulExprNode : public ASTNode {
public:
    char _op{'\0'};
    std::shared_ptr<ASTNode> _unary_expr;
    std::shared_ptr<ASTNode> _mul_expr;
};

class UnaryExprNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _primary_expr;
    std::shared_ptr<ASTNode> _unary_op;
    std::shared_ptr<ASTNode> _unary_expr;
};

class UnaryOpNode : public ASTNode {
public:
    std::string _op;
    explicit UnaryOpNode(std::string op): _op(std::move(op)){}
};

class PrimExprNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _expr;
    std::shared_ptr<ASTNode> _number;
};

class NumberNode : public ASTNode {
public:
    long long _int_literal;
    explicit NumberNode(int literal): _int_literal(literal) {}
};


    
} // namespace l24
