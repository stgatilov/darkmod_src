/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod Updater (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#pragma once

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <string>

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
 * back to the calling thread. The function passed to boost::thread
 * is decorated with a try/catch block, which is storing the exception's
 * error message locally. After joining the thread the exception is 
 * re-generated such that the calling thread gets a chance to catch it.
 */
class ExceptionSafeThread
{
private:
	// The function object to call
	boost::function<void()> _function;

	// The actual thread
	boost::shared_ptr<boost::thread> _thread;

	// The error message which is filled when exceptions are thrown within the thread
	std::string _errMsg;

	bool _interrupted;

	bool _done;

	boost::function<void()> _onFinish;

public:
	ExceptionSafeThread(const boost::function<void()>& function) :
		_function(function),
		_thread(new boost::thread(boost::bind(&ExceptionSafeThread::ThreadStart, this))),
		_interrupted(false),
		_done(false)
	{}

	// Construct this thread with a callback which will be invoked when the thread is done (failure or not)
	ExceptionSafeThread(const boost::function<void()>& function, const boost::function<void()>& callbackOnFinish) :
		_function(function),
		_thread(new boost::thread(boost::bind(&ExceptionSafeThread::ThreadStart, this))),
		_interrupted(false),
		_done(false),
		_onFinish(callbackOnFinish)
	{}

	~ExceptionSafeThread()
	{
		if (_thread != NULL && !_done)
		{
			_thread->interrupt();
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
			_thread->interrupt();
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
		catch (boost::thread_interrupted&)
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
			// This might destroy this object, don't do anything afterwards
			_onFinish(); 
		}
	}
};
typedef boost::shared_ptr<ExceptionSafeThread> ExceptionSafeThreadPtr;

} // namespace
