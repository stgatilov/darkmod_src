#pragma once

#include <list>
#include <fstream>
#include <boost/spirit.hpp>
#include <boost/bind.hpp>

namespace bs = boost::spirit;

namespace tdm
{

struct ManifestFile
{
	// The source file path relative to darkmod/
	fs::path sourceFile;

	// The destination path in the release, is almost always the same as sourceFile
	// except for some special cases (config.spec, DoomConfig.cfg, etc.)
	fs::path destFile;

	ManifestFile(const fs::path& sourceFile_) :
		sourceFile(sourceFile_),
		destFile(sourceFile_)
	{}

	ManifestFile(const fs::path& sourceFile_, const fs::path& destFile_) :
		sourceFile(sourceFile_),
		destFile(destFile_)
	{}
};

/**
 * The manifest contains all the files which should go into a TDM release.
 * The list is unsorted, it doesn't matter if files are duplicate, as they
 * are re-sorted into PK4 mappings anyways, which resolves any double entries.
 *
 * File format:
 * Each line contains a single file, its path relative to darkmod/.
 * Comments lines start with the # character.
 * It's possible to move files to a different location by using the => syntax: 
 * e.g. ./devel/release/config.spec => ./config.spec
 */
class ReleaseManifest :
	public std::list<ManifestFile>
{
public:
	void LoadFromFile(const fs::path& manifestFile)
	{
		// Start parsing
		std::ifstream file(manifestFile.file_string().c_str());

		if (!file)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "[ReleaseManifest]: Cannot open file " + manifestFile.file_string());
			return;
		}

		LoadFromStream(file);
	}

	void LoadFromStream(std::istream& stream)
	{
		// Read the whole stream into a string
		std::string buffer(std::istreambuf_iterator<char>(stream), (std::istreambuf_iterator<char>()));

		LoadFromString(buffer);
	}

	void LoadFromString(const std::string& str)
	{
		clear();

		// Comment starting character: #
		bs::rule<> char_start_comment = bs::ch_p('#');

		// Define blank characters
		bs::rule<> blanks_p = * bs::blank_p;

		// Define comment lines
		bs::rule<> l_comment = blanks_p >> char_start_comment >> *bs::print_p >> bs::eol_p; 

		// Define empty lines
		bs::rule<> l_empty = blanks_p >> bs::eol_p; 

		// A filename
		bs::rule<> filename_ident = +(bs::print_p - bs::ch_p('='));

		// The relocator => 
		bs::rule<> relocator_ident = bs::ch_p('=') >> bs::ch_p('>');

		// Define relocation rules
		bs::rule<> l_filename = 
					blanks_p >> 
					filename_ident[ boost::bind(&ReleaseManifest::AddSourceFile, this, _1, _2) ] >> 
					blanks_p >> 
					*relocator_ident >> 
					blanks_p >> 
					(*filename_ident) [ boost::bind(&ReleaseManifest::AddDestFile, this, _1, _2) ] >>
					blanks_p >> 
					bs::eol_p
		;

		bs::rule<> lines = l_comment | l_filename | l_empty;
		bs::rule<> manifestDef =  bs::lexeme_d [ * lines ] ;

		bs::parse_info<> info = bs::parse(str.c_str(), manifestDef);

		if (info.full)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Successfully parsed the whole manifest.");
		}
		else
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Could not fully parse the manifest.");
		}
	}

private:
	void AddSourceFile(char const* beg, char const* end)
	{
		if (end - beg < 2) return; // skip strings smaller than 2 chars

		if (beg[0] == '.' && beg[1] == '/') 
		{
			beg += 2; // skip leading ./
		}

		push_back(ManifestFile(std::string(beg, end)));
	}

	void AddDestFile(char const* beg, char const* end)
	{
		if (end - beg < 2) return; // skip strings smaller than 2 chars

		if (beg[0] == '.' && beg[1] == '/') 
		{
			beg += 2; // skip leading ./
		}

		// Set the destination on the last element
		assert(!empty());

		back().destFile = std::string(beg, end);
	}

};

} // namespace
