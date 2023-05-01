#include "SkyOS.h"

extern void SendEOI();

__declspec(naked) void _HDDInterruptHandler() {

	_asm
	{
		PUSHAD
		PUSHFD
		CLI
	}

	SendEOI();

	_asm
	{
		POPFD
		POPAD
		IRETD
	}
}

BYTE HardDiskHandler::GetTotalDevices()
{
	return (BYTE)HDDs.Count();
}
//---------------------------------------------------------------------
//        This function returns the description of the last error
//---------------------------------------------------------------------
char * HardDiskHandler::GetLastError(BYTE errorCode)
{
	switch (errorCode)
	{
	case HDD_NO_ERROR:
		return "No Error";
	case HDD_NOT_FOUND:
		return "HDD Not Found";
	case HDD_CONTROLLER_BUSY:
		return "Device Controller Busy";
	case HDD_DATA_NOT_READY:
		return "Device Data Not Ready";
	case HDD_DATA_COMMAND_NOT_READY:
		return "Device not ready";
	default:
		return "Undefined Error";
	}
}

BYTE ReadErrorRegister(BYTE deviceController)
{
	BYTE Status = InPortByte(IDE_Con_IOBases[deviceController][0] + IDE_CB_STATUS);
	if ((Status & 0x80) == 0 && (Status & 0x1)) //busy bit=0 and err bit=1
	{
		Status = InPortByte(IDE_Con_IOBases[deviceController][0] + IDE_CB_ERROR);
		return Status;
	}
	else
		return 0;
}


BOOLEAN IsDeviceDataReady(int deviceController, DWORD waitUpToms = 0, BOOLEAN checkDataRequest = TRUE)
{
	UINT32 Time1, Time2;
	Time1 = GetTickCount();
	do
	{
		UINT16 PortID = IDE_Con_IOBases[deviceController][0] + IDE_CB_STATUS;
		BYTE Status = InPortByte(PortID);
		if ((Status & 0x80) == 0) //Checking BSY bit, because DRDY bit is valid only when BSY is zero
		{
			if (Status & 0x40) //checking DRDY is set
				if (checkDataRequest) // if DataRequest is also needed
				{
					if (Status & 0x8) // DRQ bit set
					{
						return TRUE;
					}
				}
				else
				{
					return TRUE;
				}
		}
		Time2 = GetTickCount();
	} while ((Time2 - Time1) < waitUpToms);

	return FALSE;
}

BOOLEAN IsDeviceControllerBusy(int DeviceController, int WaitUpToms = 0)
{
	UINT32 Time1, Time2;
	Time1 = GetTickCount();
	do {

		UINT16 PortID = IDE_Con_IOBases[DeviceController][0] + IDE_CB_STATUS;
		BYTE Status = InPortByte(PortID);
		if ((Status & 0x80) == 0) //BSY bit 
			return FALSE;
		Time2 = GetTickCount();
	} while ((Time2 - Time1) <= (UINT32)WaitUpToms);

	return TRUE;
}

BYTE HardDiskHandler::DoSoftwareReset(UINT16 DeviceController)
{
	BYTE DeviceControl = 4; 
	OutPortByte(IDE_Con_IOBases[DeviceController][0] + IDE_CON_DEVICE_CONTROL, DeviceControl);
	DeviceControl = 0;      
	OutPortByte(IDE_Con_IOBases[DeviceController][0] + IDE_CON_DEVICE_CONTROL, DeviceControl);

	return InPortByte(IDE_Con_IOBases[DeviceController][0] + IDE_CB_ERROR);
}
BOOLEAN HardDiskHandler::IsRemovableDevice(BYTE * DPF)
{
	return !(HDDs.Item((char *)DPF)->DeviceID[0] & 0x70);
}
BOOLEAN HardDiskHandler::IsRemovableMedia(BYTE * DPF)
{
	return HDDs.Item((char *)DPF)->DeviceID[0] & 0x80;
}

