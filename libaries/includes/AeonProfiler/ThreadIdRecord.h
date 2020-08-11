//
// Copyright (c) 2015-2018 Jeffrey "botman" Broome
//

#pragma once

// C RunTime Header Files
#include <assert.h>
#include <new>

#include "Allocator.h"
#include "Stack.h"
#include "Hash.h"
#include "Repository.h"

extern int NumThreads;

struct DialogThreadIdRecord_t  // "static" copy of CThreadIdRecord for the Dialog to display data that's not constantly changing
{
	CThreadIdRecord* ThreadIdRecord;  // pointer to original CThreadIdRecord record (so we can set its SymbolName)

	DialogStackCallerData_t* StackArray;  // array of StackCallerData_t structs
	unsigned int StackArraySize;

	void** CallTreeArray;  // array of pointers
	unsigned int CallTreeArraySize;

	const void* Address;

	DWORD ThreadId;
	char* SymbolName;

	void Serialize(Repository& Repo)
	{
		if( Repo.bIsDebugSave )
		{
			WORD StackCallerData_Signature = 0xe44e;
			if( Repo.bIsLoading )
			{
				WORD Signature;
				Repo << Signature;
				assert( Signature == StackCallerData_Signature );
			}
			else
			{
				Repo << StackCallerData_Signature;
			}
		}

		if( Repo.bIsLoading )
		{
			NumThreads++;
		}

		Repo << StackArraySize;

		if( Repo.bIsLoading )
		{
			StackArray = (StackArraySize > 0) ? (DialogStackCallerData_t*)Repo.Allocator->AllocateBytes(StackArraySize * sizeof(DialogStackCallerData_t), sizeof(void*)) : nullptr;
		}

		assert( (StackArraySize == 0) || (StackArray != nullptr) );
		for( unsigned int index = 0; index < StackArraySize; ++index )
		{
			Repo << StackArray[index];
		}

		Repo << CallTreeArraySize;

		if( Repo.bIsLoading )
		{
			CallTreeArray = (CallTreeArraySize > 0) ? (void**)Repo.Allocator->AllocateBytes(CallTreeArraySize * sizeof(void*), sizeof(void*)) : nullptr;
		}

		assert( (CallTreeArraySize == 0) || (CallTreeArray != nullptr) );
		for( unsigned int index = 0; index < CallTreeArraySize; ++index )
		{
			if( Repo.bIsLoading )
			{
				CallTreeArray[index] = Repo.Allocator->AllocateBytes(sizeof(DialogCallTreeRecord_t), sizeof(void*));
			}

			assert( CallTreeArray[index] != nullptr );
			DialogCallTreeRecord_t* CallTreeRec = (DialogCallTreeRecord_t*)CallTreeArray[index];
			CallTreeRec->Serialize(Repo, true);
		}

		//assert( Address != nullptr );  // No, Address can be null here
		if( Repo.bIsLoading )
		{
			__int64 Address64;
			Repo << Address64;
			Address = (void*)Address64;
		}
		else
		{
			__int64 Address64 = (__int64)Address;
			Repo << Address64;
		}

		Repo << ThreadId;
		Repo << SymbolName;
	}

	friend Repository& operator<<(Repository& Repo, DialogThreadIdRecord_t& ThreadIdRec)
	{
		ThreadIdRec.Serialize(Repo);
		return Repo;
	}
};

class CThreadIdRecord
{
public:
	CAllocator ThreadIdRecordAllocator;  // allocator for this specific thread
	CRITICAL_SECTION ThreadIdCriticalSection;

	bool bIsCriticalSectionLocked;

	CStack CallStack;  // the current call stack for this thread
	CHash<CCallTreeRecord> CallTreeHashTable;

	DWORD ThreadId;
	const void* CallerAddress;

	char* SymbolName;

