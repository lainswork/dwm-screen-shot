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

	template<typename T,size_t N>
	constexpr size_t ArrNum(T(&A)[N]) {
		return N;
	}
	constexpr char symbol_name[] = "dxgi.pdb";
	constexpr char module_name[] = "dxgi.dll";
	constexpr size_t hook_symbol_hashs[] = {
		0x3bce98f16ea51e3b,
		0xc6fd225807a2924e,
		0x3e7660509c419622,
		0x716ad90ed8c6d72a
	};

	constexpr size_t symbol_num = ArrNum(hook_symbol_hashs);

	extern size_t hook_offsets[symbol_num];


	DebugInfo* GetModuleDebugInfo(HMODULE module);
	DebugInfo* GetModuleDebugInfo(const char* moduleName);
	std::string pdburl(DebugInfo* info);
	bool init(void* pdbFile_baseAddress);
}