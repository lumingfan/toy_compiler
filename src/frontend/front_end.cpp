#include <iostream>
#include <istream>
#include <memory>


#include "l24Lexer.h"
#include "l24Parser.h"
#include "antlr4-runtime.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"


#include "frontend/ast.h"
#include "frontend/ast_builder.h"
#include "frontend/front_end.h"

using namespace antlr4;


#define DEBUG_TYPE "l24-FrontEnd"

namespace l24 {

std::shared_ptr<ASTNode> FrontEnd::parse(std::istream& stream) {
    llvm::DebugFlag = true;

    ANTLRInputStream Input(stream);
    l24Lexer Lexer(&Input);
    CommonTokenStream Tokens(&Lexer);
    Tokens.fill();
    LLVM_DEBUG({
        llvm::outs() << "===== Lexer ===== \n";
        for (auto token : Tokens.getTokens()) {
            llvm::outs() << token->toString() << "\n";
        }
        llvm::outs() << "===== Lexer End ===== \n";
    });

    ANTLRErrorListener *errorListener = new BaseErrorListener();
    l24Parser Parser(&Tokens);
    Parser.addErrorListener(errorListener);
    l24Parser::EntryContext* entry = Parser.entry();
    LLVM_DEBUG({
        llvm::outs() << "===== Parser ===== \n";
        llvm::outs() << entry->toStringTree(&Parser, true) << "\n";
        if (Parser.getNumberOfSyntaxErrors()) {
            llvm::errs() << "===== Parser Failed ===== \n";
            return nullptr;
        }
        llvm::outs() << "===== Parser End ===== \n";
    });

    ASTBuilder builder;
    return builder.build(entry);
}

}  // namespace l24
