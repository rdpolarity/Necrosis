// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

#include <mutex>
#include <condition_variable>

template<typename KeyType>
class TVoxelKeyedCriticalSection
{
public:
	TVoxelKeyedCriticalSection() = default;

	bool IsLocked(KeyType Key) const
	{
		std::unique_lock<std::mutex> Lock(Mutex);
		return bAllLocked || LockedKeys.Contains(Key);
	}

	void Lock(KeyType Key)
	{
		VOXEL_FUNCTION_COUNTER();

		std::unique_lock<std::mutex> Lock(Mutex);

		while (bAllLocked || LockedKeys.Contains(Key))
		{
			VOXEL_SCOPE_COUNTER("Wait");
			Queue.wait_for(Lock, std::chrono::milliseconds(1));
		}
		LockedKeys.Add(Key);
	}
	void Unlock(KeyType Key)
	{
		VOXEL_FUNCTION_COUNTER();
		
		{
			std::unique_lock<std::mutex> Lock(Mutex);
			ensure(LockedKeys.Remove(Key));
		}

		Queue.notify_all();
	}

	void LockAll()
	{
		VOXEL_FUNCTION_COUNTER();

		std::unique_lock<std::mutex> Lock(Mutex);

		while (bAllLocked || LockedKeys.Num() > 0)
		{
			VOXEL_SCOPE_COUNTER("Wait");
			Queue.wait_for(Lock, std::chrono::milliseconds(1));
		}
		bAllLocked = true;
	}
	void UnlockAll()
	{
		VOXEL_FUNCTION_COUNTER();

		{
			std::unique_lock<std::mutex> Lock(Mutex);

			ensure(bAllLocked);
			ensure(LockedKeys.Num() == 0);
			bAllLocked = false;
		}

		Queue.notify_all();
	}

private:
	mutable std::mutex Mutex;
	mutable std::condition_variable Queue;
	mutable bool bAllLocked = false;
	mutable TSet<KeyType> LockedKeys;
};

template<typename KeyType>
class TVoxelKeyedScopeLock
{
public:
	TVoxelKeyedScopeLock(TVoxelKeyedCriticalSection<KeyType>& Lock, KeyType Key)
		: Lock(Lock)
		, Key(Key)
	{
		Lock.Lock(Key);
	}
	~TVoxelKeyedScopeLock()
	{
		Lock.Unlock(Key);
	}

private:
	TVoxelKeyedCriticalSection<KeyType>& Lock;
	const KeyType Key;
};

template<typename KeyType>
class TVoxelKeyedScopeLock_LockAll
{
public:
	explicit TVoxelKeyedScopeLock_LockAll(TVoxelKeyedCriticalSection<KeyType>& Lock, bool bCondition = true)
		: bCondition(bCondition)
		, Lock(Lock)
	{
		if (bCondition)
		{
			Lock.LockAll();
		}
	}
	~TVoxelKeyedScopeLock_LockAll()
	{
		if (bCondition)
		{
			Lock.UnlockAll();
		}
	}

private:
	const bool bCondition;
	TVoxelKeyedCriticalSection<KeyType>& Lock;
};