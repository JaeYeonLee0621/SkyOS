#ifndef _IDT_H
#define _IDT_H
#include <stdint.h>
#include "windef.h"

#define I86_MAX_INTERRUPTS		256

#define I86_IDT_DESC_BIT16		0x06	//00000110
#define I86_IDT_DESC_BIT32		0x0E	//00001110
#define I86_IDT_DESC_RING1		0x40	//01000000
#define I86_IDT_DESC_RING2		0x20	//00100000
#define I86_IDT_DESC_RING3		0x60	//01100000
#define I86_IDT_DESC_PRESENT	0x80	//10000000

typedef void (_cdecl *I86_IRQ_HANDLER)(void);

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

/*

IDT 의 위치를 CPU 에 알려주기 위해 IDTR structure 를 생성
CPU 는 IDTR register 를 통해 IDT 에 접근

*/

struct idtr {

	uint16_t limit;

	uint32_t base;
};

/*

크기 = 8byte

offsetLow, offsetHigh 를 이용해서 ISR 4byte offset 값을 얻어낼 수 있음
selector 를 이용해서 GDT descriptor 를 찾아서 segment base address 를 얻어낼 수 있음

*/

typedef struct tag_idt_descriptor
{
	USHORT offsetLow; // interrupt handler address : 0-16ㅠㅑㅅ

	// 2byte : GDT 내 global descriptor 를 찾을 수 있음
	// 이를 통해 Segment base address 를 얻어낼 수 있음
	// 여기에 offset 값을 더해서 ISR 주소를 얻을 수 있음
	USHORT selector;

	BYTE reserved; // 예약된 값 0
	BYTE flags; // 8bit bit flag
	USHORT offsetHigh; // interrupt handler address 의 16~32bit

}idt_descriptor;

#ifdef _MSC_VER
#pragma pack (pop)
#endif

extern idt_descriptor* GetInterruptDescriptor(uint32_t i);

extern bool InstallInterrputHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER);
extern bool IDTInitialize(uint16_t codeSel);

#endif
