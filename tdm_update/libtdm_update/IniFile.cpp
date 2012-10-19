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

#include "IniFile.h"

#include "TraceLog.h"
#include <set>
#include <fstream>
#include <sstream>

#include <boost/program_options/detail/convert.hpp>
#include <boost/program_options/detail/config_file.hpp>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/spirit/include/classic.hpp>
#include <boost/bind.hpp>

namespace bs = boost::spirit::classic;

namespace tdm
{

IniFile::IniFile()
{}

IniFile::IniFile(const std::string& str)
{
	ParseFromString(str);
}

IniFilePtr IniFile::Create()
{
	return IniFilePtr(new IniFile);
}

IniFilePtr IniFile::ConstructFromFile(const fs::path& filename)
{
	// Start parsing
	std::ifstream iniFile(filename.string().c_str());

	if (!iniFile)
    {
        tdm::TraceLog::WriteLine(LOG_VERBOSE, "[IniFile]: Cannot open file " + filename.string());
		return IniFilePtr();
    }

	return ConstructFromStream(iniFile);
}

IniFilePtr IniFile::ConstructFromStream(std::istream& stream)
{
	// Read the whole stream into a string
	std::string buffer(std::istreambuf_iterator<char>(stream), (std::istreambuf_iterator<char>()));

	return ConstructFromString(buffer);
}

IniFilePtr IniFile::ConstructFromString(const std::string& str)
{
	return IniFilePtr(new IniFile(str));
}

bool IniFile::IsEmpty() const
{
	return _settings.empty();
}

void IniFile::AddSection(const std::string& name)
{
	_settings.insert(SettingMap::value_type(name, KeyValues()));
}

std::string IniFile::GetValue(const std::string& section, const std::string& key) const
{
	SettingMap::const_iterator i = _settings.find(section);
	
	if (i == _settings.end()) return ""; // section not found
	
	KeyValues::const_iterator kv = i->second.find(KeyValuePair(key, ""));

	return (kv != i->second.end()) ? kv->second : "";
}

void IniFile::SetValue(const std::string& section, const std::string& key, const std::string& value)
{
	// Find the section, and create it if necessary
	SettingMap::iterator i = _settings.find(section);

	if (i == _settings.end())
	{
		AddSection(section);
	}

	// Section exists past this point

	KeyValues::iterator kv = _settings[section].find(KeyValuePair(key, ""));

	// Remove existing key value first
	if (kv != _settings[section].end())
	{
		_settings[section].erase(kv);
	}

	// Insert afresh
	_settings[section].insert(KeyValuePair(key, value));
}

bool IniFile::RemoveSection(const std::string& section)
{
	SettingMap::iterator i = _settings.find(section);

	if (i != _settings.end())
	{
		_settings.erase(i);
		return true;
	}

	return false; // not found
}

bool IniFile::RemoveKey(const std::string& section, const std::string& key)
{
	SettingMap::iterator i = _settings.find(section);

	if (i == _settings.end())
	{
		return false; // not found
	}

	KeyValues::iterator kv = i->second.find(KeyValuePair(key, ""));

	if (kv != i->second.end())
	{
		i->second.erase(kv);
		return true;
	}

	return false; // not found
}

IniFile::KeyValuePairList IniFile::GetAllKeyValues(const std::string& section) const
{
	SettingMap::const_iterator i = _settings.find(section);

	if (i == _settings.end())
	{
		return KeyValuePairList(); // not found
	}

	return KeyValuePairList(i->second.begin(), i->second.end());
}

void IniFile::ForeachSection(SectionVisitor& visitor) const
{
	for (SettingMap::const_iterator i = _settings.begin(); 
		 i != _settings.end(); /* in-loop increment */)
	{
		visitor.VisitSection(*this, (*i++).first);
	}
}

void IniFile::ExportToFile(const fs::path& file, const std::string& headerComments) const
{
	std::ofstream stream(file.string().c_str());

	if (!headerComments.empty())
	{
		// Split the header text into lines and export it as INI comment
		std::vector<std::string> lines;
		boost::algorithm::split(lines, headerComments, boost::algorithm::is_any_of("\n"));

		for (std::size_t i = 0; i < lines.size(); ++i)
		{
			stream << "# " << lines[i] << std::endl;
		}

		// add some additional line break after the header
		stream << std::endl;
	}

	for (SettingMap::const_iterator i = _settings.begin(); i != _settings.end(); ++i)
	{
		stream << "[" << i->first << "]" << std::endl;

		for (KeyValues::const_iterator kv = i->second.begin(); kv != i->second.end(); ++kv)
		{
			stream << kv->first << " = " << kv->second << std::endl;
		}
		
		stream << std::endl;
	}
}

// Functor class adding INI sections and keyvalues
// Keeps track of the most recently added section and key
// as the Add* methods are called in the order of parsing without context.
class IniParser
{
private:
	IniFile& _self;
	
