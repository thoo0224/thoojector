#pragma once

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <TlHelp32.h>
#include <memory>

namespace Injector
{

	#define API __declspec(dllexport)

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

	inline void __stdcall ShellCode(MANUAL_MAPPING_DATA* pData)
	{
		if (!pData)
			return;

		BYTE* pBase = reinterpret_cast<BYTE*>(pData);
		auto* pOpt = &reinterpret_cast<IMAGE_NT_HEADERS*>(pBase + reinterpret_cast<IMAGE_DOS_HEADER*>(pData)->e_lfanew)->OptionalHeader;

		f_LoadLibraryA   _LoadLibraryA = pData->pLoadLibraryA;
		f_GetProcAddress _GetProcAddress = pData->pGetProcAddress;
		f_DllEntryPoint  _DllMain = reinterpret_cast<f_DllEntryPoint>(pBase + pOpt->AddressOfEntryPoint);

		if (BYTE* LocationDelta = pBase - pOpt->ImageBase)
		{
			if (!pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
				return;

			IMAGE_BASE_RELOCATION* pRelocData
				= reinterpret_cast<IMAGE_BASE_RELOCATION*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
			while (pRelocData->VirtualAddress)
			{
				uint32_t NumEntries = (pRelocData->SizeOfBlock - sizeof IMAGE_BASE_RELOCATION) / sizeof WORD;
				WORD* pRelativeInfo = reinterpret_cast<WORD*>(pRelocData + 1);
				for (uint32_t i = 0; i != NumEntries; ++i, ++pRelativeInfo)
				{
					if (RELOC_FLAG(*pRelativeInfo))
					{
						UINT_PTR* pPatch = reinterpret_cast<UINT_PTR*>(pBase + pRelocData->VirtualAddress + (*pRelativeInfo & 0xFF));
						*pPatch += reinterpret_cast<UINT_PTR>(LocationDelta);
					}

					pRelocData = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(pRelocData) + pRelocData->SizeOfBlock);
				}
			}
		}

		if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
		{
			auto* pImportDescriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
			while (pImportDescriptor->Name)
			{
				char* szModule = reinterpret_cast<char*>(pBase + pImportDescriptor->Name);
				HINSTANCE hDll = _LoadLibraryA(szModule);

				uint64_t* pThunkRef = reinterpret_cast<uint64_t*>(pBase + pImportDescriptor->OriginalFirstThunk);
				uint64_t* pFuncRef = reinterpret_cast<uint64_t*>(pBase + pImportDescriptor->FirstThunk);

				if (!pThunkRef)
					pThunkRef = pFuncRef;

				for (; *pThunkRef; ++pThunkRef, ++pFuncRef)
				{
					if (IMAGE_SNAP_BY_ORDINAL(*pThunkRef))
					{
						*pFuncRef = _GetProcAddress(hDll, reinterpret_cast<char*>(*pThunkRef & 0xFFFF));
					}
					else
					{
						IMAGE_IMPORT_BY_NAME* pImport = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pBase + *pThunkRef);
						*pFuncRef = _GetProcAddress(hDll, pImport->Name);
					}
				}

				++pImportDescriptor;
			}
		}

		if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
		{
			auto* pTls = reinterpret_cast<IMAGE_TLS_DIRECTORY*>(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
			auto* pCallback = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pTls->AddressOfCallBacks);
			for (; pCallback && *pCallback; ++pCallback)
			{
				(*pCallback)(pBase, DLL_PROCESS_ATTACH, nullptr);
			}
		}

		_DllMain(pBase, DLL_PROCESS_ATTACH, nullptr);
		pData->hModule = reinterpret_cast<HINSTANCE>(pBase);
	}

	inline InjectResult InternalManualMap(HANDLE hProc, const char* szDllFile)
	{
		BYTE* pSrcData = nullptr;
		IMAGE_NT_HEADERS* pOldNtHeader = nullptr;
		IMAGE_OPTIONAL_HEADER* pOldOptHeader = nullptr;
		IMAGE_FILE_HEADER* pOldFileHeader = nullptr;
		BYTE* pTargetBase = nullptr;

		if (!GetFileAttributesA(szDllFile))
			return InjectResult::DllNotFound;

		std::ifstream File(szDllFile, std::ios::binary | std::ios::ate);
		if (File.fail())
		{
			File.close();
			return InjectResult::FailedToReadFile;
		}

		int64_t FileSize = File.tellg();
		pSrcData = new BYTE[FileSize];

		File.seekg(0, std::ios::beg);
		File.read(reinterpret_cast<char*>(pSrcData), FileSize);
		File.close();

		IMAGE_DOS_HEADER* DosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData);
		if (DosHeader->e_magic != ImageSignature)
		{
			delete[] pSrcData;
			return InjectResult::InvalidDll;
		}

		pOldNtHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + DosHeader->e_lfanew);
		pOldOptHeader = &pOldNtHeader->OptionalHeader;
		pOldFileHeader = &pOldNtHeader->FileHeader;

