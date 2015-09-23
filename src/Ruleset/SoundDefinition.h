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

#ifndef OPENXCOM_SOUNDDEF_H
#define OPENXCOM_SOUNDDEF_H

//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class SoundDefinition
{

private:
	std::string
		_catFile,
		_type;

	std::vector<int> _soundList;


	public:
		/// cTor.
		explicit SoundDefinition(const std::string& type);
		/// dTor.
		~SoundDefinition();

		///
		void load(const YAML::Node& node);

		///
		const std::vector<int>& getSoundList() const;
		///
		std::string getCatFile() const;
};

}

#endif
