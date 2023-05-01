#include "PIT.h"
#include "Hal.h"
#include "SkyConsole.h"

#define		I86_PIT_REG_COUNTER0		0x40
#define		I86_PIT_REG_COUNTER1		0x41
#define		I86_PIT_REG_COUNTER2		0x42
#define		I86_PIT_REG_COMMAND			0x43

extern void SendEOI();

volatile uint32_t _pitTicks = 0;
DWORD _lastTickCount = 0;


/*

[PIT (Programmable Interval Timer)]
- 일상적인 Timing 제어 문제를 해결하기 위해 설계된 Counter/Timer device

*/

__declspec(naked) void InterruptPITHandler() 
{	
	_asm
	{
		PUSHAD
		PUSHFD
		CLI
	}

	// 두 개의 변수 차이가 100일 때 = 1초 마다
	if (_pitTicks - _lastTickCount >= 100)
	{
		_lastTickCount = _pitTicks;
		SkyConsole::Print("Timer Count : %d\n", _pitTicks);
	}

	// Timer interrupt handler 가 호출될 때마다 Count 증가
	_pitTicks++;
	
	SendEOI();

	_asm
	{
		POPFD
		POPAD
		IRETD
	}
}

// Timer 도 interrupt 형태로 CPU 에 제공
// 우리는 interrupt service routine 을 구현해야 함
void StartPITCounter(uint32_t freq, uint8_t counter, uint8_t mode) {

	if (freq == 0)
		return;

	uint16_t divisor = uint16_t(1193181 / (uint16_t)freq);

	// command 전송
	uint8_t ocw = 0;
	ocw = (ocw & ~I86_PIT_OCW_MASK_MODE) | mode;
	ocw = (ocw & ~I86_PIT_OCW_MASK_RL) | I86_PIT_OCW_RL_DATA;
	ocw = (ocw & ~I86_PIT_OCW_MASK_COUNTER) | counter;
	SendPITCommand(ocw);

	// frequency ratio 설정
	SendPITData(divisor & 0xff, 0);
	SendPITData((divisor >> 8) & 0xff, 0);

	// timer tick count reset
	_pitTicks = 0;
}

// PIT 초기화
void InitializePIT()
{
	setvect(32, InterruptPITHandler);
}

uint32_t SetPITTickCount(uint32_t i) {

	uint32_t ret = _pitTicks;
	_pitTicks = i;
	return ret;
}


//! returns current tick count
uint32_t GetPITTickCount() {

	return _pitTicks;
}

unsigned int GetTickCount()
{
	return _pitTicks;
}

void _cdecl msleep(int ms)
{

	unsigned int ticks = ms + GetTickCount();
	while (ticks > GetTickCount())
		;
}

void SendPITCommand(uint8_t cmd) {

	OutPortByte(I86_PIT_REG_COMMAND, cmd);
}

void SendPITData(uint16_t data, uint8_t counter) {

	uint8_t	port = (counter == I86_PIT_OCW_COUNTER_0) ? I86_PIT_REG_COUNTER0 :
		((counter == I86_PIT_OCW_COUNTER_1) ? I86_PIT_REG_COUNTER1 : I86_PIT_REG_COUNTER2);

	OutPortByte(port, (uint8_t)data);
}