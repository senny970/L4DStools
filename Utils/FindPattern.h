//============= From SourceMod ==============//
#include <Windows.h>
#include <Psapi.h>
#pragma comment( lib, "psapi.lib" )

struct DynLibInfo
{
	void *baseAddress;
	size_t memorySize;
};

bool GetLibraryInfo(const void *libPtr, DynLibInfo &lib)
{
	uintptr_t baseAddr;

	if (libPtr == NULL)
	{
		return false;
	}

	MEMORY_BASIC_INFORMATION info;
	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *pe;
	IMAGE_FILE_HEADER *file;
	IMAGE_OPTIONAL_HEADER *opt;

	if (!VirtualQuery(libPtr, &info, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return false;
	}

	baseAddr = reinterpret_cast<uintptr_t>(info.AllocationBase);

	/* All this is for our insane sanity checks :o */
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(baseAddr);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(baseAddr + dos->e_lfanew);
	file = &pe->FileHeader;
	opt = &pe->OptionalHeader;

	/* Check PE magic and signature */
	if (dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE || opt->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		return false;
	}

	/* Check architecture, which is 32-bit/x86 right now
	 * Should change this for 64-bit if Valve gets their act together
	 */
	if (file->Machine != IMAGE_FILE_MACHINE_I386)
	{
		return false;
	}

	/* For our purposes, this must be a dynamic library */
	if ((file->Characteristics & IMAGE_FILE_DLL) == 0)
	{
		return false;
	}

	/* Finally, we can do this */
	lib.memorySize = opt->SizeOfImage;

	lib.baseAddress = reinterpret_cast<void *>(baseAddr);

	return true;
}

void *FindPattern(const void *libPtr, const char *pattern, size_t len)
{
	DynLibInfo lib;
	bool found;
	char *ptr, *end;

	memset(&lib, 0, sizeof(DynLibInfo));

	if (!GetLibraryInfo(libPtr, lib))
	{
		return NULL;
	}

	ptr = reinterpret_cast<char *>(lib.baseAddress);
	end = ptr + lib.memorySize - len;

	while (ptr < end)
	{
		found = true;
		for (register size_t i = 0; i < len; i++)
		{
			if (pattern[i] != '\x2A' && pattern[i] != ptr[i])
			{
				found = false;
				break;
			}
		}

		if (found)
			return ptr;

		ptr++;
	}

	return NULL;
}

//=======================================================//

inline MODULEINFO GetModuleInfo ( LPCSTR szModule ) {
        MODULEINFO modinfo = {0};
        HMODULE hModule = GetModuleHandleA ( szModule );
        if ( hModule == 0 ) return modinfo;
        GetModuleInformation ( GetCurrentProcess(), hModule, &modinfo, sizeof ( MODULEINFO ) );
        return modinfo;
}

BOOL Match ( const BYTE* pData, const BYTE* bMask, const char* szMask ) {
    for ( ; *szMask; ++szMask, ++pData, ++bMask )
        if ( *szMask == 'x' && *pData != *bMask )
            return false;
    return ( *szMask ) == NULL;
}


DWORD FindPattern (LPCSTR module, const char* szSig, const char* szMask ) {
    DWORD dwAddress = ( DWORD )GetModuleHandle(module);
    DWORD dwLen = GetModuleInfo(module).SizeOfImage;

    for ( DWORD i = 0; i < dwLen; i++ )
        if ( Match ( ( BYTE* ) ( dwAddress + i ), ( BYTE* ) szSig, szMask ) )
            return ( DWORD ) ( dwAddress + i );

    return 0;
}