#pragma once

namespace tdm
{

namespace updater
{

struct DifferentialUpdateInfo
{
	// The version we're upgrading from
	std::string fromVersion;

	// The version we're upgrading to
	std::string toVersion;

	// The package size
	std::size_t filesize;
};

} 

}
