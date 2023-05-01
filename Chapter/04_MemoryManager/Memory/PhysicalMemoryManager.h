#pragma once
#include "windef.h"
#include "stdint.h"
#include "MultiBoot.h"
#include "Hal.h"

#define PMM_BLOCKS_PER_BYTE 8	
#define PMM_BLOCK_SIZE	4096	
#define PMM_BLOCK_ALIGN	BLOCK_SIZE
#define PMM_BITS_PER_INDEX	32

namespace PhysicalMemoryManager
{

	void	Initialize(multiboot_info* bootinfo);

	// Page = Block = Frame
	// Frame 들을 Set 하거나 UnSet
	void SetBit(int bit);
	void UnsetBit(int bit);

	uint32_t GetMemoryMapSize(); // 메모리맵의 크기를 얻어낸다.
	uint32_t GetKernelEnd(); // 로드된 커널 시스템의 마지막 주소를 얻어낸다.

	// 물리 메모리의 사용 여부에 따라 초기에 프레임들을 Set하거나 Unset한다	
	void	SetAvailableMemory(uint32_t, size_t);
	void	SetDeAvailableMemory(uint32_t base, size_t);

	void*	AllocBlock();// 하나의 블록을 할당한다
	void	FreeBlock(void*);// 하나의 블록을 해제한다.

 	void*	AllocBlocks(size_t);// 연속된 블록을 할당한다.
	void	FreeBlocks(void*, size_t);// 사용된 연속된 메모리 블록을 회수한다.

	size_t GetMemorySize();// 메모리 사이즈를 얻는다.

	unsigned int GetFreeFrame(); // 사용할 수 있는 Frame number 를 얻음
	unsigned int GetFreeFrames(size_t size); // 연속된 Frame 의 시작 Frame number 를 얻음

	uint32_t GetUsedBlockCount(); // 사용된 블록수를 리턴한다
	uint32_t GetFreeBlockCount(); // 사용되지 않은 블록수를 리턴한다.

	uint32_t	GetFreeMemory();

	uint32_t GetTotalBlockCount(); // 블록의 전체수를 리턴한다
	uint32_t GetBlockSize(); // 블록의 사이즈를 리턴한다. 4KB

	bool TestMemoryMap(int bit);

	void EnablePaging(bool state);
	bool	IsPaging();

	void LoadPDBR(uint32_t physicalAddr);
	uint32_t GetPDBR();
	
	//Debug
	void Dump();
}	
