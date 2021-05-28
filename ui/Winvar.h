/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __WINVAR_H__
#define __WINVAR_H__

#include "Rectangle.h"

static const char *VAR_GUIPREFIX = "gui::";
static const int VAR_GUIPREFIX_LEN = static_cast<int>(strlen(VAR_GUIPREFIX));

class idWindow;
class idWinVar {
public:
	idWinVar();
	virtual ~idWinVar();

	void SetGuiInfo(idDict *gd, const char *_name);
	const char *GetName() const { 
		if (name) {
			if (guiDict && *name == '*') {
				return guiDict->GetString(&name[1]);
			}
			return name;
		}
		return ""; 
	}
	void SetName(const char *_name) { 
		delete []name; 
		name = NULL;
		if (_name) {
			name = new char[strlen(_name)+1]; 
			strcpy(name, _name); 
		}
	}

	idWinVar &operator=( const idWinVar &other ) {
		guiDict = other.guiDict;
		SetName(other.name);
		return *this;
	}

	idDict *GetDict() const { return guiDict; }
	bool NeedsUpdate() { return (guiDict != NULL); }

	virtual void Init(const char *_name, idWindow* win) = 0;
	virtual void Set(const char *val) = 0;
	virtual void Update() = 0;
	virtual const char *c_str() const = 0;
	virtual size_t Size() {	size_t sz = (name) ? strlen(name) : 0; return sz + sizeof(*this); }

	virtual void WriteToSaveGame( idFile *savefile ) = 0;
	virtual void ReadFromSaveGame( idFile *savefile ) = 0;

	virtual float x( void ) const = 0;

	void SetEval(bool b) {
		eval = b;
	}
	bool GetEval() {
		return eval;
	}
	
protected:
	idDict *guiDict;
	char *name;
	bool eval;
};

class idWinBool : public idWinVar {
public:
	idWinBool() : idWinVar() {};
	~idWinBool() {};
	virtual void Init(const char *_name, idWindow *win) { idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetBool(GetName());
		}
	}
	int	operator==(	const bool &other ) { return (other == data); }
	bool &operator=(	const bool &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetBool(GetName(), data);
		}
		return data;
	}
	idWinBool &operator=( const idWinBool &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}

	operator bool() const { return data; }

	virtual void Set(const char *val) { 
		int parsedVal = 0;
		if (!(idStr::IsNumeric(val) && sscanf(val, "%d", &parsedVal) == 1))
			common->Warning("Wrong idWinBool: \"%s\"", val);
		data = ( parsedVal != 0 );
		if (guiDict) {
			guiDict->SetBool(GetName(), data);
		}
	}

	virtual void Update() {	
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetBool( s );
		}
	}

	virtual const char *c_str() const {return va("%i", data); }

	// SaveGames
	virtual void WriteToSaveGame( idFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

	virtual float x( void ) const { return data ? 1.0f : 0.0f; };

protected:
	bool data = false;
};

