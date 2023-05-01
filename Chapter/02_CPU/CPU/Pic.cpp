
#ifndef ARCH_X86
#error "[pic.cpp for i86] requires i86 architecture. Define ARCH_X86"
#endif

#include <Hal.h>
#include "pic.h"


//-----------------------------------------------
//	Controller Registers
//-----------------------------------------------

//! PIC 1 register port addresses
#define I86_PIC1_REG_COMMAND	0x20
#define I86_PIC1_REG_STATUS		0x20
#define I86_PIC1_REG_DATA		0x21
#define I86_PIC1_REG_IMR		0x21

//! PIC 2 register port addresses
#define I86_PIC2_REG_COMMAND	0xA0
#define I86_PIC2_REG_STATUS		0xA0
#define I86_PIC2_REG_DATA		0xA1
#define I86_PIC2_REG_IMR		0xA1

//-----------------------------------------------
//	Initialization Command Bit Masks
//-----------------------------------------------

//! Initialization Control Word 1 bit masks
#define I86_PIC_ICW1_MASK_IC4			0x1			//00000001
#define I86_PIC_ICW1_MASK_SNGL			0x2			//00000010
#define I86_PIC_ICW1_MASK_ADI			0x4			//00000100
#define I86_PIC_ICW1_MASK_LTIM			0x8			//00001000
#define I86_PIC_ICW1_MASK_INIT			0x10		//00010000

//! Initialization Control Words 2 and 3 do not require bit masks

//! Initialization Control Word 4 bit masks
#define I86_PIC_ICW4_MASK_UPM			0x1			//00000001
#define I86_PIC_ICW4_MASK_AEOI			0x2			//00000010
#define I86_PIC_ICW4_MASK_MS			0x4			//00000100
#define I86_PIC_ICW4_MASK_BUF			0x8			//00001000
#define I86_PIC_ICW4_MASK_SFNM			0x10		//00010000

//-----------------------------------------------
//	Initialization Command 1 control bits
//-----------------------------------------------

#define I86_PIC_ICW1_IC4_EXPECT				1			//1
#define I86_PIC_ICW1_IC4_NO					0			//0
#define I86_PIC_ICW1_SNGL_YES				2			//10
#define I86_PIC_ICW1_SNGL_NO				0			//00
#define I86_PIC_ICW1_ADI_CALLINTERVAL4		4			//100
#define I86_PIC_ICW1_ADI_CALLINTERVAL8		0			//000
#define I86_PIC_ICW1_LTIM_LEVELTRIGGERED	8			//1000
#define I86_PIC_ICW1_LTIM_EDGETRIGGERED		0			//0000
#define I86_PIC_ICW1_INIT_YES				0x10		//10000
#define I86_PIC_ICW1_INIT_NO				0			//00000

//-----------------------------------------------
//	Initialization Command 4 control bits
//-----------------------------------------------

#define I86_PIC_ICW4_UPM_86MODE			1			//1
#define I86_PIC_ICW4_UPM_MCSMODE		0			//0
#define I86_PIC_ICW4_AEOI_AUTOEOI		2			//10
#define I86_PIC_ICW4_AEOI_NOAUTOEOI		0			//0
#define I86_PIC_ICW4_MS_BUFFERMASTER	4			//100
#define I86_PIC_ICW4_MS_BUFFERSLAVE		0			//0
#define I86_PIC_ICW4_BUF_MODEYES		8			//1000
#define I86_PIC_ICW4_BUF_MODENO			0			//0
#define I86_PIC_ICW4_SFNM_NESTEDMODE	0x10		//10000
#define I86_PIC_ICW4_SFNM_NOTNESTED		0			//a binary 2 (futurama joke hehe ;)

/*

[PIC (Programmable Interrupt Controller)]

- Hardware 가 제공하는 신호를 CPU 에서 어떻게 받아들이는지 처리
- Keyboard, Mouse 등을 CPU 에 전달하는 제어기

[과정 - Master PIC]
1. master PIC 에서 interrupt 가 발생
2. master PIC 는 자신의 INT 에 신호를 싣고 CPU 의 INT 에 전달
3. CPU 가 interrupt 를 받으면 EFLAG 의 IE bit 를 1로 세팅하고, INTA 를 통해 받았다는 신호를 PIC 에 전달
4. CPU 는 현재 실행 모드가 보호 모드라면 IDT descriptor 를 찾아서 interrupt handler를 실행

[과정 - Slave PIC]
1. Slave PIC 에서 interrupt 발생
2. Slave PIC 는 자신의 INT 판에 신호를 싣고, master PIC IRQ 2번에 interrup signal 을 보냄
3. master PIC 는 위에서 설명한 5가지 절차를 진행

IRQ HW interrupt 가 발생할 때 적절히 작동하도록 하기 위해 PIC가 가진 각 IRQ를 초기화 해줘야 함
이를 위해 master PIC 의 명령 register 로 명령을 전달해야함
이때 ICW (Initialization control Word) 가 사용
ICW 는 4가지 초기화 명령어로 구성

*/

// PIC 로 명령어 전송
inline void SendCommandToPIC(uint8_t cmd, uint8_t picNum) {

	if (picNum > 1)
		return;

	uint8_t	reg = (picNum==1) ? I86_PIC2_REG_COMMAND : I86_PIC1_REG_COMMAND;
	OutPortByte(reg, cmd);
}

// PIC 로 데이터를 보냄
inline void SendDataToPIC(uint8_t data, uint8_t picNum) {

	if (picNum > 1)
		return;

	uint8_t	reg = (picNum==1) ? I86_PIC2_REG_DATA : I86_PIC1_REG_DATA;
	OutPortByte(reg, data);
}

// PIC 로부터 1byte 를 읽음
inline uint8_t ReadDataFromPIC(uint8_t picNum) {

	if (picNum > 1)
		return 0;

	uint8_t	reg = (picNum==1) ? I86_PIC2_REG_DATA : I86_PIC1_REG_DATA;
	return InPortByte(reg);
}

void PICInitialize(uint8_t base0, uint8_t base1) {

	uint8_t		icw = 0;

	// PIC initialization ICW1 명령을 보냄
	icw = (icw & ~I86_PIC_ICW1_MASK_INIT) | I86_PIC_ICW1_INIT_YES;
	icw = (icw & ~I86_PIC_ICW1_MASK_IC4) | I86_PIC_ICW1_IC4_EXPECT;

	SendCommandToPIC(icw, 0);
	SendCommandToPIC(icw, 1);

	// PIC 에 ICW2 명령을 보냄
	// base0, base2 = IRQ 의 base address
	SendDataToPIC(base0, 0);
	SendDataToPIC(base1, 1);
	
	// PIC 에 ICW3 명령을 보냄
	// master & slave  PIC 와의 관계 정립
	SendDataToPIC(0x04, 0);
	SendDataToPIC(0x02, 1);

	// ICW4 명령을 보냄
	// i86 mode 활성화
	icw = (icw & ~I86_PIC_ICW4_MASK_UPM) | I86_PIC_ICW4_UPM_86MODE;

	SendDataToPIC(icw, 0);
	SendDataToPIC(icw, 1);

	// PIC initialization 완료
}

