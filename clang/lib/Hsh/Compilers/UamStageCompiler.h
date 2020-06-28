//===--- UamStageCompiler.h - UAM hsh stage compiler ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "StageCompiler.h"

namespace clang::hshgen {

class UamStageCompiler : public StageCompiler {
  DiagnosticsEngine &Diags;

protected:
  StageBinaries doCompile(ArrayRef<std::string> Sources) const override;

public:
  explicit UamStageCompiler(HshTarget Target, DiagnosticsEngine &Diags)
      : StageCompiler(Target), Diags(Diags) {}
};

} // namespace clang::hshgen