HardDiskHandler::HardDiskHandler()
{

}


/*

모든 Device Controller 를 확인해서 이용할 수 있는지 체크

1. loop 를 돌면서 각 Device controller의 busy bit 확인
이 value 가 설정돼 있으면, 해당 device controller 는 사용할 수 없음

2. device 를 진단하는 command 를 보냄

3. 특정 시간 대기 동안 Busy bit 가 clear 되면 Device Controller 에 접근 가능

4. 진단 command 결과를 Error register 로부터 읽음
	a) 0 번째 bit 값 : master disk 가 설치
	b) 7 번째 bit 값 : slave disk 가 설치

5. device head register 에 적당한 bit 값 설정

6. device command 를 보냄

7. device 로 부터 512 byte 정보를 읽음

*/

void HardDiskHandler::Initialize()
{
	char strKey[3] = "H0"; 
	
	// 아무런 역할을 하지 않는 HD Interrupt handler 지만 정의를 해야함
	// CPU 에게 제어권을 돌림
	// 등록해두지 않으면, OS 가 비정상적으로 동작
	setvect(32 + 14, _HDDInterruptHandler);
	setvect(32 + 15, _HDDInterruptHandler);

	// Collection structure : 검색된 HD 를 list 형태로 관리
	HDDs.Initialize();	

	// Device Controller 를 통해 HD 를 찾음
	for (int DeviceController = 0; DeviceController < IDE_CONTROLLER_NUM; DeviceController++)
	{
		DoSoftwareReset(DeviceController); 

		// Device Controller 를 사용할 수 없으면 패스
		if (IsDeviceControllerBusy(DeviceController, 1000)) 
			continue;
		
		// Device 진단 요청
		OutPortByte(IDE_Con_IOBases[DeviceController][0] + IDE_CB_COMMAND, IDE_COM_EXECUTE_DEVICE_DIAGNOSTIC);

		// Error Register 로부터 결과를 얻음
		BYTE result = InPortByte(IDE_Con_IOBases[DeviceController][0] + IDE_CB_ERROR);

		// Master, Slace Disk 에 대해 loop 를 돔
		for (BYTE device = 0; device < 1; device++)     
		{
			UINT16 DeviceID_Data[512], j;
			
			//if (device == 0 && !(result & 1))
				//continue;
			
			if (device == 1 && (result & 0x80)) 
				continue;

			// Device IO 가 가능하다면
			if (device == 1) // Slave 인가
				OutPortByte(IDE_Con_IOBases[DeviceController][0] + IDE_CB_DEVICE_HEAD, 0x10); //Setting 4th bit(count 5) to set device as 1
			else
				OutPortByte(IDE_Con_IOBases[DeviceController][0] + IDE_CB_DEVICE_HEAD, 0x0);

			//msleep(50);

			// Device information 요청
			OutPortByte(IDE_Con_IOBases[DeviceController][0] + IDE_CB_COMMAND, IDE_COM_IDENTIFY_DEVICE);

			// Device information 이 채워질 때까지 대기
			if (!IsDeviceDataReady(DeviceController, 600, TRUE)) 
			{
				SkyConsole::Print("Data not ready %d\n", DeviceController);
				continue;
			}

			// Device 로부터 512 byte 정보를 읽어들임
			for (j = 0; j < 256; j++)
				DeviceID_Data[j] = InPortWord(IDE_Con_IOBases[DeviceController][0] + IDE_CB_DATA);
			
			// HDD node 생성
			HDDInfo * newHDD = (HDDInfo *)kmalloc(sizeof(HDDInfo));
			if (newHDD == NULL)
			{
				SkyConsole::Print("HDD Initialize :: Allocation failed\n");
				return;
			}

			// HDD node 에 Device 정보 기록
			newHDD->IORegisterIdx = DeviceController;
			memcpy(newHDD->DeviceID, DeviceID_Data, 512);
			newHDD->DeviceNumber = device;
			newHDD->LastError = 0;

			newHDD->BytesPerSector = 512; 

			newHDD->CHSCylinderCount = DeviceID_Data[1];
			newHDD->CHSHeadCount = DeviceID_Data[3];
			newHDD->CHSSectorCount = DeviceID_Data[6];

			if (DeviceID_Data[10] == 0)
				strcpy(newHDD->SerialNumber, "N/A");
			else
				for (j = 0; j < 20; j += 2)
				{
					newHDD->SerialNumber[j] = DeviceID_Data[10 + (j / 2)] >> 8;
					newHDD->SerialNumber[j + 1] = (DeviceID_Data[10 + (j / 2)] << 8) >> 8;
				}
			if (DeviceID_Data[23] == 0)
				strcpy(newHDD->FirmwareRevision, "N/A");
			else
				for (j = 0; j < 8; j += 2)
				{
					newHDD->FirmwareRevision[j] = DeviceID_Data[23 + (j / 2)] >> 8;
					newHDD->FirmwareRevision[j + 1] = (DeviceID_Data[23 + (j / 2)] << 8) >> 8;
				}

			if (DeviceID_Data[27] == 0)
				strcpy(newHDD->ModelNumber, "N/A");
			else
				for (j = 0; j < 20; j += 2)
				{
					newHDD->ModelNumber[j] = DeviceID_Data[27 + (j / 2)] >> 8;
					newHDD->ModelNumber[j + 1] = (DeviceID_Data[27 + (j / 2)] << 8) >> 8;
				}
			newHDD->LBASupported = DeviceID_Data[49] & 0x200;
			newHDD->DMASupported = DeviceID_Data[49] & 0x100;

			UINT32 LBASectors = DeviceID_Data[61];
			LBASectors = LBASectors << 16;
			LBASectors |= DeviceID_Data[60];			
			newHDD->LBACount = LBASectors;

			// Structure 에 정보를 채운 후 HD 목록에 추가
			HDDs.Add(newHDD, strKey);

			SkyConsole::Print("DeviceId : %x, %s\n", device, newHDD->ModelNumber);
			strKey[1]++; // 새 HD node 를 위해 HD ID 변경
		}
	}
}

