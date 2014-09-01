// DeadlockAssignment.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <process.h>


#define NUM_PLAYERS		5	// Thread 개수, 1 이상이어야 함
#define ATTACK_TRIES	10	// 총 공격 횟수, 1 이상이어야 함
#define ATTACK_DAMAGE	100 // 공격시 상대의 hp 감소값
#define ATTACK_BONUS	30	// 공격시 공격자의 hp 충전값

#define MANY_LOGS		false  // 세부 lock 상태 로그를 다 화면에 출력할지 여부

struct Player {
	int					mPlayerId;
	int					mHp;
	CRITICAL_SECTION	mLock;
};



/* Lock Manager Definition  */

	int					gLockStatusArr[NUM_PLAYERS];
	CRITICAL_SECTION	gLock_ManagerLock;

	void				LockFinished(int from, int to);
	bool				IsOkToLock(int from, int to);


/* Game Definition */

	Player					GamerData[NUM_PLAYERS];

	void					attack(Player* from, Player *to);
	unsigned int WINAPI		ThreadProc(LPVOID lpParam);



int _tmain(int argc, _TCHAR* argv[])
{
	DWORD dwThreadId[NUM_PLAYERS];
	HANDLE hThread[NUM_PLAYERS];

	InitializeCriticalSection(&gLock_ManagerLock); 

	int initialHP = 2000;

	for (int i = 0; i < NUM_PLAYERS; i++)
	{
		GamerData[i].mPlayerId = i;
		GamerData[i].mHp = initialHP;
		InitializeCriticalSection(&GamerData[i].mLock);

		gLockStatusArr[i] = 0;
	}

	for (int i = 0; i < NUM_PLAYERS; i++)
	{
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, (LPVOID)i, CREATE_SUSPENDED, (unsigned int*)&dwThreadId[i]);
	}

	_tprintf_s(_T("=================BEGIN============================= \n"));
	for (int i = 0; i < NUM_PLAYERS; i++)
	{
		ResumeThread(hThread[i]);
	}

	WaitForMultipleObjects(NUM_PLAYERS, hThread, TRUE, INFINITE);

	_tprintf_s(_T("=================END============================= \n"));

	int hpTotal = 0;

	for (int i = 0; i < NUM_PLAYERS; i++)
	{
		hpTotal += GamerData[i].mHp;
		_tprintf_s(_T("--- Player # %d, HP: %d \n"), GamerData[i].mPlayerId, GamerData[i].mHp);
		DeleteCriticalSection(&GamerData[i].mLock);
		CloseHandle(hThread[i]);
	}

	_tprintf_s(_T("--- TOTAL HP of all Players = %d, Average HP Left: %d \n"), hpTotal, hpTotal / NUM_PLAYERS);
	if (hpTotal == (NUM_PLAYERS)*(initialHP - ATTACK_TRIES * (ATTACK_DAMAGE - ATTACK_BONUS)))
	{
		_tprintf_s(_T("No errors in hp values \n"));
	}
	else
	{
		_tprintf_s(_T("ERROR in hp values \n"));
	}

	DeleteCriticalSection(&gLock_ManagerLock);

	getchar();
	return 0;
}


// if both are free
// give lock ok
// else reject
bool IsOkToLock(int from, int to)
{
	bool isOk = false;

	if (from < 0 || from >= NUM_PLAYERS) return isOk;
	if (to < 0 || to >= NUM_PLAYERS) return isOk;

	EnterCriticalSection(&gLock_ManagerLock);
	{
		if (gLockStatusArr[from] == 0
			&& gLockStatusArr[to] == 0)
		{
			gLockStatusArr[from] = 1;
			gLockStatusArr[to] = 1;
			isOk = true;
		}
	}
	LeaveCriticalSection(&gLock_ManagerLock);

	return isOk;
}

// 임계영역 작업 완료시 lock 상태 재설정
void LockFinished(int from, int to)
{
	if (from < 0 || from >= NUM_PLAYERS) return;
	if (to < 0 || to >= NUM_PLAYERS) return;

	EnterCriticalSection(&gLock_ManagerLock);
	{
		gLockStatusArr[from] = 0;
		gLockStatusArr[to] = 0;
	}
	LeaveCriticalSection(&gLock_ManagerLock);
}

void attack(Player* from, Player *to)
{		
	while (IsOkToLock(from->mPlayerId, to->mPlayerId) == false)
	{
		Sleep(5);
	}
	
	_tprintf_s(_T("Begin Attack: from #%d to #%d \n"), from->mPlayerId, to->mPlayerId);
	
	EnterCriticalSection(&to->mLock);
	{
		if (MANY_LOGS) _tprintf_s(_T(" -- to #%d enter critical section \n"), to->mPlayerId);
		EnterCriticalSection(&from->mLock);
		{
			if (MANY_LOGS) _tprintf_s(_T(" -- from #%d enter critical section \n"), from->mPlayerId);

			to->mHp -= ATTACK_DAMAGE;
			from->mHp += ATTACK_BONUS;

			Sleep(100);

		}
		LeaveCriticalSection(&from->mLock);
		if (MANY_LOGS) _tprintf_s(_T(" -- from #%d EXIT critical section \n"), from->mPlayerId);
	}
	LeaveCriticalSection(&to->mLock);
	if (MANY_LOGS) _tprintf_s(_T(" -- to #%d EXIT critical section \n"), to->mPlayerId);

	LockFinished(from->mPlayerId, to->mPlayerId);

	_tprintf_s(_T("End Attack: from #%d to #%d \n"), from->mPlayerId, to->mPlayerId);
}

unsigned int WINAPI ThreadProc(LPVOID lpParam)
{
	srand(GetCurrentThreadId());

	int from = (int)lpParam;

	for (int i = 0; i < ATTACK_TRIES; i++)
	{
		int to = 0;

		while (1)
		{
			to = rand() % NUM_PLAYERS;

			if (from != to) break;
		}

		attack(&GamerData[from], &GamerData[to]);
	}
	return 0;
}