class idWinStr : public idWinVar {
public:
	idWinStr() : idWinVar() {};
	~idWinStr() {};
	virtual void Init(const char *_name, idWindow *win) {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetString(GetName());
		} 
	}
	int	operator==(	const idStr &other ) const {
		return (other == data);
	}
	int	operator==(	const char *other ) const {
		return (data == other);
	}
	idStr &operator=(	const idStr &other ) {
		data = other;
		if (guiDict) {
			guiDict->Set(GetName(), data);
		}
		return data;
	}
	idWinStr &operator=( const idWinStr &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	operator const char *() const {
		return data.c_str();
	}
	operator const idStr &() const {
		return data;
	}
	int LengthWithoutColors() {
		if (guiDict && name && *name) {
			data = guiDict->GetString(GetName());
		}
		return data.LengthWithoutColors();
	}
	int Length() {
		if (guiDict && name && *name) {
			data = guiDict->GetString(GetName());
		}
		return data.Length();
	}
	void RemoveColors() {
		if (guiDict && name && *name) {
			data = guiDict->GetString(GetName());
		}
		data.RemoveColors();
	}
	virtual const char *c_str() const {
		return data.c_str();
	}

	virtual void Set(const char *val) {
		data = val;
		if ( guiDict ) {
			guiDict->Set(GetName(), data);
		}
	}

	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetString( s );
		}
	}

	virtual size_t Size() {
		size_t sz = idWinVar::Size();
		return sz +data.Allocated();
	}

	// SaveGames
	virtual void WriteToSaveGame( idFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );

		int len = data.Length();
		savefile->Write( &len, sizeof( len ) );
		if ( len > 0 ) {
			savefile->Write( data.c_str(), len );
		}
	}
	virtual void ReadFromSaveGame( idFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );

		int len;
		savefile->Read( &len, sizeof( len ) );
		if ( len > 0 ) {
			data.Fill( ' ', len );
			savefile->Read( &data[0], len );
		}
	}

	// return wether string is emtpy
	virtual float x( void ) const { return data[0] ? 1.0f : 0.0f; };

protected:
	idStr data;
};

class idWinInt : public idWinVar {
public:
	idWinInt() : idWinVar() {};
	~idWinInt() {};
	virtual void Init(const char *_name, idWindow *win) {
		idWinVar::Init(_name,  win);
		if (guiDict) {
			data = guiDict->GetInt(GetName());
		} 
	}
	int &operator=(	const int &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetInt(GetName(), data);
		}
		return data;
	}
	idWinInt &operator=( const idWinInt &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	operator int () const {
		return data;
	}
	virtual void Set(const char *val) {
		int parsedVal = 0;
		if (!(idStr::IsNumeric(val) && sscanf(val, "%d", &parsedVal) == 1))
			common->Warning("Wrong idWinInt: \"%s\"", val);
		data = parsedVal;
		if (guiDict) {
			guiDict->SetInt(GetName(), data);
		}
	}

	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetInt( s );
		}
	}
	virtual const char *c_str() const {
		return va("%i", data);
	}

	// SaveGames
	virtual void WriteToSaveGame( idFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

	// no suitable conversion
	virtual float x( void ) const { assert( false ); return 0.0f; };

protected:
	int data = 0;
};

class idWinFloat : public idWinVar {
public:
	idWinFloat() : idWinVar() {};
	~idWinFloat() {};
	virtual void Init(const char *_name, idWindow *win) {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetFloat(GetName());
		} 
	}
	idWinFloat &operator=( const idWinFloat &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	float &operator=(	const float &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetFloat(GetName(), data);
		}
		return data;
	}
	operator float() const {
		return data;
	}
	virtual void Set(const char *val) {
		float parsedVal = 0;
		if (!(idStr::IsNumeric(val) && sscanf(val, "%f", &parsedVal) == 1))
			common->Warning("Wrong idWinFloat: \"%s\"", val);
		data = parsedVal;
		if (guiDict) {
			guiDict->SetFloat(GetName(), data);
		}
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetFloat( s );
		}
	}
	virtual const char *c_str() const {
		return va("%f", data);
	}

	virtual void WriteToSaveGame( idFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

	virtual float x( void ) const { return data; };
protected:
	float data = 0.0f;
};

class idWinRectangle : public idWinVar {
public:
	idWinRectangle() : idWinVar() {};
	~idWinRectangle() {};
	virtual void Init(const char *_name, idWindow *win) {
		idWinVar::Init(_name, win);
		if (guiDict) {
			idVec4 v = guiDict->GetVec4(GetName());
			data.x = v.x;
			data.y = v.y;
			data.w = v.z;
			data.h = v.w;
		} 
	}
	
	int	operator==(	const idRectangle &other ) const {
		return (other == data);
	}

