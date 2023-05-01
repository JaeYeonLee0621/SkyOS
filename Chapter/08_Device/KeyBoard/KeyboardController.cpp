#include "KeyboardController.h"
#include "Hal.h"
#include "SkyAPI.h"
#include "SkyConsole.h"

extern void SendEOI();

//Ư��Ű ����
bool shift = false;	
bool ctrl = false;
bool alt = false;
bool caps = false;
bool num = false;

unsigned char leds = 0; //LED ����ũ
const unsigned int KEYBUFFSIZE = 129;	//Ű ���� ������

Func_Key FKey[10] =		//���Ű�� �Լ��� �������Ѽ� Ư�� �Լ��� ������ �� �ְ� �Ѵ�.
{						
        {false, 0},
        {false, 0},
        {false, 0},
        {false, 0},
        {false, 0},
        {false, 0},
        {false, 0},
        {false, 0},
        {false, 0},
        {false, 0}
};

// scan code 와 ASCII 와의 관계
unsigned char normal[] = {					
	0x00,0x1B,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
	'q','w','e','r','t','y','u','i','o','p','[',']',0x0D,0x80,
	'a','s','d','f','g','h','j','k','l',';',047,0140,0x80,
	0134,'z','x','c','v','b','n','m',',','.','/',0x80,
	'*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,'0',0177
};

unsigned char shifted[] = {
	0,033,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
	'Q','W','E','R','T','Y','U','I','O','P','{','}',015,0x80,
	'A','S','D','F','G','H','J','K','L',':',042,'~',0x80,
	'|','Z','X','C','V','B','N','M','<','>','?',0x80,
	'*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,'7','8','9',0x80,'4','5','6',0x80,
	'1','2','3','0',177
};

unsigned char capsNormal[] = {
	0x00,0x1B,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
	'Q','W','E','R','T','Y','U','I','O','P','[',']',0x0D,0x80,
	'A','S','D','F','G','H','J','K','L',';',047,0140,0x80,
	'|','Z','X','C','V','B','N','M',',','.','/',0x80,
	'*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,'0',0177
};

//����ƮŰ Caps LockŰ �Ѵ� �������
unsigned char capsShifted[] = {
	0,033,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
	'q','w','e','r','t','y','u','i','o','p','{','}',015,0x80,
	'a','s','d','f','g','h','j','k','l',':',042,'~',0x80,
	0134,'z','x','c','v','b','n','m','<','>','?',0x80,
	'*',0x80,' ',0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x80,0x80,0x80,0x80,'7','8','9',0x80,'4','5','6',0x80,
	'1','2','3','0',177
};

char buffer[KEYBUFFSIZE];	//Ű ����
int  buffend = 0;		//���ۿ� ����� ������ Ű���� ����Ų��.
unsigned char scanCode;	//Ű����κ��� ���� ��ĵ�ڵ尪


KeyboardController::KeyboardController()
{
}

KeyboardController::~KeyboardController()
{
}

void KeyboardController::UpdateLeds(unsigned char led)	//Ű���� LED�� ������Ʈ�Ѵ�.
{
	if(led == 0)
	{
		leds = 0;
	}
	else
	{
		if (leds == (leds|led))	//LED�� ���� �ִٸ�
		{
			leds = leds^led;	//LED�� ����
		}
		else
		{
			leds = leds | led;	// LED�� �Ҵ�
		}
	}

	OutPortByte(0x64, 0xED);
	while ((InPortByte(0x64) % 2) == 2)
		;
	OutPortByte(0x60, leds);
	
}


// Stack Frame 을 형성하지 않기 때문에 함수 앞에 _declspec(naked) keyword 명시
// interrupt handler 실행 중에 또 다른 interrupt 상황을 방지하기 위해 interrupt 발생을 막는 Assembly CLI 를 사용

__declspec(naked) void KeyboardHandler()
{

	// Register 를 저장하고 interrupt 를 끔
	// _asm : inline assembly
	_asm
	{
		PUSHAD // 기본 register 값을 stack 에 저장
		PUSHFD
		CLI // interrupt 발생을 막음
	}

	// Stack 상태가 변하는 것을 막기 위해 함수 호출
	// call : 함수 호출
	_asm call KeyboardController::HandleKeyboardInterrupt

	SendEOI();

	// Register 를 복원하고 원래 수행하던 곳으로 돌아감
	_asm
	{
		POPFD // 기본 register 값을 stack 에서 복원
		POPAD
		IRETD // 원래 수행하던 코드로 복원하는 명령어 (code 수행 흐름 변경)
	}
}

