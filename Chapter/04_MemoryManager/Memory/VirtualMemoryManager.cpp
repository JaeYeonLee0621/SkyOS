﻿#include "VirtualMemoryManager.h"
#include "PhysicalMemoryManager.h"
#include "string.h"
#include "memory.h"
#include "SkyConsole.h"
#include "MultiBoot.h"	
#include "SkyAPI.h"


PageDirectory* g_pageDirectoryPool[MAX_PAGE_DIRECTORY_COUNT];
bool g_pageDirectoryAvailable[MAX_PAGE_DIRECTORY_COUNT];


/*

- Kernel 의 memory allocation 요청을 처리하는 창구 역할 담당
- Physical memory manager 로부터 실제 memory space 를 확보해서 virtual address 와 physical address 를 mapping 하는 역할 담당

[Virtual Memory Manager 의 핵심]
1. Paging 에 대한 Structure 관리
2. 할당된 Physical Address 를 Vitual Address 와 mapping
3. Page Table 의 동적인 생성 및 삭제

*/

namespace VirtualMemoryManager
{
	//! current directory table

	PageDirectory*		_kernel_directory = 0;
	PageDirectory*		_cur_directory = 0;	

	//가상 주소와 매핑된 실제 물리 주소를 얻어낸다.
	void* VirtualMemoryManager::GetPhysicalAddressFromVirtualAddress(PageDirectory* directory, uint32_t virtualAddress)
	{
		PDE* pagedir = directory->m_entries;
		if (pagedir[virtualAddress >> 22] == 0)
			return NULL;

		return (void*)((uint32_t*)(pagedir[virtualAddress >> 22] & ~0xfff))[virtualAddress << 10 >> 10 >> 12];
	}

	//페이지 디렉토리 엔트리 인덱스가 0이 아니면 이미 페이지 테이블이 존재한다는 의미
	bool CreatePageTable(PageDirectory* dir, uint32_t virt, uint32_t flags)
	{
		PDE* pageDirectory = dir->m_entries;
		if (pageDirectory[virt >> 22] == 0)
		{
			void* pPageTable = PhysicalMemoryManager::AllocBlock();
			if (pPageTable == nullptr)
				return false;					

			memset(pPageTable, 0, sizeof(PageTable));
			pageDirectory[virt >> 22] = ((uint32_t)pPageTable) | flags;					
		}
		return true;
	}

	// PDE나 PTE의 플래그는 같은 값을 공유
	// 가상주소를 물리 주소에 매핑
	// 첫 번째 Parameter : PageDirectory > Process 마다 1개의 Page Directory 존재
	// 두 번째 Parameter : virtual address
	// 세 번째 Parameter : physical address 
	// 네 번째 Parameter : PDE (Page Directory Entry) 나 PTE (Page Table Entry) 를 설정
	// 보통 I86_PTE_PRESENT (Page 가 Physical Memory 에 존재) or I86_PTE_WRITABLE (해당 memory 에 쓰기 가능) 으로 설정
	void MapPhysicalAddressToVirtualAddresss(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags)
	{
		kEnterCriticalSection();
		PhysicalMemoryManager::EnablePaging(false);
		PDE* pageDir = dir->m_entries;				

		// 1. Page directory 에서 Page Directory Entry 를 가져옴
		// 이 값이 0이면 Page table 이 없다고 판단
		if (pageDir[virt >> 22] == 0)
		{
			CreatePageTable(dir, virt, flags);
		}

		uint32_t mask = (uint32_t)(~0xfff);

		
		uint32_t* pageTable = (uint32_t*)(pageDir[virt >> 22] & mask);

		// Page Table 에서 PTE 를 구한 뒤 물리 주소와 Flag 설정
		// Virtual Address 와 Physical Address mapping
		pageTable[virt << 10 >> 10 >> 12] = phys | flags;

		PhysicalMemoryManager::EnablePaging(true);
		kLeaveCriticalSection();
	}

	void MapPhysicalAddressToVirtualAddresss2(PageDirectory* dir, uint32_t virt, uint32_t phys, uint32_t flags)
	{		
		kEnterCriticalSection();
		PhysicalMemoryManager::EnablePaging(false);		
		
		PDE* pageDir = dir->m_entries;

		if (pageDir[virt >> 22] == 0)
		{
			CreatePageTable(dir, virt, flags);
		}

		uint32_t mask = (uint32_t)(~0xfff);
		uint32_t* pageTable = (uint32_t*)(pageDir[virt >> 22] & mask);

		pageTable[virt << 10 >> 10 >> 12] = phys | flags;

		PhysicalMemoryManager::EnablePaging(true);
		kLeaveCriticalSection();
	}

