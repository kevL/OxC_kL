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

#include "ResourcePack.h"

//#include "../Engine/Adlib/adlplayer.h" // kL: func_fade()
//#include "../Engine/Font.h"
#include "../Engine/Game.h" // fadeMusic()
//#include "../Engine/Logger.h"
//#include "../Engine/Music.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/SoundSet.h"
#include "../Engine/SurfaceSet.h"

//#include "../Interface/Cursor.h"

#include "../Resource/XcomResourcePack.h"


namespace OpenXcom
{

int
	ResourcePack::BUTTON_PRESS				= 0,
	ResourcePack::WINDOW_POPUP[3]			= {1,2,3},

	ResourcePack::EXPLOSION_OFFSET			= 0,
	ResourcePack::UNDERWATER_SMOKE_OFFSET	= 0,
	ResourcePack::SMOKE_OFFSET				= 7,	// was 8

	ResourcePack::SMALL_EXPLOSION			= 2,
	ResourcePack::DOOR_OPEN					= 3,
	ResourcePack::LARGE_EXPLOSION			= 5,
	ResourcePack::FLYING_SOUND				= 15,
	ResourcePack::FLYING_SOUND_HQ			= 70,	// kL
	ResourcePack::ITEM_RELOAD				= 17,
	ResourcePack::ITEM_UNLOAD_HQ			= 74,	// kL
	ResourcePack::SLIDING_DOOR_OPEN			= 20,
	ResourcePack::SLIDING_DOOR_CLOSE		= 21,
	ResourcePack::WALK_OFFSET				= 22,
	ResourcePack::ITEM_DROP					= 38,
	ResourcePack::ITEM_THROW				= 39,
	ResourcePack::MALE_SCREAM[3]			= {41,42,43},
	ResourcePack::FEMALE_SCREAM[3]			= {44,45,46},

	ResourcePack::UFO_FIRE					= 9,	// was 8
	ResourcePack::UFO_HIT					= 12,
	ResourcePack::UFO_CRASH					= 11,	// was 10
	ResourcePack::UFO_EXPLODE				= 11,
	ResourcePack::INTERCEPTOR_HIT			= 10,
	ResourcePack::INTERCEPTOR_EXPLODE		= 13,

