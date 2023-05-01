#pragma once
#include "FileSysAdaptor.h"
#include "MultiBoot.h"
#include "HDDAdaptor.h"
#include "MemoryResourceAdaptor.h"
#include "RamDiskAdaptor.h"
#include "FloppyDiskAdaptor.h"

#define STORAGE_DEVICE_MAX 26



// 다양한 file system 에 접근할 때 kernel 에 일관성을 보장하기 위해 VFS 역할을 담당하는 기능을 추가
// kernel 에 일관된 interface 를 정의
class StorageManager
{

public:	
	~StorageManager();

	static StorageManager* GetInstance()
	{
		if (m_pStorageManager == nullptr)
			m_pStorageManager = new StorageManager();

		return m_pStorageManager;
	}

	bool Initilaize(multiboot_info* info);

// interface
	bool RegisterFileSystem(FileSysAdaptor* fsys, DWORD deviceID);
	bool UnregisterFileSystem(FileSysAdaptor* fsys);
	bool UnregisterFileSystemByID(DWORD deviceID);

	bool SetCurrentFileSystemByID(DWORD deviceID);
	bool SetCurrentFileSystem(FileSysAdaptor* fsys);

// file method
	PFILE OpenFile(const char* fname, const char *mode);
	int ReadFile(PFILE file, unsigned char* Buffer, unsigned int size, int count);
	int WriteFile(PFILE file, unsigned char* Buffer, unsigned int size, int count);
	bool CloseFile(PFILE file);

	bool GetFileList();

protected:
	bool ConstructFileSystem(multiboot_info* info);

private:
	StorageManager();
	static StorageManager* m_pStorageManager;

	FileSysAdaptor* m_fileSystems[STORAGE_DEVICE_MAX];
	int m_stroageCount;
	FileSysAdaptor* m_pCurrentFileSystem;
};