HardDiskHandler::~HardDiskHandler()
{
	HDDs.Clear();
}
HDDInfo * HardDiskHandler::GetHDDInfo(BYTE * DPF)
{

	HDDInfo * getHDD, *retHDD = (HDDInfo *)kmalloc(sizeof(HDDInfo));
	getHDD = HDDs.Item((char *)DPF);
	if (getHDD == NULL)
	{
		m_lastError = HDD_NOT_FOUND;
		return NULL;
	}
	memcpy(retHDD, getHDD, sizeof(HDDInfo));
	return retHDD;
}

/*

1. HDDInfo 객체를 읽음
2. Device 를 사용할 수 있는지 확인
3. Device bit 설정
4. Device 가 Data command 를 받아들일 준비가 됐는지 확인
5. Head, Track, 기타 값들을 설정
6. Read command 를 보냄
7. Device 가 Data transfer를 할 수 있는 준비가 됐는지 확인
8. Data Register에 접근해서 Data 읽음
9. Data Register로 부터 읽은 Data를 buffer 에 기록

*/

BYTE HardDiskHandler::ReadSectors(BYTE * DPF, UINT16 StartCylinder, BYTE StartHead, BYTE StartSector, BYTE NoOfSectors, BYTE * buffer, BOOLEAN WithRetry)
{
	HDDInfo * pHDDInfo;
	BYTE DevHead, StartCylHigh = 0, StartCylLow = 0;

	// HD id 로부터 HD 정보를 얻음
	pHDDInfo = HDDs.Item((char *)DPF);
	if (pHDDInfo == NULL)
	{
		m_lastError = HDD_NOT_FOUND;
		return HDD_NOT_FOUND;
	}

	if (pHDDInfo->DeviceNumber == 0)
		DevHead = StartHead | 0xA0;
	else
		DevHead = StartHead | 0xB0;

	// Device 가 준비될 때까지 대기
	if (IsDeviceControllerBusy(pHDDInfo->IORegisterIdx, 1 * 60))
	{
		m_lastError = HDD_CONTROLLER_BUSY;
		return HDD_CONTROLLER_BUSY;
	}

	// Device 가 Data command 를 받아들일 준비가 됐는 지 확인
	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_DEVICE_HEAD, DevHead);

	if (!IsDeviceDataReady(pHDDInfo->IORegisterIdx, 1 * 60, FALSE))
	{
		m_lastError = HDD_DATA_COMMAND_NOT_READY;
		return HDD_DATA_COMMAND_NOT_READY;
	}

	StartCylHigh = StartCylinder >> 8;
	StartCylLow = (StartCylinder << 8) >> 8;

	// 읽어들일 위치 지정
	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_CYLINDER_HIGH, StartCylHigh);
	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_CYLINDER_LOW, StartCylLow);
	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_SECTOR, StartSector);
	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_SECTOR_COUNT, NoOfSectors);

	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_COMMAND, WithRetry ? IDE_COM_READ_SECTORS_W_RETRY : IDE_COM_READ_SECTORS);

	// 요청한 sector 수만큼 data 를 읽음
	for (BYTE j = 0; j < NoOfSectors; j++)
	{
		// device 에 data 가 준비됐는가?
		if (!IsDeviceDataReady(pHDDInfo->IORegisterIdx, 1 * 60, TRUE))
		{
			m_lastError = HDD_DATA_NOT_READY;
			return HDD_DATA_NOT_READY;
		}

		// 이 loop 를 통해 sector size 인 512 byte 를 buffer 에 기록
		for (UINT16 i = 0; i < (pHDDInfo->BytesPerSector) / 2; i++)
		{
			UINT16 w = 0;
			BYTE l, h;

			// 2byte 를 읽음
			w = InPortWord(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_DATA);
			l = (w << 8) >> 8;
			h = w >> 8;
			
			// 2byte 를 씀
			buffer[(j * (pHDDInfo->BytesPerSector)) + (i * 2)] = l;
			buffer[(j * (pHDDInfo->BytesPerSector)) + (i * 2) + 1] = h;
		}
	}
	return HDD_NO_ERROR;
}