	void FreePageDirectory(PageDirectory* dir)
	{
		PhysicalMemoryManager::EnablePaging(false);
		PDE* pageDir = dir->m_entries;
		for (int i = 0; i < PAGES_PER_DIRECTORY; i++)
		{
			PDE& pde = pageDir[i];

			if (pde != 0)
			{
				/* get mapped frame */
				void* frame = (void*)(pageDir[i] & 0x7FFFF000);
				PhysicalMemoryManager::FreeBlock(frame);
				pde = 0;
			}
		}

		for (int index = 0; index < MAX_PAGE_DIRECTORY_COUNT; index++)
		{

			if (g_pageDirectoryPool[index] == dir)
			{
				g_pageDirectoryAvailable[index] = true;
				break;
			}
		}

		PhysicalMemoryManager::EnablePaging(true);
	}

	void UnmapPageTable(PageDirectory* dir, uint32_t virt)
	{
		PDE* pageDir = dir->m_entries;
		if (pageDir[virt >> 22] != 0) {

			/* get mapped frame */
			void* frame = (void*)(pageDir[virt >> 22] & 0x7FFFF000);

			/* unmap frame */
			PhysicalMemoryManager::FreeBlock(frame);
			pageDir[virt >> 22] = 0;
		}
	}


	void UnmapPhysicalAddress(PageDirectory* dir, uint32_t virt)
	{
		PDE* pagedir = dir->m_entries;
		if (pagedir[virt >> 22] != 0)
			UnmapPageTable(dir, virt);
	}

	PageDirectory* CreatePageDirectory()
	{
		PageDirectory* dir = NULL;

		/* allocate page directory */
		dir = (PageDirectory*)PhysicalMemoryManager::AllocBlock();
		if (!dir)
			return NULL;

		//memset(dir, 0, sizeof(PageDirectory));

		return dir;
	}

	void ClearPageDirectory(PageDirectory* dir)
	{
		if (dir == NULL)
			return;

		memset(dir, 0, sizeof(PageDirectory));
	}

	PDE* GetPDE(PageDirectory* dir, uint32_t addr)
	{
		if (dir == NULL)
			return NULL;

		return &dir->m_entries[GetPageTableIndex(addr)];
	}

	uint32_t GetPageTableIndex(uint32_t addr)
	{
		return (addr >= DTABLE_ADDR_SPACE_SIZE) ? 0 : addr / PAGE_SIZE;
	}

	//살펴볼 것
	uint32_t GetPageTableEntryIndex(uint32_t addr)
	{

		return (addr >= PTABLE_ADDR_SPACE_SIZE) ? 0 : addr / PAGE_SIZE;
	}

	PTE* GetPTE(PageTable* p, uint32_t addr)
	{
		if (p == NULL)
			return NULL;

		return &p->m_entries[GetPageTableEntryIndex(addr)];
	}

	void ClearPageTable(PageTable* p)
	{
		if (p != NULL)
			memset(p, 0, sizeof(PageTable));
	}

	bool AllocPage(PTE* e)
	{
		void* p = PhysicalMemoryManager::AllocBlock();

		if (p == NULL)
			return false;

		PageTableEntry::SetFrame(e, (uint32_t)p);
		PageTableEntry::AddAttribute(e, I86_PTE_PRESENT);

		return true;
	}

	void FreePage(PTE* e)
	{

		void* p = (void*)PageTableEntry::GetFrame(*e);
		if (p)
			PhysicalMemoryManager::FreeBlock(p);

		PageTableEntry::DelAttribute(e, I86_PTE_PRESENT);
	}