	idWinRectangle &operator=( const idWinRectangle &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	idRectangle &operator=(	const idVec4 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVec4(GetName(), other);
		}
		return data;
	}

	idRectangle &operator=(	const idRectangle &other ) {
		data = other;
		if (guiDict) {
			idVec4 v = data.ToVec4();
			guiDict->SetVec4(GetName(), v);
		}
		return data;
	}
	
	operator const idRectangle&() const {
		return data;
	}

	float x() const {
		return data.x;
	}
	float y() const {
		return data.y;
	}
	float w() const {
		return data.w;
	}
	float h() const {
		return data.h;
	}
	float Right() const {
		return data.Right();
	}
	float Bottom() const {
		return data.Bottom();
	}
	idVec4 &ToVec4() {
		static idVec4 ret;
		ret = data.ToVec4();
		return ret;
	}
	virtual void Set(const char *val) {
		int ret;
		idRectangle v;
		if ( strchr ( val, ',' ) ) {
			ret = sscanf( val, "%f,%f,%f,%f", &v.x, &v.y, &v.w, &v.h );
		} else {
			ret = sscanf( val, "%f %f %f %f", &v.x, &v.y, &v.w, &v.h );
		}
		if (ret != 4)
			common->Warning("Wrong idWinRectangle: \"%s\"", val);
		data = v;
		if (guiDict) {
			idVec4 v = data.ToVec4();
			guiDict->SetVec4(GetName(), v);
		}
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			idVec4 v = guiDict->GetVec4( s );
			data.x = v.x;
			data.y = v.y;
			data.w = v.z;
			data.h = v.w;
		}
	}

	virtual const char *c_str() const {
		return data.ToVec4().ToString();
	}

	virtual void WriteToSaveGame( idFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	idRectangle data;
};

class idWinVec2 : public idWinVar {
public:
	idWinVec2() : idWinVar() {};
	~idWinVec2() {};
	virtual void Init(const char *_name, idWindow *win) {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetVec2(GetName());
		} 
	}
	int	operator==(	const idVec2 &other ) const {
		return (other == data);
	}
	idWinVec2 &operator=( const idWinVec2 &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	
	idVec2 &operator=(	const idVec2 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVec2(GetName(), data);
		}
		return data;
	}
	float x() const {
		return data.x;
	}
	float y() const {
		return data.y;
	}
	virtual void Set(const char *val) {
		int ret;
		idVec2 v(0.0f, 0.0f);
		if ( strchr ( val, ',' ) ) {
			ret = sscanf( val, "%f,%f", &v.x, &v.y );
		} else {
			ret = sscanf( val, "%f %f", &v.x, &v.y);
		}
		if (ret != 2)
			common->Warning("Wrong idWinVec2: \"%s\"", val);
		data = v;
		if (guiDict) {
			guiDict->SetVec2(GetName(), data);
		}
	}
	operator const idVec2&() const {
		return data;
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetVec2( s );
		}
	}
	virtual const char *c_str() const {
		return data.ToString();
	}
	void Zero() {
		data.Zero();
	}

	virtual void WriteToSaveGame( idFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	idVec2 data = idVec2(0.0f, 0.0f);
};