// ���ͷκ��� �����͸� �о���δ�(LBA ���)
// �о���̴� ��ƾ�� CHS ���� �����ϴ�.
BYTE HardDiskHandler::ReadSectors(BYTE * DPF, UINT32 StartLBASector, BYTE NoOfSectors, BYTE * Buffer, BOOLEAN WithRetry)
{
	HDDInfo * HDD;
	BYTE LBA0_7, LBA8_15, LBA16_23, LBA24_27;

	HDD = HDDs.Item((char *)DPF);
	if (HDD == NULL)
	{
		m_lastError = HDD_NOT_FOUND;
		return HDD_NOT_FOUND;
	}
	LBA0_7 = (StartLBASector << 24) >> 24;
	LBA8_15 = (StartLBASector << 16) >> 24;
	LBA16_23 = (StartLBASector << 8) >> 24;
	LBA24_27 = (StartLBASector << 4) >> 28;

	if (HDD->DeviceNumber == 0)
		LBA24_27 = LBA24_27 | 0xE0;
	else
		LBA24_27 = LBA24_27 | 0xF0;

	if (IsDeviceControllerBusy(HDD->IORegisterIdx, 1 * 60))
	{
		m_lastError = HDD_CONTROLLER_BUSY;
		return HDD_CONTROLLER_BUSY;
	}

	OutPortByte(IDE_Con_IOBases[HDD->IORegisterIdx][0] + IDE_CB_DEVICE_HEAD, LBA24_27);

	if (!IsDeviceDataReady(HDD->IORegisterIdx, 1 * 60, FALSE))
	{
		m_lastError = HDD_DATA_COMMAND_NOT_READY;
		return HDD_DATA_COMMAND_NOT_READY;
	}

	OutPortByte(IDE_Con_IOBases[HDD->IORegisterIdx][0] + IDE_CB_LBA_16_23, LBA16_23);
	OutPortByte(IDE_Con_IOBases[HDD->IORegisterIdx][0] + IDE_CB_LBA_8_15, LBA8_15);
	OutPortByte(IDE_Con_IOBases[HDD->IORegisterIdx][0] + IDE_CB_LBA_0_7, LBA0_7);
	OutPortByte(IDE_Con_IOBases[HDD->IORegisterIdx][0] + IDE_CB_SECTOR_COUNT, NoOfSectors);

	OutPortByte(IDE_Con_IOBases[HDD->IORegisterIdx][0] + IDE_CB_COMMAND, WithRetry ? IDE_COM_READ_SECTORS_W_RETRY : IDE_COM_READ_SECTORS);
	for (BYTE j = 0; j < NoOfSectors; j++)
	{
		if (!IsDeviceDataReady(HDD->IORegisterIdx, 1 * 60, TRUE))
		{
			m_lastError = HDD_DATA_NOT_READY;
			return HDD_DATA_NOT_READY;
		}

		for (UINT16 i = 0; i < (HDD->BytesPerSector) / 2; i++)
		{
			UINT16 w = 0;
			BYTE l, h;
			w = InPortWord(IDE_Con_IOBases[HDD->IORegisterIdx][0] + IDE_CB_DATA);
			l = (w << 8) >> 8;
			h = w >> 8;
			Buffer[(j * (HDD->BytesPerSector)) + (i * 2)] = l;
			Buffer[(j * (HDD->BytesPerSector)) + (i * 2) + 1] = h;
		}
	}
	return HDD_NO_ERROR;
}


