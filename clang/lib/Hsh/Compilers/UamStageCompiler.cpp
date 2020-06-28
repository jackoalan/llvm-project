//===--- UamStageCompiler.cpp - UAM hsh stage compiler --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "UamStageCompiler.h"

#include "compiler_iface.h"

namespace clang::hshgen {

namespace {
constexpr std::array<pipeline_stage, 6> ShaderProfiles{
    pipeline_stage_vertex, pipeline_stage_tess_ctrl, pipeline_stage_tess_eval,
    pipeline_stage_geometry, pipeline_stage_fragment};

template <typename T> constexpr T Align256(T x) { return (x + 0xFF) & ~0xFF; }
constexpr unsigned s_shaderStartOffset = 0x80 - sizeof(NvShaderHeader);
} // namespace

StageBinaries UamStageCompiler::doCompile(ArrayRef<std::string> Sources) const {
  StageBinaries Binaries;
  auto *OutIt = Binaries.begin();
  auto ProfileIt = ShaderProfiles.begin();
  int StageIt = 0;
  for (const auto &Stage : Sources) {
    auto &Out = OutIt++->first;
    const pipeline_stage Profile = *ProfileIt++;
    const auto HStage = HshStage(StageIt++);
    if (Stage.empty())
      continue;

    DekoCompiler Compiler(ShaderProfiles[Profile]);

    if (!Compiler.CompileGlsl(Stage.data())) {
      llvm::errs() << Stage << '\n';
      Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error,
                                         "%0 problem from deko"))
          << HshStageToString(HStage);
      continue;
    }

    auto Write = [&](void *data, size_t size) {
      std::memcpy(&*Out.insert(Out.end(), size, 0), data, size);
    };
    auto FileAlign256 = [&]() {
      Out.insert(Out.end(), Align256(Out.size()) - Out.size(), 0);
    };

    DkshHeader hdr = {};
    hdr.magic = DKSH_MAGIC;
    hdr.header_sz = sizeof(DkshHeader);
    hdr.control_sz = Align256(sizeof(DkshHeader) + sizeof(DkshProgramHeader));
    hdr.code_sz = Align256((Profile != pipeline_stage_compute ? 0x80 : 0x00) +
                           Compiler.m_codeSize) +
                  Align256(Compiler.m_dataSize);
    hdr.programs_off = sizeof(DkshHeader);
    hdr.num_programs = 1;

    Write(&hdr, sizeof(hdr));
    Write(&Compiler.m_dkph, sizeof(Compiler.m_dkph));
    FileAlign256();

    if (Profile != pipeline_stage_compute) {
      static const char s_padding[s_shaderStartOffset] =
          "lol nvidia why did you make us waste space here";
      Write((void *)s_padding, sizeof(s_padding));
      Write(&Compiler.m_nvsh, sizeof(Compiler.m_nvsh));
    }

    Write(Compiler.m_code, Compiler.m_codeSize);
    FileAlign256();

    if (Compiler.m_dataSize) {
      Write(Compiler.m_data, Compiler.m_dataSize);
      FileAlign256();
    }
  }
  return Binaries;
}

} // namespace clang::hshgen
