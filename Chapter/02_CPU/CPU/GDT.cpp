#include "gdt.h"
#include "string.h"
#include "memory.h"
#include "windef.h"
#include "defines.h"

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

/*

CPU 에게 GDT 가 어디에 위치하는지 알려줘야 함
GDTR Register 를 이용해서 GDT 에 접근 
- Assembly command : lgdt

*/

typedef struct tag_gdtr {

	// GDT 의 크기 : 2 byte
	USHORT m_limit; 

	// GDT 의 시작 주소 : 4 byte
	UINT m_base;
}gdtr;

#ifdef _MSC_VER
#pragma pack (pop, 1)
#endif

//! global descriptor table is an array of descriptors
static struct gdt_descriptor	_gdt [MAX_DESCRIPTORS];

//! gdtr data
static gdtr				_gdtr;


//! install gdtr
static void InstallGDT () {

// lgdt : GDT register 영역에 해당 GDTR structure 선언
// _gdtr : 변수의 physical address
// paging 기능 실행 시 virtual, physical address 가 다르기 때문에 GDTR register 값 설정은 paging 활성화 전에 설정
#ifdef _MSC_VER
	_asm lgdt [_gdtr]
#endif
}

//! Setup a descriptor in the Global Descriptor Table
void gdt_set_descriptor(uint32_t i, uint64_t base, uint64_t limit, uint8_t access, uint8_t grand)
{
	if (i > MAX_DESCRIPTORS)
		return;

	//! null out the descriptor
	memset ((void*)&_gdt[i], 0, sizeof (gdt_descriptor));

	//! set limit and base addresses
	_gdt[i].baseLo	= uint16_t(base & 0xffff);
	_gdt[i].baseMid	= uint8_t((base >> 16) & 0xff);
	_gdt[i].baseHi	= uint8_t((base >> 24) & 0xff);
	_gdt[i].limit	= uint16_t(limit & 0xffff);

	//! set flags and grandularity bytes
	_gdt[i].flags = access;
	_gdt[i].grand = uint8_t((limit >> 16) & 0x0f);
	_gdt[i].grand |= grand & 0xf0;
}


//! returns descriptor in gdt
gdt_descriptor* i86_gdt_get_descriptor (int i) {

	if (i > MAX_DESCRIPTORS)
		return 0;

	return &_gdt[i];
}

/*

GDT 를 통해 주소를 변환하는 과정 = Segmentation
Segmentation 과정 = 논리 주소 > 선형 주소

*/

int GDTInitialize()
{
	/*

	GDT Register 에 load 될 _gdtr 값 초기화
	_gdtr 의 Address 는 실제 Physical Address 에 해당
	5 개의 descriptor = NULL descriptor, kernel code descriptor, kernel data descriptor, user code descriptor, user data descriptor
	descriptor 당 8byte 이므로 GDT 의 크기는 40 byte

	*/

	_gdtr.m_limit = (sizeof(struct gdt_descriptor) * MAX_DESCRIPTORS) - 1;
	_gdtr.m_base = (uint32_t)&_gdt[0];

	// NULL descriptor : descriptor 내의 첫 번째 descriptor 는 항상 NULL 로 설정
	gdt_set_descriptor(0, 0, 0, 0, 0);

	// 모든 segment 주소는 0 에서 32bit 인 0xffffffff
	
	/*
	+) 32 bit address 는 왜 4GB 의 데이터를 표현할 수 있을까?

	2^32 = 4294967296 개의 Address 사용 가능
	한 개의 address 당 1byte 데이터 저장 가능

	따라서 4294967296 * byte = 4GB
	*/

	// kernel code descriptor : kernel code 실행 시 접근 권한을 기술한 descriptor
	gdt_set_descriptor(1, 0, 0xffffffff,
		I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA |
		I86_GDT_DESC_MEMORY, I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT |
		I86_GDT_GRAND_LIMITHI_MASK);

	// kernel data descriptor : kernel data 영역에 data 를 쓰거나 읽을 때 접근 권한을 기술한 descriptor

	/*
	I86_GDT_DESC_CODEDATA : CODE 나 Data Segment
	I86_GDT_DESC_MEMORY : Memory 상세 Segment 가 존재 가능
	I86_GDT_GRAND_4K : Segment Address 는 20bit 지만 4GB Address 까지 주소 접근 가능
	I86_GDT_GRAND_32BIT : Segment 는 32bit code 를 담고 있음
	*/
	gdt_set_descriptor(2, 0, 0xffffffff,
		I86_GDT_DESC_READWRITE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY,
		I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT | I86_GDT_GRAND_LIMITHI_MASK);

	// user code descriptor : user code 실행 시 접근 권한을 기술한 descriptor
	// I86_GDT_DESC_EXEC_CODE : Code segment 일 때에만 실행 가능
	gdt_set_descriptor(3, 0, 0xffffffff,
		I86_GDT_DESC_READWRITE | I86_GDT_DESC_EXEC_CODE | I86_GDT_DESC_CODEDATA |
		I86_GDT_DESC_MEMORY | I86_GDT_DESC_DPL, I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT |
		I86_GDT_GRAND_LIMITHI_MASK);

	// user data descriptor : user data 영역에 접근할 시 접근 권한을 기술한 descriptor
	gdt_set_descriptor(4, 0, 0xffffffff, I86_GDT_DESC_READWRITE | I86_GDT_DESC_CODEDATA | I86_GDT_DESC_MEMORY | I86_GDT_DESC_DPL,
		I86_GDT_GRAND_4K | I86_GDT_GRAND_32BIT | I86_GDT_GRAND_LIMITHI_MASK);

	// GDTR Register 에 GDT load
	InstallGDT();

	return 0;
}
