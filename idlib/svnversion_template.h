#pragma once
/*
 * The header file "svnversion.h" contains revision of the current SVN working copy.
 * Since it is an automatically generated file, it should NOT be committed to SVN.
 *
 * The file "svnversion_template.h" is a template from which "svnversion.h" is generated.
 * This file should be under version control.
 *
 * Under Windows, batch file "gen_svnversion.cmd" is used to generate "svnversion.h" from template.
 * It is run automatically as a pre-build event when MSVC solution is built.
 * Under Linux, scons build script generates "svnversion.h" from template using simple string replacement.
 */
#define SVN_WORKING_COPY_VERSION "!SVNVERSION!"
