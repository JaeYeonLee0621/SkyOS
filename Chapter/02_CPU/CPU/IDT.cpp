#include "idt.h"
#include "string.h"
#include "memory.h"
#include <hal.h>
#include "SkyAPI.h"

/*

[IDT (Interput Descriptor Table)]
- Software exception / Hardware interrupt 가 발생하면 이를 처리하기 위한 Service Routine 을 기술한 descriptor 모음 table

ex) interrupt 발생 시
- CPU 는 IDT 에서 interrupt descriptor 를 찾음
- descriptor 에는 GDT 의 descriptor index
- 즉 segment selector 값과 segment 의 base address 에서 해당 ISR (Interrupt Service Routing) 까지의 offset 값이 들어있음
- ISR address =  offset 값 + GDT 의 descriptor 에서 얻은 segment base address 

[Interrupt Service]
- interrupt number = 0x1f 까지는 exception handler 를 위해 할당
- 0x1f 보다 큰 값은 software interrupt service routing 을 위한 번호

[Interrupt 종류]

> Hardware interrupt

> Software interrupt

- fault : exception interrupt 가 발생하면 system이 망가지지 않았다고 판단
exception handler 수행이 끝나면 다시 복귀해서 해당 코드 수행

- abort : process 가 망가져서 system 을 복구할 수 없음을 의미
이런 경우 window OS 에서는 process 를 더 이상 수행시키지 않고 종료

- trap : Software interrupt 라고도 하며 의도적으로 interrupt 를 발생시킨 경우
exception 처리를 수행하고 나서 복귀할 경우 예외가 발생했던 명령어 다음부터 코드 실쟁

*/


// 256 개의 interrupt descriptor가 존재하며 처음 descriptor 는 항상 NULL 로 설정
static idt_descriptor	_idt [I86_MAX_INTERRUPTS];

// idtr register 에 load 될 값
// IDT의 memory address 및 IDT size 를 담고 있음
static struct idtr				_idtr;


// IDT 값 Register 에 등록
static void IDTInstall() {
#ifdef _MSC_VER
	_asm lidt [_idtr]
#endif
}

#define DMA_PICU1       0x0020
#define DMA_PICU2       0x00A0

__declspec(naked) void SendEOI()
{
	_asm
	{
		PUSH EBP
		MOV  EBP, ESP
		PUSH EAX

		; [EBP] < -EBP
		; [EBP + 4] < -RET Addr
		; [EBP + 8] < -IRQ ��ȣ

		MOV AL, 20H; EOI ��ȣ�� ������.
		OUT DMA_PICU1, AL

		CMP BYTE PTR[EBP + 8], 7
		JBE END_OF_EOI
		OUT DMA_PICU2, AL; Send to 2 also

		END_OF_EOI :
		POP EAX
			POP EBP
			RET
	}
}

// 다룰 수 있는 handler 가 존재하지 않을 때 호출되는 기본 handler
// 어떤 수행을 액션도 취하지 않고 handler 수행을 끝내는 코드
__declspec(naked) void InterruptDefaultHandler () {
	// register 를 저장하고 interrupt 를 끔
	_asm
	{
		PUSHAD
		PUSHFD
		CLI
	}

	SendEOI();

	// register 를 복원하고 원래 수행하던 곳으로 돌아감
	_asm
	{
		POPFD
		POPAD
		IRETD
	}
}


// 특정 descriptor 값을 읽어옴
idt_descriptor* GetInterruptDescriptor(uint32_t i) {

	if (i>I86_MAX_INTERRUPTS)
		return 0;

	return &_idt[i];
}

// interrupt service routine 을 설치
bool InstallInterrputHandler(uint32_t i, uint16_t flags, uint16_t sel, I86_IRQ_HANDLER irq) {

	if (i>I86_MAX_INTERRUPTS)
		return false;

	if (!irq)
		return false;

	uint64_t		uiBase = (uint64_t)&(*irq);
	
	if ((flags & 0x0500) == 0x0500) {
		_idt[i].selector = sel;
		_idt[i].flags = uint8_t(flags);
	}
	else
	{
		_idt[i].offsetLow = uint16_t(uiBase & 0xffff);
		_idt[i].offsetHigh = uint16_t((uiBase >> 16) & 0xffff);
		_idt[i].reserved = 0;
		_idt[i].flags = uint8_t(flags);
		_idt[i].selector = sel;
	}

	return	true;
}


// IDT 초기화
bool IDTInitialize(uint16_t codeSel) {

	// IDTR Register 에 load 될 structure 초기화
	_idtr.limit = sizeof(idt_descriptor) * I86_MAX_INTERRUPTS - 1;
	_idtr.base = (uint32_t)&_idt[0];

	// NULL descriptor
	memset((void*)&_idt[0], 0, sizeof(idt_descriptor) * I86_MAX_INTERRUPTS - 1);

	// default handler 등록
	// 예외에 대한 고유 handler 를 등록하지 않고, 모든 예외에서 공통적으로 사용되는 handler 등록
	for (int i = 0; i<I86_MAX_INTERRUPTS; i++)
		InstallInterrputHandler(i, I86_IDT_DESC_PRESENT | I86_IDT_DESC_BIT32,
			codeSel, (I86_IRQ_HANDLER)InterruptDefaultHandler);

	// IDTR register setup
	IDTInstall();

	return true;
}


