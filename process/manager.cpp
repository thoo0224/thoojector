#include "manager.h"
#include <string>
#include <format>
#include <TlHelp32.h>

#include "../utils.h"

std::map<std::string, Process::ProcessEntry> g_ProcessEntries;

void Process::LoadProcessEntries()
{
	PROCESSENTRY32 pEntry = {};
	pEntry.dwSize = sizeof pEntry;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	g_ProcessEntries.clear();
	Process32First(hSnapshot, &pEntry);
	do
	{
		std::string szExeFile = pEntry.szExeFile;
		if (!string_ends_with(szExeFile, ".exe"))
		{
			continue;
		}
		
		ProcessEntry Entry = {
			pEntry.th32ProcessID,
			szExeFile
		};

		g_ProcessEntries[std::format("{} ({})", Entry.szExeFile, Entry.pId)] = Entry;
	} while (Process32Next(hSnapshot, &pEntry));

	CloseHandle(hSnapshot);
}