class idWinVec4 : public idWinVar {
public:
	idWinVec4() : idWinVar() {};
	~idWinVec4() {};
	virtual void Init(const char *_name, idWindow *win) {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetVec4(GetName());
		} 
	}
	int	operator==(	const idVec4 &other ) const {
		return (other == data);
	}
	idWinVec4 &operator=( const idWinVec4 &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	idVec4 &operator=(	const idVec4 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVec4(GetName(), data);
		}
		return data;
	}
	operator const idVec4&() const {
		return data;
	}

	float x() const {
		return data.x;
	}

	float y() const {
		return data.y;
	}

	float z() const {
		return data.z;
	}

	float w() const {
		return data.w;
	}
	virtual void Set(const char *val) {
		int ret;
		idVec4 v(0.0f, 0.0f, 0.0f, 0.0f);
		if ( strchr ( val, ',' ) ) {
			ret = sscanf( val, "%f,%f,%f,%f", &v.x, &v.y, &v.z, &v.w );
		} else {
			ret = sscanf( val, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
		}
		//stgatilov: "transition" expects vec4, but it often receives scalar for e.g. "rotation" property
		if (ret != 4 && ret != 1)
			common->Warning("Wrong idWinVec4: \"%s\"", val);
		data = v;
		if ( guiDict ) {
			guiDict->SetVec4( GetName(), data );
		}
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetVec4( s );
		}
	}
	virtual const char *c_str() const {
		return data.ToString();
	}

	void Zero() {
		data.Zero();
		if ( guiDict ) {
			guiDict->SetVec4(GetName(), data);
		}
	}

	const idVec3 &ToVec3() const {
		return data.ToVec3();
	}

	virtual void WriteToSaveGame( idFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	idVec4 data = idVec4(0.0f, 0.0f, 0.0f, 0.0f);
};

class idWinVec3 : public idWinVar {
public:
	idWinVec3() : idWinVar() {};
	~idWinVec3() {};
	virtual void Init(const char *_name, idWindow *win) {
		idWinVar::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetVector(GetName());
		} 
	}
	int	operator==(	const idVec3 &other ) const {
		return (other == data);
	}
	idWinVec3 &operator=( const idWinVec3 &other ) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	idVec3 &operator=(	const idVec3 &other ) {
		data = other;
		if (guiDict) {
			guiDict->SetVector(GetName(), data);
		}
		return data;
	}
	operator const idVec3&() const {
		return data;
	}

	float x() const {
		return data.x;
	}

	float y() const {
		return data.y;
	}

	float z() const {
		return data.z;
	}

	virtual void Set(const char *val) {
		int ret;
		idVec3 v(0.0f, 0.0f, 0.0f);
		if ( strchr ( val, ',' ) ) {
			ret = sscanf( val, "%f,%f,%f", &v.x, &v.y, &v.z );
		} else {
			ret = sscanf( val, "%f %f %f", &v.x, &v.y, &v.z );
		}
		if (ret != 3)
			common->Warning("Wrong idWinVec3: \"%s\"", val);
		data = v;
		if (guiDict) {
			guiDict->SetVector(GetName(), data);
		}
	}
	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetVector( s );
		}
	}
	virtual const char *c_str() const {
		return data.ToString();
	}

	void Zero() {
		data.Zero();
		if (guiDict) {
			guiDict->SetVector(GetName(), data);
		}
	}

	virtual void WriteToSaveGame( idFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );
		savefile->Write( &data, sizeof( data ) );
	}
	virtual void ReadFromSaveGame( idFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );
		savefile->Read( &data, sizeof( data ) );
	}

protected:
	idVec3 data = idVec3(0.0f, 0.0f, 0.0f);
};

class idWinBackground : public idWinStr {
public:
	idWinBackground() : idWinStr() {
		mat = NULL;
	};
	~idWinBackground() {};
	virtual void Init(const char *_name, idWindow *win) {
		idWinStr::Init(_name, win);
		if (guiDict) {
			data = guiDict->GetString(GetName());
		} 
	}
	int	operator==(	const idStr &other ) const {
		return (other == data);
	}
	int	operator==(	const char *other ) const {
		return (data == other);
	}
	idStr &operator=(	const idStr &other ) {
		data = other;
		if (guiDict) {
			guiDict->Set(GetName(), data);
		}
		if (mat) {
			if ( data == "" ) {
				(*mat) = NULL;
			} else {
				(*mat) = declManager->FindMaterial(data);
			}
		}
		return data;
	}
	idWinBackground &operator=( const idWinBackground &other ) {
		idWinVar::operator=(other);
		data = other.data;
		mat = other.mat;
		if (mat) {
			if ( data == "" ) {
				(*mat) = NULL;
			} else {
				(*mat) = declManager->FindMaterial(data);
			}
		}
		return *this;
	}
	operator const char *() const {
		return data.c_str();
	}
	operator const idStr &() const {
		return data;
	}
	int Length() {
		if (guiDict) {
			data = guiDict->GetString(GetName());
		}
		return data.Length();
	}
	virtual const char *c_str() const {
		return data.c_str();
	}

