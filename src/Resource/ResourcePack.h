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

#ifndef OPENXCOM_RESOURCEPACK_H
#define OPENXCOM_RESOURCEPACK_H

#include <map>
#include <string>
#include <vector>

#include <SDL.h>


namespace OpenXcom
{

class Font;
class Music;
class Palette;
class Sound;
class SoundSet;
class Surface;
class SurfaceSet;


/**
 * Packs of external game media.
 * Resource packs contain all the game media that's loaded externally,
 * like graphics, fonts, languages, audio and world map.
 * @note The game is still hardcoded to X-Com resources,
 * so for now this just serves to keep all the file loading in one place.
 */
class ResourcePack
{

private:
	std::string _playingMusic;

	Music* _muteMusic;
	Sound* _muteSound;


	protected:
		std::map<std::string, Font*>		_fonts;
		std::map<std::string, Music*>		_musics;
		std::map<std::string, Palette*>		_palettes;
		std::map<std::string, Surface*>		_surfaces;
		std::map<std::string, SurfaceSet*>	_sets;
		std::map<std::string, SoundSet*>	_sounds;

		std::map<std::string, Music*> _musicFile; // sza_MusicRules
		std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, int> > > > _musicAssignment; // sza_MusicRules

		std::vector<Uint16> _voxelData;

		std::vector<std::vector<Uint8> > _transparencyLUTs;


		public:
			static int
				BUTTON_PRESS,
				WINDOW_POPUP[3],

				EXPLOSION_OFFSET,

				SMALL_EXPLOSION,
				DOOR_OPEN,
				LARGE_EXPLOSION,
				FLYING_SOUND,
				ITEM_RELOAD,
				SLIDING_DOOR_OPEN,
				SLIDING_DOOR_CLOSE,
				WALK_OFFSET,
				ITEM_DROP,
				ITEM_THROW,
				MALE_SCREAM[3],
				FEMALE_SCREAM[3],

				UFO_FIRE,
				UFO_HIT,
				UFO_CRASH,
				UFO_EXPLODE,
				INTERCEPTOR_HIT,
				INTERCEPTOR_EXPLODE,

				SMOKE_OFFSET,
				UNDERWATER_SMOKE_OFFSET;


			/// Create a new resource pack with a folder's contents.
			ResourcePack();
			/// Cleans up the resource pack.
			virtual ~ResourcePack();

			/// Gets a particular font.
			Font* getFont(const std::string& name) const;

			/// Gets a particular surface.
			Surface* getSurface(const std::string& name) const;
			/// Gets a particular surface set.
			SurfaceSet* getSurfaceSet(const std::string& name) const;

			/// Gets a particular music.
			Music* getMusic(const std::string& name) const;
			/// Plays a particular music.
			void playMusic(
					const std::string& name,
					bool random = false,
					const std::string& terrain = ""); // kL, sza_MusicRules
			/// Gets a random music.
//			Music* getRandomMusic(const std::string& name) const;
			Music* getRandomMusic( // sza_MusicRules
					const std::string& name,
					const std::string& terrain) const;
			/// Clear a music assignment
			void ClearMusicAssignment( // sza_MusicRules
					const std::string& name,
					const std::string& terrain);
			/// Make a music assignment
			void MakeMusicAssignment( // sza_MusicRules
					const std::string& name,
					const std::string& terrain,
					const std::vector<std::string>& filenames,
					const std::vector<int>& midiIndexes);

			/// Gets a particular sound.
			Sound* getSound(
					const std::string& set,
					unsigned int sound) const;

			/// Gets a particular palette.
			Palette* getPalette(const std::string& name) const;
			/// Sets a new palette.
			void setPalette(
					SDL_Color* colors,
					int firstcolor = 0,
					int ncolors = 256);

			/// Gets list of voxel data.
			std::vector<Uint16>* getVoxelData();

			/// Returns a specific sound from either the land or underwater resource set.
			Sound* getSoundByDepth(
					unsigned int depth,
					unsigned int sound) const;

			const std::vector<std::vector<Uint8> >* getLUTs() const;
};

}

#endif
