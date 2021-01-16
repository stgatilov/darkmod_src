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
		const idPoolStr *values[2];

		ID_FORCE_INLINE Level() { type = tNone; ptr = nullptr; values[0] = values[1] = nullptr; }
		ID_FORCE_INLINE bool operator== (const Level &other) const { return ptr == other.ptr; }
		void Print(int indent) const;
		void SaveStrings();
		void FreeStrings();
		void Clear();
	};
	//max chain: entity -> skin -> material -> image ?
	static const int MAX_LEVELS = 8;

	~LoadStack();
	LoadStack() = default;
	LoadStack(const LoadStack &src);
	LoadStack& operator= (const LoadStack &) = delete;	//TODO

	void Clear();
	template<class T> static Level LevelOf(T *ptr);
	void Rollback(void *ptr);
	void Append(const Level &lvl);
	void PrintStack(int indent, const Level &addLastLevel = Level()) const;

	static void ShowMemoryUsage_f( const idCmdArgs &args );
	static void ListStrings_f( const idCmdArgs &args );

private:
	Level levels[MAX_LEVELS];

	//all strings are stored in this pool
	static idStrPool pool;
};