	PageDirectory* CreateCommonPageDirectory()
	{
				
		// 페이지 디렉토리 생성. 가상주소 공간 
		// 4GB를 표현하기 위해서 페이지 디렉토리는 하나면 충분하다.
		// 페이지 디렉토리는 1024개의 페이지 테이블을 가진다
		// 1024 * 1024 (페이지 테이블 엔트리의 개수) * 4K(프레임의 크기) = 4G
		// 자세한 내용은 ReadMe.md 를 읽으면 됨		

		int index = 0;

		// Page directory pool 에서 사용할 수 있는 Page directory 를 하나 얻어냄
		for (; index < MAX_PAGE_DIRECTORY_COUNT; index++)
		{
		
			if (g_pageDirectoryAvailable[index] == true)
				break;
		}

		if (index == MAX_PAGE_DIRECTORY_COUNT)
			return nullptr;

		PageDirectory* dir = g_pageDirectoryPool[index];
	
		if (dir == NULL)
			return nullptr;

		// 얻어낸 Page directory 는 사용 중임을 표시하고 초기화
		g_pageDirectoryAvailable[index] = false;
		memset(dir, 0, sizeof(PageDirectory));

		uint32_t frame = 0x00000000; // Physical Address 시작 주소
		uint32_t virt = 0x00000000; // Virtual Address 시작 주소

		// Page Table 2개 생성 
		// => Virtual Address & Physical Address 가 같은 identity mapping 수행
		for (int i = 0; i < 2; i++)
		{			
			PageTable* identityPageTable = (PageTable*)PhysicalMemoryManager::AllocBlock();
			if (identityPageTable == NULL)
			{
				return nullptr;
			}

			memset(identityPageTable, 0, sizeof(PageTable));
			
			// 물리 주소를 가상 주소와 동일하게 매핑시킨다
			for (int j = 0; j < PAGES_PER_TABLE; j++, frame += PAGE_SIZE, virt += PAGE_SIZE)
			{
				PTE page = 0;
				PageTableEntry::AddAttribute(&page, I86_PTE_PRESENT);

				PageTableEntry::SetFrame(&page, frame);
				
				identityPageTable->m_entries[PAGE_TABLE_INDEX(virt)] = page;
			}

			// 페이지 디렉토리에 페이지 디렉토리 엔트리(PDE)를 한 개 세트한다
			// 0번째 인덱스에 PDE를 세트한다 (가상주소가 0X00000000 일시 참조됨)
			// 앞에서 생성한 아이덴티티 페이지 테이블을 세트한다
			// 가상주소 = 물리주소
			PDE* identityEntry = &dir->m_entries[PAGE_DIRECTORY_INDEX((virt - 0x00400000))];
			PageDirectoryEntry::AddAttribute(identityEntry, I86_PDE_PRESENT | I86_PDE_WRITABLE);			
			PageDirectoryEntry::SetFrame(identityEntry, (uint32_t)identityPageTable);
		}

		return dir;
	}	

	void SetPageDirectory(PageDirectory* dir)
	{
		_asm
		{
			mov	eax, [dir]
			mov	cr3, eax		// PDBR is cr3 register in i86
		}
	}

	bool Initialize()
	{
		SkyConsole::Print("Virtual Memory Manager Init..\n");

		// Page Directory Pool 생성
		for (int i = 0; i < MAX_PAGE_DIRECTORY_COUNT; i++)
		{
			g_pageDirectoryPool[i] = (PageDirectory*)PhysicalMemoryManager::AllocBlock();
			g_pageDirectoryAvailable[i] = true;
		}

		// Page Directory 생성
		// 다음 method 는 kernel area address mapping 까지 작업
		PageDirectory* dir = CreateCommonPageDirectory();

		if (nullptr == dir)
			return false;

		//페이지 디렉토리를 PDBR 레지스터에 로드한다
		SetCurPageDirectory(dir);
		SetKernelPageDirectory(dir);

		SetPageDirectory(dir);

		//페이징 기능을 다시 활성화시킨다
		PhysicalMemoryManager::EnablePaging(true);
		
		return true;
	}

	bool SetKernelPageDirectory(PageDirectory* dir)
	{
		if (dir == NULL)
			return false;

		_kernel_directory = dir;

		return true;
	}

	PageDirectory* GetKernelPageDirectory()
	{
		return _kernel_directory;
	}



	bool SetCurPageDirectory(PageDirectory* dir)
	{
		if (dir == NULL)
			return false;

		_cur_directory = dir;		

		return true;
	}

	PageDirectory* GetCurPageDirectory()
	{
		return _cur_directory;
	}

	void FlushTranslationLockBufferEntry(uint32_t addr)
	{
#ifdef _MSC_VER
		_asm {
			cli
			invlpg	addr
			sti
		}
#endif
	}	

	bool CreateVideoDMAVirtualAddress(PageDirectory* pd, uintptr_t virt, uintptr_t phys, uintptr_t end)
	{
		//void* memory = PhysicalMemoryManager::AllocBlocks((end - start)/ PAGE_SIZE);
		for (int i = 0; virt <= end; virt += 0x1000, phys += 0x1000, i++)
		{
			MapPhysicalAddressToVirtualAddresss(pd, (uint32_t)virt, (uint32_t)phys, I86_PTE_PRESENT | I86_PTE_WRITABLE);
		}

		return true;
	}

	void Dump()
	{
		
	}
}