int KeyboardController::SpecialKey(unsigned char key)
{
	static bool specKeyUp = true;	
	switch (key)
	{
	case 0x36: //R-Shift down
	case 0x2A: //L-Shift down
		shift = true;
		break;
	case 0xB6: //R-Shift up
	case 0xAA: //L-Shift up
		shift = false;
		break;
	case 0x1D: //Control down
		ctrl = true;
		break;
	case 0x9D: //Control up
		ctrl = false;
		break;
	case 0x38: //Alt down
		alt = true;
		break;
	case 0xB8: //Alt up
		alt = false;
		break;
	case 0x3A: //Caps down
		if (specKeyUp == true)
		{
			caps = !caps;
			UpdateLeds(CapsLock);
			specKeyUp = false;
		}
		break;
	case 0x45: //Num down
		if (specKeyUp == true)
		{
			num = !num;
			UpdateLeds(NumLock);
			specKeyUp = false;
		}
		break;
	case 0x46: //Scroll down
		if (specKeyUp == true)
		{
			num = !num;
			UpdateLeds(ScrollLock);
			specKeyUp = false;
		}
		break;
	case 0x3B: //F1 Down
		if (specKeyUp && FKey[0].enabled)
		{
			FKey[0].func();
			specKeyUp = false;
		}
		break;
	case 0x3C: //F2 Down
		if (specKeyUp && FKey[1].enabled)
		{
			FKey[1].func();
			specKeyUp = false;
		}
		break;
	case 0x3D: //F3 Down
		if (specKeyUp && FKey[2].enabled)
		{
			FKey[2].func();
			specKeyUp = false;
		}
		break;
	case 0x3E: //F4 Down
		if (specKeyUp && FKey[3].enabled)
		{
			FKey[3].func();
			specKeyUp = false;
		}
		break;
	case 0x3F: //F5 Down
		if (specKeyUp && FKey[4].enabled)
		{
			FKey[4].func();
			specKeyUp = false;
		}
		break;
	case 0x40: //F6 Down
		if (specKeyUp && FKey[5].enabled)
		{
			FKey[5].func();
			specKeyUp = false;
		}
		break;
	case 0x41: //F7 Down
		if (specKeyUp && FKey[6].enabled)
		{
			FKey[6].func();
			specKeyUp = false;
		}
		break;
	case 0x42: //F8 Down
		if (specKeyUp && FKey[7].enabled)
		{
			FKey[7].func();
			specKeyUp = false;
		}
		break;
	case 0x43: //F9 Down
		if (specKeyUp && FKey[8].enabled)
		{
			FKey[8].func();
			specKeyUp = false;
		}
		break;
	case 0x44: //F10 Down
		if (specKeyUp && FKey[9].enabled)
		{
			FKey[9].func();
			specKeyUp = false;
		}
		break;
	case 0xBA: //Caps Up
	case 0xBB: //F1 Up
	case 0xBC: //F2 Up
	case 0xBD: //F3 Up
	case 0xBE: //F4 Up
	case 0xBF: //F5 Up
	case 0xC0: //F6 Up
	case 0xC1: //F7 Up
	case 0xC2: //F8 Up
	case 0xC3: //F9 Up
	case 0xC4: //F10 Up
	case 0xC5: //Num Up
	case 0xC6: //Scroll Up
		specKeyUp = true;
		break;
	case 0xE0:
		break;
	default:
		return(0);
	}
	return (1);
}

void KeyboardController::FlushBuffers()
{
	unsigned char c = 0;
	while ((c = InPortByte(0x60)) != InPortByte(0x60))
		;
}

void KeyboardController::SetupInterrupts()
{
	FlushBuffers();
	
	// interrupt number 3 에서 발생
	setvect(33, KeyboardHandler);
}

void KeyboardController::SetLEDs(bool scroll, bool number, bool capslk)
{
	unsigned char status = scroll ? 1 : 0;

	if (number)	//Bit 2 : Num Lock
		status |= 2;
	if (capslk)//Bit 3:	Caps Lock
		status |= 4;

	while ((InPortByte(0x64) & 2) == 2)
		;

	OutPortByte(0x64, 0xED);

	while ((InPortByte(0x64) % 2) == 2)
		;

	OutPortByte(0x60, status);
}

char KeyboardController::GetInput()	
{
	int i = 0;

	// Keyboard 로 data 가 들어올 때까지 기다림
	while (buffend == 0) 
	{		
		//msleep(10);
	}

	// buffer 를 수정하는 동안 interrupt 비활성화
	kEnterCriticalSection();	

	for (; i < buffend; i++)
	{
		buffer[i] = buffer[i + 1];
	}
	buffend--;

	// interrupt 활성화
	kLeaveCriticalSection();

	return buffer[0];
}

void KeyboardController::HandleKeyboardInterrupt()
{
	unsigned char asciiCode;

	scanCode = InPortByte(0x60);	// Key scan code 를 얻음

	if (!(SpecialKey(scanCode) | (scanCode >= 0x80))) // ASCII 코드라면
	{
		// shift Key 와 Caps Lock 상태의 적절한 ASCII 값을 얻음
		if (shift)
		{
			if (!caps)
			{
				asciiCode = shifted[scanCode];
			}
			else
			{
				asciiCode = capsShifted[scanCode];
			}
		}
		else
		{
			if (!caps)
			{
				asciiCode = normal[scanCode];
			}
			else
			{
				asciiCode = capsNormal[scanCode];
			}
		}

		// Key buffer 에 ASCII 값 기록
		if (buffend != (KEYBUFFSIZE - 1))
		{
			buffend++;
		}
		buffer[buffend] = asciiCode;
	}
}