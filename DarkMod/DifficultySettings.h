#ifndef __DIFFICULTY_SETTINGS_H__
#define __DIFFICULTY_SETTINGS_H__

#include <map>
#include <list>

namespace difficulty {

/**
 * greebo: A DifficultyRecord represents a spawnarg change.
 *         This can be an assignment, addition or multiplication.
 */
class Setting
{
public:
	enum EApplicationType {
		EAssign,
		EAdd,
		EMultiply,
	};

	// The classname this setting applies to
	idStr className;

	// The target spawnarg to be changed
	idStr spawnArg;
	
	// The parsed argument (the specifier (+/*) has already been removed)
	idStr argument;

	// How the argument should be applied
	EApplicationType appType;

	// Whether this setting is valid
	bool isValid;

	// Default constructor
	Setting();

	// Save/Restore methods
	void Save(idSaveGame* savefile);
	void Restore(idRestoreGame* savefile);

	// Factory function: get the setting with the given index from the given dict
	static Setting ParseFromDict(const idDict& dict, int index);

	// Factory function: get all Settings from the given dict
	// The returned list is guaranteed to contain only valid settings.
	static idList<Setting> ParseFromDict(const idDict& dict);
};

/**
 * greebo: This class encapsulates the difficulty settings
 *         for a given difficulty level (easy/medium/hard/etc).
 *
 * Use the ApplySettings() method to apply the settings on a set of spawnargs.
 */
class DifficultySettings
{
	// The settings map associates classnames with spawnarg change records.
	// Multiple settings can be made for a single classname.
	typedef std::multimap<std::string, Setting> SettingsMap;
	SettingsMap _settings;

	// A linked list for representing the inheritance chain
	typedef std::list<idStr> InheritanceChain;

	// the difficulty level these settings are referring to
	int _level; 

public:
	// Wipes the contents of this class
	void Clear();

	// Sets the level of these settings
	void SetLevel(int level);
	int GetLevel() const;

	/**
	 * greebo: Loads the difficulty settings from the given entityDef
	 */
	void LoadFromEntityDef(const idDict& defDict);

	/**
	 * greebo: This loads the difficulty settings from the given map entity.
	 * Settings loaded from the entity will replace settings with the same 
	 * classname/spawnarg combination found in the default entityDefs.
	 */
	void LoadFromMapEntity(idMapEntity* ent);

	/**
	 * greebo: Applies the contained difficulty settings on the given set of spawnargs.
	 */
	void ApplySettings(idDict& target);

	// Save/Restore methods
	void Save(idSaveGame* savefile);
	void Restore(idRestoreGame* savefile);

private:
	// greebo: Returns the value of the "inherit" spawnarg for the given classname
	// This parses the raw declaration text on a char-per-char basis, this is 
	// necessary because the "inherit" key gets removed by the entityDef parser after loading.
	idStr GetInheritValue(const idStr& className);

	// Returns the inheritance chain for the given dict
	InheritanceChain GetInheritanceChain(const idDict& dict);

	// Applies the given setting to the target dictionary.
	// The inheritance of the given target dictionary is considered.
	void ApplySetting(Setting& setting, idDict& target);
};

} // namespace difficulty

#endif /* __DIFFICULTY_SETTINGS_H__ */
