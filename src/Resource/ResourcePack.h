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

#ifndef OPENXCOM_RESOURCEPACK_H
#define OPENXCOM_RESOURCEPACK_H

//#include <map>
//#include <string>
//#include <vector>
//#include <SDL.h>


namespace OpenXcom
{

class Font;
class Game;
class Music;
class Palette;
class Sound;
class SoundSet;
class Surface;
class SurfaceSet;


/**
 * Packs of external game media.
 * @note Resource packs contain all the game media that's loaded externally,
 * like graphics, fonts, languages, audio and world map.
 * @note The game is still hardcoded to X-Com resources,
 * so for now this just serves to keep all the file loading in one place.
 */
class ResourcePack // no copy cTor.
{

private:
	std::string _playingMusic;

	Music* _muteMusic;
	Sound* _muteSound;

	/// Gets a random music. This is private to prevent access. Use playMusic() instead.
	Music* getRandomMusic( // sza_MusicRules
			const std::string& trackType,
			const std::string& terrainType) const;


	protected:
		std::map<std::string, Font*> _fonts;
		std::map<std::string, Music*> _musics;
		std::map<std::string, Palette*> _palettes;
		std::map<std::string, Surface*> _surfaces;
		std::map<std::string, SurfaceSet*> _sets;
		std::map<std::string, SoundSet*> _sounds;

		std::map<std::string, Music*> _musicFile; // sza_MusicRules
		std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, int> > > > _musicAssignment; // sza_MusicRules

		std::vector<Uint16> _voxelData;

		std::vector<std::vector<Uint8> > _transparencyLUTs;


		public:
			static int
				BUTTON_PRESS,
				WINDOW_POPUP[3],

				EXPLOSION_OFFSET,
				UNDERWATER_SMOKE_OFFSET,
				SMOKE_OFFSET,

				SMALL_EXPLOSION,
				DOOR_OPEN,
				LARGE_EXPLOSION,
				FLYING_SOUND,
				FLYING_SOUND_HQ,	// kL
				ITEM_RELOAD,
				ITEM_UNLOAD_HQ,		// kL
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

				GEOSCAPE_CURSOR,
				BASESCAPE_CURSOR,
				BATTLESCAPE_CURSOR,
				UFOPAEDIA_CURSOR,
				GRAPHS_CURSOR;


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
			Music* getMusic(const std::string& trackType) const;
			/// Checks if a particular music track is playing.
			bool isMusicPlaying(const std::string& trackType) const;
			/// Plays a particular music.
			void playMusic(
					const std::string& trackType,
					const std::string& terrainType = "", // kL, sza_MusicRules
					int loops = -1);
			/// Fades the currently playing music.
			void fadeMusic(
					Game* const game,
					const int fadeDur);
			/// Clear a music assignment
			void ClearMusicAssignment( // sza_MusicRules
					const std::string& trackType,
					const std::string& terrainType);
			/// Make a music assignment
			void MakeMusicAssignment( // sza_MusicRules
					const std::string& trackType,
					const std::string& terrainType,
					const std::vector<std::string>& files,
					const std::vector<int>& midiIdc);

			/// Gets a particular sound.
			Sound* getSound(
					const std::string& soundSet,
					unsigned int sound) const;
			/// Returns a specific sound from either the land or underwater resource set.
//			Sound* getSoundByDepth(
//					unsigned int depth,
//					unsigned int sound) const;
			/// Plays a sound effect in stereo.
			void playSoundFX(
					const int sound,
					const bool randAngle = false) const;

			/// Gets a particular palette.
			Palette* getPalette(const std::string& name) const;
			/// Sets a new palette.
			void setPalette(
					SDL_Color* colors,
					int firstcolor = 0,
					int ncolors = 256);

			/// Gets list of voxel data.
			const std::vector<Uint16>* const getVoxelData() const;

			///
//			const std::vector<std::vector<Uint8> >* getLUTs() const;

			/// Gets a random background.
			const std::string& getRandomBackground() const;
};

}

#endif
