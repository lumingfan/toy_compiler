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
    std::vector<std::shared_ptr<ASTNode>> _block_items;
};

class BlockItemNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _decl;
    std::shared_ptr<ASTNode> _stmt;
};

class DeclNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _const_decl;
};

class ConstDeclNode : public ASTNode {
public:
    std::string _b_type;
    std::vector<std::shared_ptr<ASTNode>> _const_defs;
};

class ConstDefNode : public ASTNode {
public:
    std::string _ident;
    std::shared_ptr<ASTNode> _const_init_val;
};

class ConstInitValNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _const_exp;
};

class ConstExpNode : public ASTNode {
public:
    std::shared_ptr<ASTNode> _exp;
};

class LValNode : public ASTNode {
public:
    std::shared_ptr<IdentNode> _ident;
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
    std::shared_ptr<ASTNode> _l_val;
};

class NumberNode : public ASTNode {
public:
    long long _int_literal;
    explicit NumberNode(int literal): _int_literal(literal) {}
};


    
} // namespace l24
