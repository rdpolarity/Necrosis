// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

class FVoxelSafeCriticalSection
{
public:
	FVoxelSafeCriticalSection() = default;

	FORCEINLINE void Lock()
	{
		check(LockerThreadId != ThreadId());

		Section.Lock();
		ensure(LockCounter.Set(1) == 0);

		check(LockerThreadId == -1);
		LockerThreadId = ThreadId();
	}
	FORCEINLINE void Unlock()
	{
		check(LockerThreadId == ThreadId());
		LockerThreadId = -1;

		ensure(LockCounter.Set(0) == 1);
		Section.Unlock();
	}

	FORCEINLINE bool IsLocked() const
	{
		return LockCounter.GetValue() > 0;
	}

private:
	FCriticalSection Section;
	FThreadSafeCounter LockCounter;
	uint32 LockerThreadId = -1;

	FORCEINLINE static uint32 ThreadId()
	{
		return FPlatformTLS::GetCurrentThreadId();
	}
};

class FVoxelSafeSharedCriticalSection
{
public:
	FVoxelSafeSharedCriticalSection() = default;

	FORCEINLINE void ReadLock()
	{
		{
			FScopeLock Lock(&DebugSection);
			check(!Readers.Contains(ThreadId()));
			check(!Writers.Contains(ThreadId()));
		}
		Section.ReadLock();
		{
			FScopeLock Lock(&DebugSection);
			Readers.Add(ThreadId());
		}
	}
	FORCEINLINE void ReadUnlock()
	{
		Section.ReadUnlock();
		{
			FScopeLock Lock(&DebugSection);
			verify(Readers.Remove(ThreadId()));
		}
	}

	FORCEINLINE void WriteLock()
	{
		{
			FScopeLock Lock(&DebugSection);
			check(!Readers.Contains(ThreadId()));
			check(!Writers.Contains(ThreadId()));
		}
		Section.WriteLock();
		{
			FScopeLock Lock(&DebugSection);
			Writers.Add(ThreadId());
		}
	}
	FORCEINLINE void WriteUnlock()
	{
		Section.WriteUnlock();
		{
			FScopeLock Lock(&DebugSection);
			verify(Writers.Remove(ThreadId()));
		}
	}
	
	FORCEINLINE bool IsLocked_Read() const
	{
		FScopeLock Lock(&DebugSection);
		return Readers.Num() > 0 || Writers.Num() > 0;
	}
	FORCEINLINE bool IsLocked_Write() const
	{
		FScopeLock Lock(&DebugSection);
		return Writers.Num() > 0;
	}

private:
	FRWLock Section;
	mutable FCriticalSection DebugSection;
	TSet<uint32> Readers;
	TSet<uint32> Writers;

	FORCEINLINE static uint32 ThreadId()
	{
		return FPlatformTLS::GetCurrentThreadId();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelCriticalSection
{
public:
	FVoxelCriticalSection() = default;

	// Allow copying for convenience
	FVoxelCriticalSection(const FVoxelCriticalSection&)
	{
	}
	FVoxelCriticalSection& operator=(const FVoxelCriticalSection&)
	{
		return *this;
	}

	FORCEINLINE void Lock()
	{
		Section.Lock();
	}
	FORCEINLINE void Unlock()
	{
		Section.Unlock();
	}
	
#if VOXEL_DEBUG
	bool IsLocked_Debug() const
	{
		return Section.IsLocked();
	}
#endif

private:
#if VOXEL_DEBUG
	FVoxelSafeCriticalSection Section;
#else
	FCriticalSection Section;
#endif
};

class FVoxelSharedCriticalSection
{
public:
	FVoxelSharedCriticalSection() = default;

	// Allow copying for convenience
	FVoxelSharedCriticalSection(const FVoxelSharedCriticalSection&)
	{
	}
	FVoxelSharedCriticalSection& operator=(const FVoxelSharedCriticalSection&)
	{
		return *this;
	}

	FORCEINLINE void ReadLock()
	{
		Section.ReadLock();
	}
	FORCEINLINE void ReadUnlock()
	{
		Section.ReadUnlock();
	}

	FORCEINLINE void WriteLock()
	{
		Section.WriteLock();
	}
	FORCEINLINE void WriteUnlock()
	{
		Section.WriteUnlock();
	}
	
#if VOXEL_DEBUG
	bool IsLocked_Read_Debug() const
	{
		return Section.IsLocked_Read();
	}
	bool IsLocked_Write_Debug() const
	{
		return Section.IsLocked_Write();
	}
#endif

private:
#if VOXEL_DEBUG
	FVoxelSafeSharedCriticalSection Section;
#else
	FRWLock Section;
#endif
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelScopeLock
{
public:
	FORCEINLINE explicit FVoxelScopeLock(FVoxelCriticalSection& Section)
		: Section(Section)
	{
		VOXEL_FUNCTION_COUNTER();
		Section.Lock();
	}
	FORCEINLINE ~FVoxelScopeLock()
{
		VOXEL_FUNCTION_COUNTER();
		Section.Unlock();
	}

private:
	FVoxelCriticalSection& Section;
};

class FVoxelScopeLock_Read
{
public:
	FORCEINLINE explicit FVoxelScopeLock_Read(FVoxelSharedCriticalSection& Section)
		: Section(Section)
	{
		VOXEL_FUNCTION_COUNTER();
		Section.ReadLock();
	}
	FORCEINLINE ~FVoxelScopeLock_Read()
	{
		VOXEL_FUNCTION_COUNTER();
		Section.ReadUnlock();
	}

private:
	FVoxelSharedCriticalSection& Section;
};

class FVoxelScopeLock_Write
{
public:
	FORCEINLINE explicit FVoxelScopeLock_Write(FVoxelSharedCriticalSection& Section)
		: Section(Section)
	{
		VOXEL_FUNCTION_COUNTER();
		Section.WriteLock();
	}
	FORCEINLINE ~FVoxelScopeLock_Write()
	{
		VOXEL_FUNCTION_COUNTER();
		Section.WriteUnlock();
	}

private:
	FVoxelSharedCriticalSection& Section;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelScopeLockNoStats
{
public:
	FORCEINLINE explicit FVoxelScopeLockNoStats(FVoxelCriticalSection& Section)
		: Section(Section)
	{
		Section.Lock();
	}
	FORCEINLINE ~FVoxelScopeLockNoStats()
	{
		Section.Unlock();
	}

private:
	FVoxelCriticalSection& Section;
};

#define VOXEL_SCOPE_LOCK(...) \
	{ \
		VOXEL_SCOPE_COUNTER("Lock"); \
		(__VA_ARGS__).Lock(); \
	} \
	ON_SCOPE_EXIT \
	{ \
		VOXEL_SCOPE_COUNTER("Unlock"); \
		(__VA_ARGS__).Unlock(); \
	};