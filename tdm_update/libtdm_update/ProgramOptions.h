#pragma once

#include <boost/program_options.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "Http/HttpConnection.h"
#include "TraceLog.h"

namespace bpo = boost::program_options;

namespace tdm
{

class ProgramOptions
{
protected:
	bpo::options_description _desc;
	bpo::variables_map _vm;

	// The command line arguments for reference
	std::vector<std::string> _cmdLineArgs;

public:
	virtual ~ProgramOptions()
	{}

	void ParseFromCommandLine(int argc, char* argv[])
	{
		try
		{
			bpo::store(bpo::parse_command_line(argc, argv, _desc), _vm);
			bpo::notify(_vm);
		}
		catch (bpo::unknown_option& o)
		{
			TraceLog::WriteLine(LOG_STANDARD, " " + std::string(o.what()));
		}
	}

	void Set(const std::string& key)
	{
		_vm.insert(std::make_pair(key, bpo::variable_value()));
		_cmdLineArgs.push_back("--" + key);
	}

	void Set(const std::string& key, const std::string& value)
	{
		_vm.insert(std::make_pair(key, bpo::variable_value(value, false)));
		_cmdLineArgs.push_back("--" + key + "=" + value);
	}

	void Unset(const std::string& key)
	{
		_vm.erase(key);

		for (std::vector<std::string>::iterator i = _cmdLineArgs.begin(); 
			 i != _cmdLineArgs.end(); ++i)
		{
			if (boost::algorithm::starts_with(*i, "--" + key))
			{
				_cmdLineArgs.erase(i);
				break;
			}
		}
	}

	bool Empty() const
	{
		return _vm.empty();
	}

	bool IsSet(const std::string& key) const
	{
		return _vm.count(key) > 0;
	}

	std::string Get(const std::string& key) const
	{
		return _vm.count(key) > 0 ? _vm[key].as<std::string>() : "";
	}

	const std::vector<std::string>& GetRawCmdLineArgs() const
	{
		return _cmdLineArgs;
	}

	virtual void PrintHelp()
	{
		std::ostringstream stream;
		_desc.print(stream);

		TraceLog::WriteLine(LOG_STANDARD, " " + stream.str());
	}

protected:
	/**
	 * Subclasses should implement this method to populate the available options
	 * and call it in their constructors.
	 */
	virtual void SetupDescription() = 0;
};

}
