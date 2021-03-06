
#include "TDThreading.h"
#include <algorithm>
#include <typeinfo>
namespace TD
{
	using namespace TDThreading;

	Event::Event()
	{
		Handle = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	Event::~Event()
	{
		if (Handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(Handle);
		}
	}

	void Event::Signal()
	{
		SetEvent(Handle);
	}

	bool Event::WaitForSignal(int milliseconds)
	{
		if (milliseconds < 0)
		{
			milliseconds = INFINITE;
		}
		return WaitForSingleObjectEx(Handle, milliseconds, TRUE) == WAIT_OBJECT_0;
	}

	Thread::~Thread()
	{
		if (Handle != NULL)
		{
			RequestToExit();
			WaitForThreadToFinish(-1);
		}

		CloseHandle(Handle);
	}

	DWORD WINAPI Thread::ThreadMain(void *threadAsVoidPtr)
	{
		Thread *thread = (Thread *)threadAsVoidPtr;
		while (!thread->IsRequestedToExit())
		{
			thread->JobReady.WaitForSignal(-1);
			if (thread->IsRequestedToExit())
			{
				thread->JobDone.Signal();//we have quit 
				break;
			}

			if (thread->FunctionToRun != nullptr)
			{
				thread->FunctionToRun(thread->ThreadIndex);
			}
			thread->JobDone.Signal();
		}

		return 0;
	}
};

TD::TDThreading::TaskGraph::TaskGraph(int Count)
{
	ThreadCount = Count;
	Threads = new Thread*[ThreadCount];
	for (int i = 0; i < ThreadCount; i++)
	{
		Threads[i] = new Thread(i);
	}
}
void TD::TDThreading::TaskGraph::Shutdown()
{
	for (int i = 0; i < ThreadCount; i++)
	{
		Threads[i]->RequestToExit();
		Threads[i]->JobReady.Signal();
		//Threads[i]->WaitForFunctionCompletion();
		SafeDelete(Threads[i]);
	}
}
void TD::TDThreading::TaskGraph::RunTaskOnGraph(std::function<void(int)> function, int threadstouse)
{
	if (threadstouse == 0)
	{
		threadstouse = ThreadCount;
	}
	threadstouse = std::min(threadstouse, ThreadCount);
	for (int i = 0; i < threadstouse; i++)
	{
		Threads[i]->StartFunction(function);
	}

	for (int i = 0; i < threadstouse; i++)
	{
		Threads[i]->WaitForFunctionCompletion();
	}
}

int TD::TDThreading::TaskGraph::GetThreadCount() const
{
	return ThreadCount;
}
