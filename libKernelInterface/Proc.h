#pragma once

namespace Kernel
{
	#define MAXCOMLEN 19

	template <typename T>
	struct ListEntry
	{
		T* next;
		T* prev;
	};

	template <typename T>
	struct TailQueue
	{
		T* first;
		T* last;
	};

	struct sx 
	{
		char _0x00[0x20];
	}; // 0x20

	struct lock_object {
		const char* lo_name;
		uint32_t lo_flags;
		uint32_t lo_data;
		void* lo_witness;
	};

	struct mtx {
		struct lock_object lock_object;
		volatile void* mtx_lock;
	};

	struct vm_map_entry;
	typedef uint64_t vm_offset_t;

	TYPE_BEGIN(struct vm_map_entry, 0xC0);
	TYPE_FIELD(struct vm_map_entry* prev, 0);
	TYPE_FIELD(struct vm_map_entry* next, 8);
	TYPE_FIELD(struct vm_map_entry* left, 0x10);
	TYPE_FIELD(struct vm_map_entry* right, 0x18);
	TYPE_FIELD(uint64_t start, 0x20);
	TYPE_FIELD(uint64_t end, 0x28);
	TYPE_FIELD(uint64_t offset, 0x50);
	TYPE_FIELD(uint16_t prot, 0x5C);
	TYPE_FIELD(char name[32], 0x8D);
	TYPE_END();

	TYPE_BEGIN(struct vm_map, 0x178);
	TYPE_FIELD(vm_map_entry header, 0);
	TYPE_FIELD(struct sx lock, 0xB8);
	TYPE_FIELD(struct mtx system_mtx, 0xD8);
	TYPE_FIELD(int nentries, 0x100);
	TYPE_END();

	TYPE_BEGIN(struct vmspace, 0x250);
	TYPE_FIELD(struct vm_map vm_map, 0);
	// maybe I will add more later just for documentation purposes
	TYPE_END();

	struct dynlib_obj
	{
		dynlib_obj* next;				// 0x00
		const char* Path;				// 0x08
		char _0x010[0x18];
		uint64_t Handle;				// 0x28
		uint64_t MapBase;				// 0x30
		size_t MapSize;					// 0x38
		size_t TextSize;				// 0x40
		uint64_t DataBase;				// 0x48
		size_t dataSize;				// 0x50
		char _0x058[0x10];
		size_t vaddr_base;				// 0x68
		uint64_t realloc_base;			// 0x70
		uint64_t entry;					// 0x78
		char _0x60[0xA0];
	}; //Size 0x100

	struct dynlib
	{
		dynlib_obj* objs;				// 0x00
		char _0x08[0x20];
		uint32_t ModuleCount;			// 0x28
		char _0x02C[0x44];
		sx bind_lock;					// 0x70
	};

	struct Thread;
	struct Proc
	{
		ListEntry<Proc> ProcList;		// 0x00
		TailQueue<Thread> ThreadList;	// 0x10
		char _0x20[0x88];
		int Flag;						// 0xA8
		int State;						// 0xAC
		int pid;						// 0xB0
		char _0xB4[0xB4];
		vmspace* p_vmspace;				// 0x168
		char _0x170[0x1D0];
		dynlib* DynLib;					// 0x340
		char _0x348[0x48];
		char TitleId[0x10];				// 0x390
		char _0x3A0[0xAC];
		char Name[0x20];				// 0x44C
		char _0x46C[0x64C];
	};	// 0xAB8

	static_assert(sizeof(Proc) == 0xAB8, "Proc struct must be size 0xAB8");

	struct Thread
	{
		char _0x00[0x8];
		Proc* td_proc;					// 0x08
		char _0x10[0x274];
		char td_name[MAXCOMLEN + 17];	// 0x284
		char _0x2A8[0xEC];
		enum {
			TDS_INACTIVE = 0x0,
			TDS_INHIBITED,
			TDS_CAN_RUN,
			TDS_RUNQ,
			TDS_RUNNING
		} td_state;						// 0x390
		char _0x394[0x4];
		uint64_t td_retval[2];			// 0x398
	};


	Proc* GetProcByPid(int pid);
	bool ReadWriteProcessMemory(Thread* td, Proc* proc, void* addr, void* data, uint32_t len, bool write);
}