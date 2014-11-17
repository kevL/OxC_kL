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
static const std::string // = 'type' in Music Rules file
	SZ_MUSIC_INT1	= "INT1",
	SZ_MUSIC_INT2	= "INT2",
	SZ_MUSIC_INT3	= "INT3",
	SZ_MUSIC_START	= "START",
	SZ_MUSIC_GEO	= "GEO",
	SZ_MUSIC_TAC	= "TAC",
	SZ_MUSIC_BRIEF	= "BRIEF",
	SZ_MUSIC_DBRIEF	= "DBRIEF",
	SZ_MUSIC_MEMOR	= "MEMOR",
	SZ_MUSIC_INTER	= "INTER",
	SZ_MUSIC_WIN	= "WIN",
	SZ_MUSIC_LOSE	= "LOSE",
	SZ_MUSIC_MARS	= "MARS",
	SZ_MUSIC_PED	= "PED",
	SZ_MUSIC_DEFEND	= "DEFEND",
	SZ_MUSIC_ASAULT	= "ASAULT";


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
