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

#ifndef OPENXCOM_XCOMRESOURCEPACK_H
#define OPENXCOM_XCOMRESOURCEPACK_H

#include "ResourcePack.h"

//#include "../Engine/Options.h"


namespace OpenXcom
{

class CatFile;
class ExtraSounds;
class ExtraSprites;
class GMCatFile;
class Music;
class Palette;
class Ruleset;


extern Sound* kL_soundPop;


static const std::string // = 'type' in Music Rules file
	res_MUSIC_START_INTRO1				= "START_INTRO1",
	res_MUSIC_START_INTRO2				= "START_INTRO2",
	res_MUSIC_START_INTRO3				= "START_INTRO3",
	res_MUSIC_START_MAINMENU			= "START_MAINMENU",

	res_MUSIC_GEO_GLOBE					= "GEO_GLOBE",
	res_MUSIC_GEO_INTERCEPT				= "GEO_INTERCEPT", // verySmall,small,medium,large,veryLarge
	res_MUSIC_GEO_MONTHLYREPORT			= "GEO_MONTHLYREPORT",
	res_MUSIC_GEO_MONTHLYREPORT_BAD		= "GEO_MONTHLYREPORT_BAD",

	res_MUSIC_GEO_BRIEFING				= "GEO_BRIEFING",
	res_MUSIC_GEO_BRIEF_MARS1			= "GEO_BRIEF_MARS1",
	res_MUSIC_GEO_BRIEF_MARS2			= "GEO_BRIEF_MARS2",
	res_MUSIC_GEO_BRIEF_BASEDEFENSE		= "GEO_BRIEF_BASEDEFENSE",
	res_MUSIC_GEO_BRIEF_BASEASSAULT		= "GEO_BRIEF_BASEASSAULT",
	res_MUSIC_GEO_BRIEF_TERRORSITE		= "GEO_BRIEF_TERRORSITE",
	res_MUSIC_GEO_BRIEF_UFOCRASHED		= "GEO_BRIEF_UFOCRASHED",
	res_MUSIC_GEO_BRIEF_UFOLANDED		= "GEO_BRIEF_UFOLANDED",

	res_MUSIC_TAC_BATTLE				= "TAC_BATTLE",
	res_MUSIC_TAC_BATTLE_ALIENTURN		= "TAC_BATTLE_ALIENTURN",
	res_MUSIC_TAC_BATTLE_MARS1			= "TAC_BATTLE_MARS1",
	res_MUSIC_TAC_BATTLE_MARS2			= "TAC_BATTLE_MARS2",
	res_MUSIC_TAC_BATTLE_BASEDEFENSE	= "TAC_BATTLE_BASEDEFENSE",
	res_MUSIC_TAC_BATTLE_BASEASSAULT	= "TAC_BATTLE_BASEASSAULT",
	res_MUSIC_TAC_BATTLE_TERRORSITE		= "TAC_BATTLE_TERRORSITE",
	res_MUSIC_TAC_BATTLE_UFOCRASHED		= "TAC_BATTLE_UFOCRASHED",
	res_MUSIC_TAC_BATTLE_UFOLANDED		= "TAC_BATTLE_UFOLANDED",

	res_MUSIC_TAC_DEBRIEFING			= "TAC_DEBRIEFING",
	res_MUSIC_TAC_DEBRIEFING_BAD		= "TAC_DEBRIEFING_BAD",
	res_MUSIC_TAC_AWARDS				= "TAC_AWARDS",

	res_MUSIC_BASE_AWARDS				= "BASE_AWARDS",
	res_MUSIC_BASE_MEMORIAL				= "BASE_MEMORIAL",
	res_MUSIC_UFOPAEDIA					= "UFOPAEDIA",

	res_MUSIC_WIN						= "WIN",
	res_MUSIC_LOSE						= "LOSE";


/**
 * Resource pack for the X-Com: UFO Defense game.
 */
class XcomResourcePack
	:
		public ResourcePack
{

private:
	const Ruleset* _ruleset;


	public:
		/// Creates the X-Com Resource Pack.
		explicit XcomResourcePack(const Ruleset* const rules);
		/// Cleans up the X-Com Resource Pack.
		~XcomResourcePack();

		/// Loads battlescape specific resources
		void loadBattlescapeResources();

		/// Checks if an extension is a valid image file.
		bool isImageFile(std::string extension);

		/// Loads a specified music file.
/*		Music* loadMusic(
				MusicFormat fmt,
				const std::string& file,
				int track,
				float volume,
				CatFile* adlibcat,
				CatFile* aintrocat,
				GMCatFile* gmcat); */
};

}

#endif
