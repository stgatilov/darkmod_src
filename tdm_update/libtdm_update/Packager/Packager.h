#pragma once

#include "PackagerOptions.h"
#include "../ReleaseFileset.h"
#include "../UpdatePackage.h"

namespace tdm
{

namespace packager
{

/**
 * A class containing logic and algorithms for creating
 * TDM release packages. It can be used to compare PK4 sets
 * and generate update PK4s as well as crc_info.txt files.
 */
class Packager
{
public:
	// Thrown on any errors during method execution
	class FailureException :
		public std::runtime_error
	{
	public:
		FailureException(const std::string& message) :
			std::runtime_error(message)
		{}
	};

private:
	// Program parameters
	const PackagerOptions& _options;

	// The base set ("old")
	ReleaseFileSet _baseSet;

	// The head set ("new")
	ReleaseFileSet _headSet;

	// The PK4 representing the difference between base and head
	UpdatePackage _difference;

public:
	// Pass the program options to this class
	Packager(const PackagerOptions& options);

	// Collects information about the base set (given by basedir parameter)
	void GatherBaseSet();

	// Collects information about the head set (given by headdir parameter)
	void GatherHeadSet();

	// Searches for changes between base and head, and stores information locally.
	void CalculateSetDifference();

	// Creates the update PK4 in the output folder
	void CreateUpdatePackage();

	// Creates or update the version info file in the location given by the "version-info-file" parameter
	void CreateVersionInformation();

	void RegisterUpdatePackage(const fs::path& path);
};

} // namespace

} // namespace
