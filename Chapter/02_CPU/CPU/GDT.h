#pragma once
#include <stdint.h>

//! maximum amount of descriptors allowed
#define MAX_DESCRIPTORS					10

/***	 gdt descriptor access bit flags.	***/

//! set access bit
#define I86_GDT_DESC_ACCESS			0x0001			//00000001

//! descriptor is readable and writable. default: read only
#define I86_GDT_DESC_READWRITE			0x0002			//00000010

//! set expansion direction bit
#define I86_GDT_DESC_EXPANSION			0x0004			//00000100

//! executable code segment. Default: data segment
#define I86_GDT_DESC_EXEC_CODE			0x0008			//00001000

//! set code or data descriptor. defult: system defined descriptor
#define I86_GDT_DESC_CODEDATA			0x0010			//00010000

//! set dpl bits
#define I86_GDT_DESC_DPL			0x0060			//01100000

//! set "in memory" bit
#define I86_GDT_DESC_MEMORY			0x0080			//10000000

/**	gdt descriptor grandularity bit flags	***/

//! masks out limitHi (High 4 bits of limit)
#define I86_GDT_GRAND_LIMITHI_MASK		0x0f			//00001111

//! set os defined bit
#define I86_GDT_GRAND_OS			0x10			//00010000

//! set if 32bit. default: 16 bit
#define I86_GDT_GRAND_32BIT			0x40			//01000000

//! 4k grandularity. default: none
#define I86_GDT_GRAND_4K			0x80			//10000000

//============================================================================
//    INTERFACE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    INTERFACE STRUCTURES / UTILITY CLASSES
//============================================================================

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

//! gdt descriptor. A gdt descriptor defines the properties of a specific
//! memory block and permissions.

/*

[GDT (Global Descriptor Table)]
- CPU 보호 모드 기능을 사용하기 위해 OS 개발자가 작성 해야 하는 테이블
- 물리 메모리에 직접 접근해서 데이터를 읽거나 쓰는 행위가 불가능
- GDT 를 거쳐서 조건에 맞으면 가져와 실행, 그렇지 않으면 예외
- descriptor 는 8byte 구조체

검증 조건
- 접근하려는 주소가 유효한 주소 범위 내에 있는지 여부
- 현재 실행되는 스레드의 특권 레벨이 GDT 에 기술된 특권 레벨과 같거나 높은지

[DPL (특권 레벨)]
- 다른 말로는 링레벨이라고 함

*/

// GDT descriptor 는 8byte
struct gdt_descriptor {

	//! bits 0-15 of segment limit
	uint16_t		limit;

	//! bits 0-23 of base address
	uint16_t		baseLo;
	uint8_t			baseMid;

	//! descriptor access flags
	uint8_t			flags;

	uint8_t			grand;

	//! bits 24-32 of base address
	uint8_t			baseHi;
};

#ifdef _MSC_VER
#pragma pack (pop)
#endif

//! Setup a descriptor in the Global Descriptor Table
extern void gdt_set_descriptor(uint32_t i, uint64_t base, uint64_t limit, uint8_t access, uint8_t grand);

//! returns descritor
extern gdt_descriptor* i86_gdt_get_descriptor (int i);
extern	int GDTInitialize();
