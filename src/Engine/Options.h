/*
 * Copyright 2010-2015 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_OPTIONS_H
#define OPENXCOM_OPTIONS_H

#include <string>
#include <vector>

#include <SDL.h>

#include "OptionInfo.h"


namespace OpenXcom
{

/**
 * Battlescape drag scrolling types.
 */
enum ScrollType
{
	SCROLL_NONE,	// 0
	SCROLL_TRIGGER,	// 1
	SCROLL_AUTO		// 2
};

/**
 * Keyboard input modes.
 */
enum KeyboardType
{
	KEYBOARD_OFF,		// 0
	KEYBOARD_ON,		// 1
	KEYBOARD_VIRTUAL	// 2
};

/**
 * Savegame sorting modes.
 */
enum SaveSort
{
	SORT_NAME_ASC,	// 0
	SORT_NAME_DESC,	// 1
	SORT_DATE_ASC,	// 2
	SORT_DATE_DESC	// 3
};

/**
 * Music format preferences.
 */
enum MusicFormat
{
	MUSIC_AUTO,		// 0
	MUSIC_FLAC,		// 1
	MUSIC_OGG,		// 2
	MUSIC_MP3,		// 3
	MUSIC_MOD,		// 4
	MUSIC_WAV,		// 5
	MUSIC_ADLIB,	// 6
	MUSIC_MIDI		// 7
};

/**
 * Sound format preferences.
 */
enum SoundFormat
{
	SOUND_AUTO,	// 0
	SOUND_14,	// 1
	SOUND_10	// 2
};

/**
 * Path preview modes (can be OR'd together).
 */
enum PathPreview
{
	PATH_NONE		= 0x00,	// 0000 (must always be zero)
	PATH_ARROWS		= 0x01,	// 0001
	PATH_TU_COST	= 0x02,	// 0010
	PATH_FULL		= 0x03	// 0011 (must always be all values combined)
};

/**
 * Screen scaling modes.
 */
enum ScaleType
{
	SCALE_ORIGINAL,		// 0
	SCALE_15X,			// 1
	SCALE_2X,			// 2
	SCALE_SCREEN_DIV_3,	// 3
	SCALE_SCREEN_DIV_2,	// 4
	SCALE_SCREEN		// 5
};


/**
 * Container for all the various global game options and customizable settings.
 */
namespace Options
{

#define OPT extern
	#include "Options.inc.h"
#undef OPT

	/// Creates the options info.
	void create();
	/// Restores default options.
	void resetDefault();
	/// Initializes the options settings.
	bool init(
			int argc,
			char* argv[]);
	/// Loads options from YAML.
	void load(const std::string& filename = "options");
	/// Saves options to YAML.
	void save(const std::string& filename = "options");
	/// Gets the game's data folder.
	std::string getDataFolder();
	/// Sets the game's data folder.
	void setDataFolder(const std::string& folder);
	/// Gets the game's data list.
	const std::vector<std::string>& getDataList();
	/// Gets the game's user folder.
	std::string getUserFolder();
	/// Gets the game's config folder.
	std::string getConfigFolder();
	/// Gets the game's options.
	const std::vector<OptionInfo>& getOptionInfo();
	/// Sets the game's data, user and config folders.
	void setFolders();
	/// Update game options from config file and command line.
	void updateOptions();
	/// Backup display options.
	void backupDisplay();
	/// Switches display options.
	void switchDisplay();

}

}

#endif
