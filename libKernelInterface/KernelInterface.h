#pragma once

#ifdef LIBRARY_IMPL
#define PRX_INTERFACE __declspec(dllexport)
#else
#define PRX_INTERFACE __declspec(dllimport)
#endif

struct OrbisLibraryInfo
{
	uint32_t Handle;
	char Path[256];
	uint64_t MapBase;
	size_t MapSize;
	size_t TextSize;
	uint64_t DataBase;
	size_t DataSize;
};

struct OrbisProcessPage
{
	char Name[256];
	uint64_t Start;
	uint64_t End;
	uint64_t Offset;
	size_t Size;
	uint32_t Prot;
};

PRX_INTERFACE bool ReadWriteMemory(int pid, void* addr, void* data, size_t len, bool write);
PRX_INTERFACE int GetLibraries(int pid, OrbisLibraryInfo* libraries, int maxCount); 
PRX_INTERFACE int GetPages(int pid, OrbisProcessPage* pages, int maxCount);