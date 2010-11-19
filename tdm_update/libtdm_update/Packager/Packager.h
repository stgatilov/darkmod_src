#pragma once

#include "PackagerOptions.h"
#include "../ReleaseFileset.h"
#include "../UpdatePackage.h"
#include "../ReleaseManifest.h"
#include "../Pk4Mappings.h"

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

	// ---- Package creation ----

	// The manifest of a specific release
	ReleaseManifest _manifest;
	Pk4Mappings _pk4Mappings;

	// Manifest files distributed into Pk4s
	typedef std::list<ManifestFile> ManifestFiles;
	typedef std::map<std::string, ManifestFiles> Package;
	Package _package;

	// ---- Package creation End ----

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

	// Adds the package information to the given ini file
	void RegisterUpdatePackage(const fs::path& path);

	// Loads the manifest information from the options - needs darkmoddir set to something
	void LoadManifest();

	// Checks if all files in the manifest are existing
	void CheckRepository();

	// Loads the darkmod_pk4s.txt file from the devel folder.
	void LoadPk4Mapping();

	// Sorts files into Pk4s, resulting in a ReleaseFileset
	void SortFilesIntoPk4s();

	// Creates the package at the given output folder
	void CreatePackage();

	// Create the crc_info.txt in the basedir (call GatherBaseSet() beforehand)
	void CreateCrcInfoFile();

private:
	// Worker thread for creating a release archive
	void ProcessPackageElement(Package::const_iterator p);
};

} // namespace

} // namespace
