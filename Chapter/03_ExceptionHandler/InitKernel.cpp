#include "InitKernel.h"
#include "Hal.h"
#include "Exception.h"
#include "IDT.h"

/*

원래 수행하던 흐름에서 예외가 발생해서 예외처리를 해야한다는 것
예외처리가 끝나면 예외의 종류에 따라 원래 코드 흐름으로 복귀 또는 프로그램 종료
interrupt 가 발생하면 register information 이나 Program coutner 등을 kernel stack 에 저장한 후
interrupt service 를 호출

interrupt service 를 완료하면
kernel stack 에 저장한 register 정보를 복원하고
원래 수행하고 있던 작업을 재개

*/

void SetInterruptVector()
{
	setvect(0, (void(__cdecl &)(void))kHandleDivideByZero);
	setvect(1, (void(__cdecl &)(void))kHandleSingleStepTrap);
	setvect(2, (void(__cdecl &)(void))kHandleNMITrap);

	// Break Point 에 hit 했을 때 발생하는 Software interrupt
	setvect(3, (void(__cdecl &)(void))kHandleBreakPointTrap);

	// 산술 연산 overflow
	setvect(4, (void(__cdecl &)(void))kHandleOverflowTrap);
	setvect(5, (void(__cdecl &)(void))kHandleBoundsCheckFault);

	// 유효하지 않은 OPCODE 실행
	// Reversing 을 하다 보면 명령어 pointer 를 변경해서 가끔 코드가 아니라 데이터 부분을 실행할 필요가 있음
	// 이 데이터 부분을 내용이 정상저인 OPCODE 가 아니라면 예외가 발생 가능
	setvect(6, (void(__cdecl &)(void))kHandleInvalidOpcodeFault);
	setvect(7, (void(__cdecl &)(void))kHandleNoDeviceFault);

	// 예외 처리 중 다시 예외 발생
	setvect(8, (void(__cdecl &)(void))kHandleDoubleFaultAbort);
	setvect(10, (void(__cdecl &)(void))kHandleInvalidTSSFault);
	setvect(11, (void(__cdecl &)(void))kHandleSegmentFault);
	setvect(12, (void(__cdecl &)(void))kHandleStackFault);

	// 일반 보호 오류
	setvect(13, (void(__cdecl &)(void))kHandleGeneralProtectionFault);

	// Page Fault
	// 물리 메모리에 페이지 디렉토리나 페이지 테이블이 메모리에 존재하지 않을 때
	// 보호 모드에서 체크하는 특권 레벨이 낮거나 읽기/쓰기 권한이 없을 때
	// Page directory entry 나 page table entry 의 예약 bit 가 1로 설정되어 있을 때
	setvect(14, (void(__cdecl &)(void))kHandlePageFault);
	setvect(16, (void(__cdecl &)(void))kHandlefpu_fault);
	setvect(17, (void(__cdecl &)(void))kHandleAlignedCheckFault);
	setvect(18, (void(__cdecl &)(void))kHandleMachineCheckAbort);
	setvect(19, (void(__cdecl &)(void))kHandleSIMDFPUFault);

	setvect(33, (void(__cdecl &)(void))InterrputDefaultHandler);
	setvect(38, (void(__cdecl &)(void))InterrputDefaultHandler);
}