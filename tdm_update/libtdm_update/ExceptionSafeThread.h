/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#pragma once

#include <thread>
#include <functional>
#include <string>
#include <memory>
#include "ThreadControl.h"

namespace tdm
{

// Thrown to the calling thread if the user hits Ctrl-C
class UserInterruptException :
	public std::runtime_error
{
public:
	UserInterruptException(const std::string& message) :
		std::runtime_error(message)
	{}
};

/**
 * greebo: A wrapper class which is able to pass exceptions 
 * back to the calling thread. The function passed to std::thread
 * is decorated with a try/catch block, which is storing the exception's
 * error message locally. After joining the thread the exception is 
 * re-generated such that the calling thread gets a chance to catch it.
 */
class ExceptionSafeThread
{
private:
	// The function object to call
	std::function<void()> _function;

	// The actual thread
	std::shared_ptr<std::thread> _thread;

	// The error message which is filled when exceptions are thrown within the thread
	std::string _errMsg;

	bool _interrupted;

	bool _done;

    std::function<void()> _onFinish;

public:
    ExceptionSafeThread(const std::function<void()>& function) :
		_function(function),
		_thread(new std::thread(std::bind(&ExceptionSafeThread::ThreadStart, this))),
		_interrupted(false),
		_done(false)
	{}

	// Construct this thread with a callback which will be invoked when the thread is done (failure or not)
    ExceptionSafeThread(const std::function<void()>& function, const std::function<void()>& callbackOnFinish) :
		_function(function),
        _thread(new std::thread(std::bind(&ExceptionSafeThread::ThreadStart, this))),
		_interrupted(false),
		_done(false),
		_onFinish(callbackOnFinish)
	{}

	~ExceptionSafeThread()
	{
		if (_thread != NULL)
		{
            if (!_done)
            {
                ThreadControl::InterruptThread(_thread->get_id());
            }

            // If the main app thread is calling this, let's join first
            if (_thread->joinable())
            {
                if (_thread->get_id() != std::this_thread::get_id())
                {
                    _thread->join();
                    //note: _thread is empty after joining
                }
                else {
                    // Any joinable thread must be detached before its destructor is reached
                    _thread->detach();
                }
            }
		}
	}

	void join()
	{
		if (_thread != NULL)
		{
			_thread->join();
		}

		if (_interrupted)
		{
			throw UserInterruptException("User requested termination.");
		}

		// Now that we're back in the main thread, re-throw the exception
		if (!_errMsg.empty())
		{
			throw std::runtime_error(_errMsg.c_str());
		}
	}

	// Gets called when the user attempts to terminate the program
	void interrupt()
	{
		if (_thread != NULL)
		{
            ThreadControl::InterruptThread(_thread->get_id());
		}
	}

	bool done() const
	{
		return _done;
	}

	// Returns true if the worker thread threw an exception
	bool failed() const
	{
		return !_errMsg.empty();
	}

	const std::string& GetErrorMessage() const
	{
		return _errMsg;
	}

private:
	void ThreadStart()
	{
		try
		{
			_function();
		}
        catch (ThreadInterruptedException&)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Thread interrupted.");
			_interrupted = true;
		}
		catch (std::runtime_error& ex)
		{
			_errMsg = ex.what();
		}

		_done = true;

		// Invoke the callback when done
		if (_onFinish)
		{
            // Make sure we're not joinable before calling the handler
            _thread->detach();

			// This might destroy this object, don't do anything afterwards
			_onFinish(); 
		}
	}
};
typedef std::shared_ptr<ExceptionSafeThread> ExceptionSafeThreadPtr;

} // namespace