	// Most recently added section and key
	std::string _lastSection;
	std::string _lastKey;

public:
	IniParser(IniFile& self) :
		_self(self)
	{}

	void AddSection(char const* beg, char const* end)
	{
		// Remember this section name
		_lastSection = std::string(beg, end);

		_self.AddSection(_lastSection);
	}

	void AddKey(char const* beg, char const* end)
	{
		assert(!_lastSection.empty()); // need to have parsed a section beforehand

		// Just remember the key name, an AddValue() call is imminent
		_lastKey = std::string(beg, end);

		boost::algorithm::trim(_lastKey);
	}

	void AddValue(char const* beg, char const* end)
	{
		assert(!_lastSection.empty());
		assert(!_lastKey.empty());

		_self.SetValue(_lastSection, _lastKey, std::string(beg, end));
	}
};

void IniFile::ParseFromString(const std::string& str)
{
	bs::rule<> char_ident_start = bs::alpha_p | bs::ch_p('_') ;

	// Allow anthing but the closing bracket for section names
	bs::rule<> char_section_ident_middle = ~(bs::ch_p(']'));

	// Allow anthing but the "=" for key names
	bs::rule<> char_ident_middle = ~(bs::ch_p('='));

	bs::rule<> ident = char_ident_start >> * char_ident_middle ;
	bs::rule<> sectionIdent = char_ident_start >> * char_section_ident_middle;
	bs::rule<> char_start_comment = bs::ch_p('#') | bs::ch_p(';') | bs::str_p("//");
	bs::rule<> blanks_p = * bs::blank_p;
	bs::rule<> value_p = * ( bs::alnum_p | bs::blank_p | bs::punct_p );

	// Create the helper object pushing the data
	IniParser parser(*this);
	
	bs::rule<> l_category = 
					blanks_p >> 
					bs::ch_p('[') >> 
					blanks_p >> 
					sectionIdent [ bind(&IniParser::AddSection, &parser, _1, _2) ] >>
					blanks_p >> 
					bs::ch_p(']') >> 
					blanks_p >> 
					bs::eol_p
	;
	
	bs::rule<> l_comment = blanks_p >> char_start_comment >> * bs::print_p >> bs::eol_p; 
	bs::rule<> l_empty = blanks_p >> bs::eol_p; 
	bs::rule<> c_comment_rule = bs::confix_p("/*", *bs::anychar_p, "*/");

	bs::rule<> b_comment = 
					blanks_p >> 
					c_comment_rule >>
					blanks_p >> 
					bs::eol_p
	; 

	bs::rule<> l_entry =  
					blanks_p >>
					ident [ bind(&IniParser::AddKey, &parser, _1, _2) ] >> 
					blanks_p >>
					bs::ch_p('=') >> 
					blanks_p >>
					value_p [ bind(&IniParser::AddValue, &parser, _1, _2) ] >> 
					blanks_p >>
					bs::eol_p
	; 

	bs::rule<> lines = l_comment | b_comment | l_category | l_entry | l_empty;
	bs::rule<> iniFileDef =  bs::lexeme_d [ * lines ] ;

	bs::parse_info<> info = bs::parse(str.c_str(), iniFileDef);

	if (info.full)
	{
		tdm::TraceLog::WriteLine(LOG_VERBOSE, "Successfully parsed the whole INI file.");
	}
	else
	{
		tdm::TraceLog::WriteLine(LOG_VERBOSE, "Could not fully parse the INI file.");
	}
}

} // namespace
