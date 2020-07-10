//===- COFFObjcopy.cpp ----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "COFFObjcopy.h"
#include "Buffer.h"
#include "CopyConfig.h"
#include "Object.h"
#include "Reader.h"
#include "Writer.h"
#include "llvm-objcopy.h"

#include "llvm/DebugInfo/CodeView/RecordName.h"
#include "llvm/DebugInfo/CodeView/SymbolDeserializer.h"
#include "llvm/DebugInfo/PDB/Native/DbiStream.h"
#include "llvm/DebugInfo/PDB/Native/NativeSession.h"
#include "llvm/DebugInfo/PDB/Native/SymbolStream.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/COFF.h"
#include "llvm/Support/CRC.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Path.h"
#include <cassert>

namespace llvm {
namespace objcopy {
namespace coff {

using namespace object;
using namespace COFF;
using namespace codeview;
using namespace pdb;
using namespace support::endian;

static bool isDebugSection(const Section &Sec) {
  return Sec.Name.startswith(".debug");
}

static uint64_t getNextRVA(const Object &Obj) {
  if (Obj.getSections().empty())
    return 0;
  const Section &Last = Obj.getSections().back();
  return alignTo(Last.Header.VirtualAddress + Last.Header.VirtualSize,
                 Obj.IsPE ? Obj.PeHeader.SectionAlignment : 1);
}

static std::vector<uint8_t> createGnuDebugLinkSectionContents(StringRef File) {
  ErrorOr<std::unique_ptr<MemoryBuffer>> LinkTargetOrErr =
      MemoryBuffer::getFile(File);
  if (!LinkTargetOrErr)
    error("'" + File + "': " + LinkTargetOrErr.getError().message());
  auto LinkTarget = std::move(*LinkTargetOrErr);
  uint32_t CRC32 = llvm::crc32(arrayRefFromStringRef(LinkTarget->getBuffer()));

  StringRef FileName = sys::path::filename(File);
  size_t CRCPos = alignTo(FileName.size() + 1, 4);
  std::vector<uint8_t> Data(CRCPos + 4);
  memcpy(Data.data(), FileName.data(), FileName.size());
  support::endian::write32le(Data.data() + CRCPos, CRC32);
  return Data;
}

// Adds named section with given contents to the object.
static void addSection(Object &Obj, StringRef Name, ArrayRef<uint8_t> Contents,
                       uint32_t Characteristics) {
  bool NeedVA = Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                                   IMAGE_SCN_MEM_WRITE);

  Section Sec;
  Sec.setOwnedContents(Contents);
  Sec.Name = Name;
  Sec.Header.VirtualSize = NeedVA ? Sec.getContents().size() : 0u;
  Sec.Header.VirtualAddress = NeedVA ? getNextRVA(Obj) : 0u;
  Sec.Header.SizeOfRawData =
      NeedVA ? alignTo(Sec.Header.VirtualSize,
                       Obj.IsPE ? Obj.PeHeader.FileAlignment : 1)
             : Sec.getContents().size();
  // Sec.Header.PointerToRawData is filled in by the writer.
  Sec.Header.PointerToRelocations = 0;
  Sec.Header.PointerToLinenumbers = 0;
  // Sec.Header.NumberOfRelocations is filled in by the writer.
  Sec.Header.NumberOfLinenumbers = 0;
  Sec.Header.Characteristics = Characteristics;

