#pragma once

#include "HouseBuilder.h"

// this is a multi threaded worker for generating the house data for single houses (that being the main bottleneck in the program) faster

class AHouseBuilder;
//~~~~~ Multi Threading ~~~
class ThreadedWorker : public FRunnable
{
	/** Singleton instance, can access the thread any time via static accessor, if it is active! */
	static  ThreadedWorker* Runnable;

	/** Thread to run the worker FRunnable on */
	FRunnableThread* Thread;

	AHouseBuilder *houseBuilder;
	//FHousePolygon housePol;

	/** Stop this thread? Uses Thread Safe Counter */
	FThreadSafeCounter StopTaskCounter;

private:

public:
	FHouseInfo resultingInfo;
	bool done = false;

	//Done?
	bool IsFinished() const
	{
		return done;
	}

	//~~~ Thread Core Functions ~~~

	//Constructor / Destructor
	ThreadedWorker(AHouseBuilder *house);
	virtual ~ThreadedWorker();

	// Begin FRunnable interface.
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	// End FRunnable interface

	/** Makes sure this thread has stopped properly */
	void EnsureCompletion();

	/** Shuts down the thread. Static so it can easily be called from outside the thread context */
	static void Shutdown();

	static bool IsThreadFinished();

};