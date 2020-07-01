//==- raw_carray_ostream.h - raw_ostream that builds C array    --*- C++ -*-==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the raw_carray_ostream class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_RAW_CARRAY_OSTREAM_H
#define LLVM_SUPPORT_RAW_CARRAY_OSTREAM_H

#include "llvm/Support/NativeFormatting.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

/// A raw_ostream adapter that builds a C uint8_t array declaration of the
/// bytes in.
class raw_carray_ostream : public raw_ostream {
  raw_ostream &OS;
  static constexpr size_t LineSize = 12;
  size_t LineRem = 0;

  /// See raw_ostream::write_impl.
  void write_impl(const char *Ptr, size_t Size) override {
    while (Size) {
      if (LineRem == 0) {
        OS << "\n ";
        LineRem = LineSize;
      }
      const size_t ThisLineSize = std::min(Size, LineRem);
      for (size_t i = 0; i < ThisLineSize; ++i) {
        OS << ' ';
        llvm::write_hex(OS, *(const unsigned char *)Ptr++,
                        HexPrintStyle::PrefixLower, {4});
        OS << ',';
      }
      Size -= ThisLineSize;
      LineRem -= ThisLineSize;
    }
  }

  uint64_t current_pos() const override { return 0; }

public:
  explicit raw_carray_ostream(raw_ostream &OS, StringRef Name,
                              StringRef AttrString = {})
      : OS(OS) {
    OS << "const uint8_t " << Name << "[]";
    if (!AttrString.empty())
      OS << '\n' << AttrString;
    OS << " = {";
  }
  ~raw_carray_ostream() override {
    flush();
    OS << "\n};";
  }
};

/// A raw_ostream adapter that builds a C uint32_t array declaration of the
/// bytes in.
class raw_carray32_ostream {
  raw_ostream &OS;
  static constexpr size_t LineSize = 6;
  size_t LineRem = 0;

  void write_impl(const uint32_t *Ptr, size_t Size) {
    while (Size) {
      if (LineRem == 0) {
        OS << "\n ";
        LineRem = LineSize;
      }
      const size_t ThisLineSize = std::min(Size, LineRem);
      for (size_t i = 0; i < ThisLineSize; ++i) {
        OS << ' ';
        llvm::write_hex(OS, *Ptr++, HexPrintStyle::PrefixLower, {10});
        OS << ',';
      }
      Size -= ThisLineSize;
      LineRem -= ThisLineSize;
    }
  }

public:
  explicit raw_carray32_ostream(raw_ostream &OS, StringRef Name,
                                StringRef AttrString = {})
      : OS(OS) {
    OS << "const uint32_t " << Name << "[]";
    if (!AttrString.empty())
      OS << '\n' << AttrString;
    OS << " = {";
  }
  ~raw_carray32_ostream() {
    OS << "\n};";
  }
  void write(const uint32_t *Ptr, size_t Size) {
    write_impl(Ptr, Size);
  }
};

} // namespace llvm

#endif