	CThreadIdRecord(DWORD InThreadId, const void* InCallerAddress) :
		ThreadIdRecordAllocator(false),
		CallStack(&ThreadIdRecordAllocator),
		CallTreeHashTable(&ThreadIdRecordAllocator, CALLRECORD_HASH_TABLE_SIZE, false),
		ThreadId( InThreadId ),
		CallerAddress( InCallerAddress )
	{
		InitializeCriticalSection(&ThreadIdCriticalSection);
		SetCriticalSectionSpinCount(&ThreadIdCriticalSection, 4000);  // 4000 is what the Windows heap manager uses (https://msdn.microsoft.com/en-us/library/windows/desktop/ms686197%28v=vs.85%29.aspx)

		bIsCriticalSectionLocked = false;

		NumThreads++;
	}

	~CThreadIdRecord()
	{
		NumThreads--;

		DeleteCriticalSection(&ThreadIdCriticalSection);

		ThreadId = 0;
		SymbolName = NULL;

		ThreadIdRecordAllocator.FreeBlocks();  // free all the memory allocated by this thread's allocator
	}

	void PrintStats(char* Header, int NestLevel)
	{
		char buffer[64];
		buffer[0] = 0;

		for( int i = 0; (i < NestLevel) && (i < 32); i++ )
		{
			strcat_s(buffer, sizeof(buffer), "  ");
		}

		DebugLog("%s%sCThreadIdRecord Stats: ThreadId = %d (0x%08x), SymbolName = '%s'", buffer, Header, ThreadId, ThreadId, SymbolName ? SymbolName : "(Unknown)" );

		ThreadIdRecordAllocator.PrintStats("ThreadIdRecordAllocator - ", NestLevel + 1);
		CallTreeHashTable.PrintStats("CallTreeHashTable - ", NestLevel + 1);
	}

	void Lock()
	{
		EnterCriticalSection(&ThreadIdCriticalSection);
		bIsCriticalSectionLocked = true;
	}

	void Unlock()
	{
		if( bIsCriticalSectionLocked )
		{
			LeaveCriticalSection(&ThreadIdCriticalSection);
			bIsCriticalSectionLocked = false;
		}
	}

	unsigned int GetNumRecordsToCopy()
	{
		return 1;
	}

	DialogThreadIdRecord_t* GetArrayCopy(CAllocator* InCopyAllocator, bool bCopyMemberHashTables)
	{
		DialogThreadIdRecord_t* pRec = (DialogThreadIdRecord_t*)InCopyAllocator->AllocateBytes(sizeof(DialogThreadIdRecord_t), sizeof(void*));

		pRec->ThreadIdRecord = this;

		pRec->StackArray = nullptr;
		pRec->StackArraySize = 0;

		pRec->CallTreeArray = nullptr;
		pRec->CallTreeArraySize = 0;

		pRec->Address = CallerAddress;

		pRec->ThreadId = ThreadId;
		pRec->SymbolName = SymbolName;

		if( CallStack.pTop )  // copy the thread's Stack
		{
			pRec->StackArray = CallStack.CopyStackToArray(InCopyAllocator, pRec->StackArraySize);
		}

		pRec->CallTreeArray = CallTreeHashTable.CopyHashToArray(InCopyAllocator, pRec->CallTreeArraySize, true);

		return pRec;
	}

	void ResetCounters(DWORD64 TimeNow)
	{
		// reset the calltree records in the hash table first (the calltree records on the stack need special handling){
		CallTreeHashTable.ResetCounters(TimeNow);

		// reset the calltree records on the stack last (to set the proper CallCount and MaxRecursionLevel)
		CallStack.ResetCounters(TimeNow);
	}

	void SetSymbolName(char* InSymbolName)
	{
		SymbolName = InSymbolName;
	}

private:
	CThreadIdRecord(const CThreadIdRecord& other) :  // copy constructor (this should never get called)
		ThreadIdRecordAllocator(false),
		CallStack(&ThreadIdRecordAllocator),
		CallTreeHashTable(&ThreadIdRecordAllocator, CALLRECORD_HASH_TABLE_SIZE, false)
	{
		assert(false);
	}

	CThreadIdRecord& operator=(const CThreadIdRecord&)  // assignment operator (this should never get called)
	{
		assert(false);
	}
};
