
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include <functional>

#include "TaskManager.h"
#include "Profiler.h"

extern CProfiler *g_pProfilerInternal;

#if defined(_WINDOWS)
static void SetThreadName(HANDLE hThread, const char *threadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = GetThreadId(hThread);
	info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
#pragma warning(pop)
}
#endif

class CTaskForLoop: public ITaskImpl<ITask>
{
public:
	CTaskForLoop(ID id, const IParallelForBody *pBody, int iStart, int iEnd, UINT iFlags = CORE_TASK_FLAG_FOR_LOOP | CORE_TASK_FLAG_THREADSAFE):
		ITaskImpl(iFlags),
		m_id(id),
		m_pBody(pBody),
		m_iStart(iStart),
		m_iEnd(iEnd)
	{
	}

	void run()
	{
		m_pBody->forLoop(m_iStart, m_iEnd);
	}

	ID getID() const
	{
		return(m_id);
	}

protected:
	ID m_id;
	const IParallelForBody *m_pBody;
	int m_iStart;
	int m_iEnd;
};

//##########################################################################

class CTaskStub: public ITaskImpl<ITask>
{
public:
	CTaskStub():
		ITaskImpl(CORE_TASK_FLAG_BACKGROUND)
	{
	}

	void run()
	{
	}
};

//##########################################################################

CTaskManager::CTaskManager(unsigned int numThreads)
{
	m_iNumThreads = numThreads;
	if(!numThreads)
	{
		m_iNumThreads = std::thread::hardware_concurrency();
		if(!m_iNumThreads)
		{
			m_iNumThreads = 1;
		}
	}

	m_iWriteList = 0;
	m_iReadList = 1;

	m_iNumTasksToWaitFor = 0;
}

CTaskManager::~CTaskManager()
{
	stop();
}

void CTaskManager::forceSinglethreaded()
{
	if(m_isRunning)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Cannot switch to singlethreaded while running!\n");
		return;
	}
	m_isSingleThreaded = true;
	m_iNumThreads = 1;
}

void CTaskManager::addTask(TaskPtr task, std::chrono::steady_clock::time_point tpWhen)
{
	{
		ScopedLock lock(m_mutexSchedulerThread);
		m_aScheduled.insert({task, tpWhen}, [](const ScheduledTask &a, const ScheduledTask &b){
			return(a.tpRunAt > b.tpRunAt);
		});
	}
	m_ConditionSchedulerThread.notify_one();
}

void CTaskManager::addTask(TaskPtr task)
{
	unsigned int flags = task->getFlags();

	if(flags & CORE_TASK_FLAG_ON_SYNC)
	{
		m_OnSyncTasks.push(task);
	}
	else
	{
		if((flags & CORE_TASK_FLAG_THREADSAFE) && !m_isSingleThreaded)
		{
			if(flags & CORE_TASK_FLAG_FRAME_SYNC)
			{
				m_SyncTasks.push(task);
			}
			else
			{
				m_BackgroundTasks.push(task);
			}
		}
		else
		{
			m_TaskList[m_iWriteList].push(task);
		}
	}
	//printf("addTask()\n");
	notifyWorkers();
}

void CTaskManager::addTaskIO(TaskPtr task)
{
	task->stopRepeating();
	m_aIOTasks.push(task);
	m_ConditionIOThread.notify_one();
}

void CTaskManager::add(THREAD_UPDATE_FUNCTION fnFunc, DWORD dwFlag)
{
	TaskPtr pTask = new CTask(fnFunc, dwFlag);
	addTask(pTask);
	// mem_release(pTask);
}

void CTaskManager::start()
{
	m_isRunning = true;

	char name[64];

	ID idThread = Core_MGetThreadID();
	assert(idThread == 0);

	//< Инициализируем пул рабочих потоков
	for(unsigned int i = 0; i < m_iNumThreads - 1; ++i)
	{
		std::thread * t = new std::thread(std::bind(&CTaskManager::workerMain, this));
#if defined(_WINDOWS)
		sprintf(name, "Worker #%u", i);
		SetThreadName(t->native_handle(), name);
#endif
		m_aThreads.push_back(t);
	}
	m_pIOThread = new std::thread(std::bind(&CTaskManager::workerIOMain, this));
#if defined(_WINDOWS)
	sprintf(name, "Worker IO");
	SetThreadName(m_pIOThread->native_handle(), name);
#endif
	m_pShedulerThread = new std::thread(std::bind(&CTaskManager::schedulerMain, this));
#if defined(_WINDOWS)
	sprintf(name, "Scheduler");
	SetThreadName(m_pShedulerThread->native_handle(), name);
#endif

	sheduleNextBunch();

	while(m_isRunning)
	{
		if(!m_TaskList[m_iReadList].empty())
		{
			TaskPtr t = m_TaskList[m_iReadList].pop();
			execute(t);
		}
		else
		{
			// if(m_isSingleThreaded)
			// {
				worker(true);
			// }

			synchronize();


			std::swap(m_iReadList, m_iWriteList);

			/*if(m_isSingleThreaded)
			{
				worker(true);
			}
			*/

			m_aiNumWaitFor.clearFast();
			sheduleNextBunch();

		}

		//std::this_thread::yield();
	}
}