	ResourcePack::GEOSCAPE_CURSOR			= 252,
	ResourcePack::BASESCAPE_CURSOR			= 252,
	ResourcePack::BATTLESCAPE_CURSOR		= 144,
	ResourcePack::UFOPAEDIA_CURSOR			= 252,
	ResourcePack::GRAPHS_CURSOR				= 252;



/**
 * Initializes a blank resource set pointing to a folder.
 */
ResourcePack::ResourcePack()
{
	//Log(LOG_INFO) << "Create ResourcePack";
	_muteMusic = new Music();
	_muteSound = new Sound();
}

/**
 * Deletes all the loaded resources.
 */
ResourcePack::~ResourcePack()
{
	delete _muteMusic;
	delete _muteSound;

	for (std::map<std::string, Font*>::const_iterator
			i = _fonts.begin();
			i != _fonts.end();
			++i)
	{
		delete i->second;
	}

	for (std::map<std::string, Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		delete i->second;
	}

	for (std::map<std::string, SurfaceSet*>::const_iterator
			i = _sets.begin();
			i != _sets.end();
			++i)
	{
		delete i->second;
	}

	for (std::map<std::string, Palette*>::const_iterator
			i = _palettes.begin();
			i != _palettes.end();
			++i)
	{
		delete i->second;
	}

	for (std::map<std::string, Music*>::const_iterator
			i = _musics.begin();
			i != _musics.end();
			++i)
	{
		delete i->second;
	}

	for (std::map<std::string, Music*>::const_iterator // sza_MusicRules
			i = _musicFile.begin();
			i != _musicFile.end();
			++i)
	{
		delete i->second;
	}

	for (std::map<std::string, SoundSet*>::const_iterator
			i = _sounds.begin();
			i != _sounds.end();
			++i)
	{
		delete i->second;
	}
}

/**
 * Returns a specific font from the resource set.
 * @param name - reference the name of a Font
 * @return, pointer to the Font
 */
Font* ResourcePack::getFont(const std::string& name) const
{
	const std::map<std::string, Font*>::const_iterator i = _fonts.find(name);

	if (i != _fonts.end())
		return i->second;

	return NULL;
}

/**
 * Returns a specific surface from the resource set.
 * @param name - reference to name of a Surface
 * @return, pointer to the Surface
 */
Surface* ResourcePack::getSurface(const std::string& name) const
{
	const std::map<std::string, Surface*>::const_iterator i = _surfaces.find(name);
	if (i != _surfaces.end())
		return i->second;

	return NULL;
}

/**
 * Returns a specific surface set from the resource set.
 * @param name - reference name of a SurfaceSet
 * @return, pointer to the SurfaceSet
 */
SurfaceSet* ResourcePack::getSurfaceSet(const std::string& name) const
{
	const std::map<std::string, SurfaceSet*>::const_iterator i = _sets.find(name);
	if (i != _sets.end())
		return i->second;

	return NULL;
}

/**
 * Returns a specific music from the resource set.
 * kL_NOTE: This has become redundant w/ getRandomMusic() below.
 * @param track - reference the track of a Music
 * @return, pointer to the Music
 */
Music* ResourcePack::getMusic(const std::string& track) const
{
	if (Options::mute == true)
		return _muteMusic;

	return getRandomMusic(track, ""); // sza_MusicRules
}

/**
 * Checks if a particular music track is playing.
 * @param track - reference the track to check for
 * @return, true if playing
 */
bool ResourcePack::isMusicPlaying(const std::string& track) const
{
	return (_playingMusic == track);
//	return _musics[_playingMusic]->isPlaying();
}

/**
 * Plays the specified track if it's not already playing.
 * @param track			- reference the track of a Music
 * @param terrainRule	- reference the RuleTerrain type (default "")
 * @param loops			- number of times to play the track (default -1 infinite)
 */
void ResourcePack::playMusic(
		const std::string& track,
		const std::string& terrainRule, // kL: sza_MusicRules
		int loops)
{
	if (_playingMusic != track
		&& Options::mute == false)
	{
		_playingMusic = track;

		if (Options::musicAlwaysLoop == false
			&& (track == OpenXcom::res_MUSIC_WIN
				|| track == OpenXcom::res_MUSIC_LOSE))
		{
			loops = 1;
		}

		getRandomMusic(
					track,
					terrainRule)->play(loops);
	}
}

/**
 * Fades the currently playing music.
 * @param game		- pointer to the Game object
 * @param fadeDur	- duration of the fade in milliseconds
 */
void ResourcePack::fadeMusic(
		Game* const game,
		const int fadeDur)
{
	_playingMusic.clear();

#ifndef __NO_MUSIC
	if (Mix_PlayingMusic() == 0)
		return;

	if (Mix_GetMusicType(NULL) != MUS_MID)
	{
//		game->getCursor()->setVisible(false);

		game->setInputActive(false);

		Mix_FadeOutMusic(fadeDur); // fade out!
//kL	func_fade();

//		game->setFadeMusic();
		while (Mix_PlayingMusic() == 1)
		{}

//		game->getCursor()->setVisible();
	}
	else // SDL_Mixer has trouble with native midi and volume on windows, which is the most likely use case, so f@%# it.
		Mix_HaltMusic();
#endif
}

/**
 * Returns a random music from the resource set.
 * @param track		- reference the track of a Music to pick from
 * @param terrainRule	- reference the RuleTerrain type
 * @return, pointer to the Music
 */
Music* ResourcePack::getRandomMusic( // private.
		const std::string& track,
		const std::string& terrainRule) const // sza_MusicRules
{
	if (Options::mute == true)
		return _muteMusic;

	if (terrainRule.empty() == true)
		Log(LOG_DEBUG) << "MUSIC: Request " << track;
	else
		Log(LOG_DEBUG) << "MUSIC: Request " << track << " for terrainType " << terrainRule;

	if (_musicAssignment.find(track) == _musicAssignment.end())
	{
		Log(LOG_INFO) << "ResourcePack::getRandomMusic(), no music assignment: return MUTE [0]";
		return _muteMusic;
	}

	const std::map<std::string, std::vector<std::pair<std::string, int> > > assignment = _musicAssignment.at(track);
	if (assignment.find(terrainRule) == assignment.end())
	{
		Log(LOG_INFO) << "ResourcePack::getRandomMusic(), no music for terrainRule: return MUTE [1]";
		return _muteMusic;
	}

	const std::vector<std::pair<std::string, int> > codeList = assignment.at(terrainRule);
	const size_t code = static_cast<size_t>(RNG::seedless(0, codeList.size() - 1));
//	const size_t code = static_cast<size_t>(SDL_GetTicks() % codeList.size());
	const std::pair<std::string, int> music = codeList[code];

	//Log(LOG_DEBUG) << "MUSIC: " << music.first;
	Log(LOG_INFO) << "MUSIC: " << music.first;
//	Music* const ret = _musicFile.at(music.first);

	return _musicFile.at(music.first);
}

/**
 * Clear a music assignment.
 * @param track		- reference the track of a Music
 * @param terrainRule	- reference the RuleTerrain type
 */
void ResourcePack::ClearMusicAssignment( // sza_MusicRules
		const std::string& track,
		const std::string& terrainRule)
{
	if (_musicAssignment.find(track) == _musicAssignment.end()
		|| _musicAssignment.at(track).find(terrainRule) == _musicAssignment.at(track).end())
	{
		return;
	}

	_musicAssignment.at(track).at(terrainRule).clear();
}

/**
 * Make a music assignment.
 * @param track			- reference the track of a Music
 * @param terrainRule		- reference the RuleTerrain type
 * @param filenames		- reference a vector of filenames
 * @param midiIndexes	- reference a vector of indices
 */
void ResourcePack::MakeMusicAssignment( // sza_MusicRules
		const std::string& track,
		const std::string& terrainRule,
		const std::vector<std::string>& filenames,
		const std::vector<int>& midiIndexes)
{
	if (_musicAssignment.find(track) == _musicAssignment.end())
		_musicAssignment[track] = std::map<std::string, std::vector<std::pair<std::string, int> > >();

	if (_musicAssignment.at(track).find(terrainRule) == _musicAssignment.at(track).end())
		_musicAssignment[track]
						[terrainRule] = std::vector<std::pair<std::string, int> >();

	for (size_t
			i = 0;
			i != filenames.size();
			++i)
	{
		const std::pair<std::string, int> toAdd = std::make_pair<std::string, int>(
																				filenames.at(i),
																				midiIndexes.at(i));
		_musicAssignment[track]
						[terrainRule].push_back(toAdd);
	}
}

/**
 * Returns a specific sound from the resource set.
 * @param set	- reference the name of a Sound set
 * @param sound	- ID of the Sound
 * @return, pointer to the Sound
 */
Sound* ResourcePack::getSound(
		const std::string& soundSet,
		unsigned int sound) const
{
	if (Options::mute == true)
		return _muteSound;
	else
	{
		const std::map<std::string, SoundSet*>::const_iterator i = _sounds.find(soundSet);
		if (i != _sounds.end())
			return i->second->getSound(sound);
	}

	return NULL;
}

/**
 * Returns a specific sound from either the land or underwater resource set.
 * @param depth - the depth of the battlescape
 * @param sound - ID of the sound
 * @return, pointer to the sound
 */
/* Sound* ResourcePack::getSoundByDepth(
		unsigned int depth,
		unsigned int sound) const
{
	if (depth == 0)
		return getSound("BATTLE.CAT", sound);

	return getSound("BATTLE2.CAT", sound);
} */

/**
 * Plays a sound effect in stereo.
 * @param sound		- sound to play
 * @param randAngle	- true to randomize the sound angle (default false to center it)
 */
void ResourcePack::playSoundFX(
		const int sound,
		const bool randAngle) const
{
	int dir = 360; // stereo center

	if (randAngle == true)
	{
		const int var = 67; // maximum deflection left or right
//		dir += (RNG::generate(-var, var)
//			  + RNG::generate(-var, var))
		dir += (RNG::seedless(-var, var)
			  + RNG::seedless(-var, var))
			/ 2;
	}

	getSound(
			"GEO.CAT",
			sound)
		->play(-1, dir);
}

/**
 * Returns a specific palette from the resource set.
 * @param name - reference the name of a Palette
 * @return, pointer to the Palette
 */
Palette* ResourcePack::getPalette(const std::string& name) const
{
	const std::map<std::string, Palette*>::const_iterator i = _palettes.find(name);
	if (i != _palettes.end())
		return i->second;

	return NULL;
}

/**
 * Changes the palette of all the graphics in the resource set.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void ResourcePack::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	for (std::map<std::string, Font*>::const_iterator
			i = _fonts.begin();
			i != _fonts.end();
			++i)
	{
		i->second->getSurface()->setPalette(colors, firstcolor, ncolors);
	}

	for (std::map<std::string, Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		if (i->first.substr(i->first.length() - 3, i->first.length()) != "LBM")
			i->second->setPalette(colors, firstcolor, ncolors);
	}

	for (std::map<std::string, SurfaceSet*>::const_iterator
			i = _sets.begin();
			i != _sets.end();
			++i)
	{
		i->second->setPalette(colors, firstcolor, ncolors);
	}
}

/**
 * Returns the list of voxeldata in the resource set.
 * @return, pointer to a vector containing the voxeldata
 */
const std::vector<Uint16>* const ResourcePack::getVoxelData() const
{
	return &_voxelData;
}

/**
 * Gets transparency lookup tables.
 * @return, pointer to a vector of vectors of colors
 */
/* const std::vector<std::vector<Uint8> >* ResourcePack::getLUTs() const
{
	return &_transparencyLUTs;
} */

/**
 * Gets a random background.
 * @note This is mainly used in error messages.
 * @return, reference to a random background screen (.SCR file)
 */
const std::string& ResourcePack::getRandomBackground() const
{
	static const std::string bg[10] =
	{
		"BACK01.SCR",	// main
		"BACK02.SCR",	// bad boy
		"BACK03.SCR",	// terror
		"BACK04.SCR",	// shooter
		"BACK05.SCR",	// research
		"BACK12.SCR",	// craft in flight
		"BACK13.SCR",	// cash
		"BACK14.SCR",	// service craft
		"BACK16.SCR",	// craft in hangar
		"BACK17.SCR"	// manufacturing
	};

	return bg[static_cast<size_t>(RNG::seedless(0,9))];
}

}