#ifdef _WIN64
		if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_AMD64)
		{
			delete[] pSrcData;
			return InjectResult::InvalidArchitecture;
		}
#else
		if (pOldFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
		{
			delete[] pSrcData;
			return InjectResult::InvalidArchitecture;
		}
#endif

		pTargetBase = static_cast<BYTE*>(VirtualAllocEx(
			hProc,
			reinterpret_cast<void*>(pOldOptHeader->ImageBase),
			pOldOptHeader->SizeOfImage,
			MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		if (!pTargetBase)
		{
			delete[] pSrcData;
			return InjectResult::FailedToAllocateMemory;
		}

		MANUAL_MAPPING_DATA Data{};
		Data.pLoadLibraryA = LoadLibraryA;
		Data.pGetProcAddress = reinterpret_cast<f_GetProcAddress>(GetProcAddress);

		PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
		for (uint32_t i = 0; i != pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader)
		{
			if (pSectionHeader->SizeOfRawData)
			{
				if (!WriteProcessMemory(hProc, pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr))
				{
					delete[] pSrcData;
					// todo
					VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
					return InjectResult::FailedToMapSections;
				}
			}
		}

		memcpy(pSrcData, &Data, sizeof Data);
		WriteProcessMemory(hProc, pTargetBase, pSrcData, 0x1000, nullptr);

		delete[] pSrcData;
		void* pShellCode = VirtualAllocEx(hProc, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!pShellCode)
		{
			VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
			return InjectResult::FailedToAllocateMemory;
		}

		WriteProcessMemory(hProc, pShellCode, ShellCode, 0x1000, nullptr);

		HANDLE hThread = CreateRemoteThread(
			hProc,
			nullptr,
			0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellCode),
			pTargetBase,
			0,
			nullptr);
		if (!hThread)
		{
			VirtualFreeEx(hProc, pTargetBase, 0, MEM_RELEASE);
			VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);
			return InjectResult::ThreadCreationFailed;
		}

		CloseHandle(hThread);

		HINSTANCE hCheck = nullptr;
		while (!hCheck)
		{
			MANUAL_MAPPING_DATA DataChecked{};
			ReadProcessMemory(hProc, pTargetBase, &DataChecked, sizeof DataChecked, nullptr);
			hCheck = DataChecked.hModule;
		}

		VirtualFreeEx(hProc, pShellCode, 0, MEM_RELEASE);

		return InjectResult::Success;
	}

	inline API HANDLE OpenProcessHandle(DWORD pId)
	{
		HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);
		if (!pHandle)
			return INVALID_HANDLE_VALUE;

		return pHandle;
	}

	inline API DWORD FindProcessId(const char* ProcessExeFile)
	{
		PROCESSENTRY32 pEntry{ 0 };
		pEntry.dwSize = sizeof pEntry;

		HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (Snapshot == INVALID_HANDLE_VALUE)
			return GetLastError();

		DWORD pId = 0;
		Process32First(Snapshot, &pEntry);
		do
		{
			if (!strcmp(ProcessExeFile, pEntry.szExeFile))
			{
				pId = pEntry.th32ProcessID;
				break;
			}
		} while (Process32Next(Snapshot, &pEntry));

		CloseHandle(Snapshot);
		return pId;
	}

	inline API InjectResult ManualMap(HANDLE hProc, const char* szDllFile)
	{
		InjectResult Result = InternalManualMap(hProc, szDllFile);
		CloseHandle(hProc);

		return Result;
	}

	inline InjectResult InternalNativeInject(HANDLE hProc, const char* szDllFile)
	{
		size_t DllSize = std::strlen(szDllFile);
		LPVOID pDllPath = VirtualAllocEx(hProc, nullptr, DllSize, MEM_COMMIT, PAGE_READWRITE);
		if (!pDllPath)
		{
			return InjectResult::FailedToAllocateMemory;
		}

		WriteProcessMemory(hProc, pDllPath, szDllFile, DllSize, nullptr);
		if (HANDLE hThread = CreateRemoteThread(
			hProc,
			nullptr,
			0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA")),
			pDllPath,
			0,
			nullptr)) CloseHandle(hThread);

		return InjectResult::Success;
	}

	inline API InjectResult NativeInject(HANDLE hProc, const char* szDllFile, bool CloseHandleAfterInject = true)
	{
		InjectResult Result = InternalNativeInject(hProc, szDllFile);
		if (CloseHandleAfterInject)
			CloseHandle(hProc);

		return Result;
	}
}