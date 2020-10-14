//===--- Dumper.h - Debug dumper for hshgen -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#define ENABLE_DUMP 0

#include "clang/AST/PrettyPrinter.h"
#include "clang/Hsh/HshGenerator.h"

namespace clang::hshgen {

class Dumper {
public:
#if ENABLE_DUMP
  PrintingPolicy Policy{LangOptions{}};
  static void PrintStageBits(raw_ostream &OS, StageBits Bits) {
    CommaArgPrinter ArgPrinter(OS);
    for (int i = HshVertexStage; i < HshMaxStage; ++i) {
      if ((1 << i) & Bits) {
        ArgPrinter.addArg() << HshStageToString(HshStage(i));
      }
    }
  }

  template <
      typename T,
      std::enable_if_t<!std::is_base_of_v<Stmt, std::remove_pointer_t<T>> &&
                           !std::is_base_of_v<Decl, std::remove_pointer_t<T>> &&
                           !std::is_same_v<QualType, std::decay_t<T>> &&
                           !std::is_same_v<StageBits, std::decay_t<T>> &&
                           !std::is_same_v<HshStage, std::decay_t<T>>,
                       int> = 0>
  Dumper &operator<<(const T &Obj) {
    llvm::errs() << Obj;
    return *this;
  }
  Dumper &operator<<(const Stmt *S) {
    S->printPretty(llvm::errs(), nullptr, Policy);
    return *this;
  }
  Dumper &operator<<(const Decl *D) {
    D->print(llvm::errs(), Policy);
    return *this;
  }
  Dumper &operator<<(const QualType T) {
    T.print(llvm::errs(), Policy);
    return *this;
  }
  Dumper &operator<<(const StageBits B) {
    PrintStageBits(llvm::errs(), B);
    return *this;
  }
  Dumper &operator<<(const HshStage S) {
    llvm::errs() << HshStageToString(S);
    return *this;
  }
  void setPrintingPolicy(const PrintingPolicy &PP) { Policy = PP; }
#else
  template <typename T> Dumper &operator<<(const T &Obj) { return *this; }
  void setPrintingPolicy(const PrintingPolicy &PP) {}
#endif
};

Dumper &dumper();

} // namespace clang::hshgen
