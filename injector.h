#pragma once

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <TlHelp32.h>
#include <memory>

namespace Injector
{

	constexpr WORD ImageSignature = 0x5A4D;

	typedef HINSTANCE(WINAPI* f_LoadLibraryA)(const char* lpLibFilename);
	typedef UINT_PTR(WINAPI* f_GetProcAddress)(HINSTANCE hModule, const char* lpProcName);
	typedef BOOL(WINAPI* f_DllEntryPoint)(void* hDll, DWORD dwReason, void* pReserved);

	enum class InjectResult : int8_t
	{
		Unknown = -1,
		Success = 0,
		DllNotFound = 1,
		InvalidDll,
		FailedToReadFile,
		InvalidFileSize,
		InvalidArchitecture,
		FailedToAllocateMemory,
		FailedToMapSections,
		ThreadCreationFailed
	};

	struct MANUAL_MAPPING_DATA
	{
		f_LoadLibraryA     pLoadLibraryA;
		f_GetProcAddress   pGetProcAddress;
		HINSTANCE          hModule;
	};

#define RELOC_FLAG32(RelativeInfo) (((RelativeInfo) >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)
#define RELOC_FLAG64(RelativeInfo) (((RelativeInfo) >> 0x0C) == IMAGE_REL_BASED_DIR64)

#ifdef _WIN64
#define RELOC_FLAG RELOC_FLAG64
#else
#define RELOC_FLAG RELOC_FLAG32
#endif

	void __stdcall ShellCode(MANUAL_MAPPING_DATA* pData);

	HANDLE OpenProcessHandle(DWORD pId);
	DWORD FindProcessId(const char* ProcessExeFile);

	InjectResult InternalManualMap(HANDLE hProc, const char* szDllFile);
	InjectResult ManualMap(HANDLE hProc, const char* szDllFile);

	InjectResult InternalNativeInject(HANDLE hProc, const char* szDllFile);
	InjectResult NativeInject(HANDLE hProc, const char* szDllFile, bool CloseHandleAfterInject = true);
}