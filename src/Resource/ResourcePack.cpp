/*
 * Copyright 2010-2014 OpenXcom Developers.
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

//#include <SDL_mixer.h>

#include "../Engine/Adlib/adlplayer.h" // kL_fade
#include "../Engine/Font.h"
#include "../Engine/Game.h" // fadeMusic()
#include "../Engine/Logger.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Sound.h"
#include "../Engine/SoundSet.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/XcomResourcePack.h"


namespace OpenXcom
{

int
	ResourcePack::BUTTON_PRESS				= 0,
	ResourcePack::WINDOW_POPUP[3]			= {1, 2, 3},

	ResourcePack::EXPLOSION_OFFSET			= 0,

	ResourcePack::SMALL_EXPLOSION			= 2,
	ResourcePack::DOOR_OPEN					= 3,
	ResourcePack::LARGE_EXPLOSION			= 5,
	ResourcePack::FLYING_SOUND				= 15,
	ResourcePack::FLYING_SOUND_HQ			= 70, // kL
	ResourcePack::ITEM_RELOAD				= 17,
	ResourcePack::SLIDING_DOOR_OPEN			= 20,
	ResourcePack::SLIDING_DOOR_CLOSE		= 21,
	ResourcePack::WALK_OFFSET				= 22,
	ResourcePack::ITEM_DROP					= 38,
	ResourcePack::ITEM_THROW				= 39,
	ResourcePack::MALE_SCREAM[3]			= {41, 42, 43},
	ResourcePack::FEMALE_SCREAM[3]			= {44, 45, 46},

	ResourcePack::UFO_FIRE					= 9, // was 8
	ResourcePack::UFO_HIT					= 12,
	ResourcePack::UFO_CRASH					= 11, // was 10
	ResourcePack::UFO_EXPLODE				= 11,
	ResourcePack::INTERCEPTOR_HIT			= 10,
	ResourcePack::INTERCEPTOR_EXPLODE		= 13,

	ResourcePack::SMOKE_OFFSET				= 7, // was 8
	ResourcePack::UNDERWATER_SMOKE_OFFSET	= 0;


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
	else
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
	else
		return NULL;
}

/**
 * Returns a specific surface set from the resource set.
 * @param name - reference name of a SurfaceSet
 * @return, pointer to the SurfaceSet
 */
SurfaceSet* ResourcePack::getSurfaceSet(const std::string& name) const
{
	//Log(LOG_INFO) << "ResourcePack::getSurfaceSet()";
	const std::map<std::string, SurfaceSet*>::const_iterator i = _sets.find(name);

	if (i != _sets.end())
	{
		//Log(LOG_INFO) << ". _set = " << i->second;
		return i->second;
	}
	else
		return NULL;
}

/**
 * Returns a specific music from the resource set.
 * kL_NOTE: This has become redundant w/ getRandomMusic() below.
 * @param name - reference the name of a Music
 * @return, pointer to the Music
 */
Music* ResourcePack::getMusic(const std::string& name) const
{
	if (Options::mute)
		return _muteMusic;

	return getRandomMusic(name, ""); // sza_MusicRules
}

/**
 * Plays the specified track if it's not already playing.
 * @param name		- reference the name of a Music
 * @param terrain	- reference the RuleTerrain type (default "")
 */
void ResourcePack::playMusic(
		const std::string& name,
		const std::string& terrain) // kL: sza_MusicRules
{
	if (Options::mute)
		return;

	if (_playingMusic != name)
	{
		int loop = -1;
		_playingMusic = name;

		if (Options::musicAlwaysLoop == false
			&& (name == OpenXcom::res_MUSIC_WIN
				|| name == OpenXcom::res_MUSIC_LOSE))
		{
			loop = 1;
		}

		getRandomMusic(name, terrain)->play(loop);
	}
}

/**
 * Fades the currently playing music.
 * @param game - pointer to the Game object
 */
void ResourcePack::fadeMusic(
		Game* const game,
		const int fadeDur)
{
#ifndef __NO_MUSIC
	if (Mix_GetMusicType(NULL) != MUS_MID)
	{
		game->setInputActive(false);

		Mix_FadeOutMusic(fadeDur); // fade out!
		func_fade();

		while (Mix_PlayingMusic() == 1)
		{
		}
	}
	else
		Mix_HaltMusic();
#endif
}

/**
 * Returns a random music from the resource set.
 * @param name		- reference the name of a Music to pick from
 * @param terrain	- reference the RuleTerrain type
 * @return, pointer to the Music
 */
