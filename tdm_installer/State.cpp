#include "State.h"
#include "ZipSync.h"

State *g_state = new State();

State::~State() {}

void State::Reset() {
	_config.Clear();
	_localManifest.Clear();
	_loadedManifests.clear();
	_versionRefreshed.clear();
	_updater.reset();
	_oldConfigFilename.clear();
	_preferredMirror.clear();
}
