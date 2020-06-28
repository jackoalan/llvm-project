//===--- DxcStageCompiler.h - DXC hsh stage compiler ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "StageCompiler.h"

namespace clang::hshgen {

class DxcStageCompiler : public StageCompiler {
public:
  using StageCompiler::StageCompiler;
  static std::unique_ptr<DxcStageCompiler>
  Create(HshTarget Target, bool DebugInfo, StringRef ResourceDir,
         DiagnosticsEngine &Diags, HshBuiltins &Builtins);
};

} // namespace clang::hshgen
