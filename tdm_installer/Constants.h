#pragma once

#define TDM_INSTALLER_VERSION "1.01"

#define TDM_INSTALLER_LOG_FILENAME "tdm_installer.log"
#define TDM_INSTALLER_CONFIG_FILENAME "tdm_installer.ini"
#define TDM_INSTALLER_LOCAL_MANIFEST "manifest.iniz"

#define TDM_INSTALLER_ZIPSYNC_DIR ".zipsync"
#define TDM_INSTALLER_MANICACHE_SUBDIR "mani"
#define TDM_INSTALLER_LASTSCAN_PATH TDM_INSTALLER_ZIPSYNC_DIR "/lastscan.ini"
#define TDM_INSTALLER_LASTINSTALL_PATH TDM_INSTALLER_ZIPSYNC_DIR "/lastinstall.ini"

#define TDM_INSTALLER_CONFIG_URL "http://darkmod.taaaki.za.net/zipsync/tdm_installer.ini"
#define TDM_INSTALLER_EXECUTABLE_URL_PREFIX "http://darkmod.taaaki.za.net/zipsync/"
#define TDM_INSTALLER_TRUSTED_URL_PREFIX "http://darkmod.taaaki.za.net/"

#define TDM_INSTALLER_FREESPACE_MINIMUM 100				//100 MB --- hardly enough even for differential update
#define TDM_INSTALLER_FREESPACE_RECOMMENDED (5<<10)		//5 GB --- enough for any update, since it is larger than size of TDM

#define TDM_DARKMOD_CFG_FILENAME "Darkmod.cfg"
#define TDM_DARKMOD_CFG_OLD_FORMAT "Darkmod_%y%m%d_%H%M%S.cfg"
#define TDM_DARKMOD_SHORTCUT_NAME "TheDarkMod"
#define TDM_DARKMOD_SHORTCUT_ICON "TDM_icon.ico"
#define TDM_DARKMOD_SHORTCUT_COMMENT "TheDarkMod: Stealth Gaming in a Gothic Steampunk World"
#ifdef _WIN32
#define TDM_DARKMOD_SHORTCUT_EXECUTABLES {"TheDarkModx64.exe", "TheDarkMod.exe"}
#else
#define TDM_DARKMOD_SHORTCUT_EXECUTABLES {"thedarkmod.x64", "thedarkmod.x86"}
#endif

#define TDM_INSTALLER_USERAGENT "tdm_installer/" TDM_INSTALLER_VERSION
