//==- raw_comment_ostream.h - raw_ostream that builds C comment --*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the raw_comment_ostream class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_RAW_COMMENT_OSTREAM_H
#define LLVM_SUPPORT_RAW_COMMENT_OSTREAM_H

#include "llvm/Support/raw_ostream.h"

namespace llvm {

/// A raw_ostream adapter that builds a block comment of the input.
class raw_comment_ostream : public raw_ostream {
  raw_ostream &OS;

  /// See raw_ostream::write_impl.
  void write_impl(const char *Ptr, size_t Size) override {
    do {
      std::string_view SV(Ptr, Size);
      auto NextNL = SV.find('\n');
      if (NextNL == std::string::npos) {
        OS.write(SV.data(), SV.size());
        break;
      } else {
        OS.write(SV.data(), NextNL);
        OS << "\n * ";
        Ptr += NextNL + 1;
        Size -= NextNL + 1;
      }
    } while (true);
  }

  uint64_t current_pos() const override { return 0; }

public:
  explicit raw_comment_ostream(raw_ostream &OS) : OS(OS) { OS << "/*\n * "; }
  ~raw_comment_ostream() override {
    flush();
    OS << "\n */\n";
  }
};

} // namespace llvm

#endif
