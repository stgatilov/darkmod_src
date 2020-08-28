#pragma once

class idDecl;
class idImage;
class idSoundSample;
class idRenderModel;
class idEntity;
class idMapEntity;
class idWindow;

extern idCVar decl_stack;

//stgatilov: represents the sequence of nested object loads
//used for better reports of missing assets
class LoadStack {
public:
	enum Type {
		tNone = 0,
		tDecl,
		tImage,
		tSoundSample,
		tModel,
		tEntity,
		tMapEntity,
		tWindow,
	};
	struct Level {
		Type type;
		union {
			void *ptr;
			idDecl *decl;
			idImage *image;
			idSoundSample *soundSample;
			idRenderModel *model;
			idEntity *entity;
			idMapEntity *mapEntity;
			idWindow *window;
		};
		ID_FORCE_INLINE bool operator== (const Level &other) const { return ptr == other.ptr; }
		void Print(int indent) const;
	};
	//max chain: entity -> skin -> material -> image ?
	static const int MAX_LEVELS = 8;

	void Clear();
	template<class T> static Level LevelOf(T *ptr);
	void Rollback(void *ptr);
	void Append(const Level &lvl);
	void PrintStack(int indent, const Level &addLastLevel = Level()) const;

private:
	Level levels[MAX_LEVELS];
};
