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

#include "Sound.h"

//#include "Exception.h"
#include "Language.h"
//#include "Logger.h"
//#include "Options.h"


namespace OpenXcom
{

/**
 * Initializes a new sound effect.
 */
Sound::Sound()
	:
		_sound(NULL)
{}

/**
 * Deletes the loaded sound content.
 */
Sound::~Sound()
{
	Mix_FreeChunk(_sound);
}

/**
 * Loads a sound file from a specified file.
 * @param file - reference to name of the sound file
 */
void Sound::load(const std::string& file)
{
	// SDL only takes UTF-8 filenames
	// so here's an ugly hack to match this ugly reasoning
	const std::string utf8 = Language::wstrToUtf8(Language::fsToWstr(file));
	_sound = Mix_LoadWAV(utf8.c_str());

	if (_sound == NULL)
	{
		const std::string err = file + ":" + Mix_GetError();
		throw Exception(err);
	}
}

/**
 * Loads a sound file from a specified memory chunk.
 * @param data	- pointer to the sound file in memory
 * @param bytes	- size of the sound file in bytes
 */
void Sound::load(
		const void* data,
		unsigned int bytes)
{
	SDL_RWops* const rw = SDL_RWFromConstMem(data, bytes);
	_sound = Mix_LoadWAV_RW(rw, 1);

	if (_sound == NULL)
	{
		throw Exception(Mix_GetError());
	}
}

/**
 * Plays the contained sound effect.
 * @param channel	- use specified channel (default -1 any channel)
 * @param angle		- stereo (default 0)
 * @param distance	- stereo (default 0)
 */
void Sound::play(
		int channel,
		int angle,
		int distance) const
{
	if (Options::mute == false
		&& _sound != NULL)
	{
		const int chan = Mix_PlayChannel(channel, _sound, 0);
		if (chan == -1)
		{
			Log(LOG_WARNING) << Mix_GetError();
		}
		else if (Options::StereoSound == true)
		{
			if (Mix_SetPosition(
							chan,
							static_cast<Sint16>(angle),
							static_cast<Uint8>(distance)) == 0)
			{
				Log(LOG_WARNING) << Mix_GetError();
			}
		}
	}
}

/**
 * Stops all sounds playing.
 */
void Sound::stop()
{
	if (Options::mute == false)
		Mix_HaltChannel(-1);
}

/**
 * Plays the contained sound effect repeatedly on the reserved ambience channel.
 */
void Sound::loop()
{
	if (Options::mute == false
		&& _sound != NULL
		&& Mix_Playing(3) == 0)
	{
		const int chan = Mix_PlayChannel(
									3,
									_sound,
									-1);
		if (chan == -1)
			Log(LOG_WARNING) << Mix_GetError();
	}
}

/**
 * Stops the contained sound from looping.
 */
void Sound::stopLoop()
{
	if (Options::mute == false)
		Mix_HaltChannel(3);
}

}
