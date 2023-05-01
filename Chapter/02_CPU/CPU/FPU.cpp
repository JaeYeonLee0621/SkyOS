#include "FPU.h"

extern "C" void __writecr4(unsigned __int64 Data);
extern "C"  unsigned long __readcr4(void);

/*

[FPU (Floating Point Unit)]
- CPU 의 일부로 부동 소수점 연산을 효율적으로 처리하기 위한 HW 논리회로 모듈
- 이 장치를 활성화하지 않으면 double 형이나 float형은 사용할 수 없음
- FPU 를 활성화하려면 Assembly code 의 도움이 필요

*/

bool InitFPU()
{	
	int result = 0;
	unsigned short temp;

	__asm
	{
		pushad; 모든 register 를 stack 에 저장
		mov eax, cr0; eax = CR0
		and al, ~6; Clear the EM and MP flags(just in case) ~0110 = 1001
		mov cr0, eax; Set CR0
		fninit; Reset FPU status word
		mov temp, 0x5A5A; Make sure temporary word is non - zero
		fnstsw temp; Save the FPU status word in the temporary word
		cmp temp, 0; Was the correct status written to the temporary word, 상태값이 0이면 FPU 가 존재하지 않음
		jne noFPU; no, no FPU present
		fnstcw temp; Save the FPU control word in the temporary word
		mov ax, temp; ax = saved FPU control word
		and ax, 0x103F; ax = bits to examine
		cmp ax, 0x003F; Are the bits to examine correct ?
		jne noFPU; no, no FPU present
		mov result, 1; 이 구문이 실행되면 FPU 가 존재
		noFPU:
	
		popad
	}

	return result == 1;	
}

bool EnableFPU()
{
#ifdef _WIN32
	unsigned long regCR4 = __readcr4();
	__asm or regCR4, 0x200
	__writecr4(regCR4);
#else
	//mov eax, cr4;
	//or eax, 0x200
	//mov cr4, eax
#endif	
}


