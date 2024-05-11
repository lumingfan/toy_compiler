/* Copyright (c) 2012-2017 The ANTLR Project. All rights reserved.
 * Use of this file is governed by the BSD 3-clause license that
 * can be found in the LICENSE.txt file in the project root.
 */

//
//  main.cpp
//  antlr4-cpp-demo
//
//  Created by Mike Lischke on 13.03.16.
//

#include "antlr4-runtime.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"

#include "frontend/ast.h"
#include "frontend/front_end.h"
#include "frontend/code_gen.h"

using namespace antlr4;
using namespace l24;

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        llvm::errs() << "Usage: ./bin/l24 <filename>" << "\n";
        return 1;
    }

    std::string filename(argv[1]);
    std::ifstream stream(filename);
    if (!stream.good()) {
      llvm::errs() << "error: no such file: '" << filename << "'\n";
      return 1;
    }

    FrontEnd front_end;
    auto prog_node = front_end.parse(stream);

    CodeGenBase cgb;
    cgb.codeGenProgram(prog_node);

    return 0;
}
