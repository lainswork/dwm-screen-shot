#include "dwm_symbol.h"
#include "PDB.h"
#include "PDB_DBIStream.h"
#include "PDB_InfoStream.h"
#include "PDB_RawFile.h"
#include <Windows.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

#define PrintErro(text) MessageBoxA((HWND)0, text, "Erro", MB_OK | MB_TOPMOST)

namespace dwm_symbol {

size_t hook_offsets[symbol_num] = {0};

namespace {
PDB_NO_DISCARD static bool IsError(PDB::ErrorCode errorCode) {

    switch (errorCode) {
    case PDB::ErrorCode::Success:
        return false;

    case PDB::ErrorCode::InvalidSuperBlock:
        PrintErro("Invalid Superblock\n");
        return true;

    case PDB::ErrorCode::InvalidFreeBlockMap:
        PrintErro("Invalid free block map\n");
        return true;

    case PDB::ErrorCode::InvalidSignature:
        PrintErro("Invalid stream signature\n");
        return true;

    case PDB::ErrorCode::InvalidStreamIndex:
        PrintErro("Invalid stream index\n");
        return true;

    case PDB::ErrorCode::UnknownVersion:
        PrintErro("Unknown version\n");
        return true;
    }

    // only ErrorCode::Success means there wasn't an error, so all other paths
    // have to assume there was an error
    return true;
}

PDB_NO_DISCARD static bool HasValidDBIStreams(const PDB::RawFile &rawPdbFile, const PDB::DBIStream &dbiStream) {
    // check whether the DBI stream offers all sub-streams we need
    if (IsError(dbiStream.HasValidImageSectionStream(rawPdbFile))) {
        return false;
    }

    if (IsError(dbiStream.HasValidPublicSymbolStream(rawPdbFile))) {
        return false;
    }

    if (IsError(dbiStream.HasValidGlobalSymbolStream(rawPdbFile))) {
        return false;
    }

    if (IsError(dbiStream.HasValidSectionContributionStream(rawPdbFile))) {
        return false;
    }

    return true;
}

} // namespace
DebugInfo *GetModuleDebugInfo(HMODULE module) {
    IMAGE_DOS_HEADER *pDos = (IMAGE_DOS_HEADER *)module;
    IMAGE_NT_HEADERS *pNt  = (IMAGE_NT_HEADERS *)((CHAR *)module + pDos->e_lfanew);

    auto pDebug =
        (IMAGE_DEBUG_DIRECTORY *)(pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress +
                                  (CHAR *)module);

    auto pDebugInfo = (DebugInfo *)(pDebug->AddressOfRawData + (CHAR *)module);

    return pDebugInfo;
}
DebugInfo *GetModuleDebugInfo(const char *moduleName) {
    auto image_base = GetModuleHandleA(moduleName);
    if (image_base == nullptr) {
        return nullptr;
    }
    return GetModuleDebugInfo(image_base);
}
std::string pdburl(DebugInfo *pdb_info) {
    wchar_t w_GUID[100]{0};
    if (!StringFromGUID2(pdb_info->Guid, w_GUID, 100)) {
        return {};
    }

    char   a_GUID[100]{0};
    size_t l_GUID = 0;
    if (wcstombs_s(&l_GUID, a_GUID, w_GUID, sizeof(a_GUID)) || !l_GUID) {
        return {};
    }

    std::string guid_filtered;
    for (UINT i = 0; i != l_GUID; ++i) {
        if ((a_GUID[i] >= '0' && a_GUID[i] <= '9') || (a_GUID[i] >= 'A' && a_GUID[i] <= 'F') ||
            (a_GUID[i] >= 'a' && a_GUID[i] <= 'f')) {
            guid_filtered += a_GUID[i];
        }
    }

    char age[3]{0};
    _itoa_s(pdb_info->Age, age, 10);

    std::string url = "https://msdl.microsoft.com/download/symbols/";
    url += pdb_info->PdbFileName;
    url += '/';
    url += guid_filtered;
    url += age;
    url += '/';
    url += pdb_info->PdbFileName;
    return url;
}
size_t find_sym_rva(const PDB::RawFile &rawPdbFile, const PDB::DBIStream &dbiStream, size_t symbol_hash) {

    const PDB::ImageSectionStream imageSectionStream            = dbiStream.CreateImageSectionStream(rawPdbFile);
    const PDB::ModuleInfoStream   moduleInfoStream              = dbiStream.CreateModuleInfoStream(rawPdbFile);
    const PDB::ArrayView<PDB::ModuleInfoStream::Module> modules = moduleInfoStream.GetModules();
    const PDB::PublicSymbolStream         publicSymbolStream    = dbiStream.CreatePublicSymbolStream(rawPdbFile);
    const PDB::CoalescedMSFStream         symbolRecordStream    = dbiStream.CreateSymbolRecordStream(rawPdbFile);
    const PDB::ArrayView<PDB::HashRecord> hashRecords           = publicSymbolStream.GetRecords();

    std::hash<std::string> strhash;

    for (const PDB::HashRecord &hashRecord : hashRecords) {
        const PDB::CodeView::DBI::Record *record = publicSymbolStream.GetRecord(symbolRecordStream, hashRecord);
        if ((PDB_AS_UNDERLYING(record->data.S_PUB32.flags) &
             PDB_AS_UNDERLYING(PDB::CodeView::DBI::PublicSymbolFlags::Function)) == 0u)
            continue;

        const uint32_t rva =
            imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_PUB32.section, record->data.S_PUB32.offset);
        if (rva == 0u)
            continue;

        if (record->data.S_PUB32.name) {
            if (strhash(record->data.S_PUB32.name) == symbol_hash) {
                return static_cast<size_t>(rva);
            }
        }
    }

