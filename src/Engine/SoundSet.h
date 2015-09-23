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

#ifndef OPENXCOM_SOUNDSET_H
#define OPENXCOM_SOUNDSET_H

//#include <map>
//#include <string>


namespace OpenXcom
{

class Sound;

/**
 * Container of a set of sounds.
 * @note Used to manage file sets that contain a pack of sounds inside.
 */
class SoundSet
{

private:
	std::map<int, Sound*> _sounds;

	/// Gets the total sounds in the set.
//	size_t getTotalSounds() const;


	public:
		/// Creates a sound set.
		SoundSet();
		/// Cleans up the sound set.
		~SoundSet();

		/// Loads an X-Com CAT set of sound files.
		void loadCat(
				const std::string& file,
				bool wav = true);

		/// Gets a particular sound from the set.
		Sound* getSound(size_t id);
		/// Creates a new sound and returns a pointer to it.
		Sound* addSound(size_t id);

		/// Loads a specific entry from a CAT file into the soundset.
//		void loadCatByIndex(const std::string& file, int index);
};

}

#endif