void CTaskManager::synchronize()
{
	while(!m_OnSyncTasks.empty())
	{
		execute(m_OnSyncTasks.pop());
	}
	XPROFILE_EVENT("Sync");
	//g_pPerfMon->syncBegin();
	/*std::unique_lock<std::mutex> lock(m_mutexSync);

	while(m_iNumTasksToWaitFor > 0)
	{
		m_Condition.wait(lock);
	}

	g_pPerfMon->syncBegin();

	while(!m_OnSyncTasks.empty())
	{
		m_BackgroundTasks.push(m_OnSyncTasks.pop());
		++m_iNumTasksToWaitFor;
	}
	//printf("synchronize()\n");
	notifyWorkers();
	*/
}

void CTaskManager::sheduleNextBunch()
{
	std::unique_lock<mutex> lock(m_mutexSync);

	while(m_iNumTasksToWaitFor > 0)
	{
		m_Condition.wait(lock);
	}

	SAFE_CALL(g_pProfilerInternal->getAPI(), onFrameStart);

	while(!m_SyncTasks.empty())
	{
		m_BackgroundTasks.push(m_SyncTasks.pop());
		++m_iNumTasksToWaitFor;
	}
	//printf("sheduleNextBunch()\n");
	notifyWorkers(min(getThreadCount(), m_iNumTasksToWaitFor));
}

void CTaskManager::stop()
{
	m_isRunning = false;
	m_ConditionIOThread.notify_all();
	m_ConditionWorker.notify_all();
	m_ConditionSchedulerThread.notify_all();
	
	ITask *pStubTask = new CTaskStub();
	for(int i = 0, l = m_aThreads.size() - 1; i < l; ++i)
	{
		pStubTask->AddRef();
	}
	for(int i = 0, l = m_aThreads.size(); i < l; ++i)
	{
		addTask(pStubTask);
	}

	for(int i = 0, l = m_aThreads.size(); i < l; ++i)
	{
		m_aThreads[i]->join();
		mem_delete(m_aThreads[i]);
	}
	m_aThreads.clear();
	if(m_pIOThread)
	{
		m_pIOThread->join();
		mem_delete(m_pIOThread);
	}
	if(m_pShedulerThread)
	{
		m_pShedulerThread->join();
		mem_delete(m_pShedulerThread);
	}
}

void CTaskManager::execute(TaskPtr t)
{
	std::chrono::steady_clock::time_point tpStarted(std::chrono::steady_clock::now());
	t->run();
	// std::chrono::steady_clock::time_point tpFinished(std::chrono::steady_clock::now());

	if(t->getFlags() & CORE_TASK_FLAG_REPEATING)
	{
		uint64_t uRepeatInterval = t->getRepeatInterval();
		if(uRepeatInterval)
		{
			addTask(t, std::chrono::steady_clock::time_point(tpStarted + std::chrono::microseconds(uRepeatInterval)));
		}
		else
		{
			addTask(t);
		}
	}
	else
	{
		mem_release(t);
	}
}

void CTaskManager::workerMain()
{
	srand((UINT)time(0));
	SAFE_CALL(g_pProfilerInternal->getAPI(), registerThread, "Worker");
	worker(false);
	SAFE_CALL(g_pProfilerInternal->getAPI(), unregisterThread);
}

void CTaskManager::workerIOMain()
{
	srand((UINT)time(0));
	SAFE_CALL(g_pProfilerInternal->getAPI(), registerThread, "Worker IO");
	workerIO();
	SAFE_CALL(g_pProfilerInternal->getAPI(), unregisterThread);
}

void CTaskManager::schedulerMain()
{
	srand((UINT)time(0));

	SAFE_CALL(g_pProfilerInternal->getAPI(), registerThread, "Scheduler");

	//TaskPtr task;

	ScheduledTask *pScheduledTask = NULL;
	std::chrono::steady_clock::time_point tpNow, tpWaitUntil;
	while(m_isRunning)
	{
		tpNow = tpWaitUntil = std::chrono::steady_clock::now();

		std::unique_lock<std::mutex> lock(m_mutexSchedulerThread);
		for(UINT i = 0, l = m_aScheduled.size(); i < l; ++i)
		{
			pScheduledTask = &m_aScheduled[i];

			if(pScheduledTask->tpRunAt <= tpNow)
			{
				addTask(pScheduledTask->pTask);
				m_aScheduled.erase(i);
				--i; --l;
			}
			else
			{
				tpWaitUntil = pScheduledTask->tpRunAt;
				break;
			}
		}

		if(tpWaitUntil != tpNow)
		{
			m_ConditionSchedulerThread.wait_until(lock, tpWaitUntil);
		}
		else
		{
			m_ConditionSchedulerThread.wait(lock);
		}
	}

	SAFE_CALL(g_pProfilerInternal->getAPI(), unregisterThread);
}