	virtual void Set(const char *val) {
		data = val;
		if (guiDict) {
			guiDict->Set(GetName(), data);
		}
		if (mat) {
			if ( data == "" ) {
				(*mat) = NULL;
			} else {
				(*mat) = declManager->FindMaterial(data);
			}
		}
	}

	virtual void Update() {
		const char *s = GetName();
		if ( guiDict && s[0] != '\0' ) {
			data = guiDict->GetString( s );
			if (mat) {
				if ( data == "" ) {
					(*mat) = NULL;
				} else {
					(*mat) = declManager->FindMaterial(data);
				}
			}
		}
	}

	virtual size_t Size() {
		size_t sz = idWinVar::Size();
		return sz +data.Allocated();
	}

	void SetMaterialPtr( const idMaterial **m ) {
		mat = m;
	}

	virtual void WriteToSaveGame( idFile *savefile ) {
		savefile->Write( &eval, sizeof( eval ) );

		int len = data.Length();
		savefile->Write( &len, sizeof( len ) );
		if ( len > 0 ) {
			savefile->Write( data.c_str(), len );
		}
	}
	virtual void ReadFromSaveGame( idFile *savefile ) {
		savefile->Read( &eval, sizeof( eval ) );

		int len;
		savefile->Read( &len, sizeof( len ) );
		if ( len > 0 ) {
			data.Fill( ' ', len );
			savefile->Read( &data[0], len );
		}
		if ( mat ) {
			if ( len > 0 ) {
				(*mat) = declManager->FindMaterial( data );
			} else {
				(*mat) = NULL;
			}
		}
	}

protected:
	idStr data;
	const idMaterial **mat;
};

/*
================
idMultiWinVar
multiplexes access to a list if idWinVar*
================
*/
class idMultiWinVar : public idList< idWinVar * > {
public:
	void Set( const char *val );
	void Update( void );
	void SetGuiInfo( idDict *dict );
};


//stgatilov: sometimes pointers are stored in idWinVar
//so I added this type specifically for such hacky cases
class idWinUIntPtr : public idWinVar {
public:
	idWinUIntPtr() : idWinVar() {};
	~idWinUIntPtr() {};
	virtual void Init(const char *_name, idWindow *win) {
		idWinVar::Init(_name, win);
		assert(!guiDict);
	}
	size_t &operator=(const size_t &other) {
		data = other;
		assert(!guiDict);
		return data;
	}
	idWinUIntPtr &operator=(const idWinUIntPtr &other) {
		idWinVar::operator=(other);
		data = other.data;
		return *this;
	}
	operator size_t() const {
		return data;
	}
	virtual void Set(const char *val) {
		size_t parsedVal = 0;
		if (!(idStr::IsNumeric(val) && sscanf(val, "%zu", &parsedVal) == 1))
			common->Warning("Wrong idWinUIntPtr: \"%s\"", val);
		data = parsedVal;
		assert(!guiDict);
	}

	virtual void Update() {
		const char *s = GetName();
		assert(!guiDict);
	}
	virtual const char *c_str() const {
		return va("%zu", data);
	}

	// SaveGames
	virtual void WriteToSaveGame(idFile *savefile) {
		assert(false);
	}
	virtual void ReadFromSaveGame(idFile *savefile) {
		assert(false);
	}

	// no suitable conversion
	virtual float x(void) const { assert(false); return 0.0f; };

protected:
	size_t data = 0;
};



#endif /* !__WINVAR_H__ */