  Obj.addSections(Sec);
}

static void addGnuDebugLink(Object &Obj, StringRef DebugLinkFile) {
  std::vector<uint8_t> Contents =
      createGnuDebugLinkSectionContents(DebugLinkFile);
  addSection(Obj, ".gnu_debuglink", Contents,
             IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |
                 IMAGE_SCN_MEM_DISCARDABLE);
}

static void setSectionFlags(Section &Sec, SectionFlag AllFlags) {
  // Need to preserve alignment flags.
  const uint32_t PreserveMask =
      IMAGE_SCN_ALIGN_1BYTES | IMAGE_SCN_ALIGN_2BYTES | IMAGE_SCN_ALIGN_4BYTES |
      IMAGE_SCN_ALIGN_8BYTES | IMAGE_SCN_ALIGN_16BYTES |
      IMAGE_SCN_ALIGN_32BYTES | IMAGE_SCN_ALIGN_64BYTES |
      IMAGE_SCN_ALIGN_128BYTES | IMAGE_SCN_ALIGN_256BYTES |
      IMAGE_SCN_ALIGN_512BYTES | IMAGE_SCN_ALIGN_1024BYTES |
      IMAGE_SCN_ALIGN_2048BYTES | IMAGE_SCN_ALIGN_4096BYTES |
      IMAGE_SCN_ALIGN_8192BYTES;

  // Setup new section characteristics based on the flags provided in command
  // line.
  uint32_t NewCharacteristics =
      (Sec.Header.Characteristics & PreserveMask) | IMAGE_SCN_MEM_READ;

  if ((AllFlags & SectionFlag::SecAlloc) && !(AllFlags & SectionFlag::SecLoad))
    NewCharacteristics |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;
  if (AllFlags & SectionFlag::SecNoload)
    NewCharacteristics |= IMAGE_SCN_LNK_REMOVE;
  if (!(AllFlags & SectionFlag::SecReadonly))
    NewCharacteristics |= IMAGE_SCN_MEM_WRITE;
  if (AllFlags & SectionFlag::SecDebug)
    NewCharacteristics |=
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_DISCARDABLE;
  if (AllFlags & SectionFlag::SecCode)
    NewCharacteristics |= IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
  if (AllFlags & SectionFlag::SecData)
    NewCharacteristics |= IMAGE_SCN_CNT_INITIALIZED_DATA;
  if (AllFlags & SectionFlag::SecShare)
    NewCharacteristics |= IMAGE_SCN_MEM_SHARED;
  if (AllFlags & SectionFlag::SecExclude)
    NewCharacteristics |= IMAGE_SCN_LNK_REMOVE;

  Sec.Header.Characteristics = NewCharacteristics;
}

static void applyRelX64(uint8_t *Off, uint16_t Type, uint64_t S,
                        uint64_t P) {
  switch (Type) {
  case IMAGE_REL_AMD64_ADDR32:
    write32le(Off, S);
    break;
  case IMAGE_REL_AMD64_ADDR64:
    write64le(Off, S);
    break;
  case IMAGE_REL_AMD64_ADDR32NB:
    write32le(Off, S);
    break;
  case IMAGE_REL_AMD64_REL32:
    write32le(Off, S - P - 4);
    break;
  case IMAGE_REL_AMD64_REL32_1:
    write32le(Off, S - P - 5);
    break;
  case IMAGE_REL_AMD64_REL32_2:
    write32le(Off, S - P - 6);
    break;
  case IMAGE_REL_AMD64_REL32_3:
    write32le(Off, S - P - 7);
    break;
  case IMAGE_REL_AMD64_REL32_4:
    write32le(Off, S - P - 8);
    break;
  case IMAGE_REL_AMD64_REL32_5:
    write32le(Off, S - P - 9);
    break;
  default:
    error("unsupported relocation type 0x" + Twine::utohexstr(Type));
  }
}

static void moveSection(Section &RefSec, Object &Obj, DbiStream &Dbi,
                        uint32_t NewBase,
                        const std::function<void(uint32_t)> &RemoveBaseReloc) {
  const uint32_t SecVirtualAddress = RefSec.Header.VirtualAddress;
  const uint32_t SecVirtualSize = RefSec.Header.VirtualSize;

  Section *LastApplySec = nullptr;
  auto FindSectionOfRVA = [&](uint32_t RVA) -> Section * {
    if (!LastApplySec || RVA < LastApplySec->Header.VirtualAddress ||
        RVA >= LastApplySec->Header.VirtualAddress +
                   LastApplySec->Header.VirtualSize) {
      for (auto &Sec : Obj.getMutableSections()) {
        if (RVA >= Sec.Header.VirtualAddress &&
            RVA < Sec.Header.VirtualAddress + Sec.Header.VirtualSize) {
          LastApplySec = &Sec;
          return LastApplySec;
        }
      }
      return nullptr;
    }
    return LastApplySec;
  };

  for (const auto &Fixup : Dbi.getFixupRecords()) {
    if (Fixup.RVATarget >= SecVirtualAddress &&
        Fixup.RVATarget < SecVirtualAddress + SecVirtualSize) {
      if (Section *ApplySec = FindSectionOfRVA(Fixup.RVA)) {
        uint8_t *SecData = ApplySec->mutableContents().data();
        uint64_t Offset = Fixup.RVA - ApplySec->Header.VirtualAddress;
        uint8_t *Data = SecData + Offset;
        // TODO: Other architectures
        applyRelX64(Data, Fixup.Type,
                    Fixup.RVATarget - SecVirtualAddress + NewBase, Fixup.RVA);
        if (RemoveBaseReloc)
          RemoveBaseReloc(Fixup.RVA);
      }
    }
  }
}

static Error handleArgs(const CopyConfig &Config, Object &Obj,
                        COFFObjectFile &ObjFile) {
  SmallVector<ssize_t, 16> SecIdsToRemove;

  if (Config.HshObjcopy) {
    StringRef PdbPath;
    const DebugInfo *PdbInfo = nullptr;
    if (Error E = ObjFile.getDebugPDBInfo(PdbInfo, PdbPath))
      return E;

    std::unique_ptr<IPDBSession> PDBSession;
    if (Error E = NativeSession::createFromPdbPath(PdbPath, PDBSession))
      return E;

    NativeSession &PDBNative = static_cast<NativeSession &>(*PDBSession);
    auto Dbi = PDBNative.getPDBFile().getPDBDbiStream();
    if (!Dbi)
      return createStringError(inconvertibleErrorCode(),
                               "DBI stream not present in PDB");

    if (!Dbi->hasFixupRecords())
      return createStringError(inconvertibleErrorCode(),
                               "Fixup stream not present in PDB");

    auto Symbols = PDBNative.getPDBFile().getPDBSymbolStream();
    if (!Symbols)
      return createStringError(inconvertibleErrorCode(),
                               "Symbol stream not present in PDB");

    // Set __hsh_objcopy to 1 and find offsets table
    bool FoundSym = false;
    ssize_t HshOffsetsSecId = -1;
    uint32_t HshOffsetsSecOffset = 0;
    for (const auto &Sym : Symbols->getSymbolArray()) {
      if (Sym.kind() == S_PUB32) {
        StringRef Name = getSymbolName(Sym);
        if (Name == "__hsh_objcopy") {
          auto Pub =
              cantFail(SymbolDeserializer::deserializeAs<PublicSym32>(Sym));
          if (auto *Sec = const_cast<Section *>(Obj.findSection(Pub.Segment))) {
            Sec->mutableContents()[Pub.Offset] = 1;
            FoundSym = true;
            if (HshOffsetsSecId != -1)
              break;
          }
        } else if (Name == "__hsh_offsets") {
          auto Pub =
              cantFail(SymbolDeserializer::deserializeAs<PublicSym32>(Sym));
          if (auto *Sec = const_cast<Section *>(Obj.findSection(Pub.Segment))) {
            HshOffsetsSecId = Sec->UniqueId;
            HshOffsetsSecOffset = Pub.Offset;
            if (FoundSym)
              break;
          }
        }
      }
    }
    if (!FoundSym)
      return createStringError(object_error::parse_failed,
                               "'__hsh_objcopy' symbol not found");
    if (HshOffsetsSecId == -1)
      return createStringError(object_error::parse_failed,
                               "'__hsh_offsets' symbol not found");

    Obj.HshSectionsTableSectionId = HshOffsetsSecId;

    std::function<void(uint32_t)> RemoveBaseReloc{};
    for (auto &Sec : Obj.getMutableSections()) {
      if (Sec.Name == ".reloc") {
        RemoveBaseReloc = [RelocData = Sec.mutableContents()](uint32_t RVA) {
          uint8_t *Ptr = RelocData.data();
          uint8_t *End = Ptr + RelocData.size();
          while (Ptr < End) {
            uint32_t PageRVA = read32le(Ptr);
            uint32_t SizeOfBlock = read32le(Ptr + 4);
            uint8_t *BlockEnd = Ptr + SizeOfBlock;
            if ((RVA & 0xfffff000) == PageRVA) {
              Ptr += 8;
              while (Ptr < End && Ptr < BlockEnd) {
                uint16_t Entry = read16le(Ptr);
                if (Entry >> 12) {
                  uint32_t TestRVA = PageRVA + (Entry & 0xfff);
                  if (TestRVA == RVA) {
                    std::move(Ptr + 2, BlockEnd, Ptr);
                    write16le(BlockEnd - 2, 0);
                    return;
                  }
                }
                Ptr += 2;
              }
              return;
            } else {
              Ptr = BlockEnd;
            }
          }
        };
        break;
      }
    }

    uint32_t NewSectionBaseAddr = UINT32_MAX;
    for (auto &Sec : Obj.getMutableSections()) {
      if (Sec.Name.startswith(".hsh")) {
        // Process hsh sections and mark for removal
        if (NewSectionBaseAddr == UINT32_MAX)
          NewSectionBaseAddr = Sec.Header.VirtualAddress;

        uint32_t HshIdx = 0;
        if (!Sec.Name.drop_front(4).getAsInteger(10, HshIdx)) {
          if (!Sec.getContents().empty()) {
            moveSection(Sec, Obj, *Dbi, 0, RemoveBaseReloc);
            Obj.HshSections.push_back(
                {Sec.getContents(), uint32_t(HshOffsetsSecOffset +
                                             HshIdx * 2 * sizeof(uint32_t))});
            SecIdsToRemove.push_back(Sec.UniqueId);
          }
        }
      } else if (NewSectionBaseAddr != UINT32_MAX) {
        // Re-relocate sections after hsh sections
        moveSection(Sec, Obj, *Dbi, NewSectionBaseAddr, {});

        // Fix any shifted directory RVAs
        for (auto &Dir : Obj.DataDirectories) {
          if (Dir.Size > 0 &&
              Dir.RelativeVirtualAddress >= Sec.Header.VirtualAddress &&
              Dir.RelativeVirtualAddress <
                  Sec.Header.VirtualAddress + Sec.Header.VirtualSize) {
            Dir.RelativeVirtualAddress =
                NewSectionBaseAddr +
                (Dir.RelativeVirtualAddress - Sec.Header.VirtualAddress);
          }
        }

        // Assign new section RVA
        Sec.Header.VirtualAddress = NewSectionBaseAddr;

        // Next section
        NewSectionBaseAddr =
            alignTo(NewSectionBaseAddr + Sec.Header.VirtualSize,
                    Obj.PeHeader.SectionAlignment);
      }
    }
  }

  // Perform the actual section removals.
  Obj.removeSections([&Config, &SecIdsToRemove](const Section &Sec) {
    // Contrary to --only-keep-debug, --only-section fully removes sections that
    // aren't mentioned.
    if (!Config.OnlySection.empty() && !Config.OnlySection.matches(Sec.Name))
      return true;

    if (Config.StripDebug || Config.StripAll || Config.StripAllGNU ||
        Config.DiscardMode == DiscardType::All || Config.StripUnneeded) {
      if (isDebugSection(Sec) &&
          (Sec.Header.Characteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0)
        return true;
    }

    if (Config.ToRemove.matches(Sec.Name))
      return true;

    if (std::find(SecIdsToRemove.begin(), SecIdsToRemove.end(), Sec.UniqueId) !=
        SecIdsToRemove.end())
      return true;

    return false;
  });

  if (Config.OnlyKeepDebug) {
    // For --only-keep-debug, we keep all other sections, but remove their
    // content. The VirtualSize field in the section header is kept intact.
    Obj.truncateSections([](const Section &Sec) {
      return !isDebugSection(Sec) && Sec.Name != ".buildid" &&
             ((Sec.Header.Characteristics &
               (IMAGE_SCN_CNT_CODE | IMAGE_SCN_CNT_INITIALIZED_DATA)) != 0);
    });
  }

  // StripAll removes all symbols and thus also removes all relocations.
  if (Config.StripAll || Config.StripAllGNU)
    for (Section &Sec : Obj.getMutableSections())
      Sec.Relocs.clear();

  // If we need to do per-symbol removals, initialize the Referenced field.
  if (Config.StripUnneeded || Config.DiscardMode == DiscardType::All ||
      !Config.SymbolsToRemove.empty())
    if (Error E = Obj.markSymbols())
      return E;

  for (Symbol &Sym : Obj.getMutableSymbols()) {
    auto I = Config.SymbolsToRename.find(Sym.Name);
    if (I != Config.SymbolsToRename.end())
      Sym.Name = I->getValue();
  }

  // Actually do removals of symbols.
  Obj.removeSymbols([&](const Symbol &Sym) {
    // For StripAll, all relocations have been stripped and we remove all
    // symbols.
    if (Config.StripAll || Config.StripAllGNU)
      return true;

    if (Config.SymbolsToRemove.matches(Sym.Name)) {
      // Explicitly removing a referenced symbol is an error.
      if (Sym.Referenced)
        reportError(Config.OutputFilename,
                    createStringError(llvm::errc::invalid_argument,
                                      "not stripping symbol '%s' because it is "
                                      "named in a relocation",
                                      Sym.Name.str().c_str()));
      return true;
    }

    if (!Sym.Referenced) {
      // With --strip-unneeded, GNU objcopy removes all unreferenced local
      // symbols, and any unreferenced undefined external.
      // With --strip-unneeded-symbol we strip only specific unreferenced
      // local symbol instead of removing all of such.
      if (Sym.Sym.StorageClass == IMAGE_SYM_CLASS_STATIC ||
          Sym.Sym.SectionNumber == 0)
        if (Config.StripUnneeded ||
            Config.UnneededSymbolsToRemove.matches(Sym.Name))
          return true;

      // GNU objcopy keeps referenced local symbols and external symbols
      // if --discard-all is set, similar to what --strip-unneeded does,
      // but undefined local symbols are kept when --discard-all is set.
      if (Config.DiscardMode == DiscardType::All &&
          Sym.Sym.StorageClass == IMAGE_SYM_CLASS_STATIC &&
          Sym.Sym.SectionNumber != 0)
        return true;
    }

    return false;
  });

  if (!Config.SetSectionFlags.empty())
    for (Section &Sec : Obj.getMutableSections()) {
      const auto It = Config.SetSectionFlags.find(Sec.Name);
      if (It != Config.SetSectionFlags.end())
        setSectionFlags(Sec, It->second.NewFlags);
    }

  for (const auto &Flag : Config.AddSection) {
    StringRef SecName, FileName;
    std::tie(SecName, FileName) = Flag.split("=");

    auto BufOrErr = MemoryBuffer::getFile(FileName);
    if (!BufOrErr)
      return createFileError(FileName, errorCodeToError(BufOrErr.getError()));
    auto Buf = std::move(*BufOrErr);

    addSection(
        Obj, SecName,
        makeArrayRef(reinterpret_cast<const uint8_t *>(Buf->getBufferStart()),
                     Buf->getBufferSize()),
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_1BYTES);
  }

  if (!Config.AddGnuDebugLink.empty())
    addGnuDebugLink(Obj, Config.AddGnuDebugLink);

  if (Config.AllowBrokenLinks || !Config.BuildIdLinkDir.empty() ||
      Config.BuildIdLinkInput || Config.BuildIdLinkOutput ||
      !Config.SplitDWO.empty() || !Config.SymbolsPrefix.empty() ||
      !Config.AllocSectionsPrefix.empty() || !Config.DumpSection.empty() ||
      !Config.KeepSection.empty() || Config.NewSymbolVisibility ||
      !Config.SymbolsToGlobalize.empty() || !Config.SymbolsToKeep.empty() ||
      !Config.SymbolsToLocalize.empty() || !Config.SymbolsToWeaken.empty() ||
      !Config.SymbolsToKeepGlobal.empty() || !Config.SectionsToRename.empty() ||
      !Config.SetSectionAlignment.empty() || Config.ExtractDWO ||
      Config.LocalizeHidden || Config.PreserveDates || Config.StripDWO ||
      Config.StripNonAlloc || Config.StripSections ||
      Config.StripSwiftSymbols || Config.Weaken ||
      Config.DecompressDebugSections ||
      Config.DiscardMode == DiscardType::Locals ||
      !Config.SymbolsToAdd.empty() || Config.EntryExpr) {
    return createStringError(llvm::errc::invalid_argument,
                             "option not supported by llvm-objcopy for COFF");
  }

  return Error::success();
}

Error executeObjcopyOnBinary(const CopyConfig &Config, COFFObjectFile &In,
                             Buffer &Out) {
  COFFReader Reader(In);
  Expected<std::unique_ptr<Object>> ObjOrErr = Reader.create();
  if (!ObjOrErr)
    return createFileError(Config.InputFilename, ObjOrErr.takeError());
  Object *Obj = ObjOrErr->get();
  assert(Obj && "Unable to deserialize COFF object");
  if (Error E = handleArgs(Config, *Obj, In))
    return createFileError(Config.InputFilename, std::move(E));
  COFFWriter Writer(*Obj, Out);
  if (Error E = Writer.write())
    return createFileError(Config.OutputFilename, std::move(E));
  return Error::success();
}

} // end namespace coff
} // end namespace objcopy
} // end namespace llvm
