#pragma once

#include <istream>
#include <memory>

#include "frontend/ast.h"

namespace l24 {

class FrontEnd {

public:
    // Parse an input stream and return an AST.
    std::shared_ptr<ASTNode> parse(std::istream& Stream);
};

}  // namespace l24