    size_t _ret = 0;
    for (const PDB::ModuleInfoStream::Module &module : modules) {
        if (!module.HasSymbolStream()) {
            continue;
        }
        const PDB::ModuleSymbolStream moduleSymbolStream = module.CreateSymbolStream(rawPdbFile);
        moduleSymbolStream.ForEachSymbol(
            [symbol_hash, &_ret, &strhash, &imageSectionStream](const PDB::CodeView::DBI::Record *record) {
                // only grab function symbols from the module streams
                const char *name = nullptr;
                uint32_t    rva  = 0u;
                uint32_t    size = 0u;
                if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_THUNK32) {
                    if (record->data.S_THUNK32.thunk == PDB::CodeView::DBI::ThunkOrdinal::TrampolineIncremental) {
                        // we have never seen incremental linking thunks stored inside a
                        // S_THUNK32 symbol, but better safe than sorry
                        name = "ILT";
                        rva  = imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_THUNK32.section,
                                                                           record->data.S_THUNK32.offset);
                        size = 5u;
                    }
                } else if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_TRAMPOLINE) {
                    // incremental linking thunks are stored in the linker module
                    name = "ILT";
                    rva  = imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_TRAMPOLINE.thunkSection,
                                                                       record->data.S_TRAMPOLINE.thunkOffset);
                    size = 5u;
                } else if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32) {
                    name = record->data.S_LPROC32.name;
                    rva  = imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_LPROC32.section,
                                                                       record->data.S_LPROC32.offset);
                    size = record->data.S_LPROC32.codeSize;
                } else if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32) {
                    name = record->data.S_GPROC32.name;
                    rva  = imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_GPROC32.section,
                                                                       record->data.S_GPROC32.offset);
                    size = record->data.S_GPROC32.codeSize;
                } else if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_LPROC32_ID) {
                    name = record->data.S_LPROC32_ID.name;
                    rva  = imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_LPROC32_ID.section,
                                                                       record->data.S_LPROC32_ID.offset);
                    size = record->data.S_LPROC32_ID.codeSize;
                } else if (record->header.kind == PDB::CodeView::DBI::SymbolRecordKind::S_GPROC32_ID) {
                    name = record->data.S_GPROC32_ID.name;
                    rva  = imageSectionStream.ConvertSectionOffsetToRVA(record->data.S_GPROC32_ID.section,
                                                                       record->data.S_GPROC32_ID.offset);
                    size = record->data.S_GPROC32_ID.codeSize;
                }
                if (name) {
                    // file << name << "\n";
                    if (strhash(name) == symbol_hash) {
                        _ret = rva;
                    }
                }

                if (rva == 0u)
                    return;
            });
    }
    return _ret;
}

bool init(void *pdbFile_baseAddress) {
    // std::hash<std::string> strhash;

    if (IsError(PDB::ValidateFile(pdbFile_baseAddress))) {
        false;
    }

    const PDB::RawFile rawPdbFile = PDB::CreateRawFile(pdbFile_baseAddress);
    if (IsError(PDB::HasValidDBIStream(rawPdbFile))) {
        false;
    }

    const PDB::InfoStream infoStream(rawPdbFile);
    if (infoStream.UsesDebugFastLink()) {
        PrintErro("PDB was linked using unsupported option /DEBUG:FASTLINK\n");

        false;
    }

    const PDB::DBIStream dbiStream = PDB::CreateDBIStream(rawPdbFile);
    if (!HasValidDBIStreams(rawPdbFile, dbiStream)) {
        false;
    }

    for (size_t idx = 0; idx < symbol_num; idx++)
        hook_offsets[idx] = find_sym_rva(rawPdbFile, dbiStream, hook_symbol_hashs[idx]);

    for (size_t idx = 0; idx < symbol_num; idx++)
        if (hook_offsets[idx] == 0)
            return false;

    return true;
}

} // namespace dwm_symbol