/*���Ϳ� �����͸� ����.
1) HDDInfo ��ü�� ����.
2) ����̽��� ����� �� �ִ��� üũ�Ѵ�.
3) ����̽� ��Ʈ�� �����Ѵ�.
4) ����̽��� ������ Ŀ�ǵ带 �޾Ƶ��� �� �ִ��� üũ�Ѵ�.
5) ���, Ʈ��, ��Ÿ ������ �����Ѵ�.
6) ���� Ŀ�ǵ带 �����Ѵ�.
7) ����̽��� �����͸� ���� �غ� �Ǿ����� üũ�Ѵ�.
8) �����͸� �����ϱ� ���� ������ �������Ϳ� �����͸� ����Ѵ�.
*/
BYTE HardDiskHandler::WriteSectors(BYTE * DPF, UINT16 StartCylinder, BYTE StartHead, BYTE dwStartLBASector, BYTE NoOfSectors, BYTE * lpBuffer, BOOLEAN WithRetry)
{
	HDDInfo * pHDDInfo;
	BYTE LBA0_7, LBA8_15, LBA16_23, LBA24_27;

	pHDDInfo = HDDs.Item((char *)DPF);
	if (pHDDInfo == NULL)
	{
		m_lastError = HDD_NOT_FOUND;
		return HDD_NOT_FOUND;
	}

	LBA0_7 = (dwStartLBASector << 24) >> 24;
	LBA8_15 = (dwStartLBASector << 16) >> 24;
	LBA16_23 = (dwStartLBASector << 8) >> 24;
	LBA24_27 = (dwStartLBASector << 4) >> 28;

	if (pHDDInfo->DeviceNumber == 0)
		LBA24_27 = LBA24_27 | 0xE0;
	else
		LBA24_27 = LBA24_27 | 0xF0;

	if (IsDeviceControllerBusy(pHDDInfo->IORegisterIdx, 400))
	{		
		SetLastError(ERROR_BUSY);
		return HDD_CONTROLLER_BUSY;
	}

	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_DEVICE_HEAD, LBA24_27);

	if (!IsDeviceDataReady(pHDDInfo->IORegisterIdx, 1000, FALSE))
	{
		SetLastError(ERROR_NOT_READY);
		return HDD_DATA_COMMAND_NOT_READY;
	}
	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_LBA_16_23, LBA16_23);
	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_LBA_8_15, LBA8_15);
	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_LBA_0_7, LBA0_7);
	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_SECTOR_COUNT, NoOfSectors);

	OutPortByte(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_COMMAND, IDE_COM_WRITE_SECTORS_W_RETRY);
	for (UINT16 j = 0; j < NoOfSectors; j++)
	{
		if (!IsDeviceDataReady(pHDDInfo->IORegisterIdx, 1000, TRUE))
		{
			SetLastError(ERROR_NOT_READY);
			return HDD_DATA_NOT_READY;
		}
		for (UINT16 i = 0; i < pHDDInfo->BytesPerSector / 2; i++)
		{
			OutPortWord(IDE_Con_IOBases[pHDDInfo->IORegisterIdx][0] + IDE_CB_DATA, ((UINT16 *)lpBuffer)[(j * (pHDDInfo->BytesPerSector) / 2) + i]);
		}
	}

	return HDD_NO_ERROR;
}

