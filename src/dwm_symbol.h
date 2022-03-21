#pragma once
#include <iostream>
#include <Windows.h>
namespace dwm_symbol{
	struct DebugInfo
	{
		DWORD	Signature;
		GUID	Guid;
		DWORD	Age;
		char	PdbFileName[1];
	};
	extern size_t hook_offset;
	DebugInfo* GetModuleDebugInfo(HMODULE module);
	DebugInfo* GetModuleDebugInfo(const char* moduleName);
	std::string pdburl(DebugInfo* info);
	bool init(void* pdbFile_baseAddress);
}