Music* ResourcePack::getRandomMusic(
		const std::string& name,
		const std::string& terrain) const // kL: sza_MusicRules
{
	if (Options::mute)
		return _muteMusic;

	if (terrain.empty() == true)
		Log(LOG_DEBUG) << "MUSIC : Request " << name;
	else
		Log(LOG_DEBUG) << "MUSIC : Request " << name << " for terrainType " << terrain;

	if (_musicAssignment.find(name) == _musicAssignment.end())
	{
		Log(LOG_INFO) << "ResourcePack::getRandomMusic(), no music assignment: return MUTE [0]";
		return _muteMusic;
	}

	const std::map<std::string, std::vector<std::pair<std::string, int> > > assignment = _musicAssignment.at(name);
	if (assignment.find(terrain) == assignment.end())
	{
		Log(LOG_INFO) << "ResourcePack::getRandomMusic(), no music for terrain: return MUTE [1]";
		return _muteMusic;
	}

	const std::vector<std::pair<std::string, int> > musicCodes = assignment.at(terrain);
	const int musicRand = SDL_GetTicks() %musicCodes.size();
	const std::pair<std::string, int> randMusic = musicCodes[musicRand];

	//Log(LOG_DEBUG) << "MUSIC : " << randMusic.first;
	Log(LOG_INFO) << "MUSIC : " << randMusic.first;

	Music* const music = _musicFile.at(randMusic.first);

	return music;
}

/**
 * Clear a music assignment.
 * @param name		- reference the name of a Music
 * @param terrain	- reference the RuleTerrain type
 */
void ResourcePack::ClearMusicAssignment( // sza_MusicRules
		const std::string& name,
		const std::string& terrain)
{
	if (_musicAssignment.find(name) == _musicAssignment.end()
		|| _musicAssignment.at(name).find(terrain) == _musicAssignment.at(name).end())
	{
		return;
	}

	_musicAssignment.at(name).at(terrain).clear();
}

/**
 * Make a music assignment.
 * @param name			- reference the name of a Music
 * @param terrain		- reference the RuleTerrain type
 * @param filenames		- reference a vector of filenames
 * @param midiIndexes	- reference a vector of indices
 */
void ResourcePack::MakeMusicAssignment( // sza_MusicRules
		const std::string& name,
		const std::string& terrain,
		const std::vector<std::string>& filenames,
		const std::vector<int>& midiIndexes)
{
	if (_musicAssignment.find(name) == _musicAssignment.end())
		_musicAssignment[name] = std::map<std::string, std::vector<std::pair<std::string, int> > >();

	if (_musicAssignment.at(name).find(terrain) == _musicAssignment.at(name).end())
		_musicAssignment[name][terrain] = std::vector<std::pair<std::string, int> >();

	for (size_t
			i = 0;
			i < filenames.size();
			++i)
	{
		const std::pair<std::string, int> toAdd = std::make_pair<std::string, int>(
																				filenames.at(i),
																				midiIndexes.at(i));
		_musicAssignment[name][terrain].push_back(toAdd);
	}
}

/**
 * Returns a specific sound from the resource set.
 * @param set	- reference the name of a Sound set
 * @param sound	- ID of the Sound
 * @return, pointer to the Sound
 */
Sound* ResourcePack::getSound(
		const std::string& set,
		unsigned int sound) const
{
	if (Options::mute)
		return _muteSound;
	else
	{
		const std::map<std::string, SoundSet*>::const_iterator i = _sounds.find(set);

		if (_sounds.end() != i)
			return i->second->getSound(sound);
		else
			return NULL;
	}
}

/**
 * Returns a specific palette from the resource set.
 * @param name - reference the name of a Palette
 * @return, pointer to the Palette
 */
Palette* ResourcePack::getPalette(const std::string& name) const
{
	const std::map<std::string, Palette*>::const_iterator i = _palettes.find(name);

	if (_palettes.end() != i)
		return i->second;
	else
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
std::vector<Uint16>* ResourcePack::getVoxelData()
{
	return &_voxelData;
}

/**
 * Returns a specific sound from either the land or underwater resource set.
 * @param depth - the depth of the battlescape
 * @param sound - ID of the sound
 * @return, pointer to the sound
 */
Sound* ResourcePack::getSoundByDepth(
		unsigned int depth,
		unsigned int sound) const
{
	if (depth == 0)
		return getSound("BATTLE.CAT", sound);
	else
		return getSound("BATTLE2.CAT", sound);
}

/**
 * Gets transparency lookup tables.
 * @return, pointer to a vector of vectors of colors
 */
const std::vector<std::vector<Uint8> >* ResourcePack::getLUTs() const
{
	return &_transparencyLUTs;
}

}