BYTE HardDiskHandler::GetNoOfDevices()
{
	return GetTotalDevices();
}

//Ư�� ����̽��� �Ķ���� ������ ����.
UINT16 HardDiskHandler::GetDeviceParameters(BYTE * DPF, BYTE * pBuffer)
{
	VFS_IO_PARAMETER deviceInfo;

	HDDInfo * getHDD;
	getHDD = this->HDDs.Item((char *)DPF);
	if (getHDD == NULL)
	{
		this->m_lastError = HDD_NOT_FOUND;
		return HDD_NOT_FOUND;
	}

	deviceInfo.Cylinder = getHDD->CHSCylinderCount;
	deviceInfo.Head = getHDD->CHSHeadCount;
	deviceInfo.Sector = getHDD->CHSSectorCount;
	deviceInfo.LBASector = getHDD->LBACount;
	memcpy(pBuffer, &deviceInfo, sizeof(VFS_IO_PARAMETER));

	return HDD_NO_ERROR;
}

//�־��� ����̽� ��Ʈ�ѷ��� �����ϰ� �� ����� �����Ѵ�.
//����̽� ��Ʈ�ѷ��� ������ DoSoftwareReset �޼ҵ忡�� �����Ѵ�.
BYTE HardDiskHandler::Reset(BYTE * DPF)
{
	HDDInfo * getHDD;
	getHDD = this->HDDs.Item((char *)DPF);
	if (getHDD == NULL)
	{
		this->m_lastError = HDD_NOT_FOUND;
		return 0;
	}
	return this->DoSoftwareReset(getHDD->IORegisterIdx);

}

//�ּ� ��� ���� CHS => LBA
UINT32 HardDiskHandler::CHSToLBA(BYTE * DPF, UINT32 Cylinder, UINT32 Head, UINT32 Sector)
{
	HDDInfo * getHDD;
	getHDD = this->HDDs.Item((char *)DPF);

	return (Sector - 1) + (Head*getHDD->CHSSectorCount) + (Cylinder * (getHDD->CHSHeadCount + 1) * getHDD->CHSSectorCount);
}

//�ּ� ��� ���� LBA => CHS
void HardDiskHandler::LBAToCHS(BYTE * DPF, UINT32 LBA, UINT32 * Cylinder, UINT32 * Head, UINT32 * Sector)
{
	HDDInfo * getHDD;
	getHDD = this->HDDs.Item((char *)DPF);

	*Sector = ((LBA % getHDD->CHSSectorCount) + 1);
	UINT32 CylHead = (LBA / getHDD->CHSSectorCount);
	*Head = (CylHead % (getHDD->CHSHeadCount + 1));
	*Cylinder = (CylHead / (getHDD->CHSHeadCount + 1));
}
