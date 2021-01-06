#include "precompiled.h"
#include "LoadStack.h"
#include "../renderer/Image.h"
#include "DeclManager.h"
#include "MapFile.h"
#include "../sound/snd_local.h"
#include "../ui/Window.h"


idCVar decl_stack( "decl_stack", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "when enabled, print load stack after every warning about missing asset" );

void LoadStack::Clear() {
	memset(this, 0, sizeof(*this));
}

template<> LoadStack::Level LoadStack::LevelOf(idDecl *ptr) {
	return Level{tDecl, ptr};
}
template<> LoadStack::Level LoadStack::LevelOf(idImage *ptr) {
	return Level{tImage, ptr};
}
template<> LoadStack::Level LoadStack::LevelOf(idSoundSample *ptr) {
	return Level{tSoundSample, ptr};
}
template<> LoadStack::Level LoadStack::LevelOf(idRenderModel *ptr) {
	return Level{tModel, ptr};
}
template<> LoadStack::Level LoadStack::LevelOf(idEntity *ptr) {
	return Level{tEntity, ptr};
}
template<> LoadStack::Level LoadStack::LevelOf(idMapEntity *ptr) {
	return Level{tMapEntity, ptr};
}
template<> LoadStack::Level LoadStack::LevelOf(idWindow *ptr) {
	return Level{tWindow, ptr};
}

void LoadStack::Append(const Level &lvl) {
	int i;
	for (i = 0; i < MAX_LEVELS; i++)
		if (levels[i].type == tNone)
			break;
	if (i == MAX_LEVELS)
		return;	//overflow, save top-level part
	levels[i] = lvl;
}

void LoadStack::Rollback(void *ptr) {
	int i;
	for (i = MAX_LEVELS - 1; i >= 0; i--)
		if (levels[i].ptr == ptr)
			break;
	if (i < 0)
		return;	//such level does not exist
	for (; i < MAX_LEVELS; i++) {
		levels[i].type = tNone;
		levels[i].ptr = nullptr;
	}
}

void LoadStack::PrintStack(int indent, const Level &addLastLevel) const {
	if (!decl_stack.GetBool())
		return;	//disabled
	for (int i = 0; i < MAX_LEVELS; i++) {
		if (levels[i].type == tNone || levels[i].ptr == nullptr)
			continue;
		levels[i].Print(indent);
	}
	if (addLastLevel.type != tNone)
		addLastLevel.Print(indent);
}
void LoadStack::Level::Print(int indent) const {
	if (!decl_stack.GetBool())
		return;	//disabled
	char spaces[256];
	memset(spaces, ' ', indent);
	spaces[indent] = 0;
	if (type == tNone)
		common->Printf("%s[none]\n", spaces);
	else if (type == tDecl)
		common->Printf("%s[decl: %s in %s]\n", spaces, decl->GetName(), decl->GetFileName());
	else if (type == tImage)
		common->Printf("%s[image: %s]\n", spaces, image->imgName.c_str());
	else if (type == tSoundSample)
		common->Printf("%s[sound: %s]\n", spaces, soundSample->name.c_str());
	else if (type == tModel)
		common->Printf("%s[model: %s]\n", spaces, model->Name());
	else if (type == tMapEntity)
		common->Printf("%s[map entity: %s]\n", spaces, mapEntity->epairs.GetString("name"));
	else if (type == tEntity)
		common->Printf("%s[game entity: %s]\n", spaces, entity->GetName());
	else if (type == tWindow)
		common->Printf("%s[window: %s]\n", spaces, window->GetName());
	else
		common->Printf("%s[corrupted: %d]\n", spaces, type);
}
