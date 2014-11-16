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

#ifndef OPENXCOM_XCOMRESOURCEPACK_H
#define OPENXCOM_XCOMRESOURCEPACK_H

#include "ResourcePack.h"

#include "../Engine/Options.h"


namespace OpenXcom
{

class CatFile;
class ExtraSounds;
class ExtraSprites;
class GMCatFile;
class Music;
class Palette;
class Ruleset;


static const std::string // = 'type' in Music Rules file
	XCOM_RESOURCE_MUSIC_GMDEFEND	= "GMDEFEND",
	XCOM_RESOURCE_MUSIC_GMENBASE	= "GMENBASE",
	XCOM_RESOURCE_MUSIC_GMGEO		= "GMGEO",
	XCOM_RESOURCE_MUSIC_GMGEO1		= "GMGEO1",
	XCOM_RESOURCE_MUSIC_GMGRAVES	= "GMGRAVES",
	XCOM_RESOURCE_MUSIC_GMINTER		= "GMINTER",
	XCOM_RESOURCE_MUSIC_GMINTRO1	= "GMINTRO1",
	XCOM_RESOURCE_MUSIC_GMINTRO2	= "GMINTRO2",
	XCOM_RESOURCE_MUSIC_GMINTRO3	= "GMINTRO3",
	XCOM_RESOURCE_MUSIC_GMLOSE		= "GMLOSE",
	XCOM_RESOURCE_MUSIC_GMMARS		= "GMMARS",
	XCOM_RESOURCE_MUSIC_GMNEWMAR	= "GMNEWMAR",
	XCOM_RESOURCE_MUSIC_GMSTORY		= "GMSTORY",
	XCOM_RESOURCE_MUSIC_GMTACTIC	= "GMTACTIC",
	XCOM_RESOURCE_MUSIC_GMWIN		= "GMWIN",
	XCOM_RESOURCE_MUSIC_LRC_UFOPED	= "LCUFOPED";


/**
 * Resource pack for the X-Com: UFO Defense game.
 */
class XcomResourcePack
	:
		public ResourcePack
{

private:
	Ruleset* _ruleset;


	public:
		/// Creates the X-Com Resource Pack.
		XcomResourcePack(Ruleset* rules);
		/// Cleans up the X-Com Resource Pack.
		~XcomResourcePack();

		/// Loads battlescape specific resources
		void loadBattlescapeResources();

		/// Checks if an extension is a valid image file.
		bool isImageFile(std::string extension);

		/// Loads a specified music file.
		Music* loadMusic(
				MusicFormat fmt,
				const std::string& file,
				int track,
				float volume,
				CatFile* adlibcat,
				CatFile* aintrocat,
				GMCatFile* gmcat);

		/// Creates a transparency lookup table for a given palette.
		void createTransparencyLUT(Palette* pal);
};

}

#endif
