#include "Common.h"
#include "KernelInterface.h"
#include "Proc.h"
#include "Misc.h"
#include <KernelExt.h>
#include "Syscall.h"
#include <libdbg.h>

struct sysReadWriteMemoryArgs
{
	void* Syscall;
	int pid;
	void* addr;
	void* data;
	size_t len;
};

struct sysGetLibrariesArgs
{
	void* Syscall;
	int pid;
	OrbisLibraryInfo* libOut;
	int* libCount;
};

struct sysGetPagesArgs
{
	void* Syscall;
	int pid;
	OrbisProcessPage* pagesOut;
	int* pageCount;
};

int SysWriteMemory(Kernel::Thread* td, sysReadWriteMemoryArgs* args)
{
	auto proc = Kernel::GetProcByPid(args->pid);
	if (proc && Kernel::ReadWriteProcessMemory(td, proc, args->addr, args->data, args->len, true))
	{
		return 1;
	}

	return 0;
}

int SysReadMemory(Kernel::Thread* td, sysReadWriteMemoryArgs* args)
{
	auto proc = Kernel::GetProcByPid(args->pid);
	if (proc && Kernel::ReadWriteProcessMemory(td, proc, args->addr, args->data, args->len, false))
	{
		return 1;
	}

	return 0;
}

bool ReadWriteMemory(int pid, void* addr, void* data, size_t len, bool write)
{
	syscall(11, write ? SysWriteMemory : SysReadMemory, pid, addr, data, len);
	return true;
}

int sysGetLibraries(Kernel::Thread* td, sysGetLibrariesArgs* args)
{
	if (args->libOut == 0 || args->libCount == 0)
	{
		Kernel::printf("sysGetLibraries(): Invalid Args.\n");
		return -1;
	}

	auto proc = Kernel::GetProcByPid(args->pid);
	if (proc == 0)
	{
		Kernel::printf("sysGetLibraries(): Failed to get the proc.\n");
		return -1;
	}

	size_t numModules;
	int handles[256];
	Kernel::sys_dynlib_get_list(proc->ThreadList.first, handles, 256, &numModules);

	auto libTemp = (OrbisLibraryInfo*)Kernel::malloc(sizeof(OrbisLibraryInfo) * numModules);
	if (!libTemp)
	{
		Kernel::printf("sysGetLibraries(): Failed to allocate memory for libTemp.\n");
		return -1;
	}

	for (int i = 0; i < numModules; i++)
	{
		Kernel::dynlib_info_ex info;
		info.size = sizeof(Kernel::dynlib_info_ex);
		sys_dynlib_get_info_ex(proc->ThreadList.first, handles[i], 1, &info);

		libTemp[i].Handle = handles[i];
		Kernel::strncpy(libTemp[i].Path, info.name, 256);
		libTemp[i].MapBase = (uint64_t)info.segmentInfo[0].baseAddr;
		libTemp[i].MapSize = info.segmentInfo[0].size + info.segmentInfo[1].size;
		libTemp[i].TextSize = info.segmentInfo[0].size;
		libTemp[i].DataBase = (uint64_t)info.segmentInfo[1].baseAddr;
		libTemp[i].DataSize = info.segmentInfo[1].size;
	}

	// Write the data out to userland.
	Kernel::ReadWriteProcessMemory(td, td->td_proc, (void*)args->libOut, (void*)libTemp, sizeof(OrbisLibraryInfo) * numModules, true);
	Kernel::ReadWriteProcessMemory(td, td->td_proc, (void*)args->libCount, (void*)&numModules, sizeof(int), true);

	// Free our memory.
	Kernel::free(libTemp);

	return 0;
}

int sysGetPages(Kernel::Thread* td, sysGetPagesArgs* args)
{

	Kernel::vm_map_entry* entry = nullptr;

	auto proc = Kernel::GetProcByPid(args->pid);
	auto entries = args->pagesOut;
	auto num = args->pageCount;

	Kernel::vmspace* vm = proc->p_vmspace;
	Kernel::vm_map* map = &vm->vm_map;

	int entriesCount = map->nentries;

	if (!entries)
	{
		Kernel::ReadWriteProcessMemory(td, td->td_proc, (void*)args->pageCount, (void*)&entriesCount, sizeof(int), true);
		return true;
	}

	((int(*)(...))(GetKernelBase() + OffsetTable->vm_map_lock_read))(map);
	((int(*)(...))(GetKernelBase() + OffsetTable->vm_map_lookup))(map, nullptr, &entry);
	((int(*)(...))(GetKernelBase() + OffsetTable->vm_map_unlock_read))(map);

	OrbisProcessPage* info = (OrbisProcessPage*)Kernel::malloc(entriesCount * sizeof(OrbisProcessPage));
	if (!info)
	{
		Kernel::free(info);
		info = nullptr;
		return -1;
	}

	for (int i = 0; i < entriesCount; i++) {
		info[i].Start = entry->start;
		info[i].End = entry->end;
		info[i].Offset = entry->offset;
		info[i].Size = entry->end - entry->start;
		info[i].Prot = entry->prot & (entry->prot >> 8);
		Kernel::memcpy(info[i].Name, entry->name, sizeof(info[i].Name));

		if (!(entry = entry->next)) {
			break;
		}
	}

	Kernel::ReadWriteProcessMemory(td, td->td_proc, (void*)args->pagesOut, (void*)info, sizeof(OrbisProcessPage) * entriesCount, true);
	Kernel::ReadWriteProcessMemory(td, td->td_proc, (void*)args->pageCount, (void*)&entriesCount, sizeof(int), true);

	Kernel::free(info);

	return false;
}

int GetLibraries(int pid, OrbisLibraryInfo* libraries, int maxCount)
{
	OrbisLibraryInfo librariesTemp[256];
	int libCount = 0;
	syscall(11, sysGetLibraries, pid, &librariesTemp, &libCount);

	if (libCount > 0)
		memcpy(libraries, librariesTemp, sizeof(OrbisLibraryInfo) * maxCount);

	return libCount;
}

int GetPages(int pid, OrbisProcessPage* pages, int maxCount)
{
	OrbisProcessPage pagesTemp[500];
	int pagesCount = 0;
	syscall(11, sysGetPages, pid, pages ? &pagesTemp : nullptr, &pagesCount);

	if (pagesCount > 0)
		memcpy(pages, pagesTemp, sizeof(OrbisProcessPage) * maxCount);

	return pagesCount;
}

struct sysMmapArgs
{
	void* Syscall;
	uint64_t addr;
	size_t len;
	int prot;
	int flags;
	int fd;
	off_t pos;
};

uint64_t mmap(uint64_t addr, size_t len, int prot, int flags, int fd, off_t pos)
{

}