void CTaskManager::worker(bool bOneRun)
{
	TaskPtr task;

	while(m_isRunning)
	{
		if(bOneRun)
		{
			if(!m_BackgroundTasks.tryPop(task))
			{
				return;
			}
		}
		else
		{
			task = m_BackgroundTasks.pop();
		}
		//if(m_BackgroundTasks.tryPop(task))
		{
			//printf("#%d: Executing task\n", Core_MGetThreadID());
			task->AddRef();
			execute(task);

			if(task->getFlags() & (CORE_TASK_FLAG_FRAME_SYNC/* | CORE_TASK_FLAG_ON_SYNC*/))
			{
				{
					ScopedLock lock(m_mutexSync);
					m_iNumTasksToWaitFor -= 1;
				}

				m_Condition.notify_one();
			}

			if(task->getFlags() & CORE_TASK_FLAG_FOR_LOOP)
			{
				{
					ScopedLock lock(m_mutexFor);
					m_aiNumWaitFor[((CTaskForLoop*)task)->getID()] -= 1;
					//m_aiNumWaitFor[std::static_pointer_cast<CTaskForLoop, ITask>(task)->getID()] -= 1;
				}

				m_ConditionFor.notify_one();
			}
			mem_release(task);
			//std::this_thread::yield();
		}
		/*else
		{
			if(bOneRun)
			{
				return;
			}

			//printf("#%d: Sleeping\n", Core_MGetThreadID());
			{
				std::unique_lock<std::mutex> lock(m_mutexWorker);
				m_ConditionWorker.wait(lock);
			}
			//printf("#%d: Waked up\n", Core_MGetThreadID());
			// std::this_thread::yield();
			//std::this_thread::sleep_for(std::chrono::microseconds(166));

		}*/
	}
}

void CTaskManager::workerIO()
{
	TaskPtr task;

	while(m_isRunning)
	{
		if(m_aIOTasks.tryPop(task))
		{
			execute(task);
		}
		else
		{
			std::unique_lock<std::mutex> lock(m_mutexIOThread);
			m_ConditionIOThread.wait(lock);
		}
	}
}

ID CTaskManager::forLoop(int iStart, int iEnd, const IParallelForBody *pBody, int iMaxChunkSize)
{
	if(iStart >= iEnd)
	{
		return(-1);
	}

	if(m_isSingleThreaded)
	{
		pBody->forLoop(iStart, iEnd);
		m_aiNumWaitFor[0] = 0;
		return(0);
	}
	int iTotal = iEnd - iStart;
	int iChunkSize = (int)(ceilf((float)iTotal / (float)(m_iNumThreads - 1)) + 0.5f);
	if(iMaxChunkSize > 0 && iChunkSize > iMaxChunkSize)
	{
		iChunkSize = iMaxChunkSize;
	}

	int iTaskCount = (int)(ceilf((float)iTotal / (float)iChunkSize) + 0.5f);

	ID id = -1;
	{
		ScopedLock lock(m_mutexFor);
		id = m_aiNumWaitFor.size();
		m_aiNumWaitFor.push_back(iTaskCount);
	}

	int iCur;

	while(iTotal > 0)
	{
		iCur = (std::min)(iTotal, iChunkSize);
		TaskPtr pTask = new CTaskForLoop(id, pBody, iStart, iStart + iCur);
		addTask(pTask);
		// mem_release(pTask);
		iTotal -= iCur;
		iStart += iCur;
	}

	return(id);
}

void CTaskManager::waitFor(ID id)
{
	assert(ID_VALID(id) && (UINT)id < m_aiNumWaitFor.size());

	std::unique_lock<std::mutex> lock(m_mutexFor);

	while(m_aiNumWaitFor[id] > 0)
	{
		m_ConditionFor.wait(lock);
	}
}

void CTaskManager::notifyWorkers(UINT uCount)
{
	for(UINT i = 0; i < uCount; ++i)
	{
		m_ConditionWorker.notify_one();
	}
}

//##########################################################################

class CThreadsafeCounter
{
	ID m_idCounter;
	std::mutex m_mutex;

public:
	CThreadsafeCounter()
	{
		m_idCounter = -1;
	}

	ID getNext()
	{
		m_mutex.lock();
		++m_idCounter;

		if(m_idCounter >= SX_MAX_THREAD_COUNT)
		{
			assert(!"thread counter exceeded");
			// wrap back to the first worker index
			m_idCounter = 1;
		}

		ID val = m_idCounter;
		m_mutex.unlock();
		return(val);
	}
};

SX_LIB_API ID Core_MGetThreadID()
{
	static CThreadsafeCounter s_threadCounter;

	const ID c_idNullIndex = -1;
	__declspec(thread) static ID s_idThreadIndex = c_idNullIndex;
	if(s_idThreadIndex == c_idNullIndex)
	{
		s_idThreadIndex = s_threadCounter.getNext();
		assert(s_idThreadIndex < SX_MAX_THREAD_COUNT);
	}

	return(s_idThreadIndex);
}
