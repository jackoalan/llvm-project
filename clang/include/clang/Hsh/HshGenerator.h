#pragma once

#include "clang/Frontend/FrontendAction.h"

namespace clang::hshgen {

class GenerateAction : public ASTFrontendAction {
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override;
};

}
