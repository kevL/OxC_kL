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

#include "XcomResourcePack.h"

//#include <climits>
//#include <sstream>
//#include "../fmath.h"

//#include "../Basescape/BasescapeState.h" // kL: soundPop

#include "../Battlescape/Position.h"

//#include "../Engine/AdlibMusic.h"
//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
//#include "../Engine/Font.h"
//#include "../Engine/GMCat.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/Music.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h" // kL
//#include "../Engine/ShaderDraw.h"
//#include "../Engine/ShaderMove.h"
#include "../Engine/Sound.h"
#include "../Engine/SoundSet.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

//#include "../Geoscape/GeoscapeState.h" // kL: soundPop
//#include "../Geoscape/GraphsState.h" // kL: soundPop

#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

//#include "../Ruleset/ExtraMusic.h" // sza_ExtraMusic
#include "../Ruleset/ExtraSounds.h"
#include "../Ruleset/ExtraSprites.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleMusic.h" // sza_MusicRules
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/SoundDefinition.h"

#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

Sound* kL_soundPop = NULL;
const Uint8 ShadeMax = 15;


namespace
{

/**
 * Recolor class used in UFO.
 */
struct HairXCOM1
{
	static const Uint8
		Hair = 9 << 4,
		Face = 6 << 4;

	///
	static inline void func(
			Uint8& src,
			const Uint8& cutoff,
			int,
			int,
			int)
	{
		if (src > cutoff
			&& src <= Face + ShadeMax)
		{
			src = Hair + (src & ShadeMax) - 6; // make hair color like male in xcom_0.pck
		}
	}
};

/**
 * Recolor class used in TFTD.
 */
struct HairXCOM2
{
	static const Uint8
		ManHairColor = 4 << 4,
		WomanHairColor = 1 << 4;

	static inline void func
			(Uint8& src,
			int,
			int,
			int,
			int)
	{
		if (   src >= WomanHairColor
			&& src <= WomanHairColor + ShadeMax)
		{
			src = ManHairColor + (src & ShadeMax);
		}
	}
};

/**
 * Recolor class used in TFTD.
 */
struct FaceXCOM2
{
	static const Uint8
		FaceColor = 10 << 4,
		PinkColor = 14 << 4;

	static inline void func(
			Uint8& src,
			int,
			int,
			int,
			int)
	{
		if (   src >= FaceColor
			&& src <= FaceColor + ShadeMax)
		{
			src = PinkColor + (src & ShadeMax);
		}
	}
};

/**
 * Recolor class used in TFTD.
 */
struct BodyXCOM2
{
	static const Uint8 IonArmorColor = 8 << 4;

	static inline void func(
			Uint8& src,
			int,
			int,
			int,
			int)
	{
		if (src == 153)
			src = IonArmorColor + 12;
		else if (src == 151)
			src = IonArmorColor + 10;
		else if (src == 148)
			src = IonArmorColor + 4;
		else if (src == 147)
			src = IonArmorColor + 2;
		else if (src >= HairXCOM2::WomanHairColor
			&&   src <= HairXCOM2::WomanHairColor + ShadeMax)
		{
			src = IonArmorColor + (src & ShadeMax);
		}
	}
};

/**
 * Recolor class used in TFTD
 */
struct FallXCOM2
{
	static const Uint8
		HairFall = 8 << 4,
		RoguePixel = 151;

	static inline void func(
			Uint8& src,
			int,
			int,
			int,
			int)
	{
		if (src == RoguePixel)
			src = FaceXCOM2::PinkColor + (src & ShadeMax) + 2;
		else if (src >= BodyXCOM2::IonArmorColor
			&&   src <= BodyXCOM2::IonArmorColor + ShadeMax)
		{
			src = FaceXCOM2::PinkColor + (src & ShadeMax);
		}
	}
};

}


/**
 * Initializes the resource pack by loading all the
 * resources contained in the original game folder.
 * @param rules - pointer to the Ruleset
 */
XcomResourcePack::XcomResourcePack(Ruleset* rules)
	:
		ResourcePack(),
		_ruleset(rules)
{
	/* PALETTES */
	Log(LOG_INFO) << "Loading palettes ...";

	const char* pal[] = // Load palettes
	{
		"PAL_GEOSCAPE",
		"PAL_BASESCAPE",
		"PAL_GRAPHS",
		"PAL_UFOPAEDIA",
		"PAL_BATTLEPEDIA"
	};

	for (size_t
			i = 0;
			i < sizeof(pal) / sizeof(pal[0]);
			++i)
	{
		const std::string s = "GEODATA/PALETTES.DAT";
		_palettes[pal[i]] = new Palette();
		_palettes[pal[i]]->loadDat(
								CrossPlatform::getDataFile(s),
								256,
								Palette::palOffset(i));
	}

	{
		const std::string
			s1 = "GEODATA/BACKPALS.DAT",
			s2 = "BACKPALS.DAT";
		_palettes[s2] = new Palette();
		_palettes[s2]->loadDat(
							CrossPlatform::getDataFile(s1),
							128);
	}

	{
		// Correct Battlescape palette
		const std::string
			s1 = "GEODATA/PALETTES.DAT",
			s2 = "PAL_BATTLESCAPE";
		_palettes[s2] = new Palette();
		_palettes[s2]->loadDat(
							CrossPlatform::getDataFile(s1),
							256,
							Palette::palOffset(4));

		SDL_Color gradient[] = // Last 16 colors are a greyish gradient
		{
			{140, 152, 148, 255},
			{132, 136, 140, 255},
			{116, 124, 132, 255},
			{108, 116, 124, 255},
			{ 92, 104, 108, 255},
			{ 84,  92, 100, 255},
			{ 76,  80,  92, 255},
			{ 56,  68,  84, 255},
			{ 48,  56,  68, 255},
			{ 40,  48,  56, 255},
			{ 32,  36,  48, 255},
			{ 24,  28,  32, 255},
			{ 16,  20,  24, 255},
			{  8,  12,  16, 255},
			{  3,   4,   8, 255},
			{  3,   3,   6, 255}
		};

		for (size_t
				i = 0;
				i < sizeof(gradient) / sizeof(gradient[0]);
				++i)
		{
			SDL_Color* const color = _palettes[s2]->getColors(Palette::backPos + 16 + i);
			*color = gradient[i];
		}
	}


	/* FONTS */
	Log(LOG_INFO) << "Loading fonts ...";

	// Load fonts
	YAML::Node doc = YAML::LoadFile(CrossPlatform::getDataFile("Language/Font.dat"));
	Font::setIndex(Language::utf8ToWstr(doc["chars"].as<std::string>()));
	for (YAML::const_iterator
			i = doc["fonts"].begin();
			i != doc["fonts"].end();
			++i)
	{
		const std::string id = (*i)["id"].as<std::string>();
		Font *font = new Font();
		font->load(*i);
		_fonts[id] = font;
	}


	/* GRAPHICS */
	Log(LOG_INFO) << "Loading graphics ...";

/*kL
	{
		// Load surfaces
		std::ostringstream s;
		s << "GEODATA/" << "INTERWIN.DAT";
		_surfaces["INTERWIN.DAT"] = new Surface(160, 556);
		_surfaces["INTERWIN.DAT"]->loadScr(CrossPlatform::getDataFile(s.str()));
	} */

	// kL_begin:
//	_surfaces["INTERWIN"] = new Surface(160, 96);
	// kL_end.

	const std::string geograph = CrossPlatform::getDataFolder("GEOGRAPH/");
	std::vector<std::string> scrs = CrossPlatform::getFolderContents(geograph, "SCR");
	for (std::vector<std::string>::iterator
			i = scrs.begin();
			i != scrs.end();
			++i)
	{
		const std::string path = geograph + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					std::toupper);
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadScr(path);
	}

	std::vector<std::string> bdys = CrossPlatform::getFolderContents(geograph, "BDY");
	for (std::vector<std::string>::iterator
			i = bdys.begin();
			i != bdys.end();
			++i)
	{
		const std::string path = geograph + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					std::toupper);
//		*i = (*i).substr(0, (*i).length() - 3);
//		*i = *i + "PCK";
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadBdy(path);
	}

	// kL_begin:
/*	Surface* kL_Geo = new Surface(
								Screen::ORIGINAL_WIDTH - 64,	// 256
								Screen::ORIGINAL_HEIGHT);		// 200
	Surface* oldGeo = _surfaces["GEOBORD.SCR"];
	for (int
			x = 0;
			x < Screen::ORIGINAL_WIDTH - 64;
			++x)
	{
		for (int
				y = 0;
				y < Screen::ORIGINAL_HEIGHT;
				++y)
		{
			kL_Geo->setPixelColor(x, y, oldGeo->getPixelColor(x, y));
		}
	}
	_surfaces["LGEOBORD.SCR"] = kL_Geo; */ // kL_end.

	// bigger Geoscape background
/*	int
//		newWidth	= 320 - 64,
//		newHeight	= 200;
		newWidth	= Screen::ORIGINAL_WIDTH - 64,
		newHeight	= Screen::ORIGINAL_HEIGHT;
	double
		mult_x = static_cast<double>(Options::baseXGeoscape) / static_cast<double>(Screen::ORIGINAL_WIDTH),
		mult_y = static_cast<double>(Options::baseYGeoscape) / static_cast<double>(Screen::ORIGINAL_HEIGHT);

	int
		width_mult = static_cast<int>(static_cast<double>(newWidth) * mult_x),
		height_mult = static_cast<int>(static_cast<double>(newHeight) * mult_y);

	Surface* newGeo = new Surface(
//								newWidth * 3,
//								newHeight * 3);
								width_mult,
								height_mult);
	Surface* oldGeo = _surfaces["GEOBORD.SCR"];

	for (int
			x = 0;
			x < newWidth;
			++x)
	{
		for (int
				y = 0;
				y < newHeight;
				++y)
		{
			newGeo->setPixelColor(
							newWidth + x,
							newHeight + y,
							oldGeo->getPixelColor(x, y));
			newGeo->setPixelColor(
							newWidth - x - 1,
							newHeight + y,
							oldGeo->getPixelColor(x, y));
			newGeo->setPixelColor(
//							newWidth * 3 - x - 1,
							width_mult - x - 1,
							newHeight + y,
							oldGeo->getPixelColor(x, y));

			newGeo->setPixelColor(
							newWidth + x,
							newHeight - y - 1,
							oldGeo->getPixelColor(x, y));
			newGeo->setPixelColor(
							newWidth - x - 1,
							newHeight - y - 1,
							oldGeo->getPixelColor(x, y));
			newGeo->setPixelColor(
//							newWidth * 3 - x - 1,
							width_mult - x - 1,
							newHeight - y - 1,
							oldGeo->getPixelColor(x, y));

			newGeo->setPixelColor(
							newWidth + x,
//							newHeight * 3 - y - 1,
							height_mult - y - 1,
							oldGeo->getPixelColor(x, y));
			newGeo->setPixelColor(
							newWidth - x - 1,
//							newHeight * 3 - y - 1,
							height_mult - y - 1,
							oldGeo->getPixelColor(x, y));
			newGeo->setPixelColor(
//							newWidth * 3 - x - 1,
//							newHeight * 3 - y - 1,
							width_mult - x - 1,
							height_mult - y - 1,
							oldGeo->getPixelColor(x, y));
		}
	}
	_surfaces["ALTGEOBORD.SCR"] = newGeo; */

	std::vector<std::string> spks = CrossPlatform::getFolderContents(geograph, "SPK");
	for (std::vector<std::string>::iterator
			i = spks.begin();
			i != spks.end();
			++i)
	{
		const std::string path = geograph + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					std::toupper);
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadSpk(path);
	}

	const std::string ufointro = CrossPlatform::getDataFolder("UFOINTRO/"); // Load intro
	if (CrossPlatform::folderExists(ufointro) == true)
	{
		std::vector<std::string> lbms = CrossPlatform::getFolderContents(ufointro, "LBM");
		for (std::vector<std::string>::iterator
				i = lbms.begin();
				i != lbms.end();
				++i)
		{
			const std::string path = ufointro + *i;
			std::transform(
						i->begin(),
						i->end(),
						i->begin(),
						std::toupper);
			_surfaces[*i] = new Surface(320, 200);
			_surfaces[*i]->loadImage(path);
		}
	}

	const std::string sets[] = // Load surface sets
	{
		"BASEBITS.PCK",
		"INTICON.PCK",
		"TEXTURE.DAT"
	};

	for (size_t
			i = 0;
			i < sizeof(sets) / sizeof(sets[0]);
			++i)
	{
		std::ostringstream s;
		s << "GEOGRAPH/" << sets[i];

		const std::string ext = sets[i].substr(sets[i].find_last_of('.') + 1, sets[i].length());
		if (ext == "PCK")
		{
			const std::string tab = CrossPlatform::noExt(sets[i]) + ".TAB";
			std::ostringstream s2;
			s2 << "GEOGRAPH/" << tab;
			_sets[sets[i]] = new SurfaceSet(32, 40);
			_sets[sets[i]]->loadPck(
								CrossPlatform::getDataFile(s.str()),
								CrossPlatform::getDataFile(s2.str()));
		}
		else
		{
			_sets[sets[i]] = new SurfaceSet(32, 32);
			_sets[sets[i]]->loadDat(CrossPlatform::getDataFile(s.str()));
		}
	}

	_sets["SCANG.DAT"] = new SurfaceSet(4, 4);
	std::ostringstream scang;
	scang << "GEODATA/" << "SCANG.DAT";
	_sets["SCANG.DAT"]->loadDat(CrossPlatform::getDataFile(scang.str()));


	/* MUSICS */
	if (Options::mute == false)
	{
#ifndef __NO_MUSIC // sza_MusicRules
		Log(LOG_INFO) << "Loading music ...";

		// Load musics
		// gather the assignments first.
		const std::vector<std::pair<std::string, RuleMusic*> > musicRules = rules->getMusic();
		for (std::vector<std::pair<std::string, RuleMusic*> >::const_iterator
				i = musicRules.begin();
				i != musicRules.end();
				++i)
		{
			const std::string type = i->first;
			const RuleMusic* const musicRule = i->second;

			const std::vector<std::string> terrains = musicRule->getMusicalTerrains();
			const std::string mode = musicRule->getMode();
			if (mode == "replace")
			{
				for (std::vector<std::string>::const_iterator
						terrain = terrains.begin();
						terrain != terrains.end();
						++terrain)
				{
					ClearMusicAssignment(
										type,
										*terrain);
				}
			}

			for (std::vector<std::string>::const_iterator
					terrain = terrains.begin();
					terrain != terrains.end();
					++terrain)
			{
				MakeMusicAssignment(
								type,
								*terrain,
								musicRule->getFiles(),
								musicRule->getIndexes());
			}
		}

/*		const std::string mus[] =	// these are the filenames in /SOUND directory. THIS ARRAY IS USED ONLY FOR EXTRAMUSIC !!!!
		{							// Loads only those files that are found in the Music Rule .RUL-file as 'files'.
			"12GEO2",
			"12GEO3",
			"12MARS",
			"12TACTIC",
			"GMDEFEND",
			"GMENBASE",
			"GMGEO1",
			"GMGEO2",
			"GMINTER",
			"GMINTRO1",
			"GMINTRO2",
			"GMINTRO3",
			"GMLOSE",
			"GMMARS",
			"GMNEWMAR",
			"GMSTORY",
			"GMTACTIC",
			"GMWIN",
			"LCDEBRIEF",
			"LCDEFEND",
			"LCGEO1",
			"LCGEO2",
			"LCINTER",
			"LCINTRO1",
			"LCINTRO2",
			"LCINTRO3",
			"LCLOSE",
			"LCMARS",
			"LCNIGHT",
			"LCTACTIC",
			"LCUFOPED",
			"LCWIN",
			"PSDEFEND",
			"PSENBASE",
			"PSGEO1",
			"PSGEO2",
			"PSGEO3",
			"PSGEO4",
			"PSINTER",
			"PSMARS",
			"PSNEWMAR",
			"PSSTORY",
			"PSTACTIC",
			"PSTACTIC2"
		}; */

/*		int tracks[] = {3, 6, 0, 18, 2, 19, 20, 21, 10, 9, 8, 12, 17, 11};
		float tracks_normalize[] = {0.76f, 0.83f, 1.19f, 1.0f, 0.74f, 0.8f, 0.8f, 0.8f, 1.0f, 0.92f, 0.81f, 1.0f, 1.14f, 0.84f}; */

		// Check which music version is available
		// Check if GM.CAT data is available // sza_MusicRules
//		CatFile
//			* adlibcat = NULL,
//			* aintrocat = NULL;
//		GMCatFile* gmcat = NULL;

//		const std::string
//			musicAdlib = "SOUND/ADLIB.CAT",
//			musicIntro = "SOUND/AINTRO.CAT",
//			musicGM = "SOUND/GM.CAT";
/*		if (CrossPlatform::fileExists(CrossPlatform::getDataFile(musicAdlib)))
		{
			adlibcat = new CatFile(CrossPlatform::getDataFile(musicAdlib).c_str());

			if (CrossPlatform::fileExists(CrossPlatform::getDataFile(musicIntro)))
				aintrocat = new CatFile(CrossPlatform::getDataFile(musicIntro).c_str());
		}

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile(musicGM)))
			gmcat = new GMCatFile(CrossPlatform::getDataFile(musicGM).c_str()); */

		// We have the assignments, only need to load the required files now.
		for (std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, int> > > >::const_iterator
				i = _musicAssignment.begin();
				i != _musicAssignment.end();
				++i)
		{
//			std::string type = i->first;
			const std::map<std::string, std::vector<std::pair<std::string, int> > > assignment = i->second;
			for (std::map<std::string, std::vector<std::pair<std::string, int> > >::const_iterator
					j = assignment.begin();
					j != assignment.end();
					++j)
			{
				const std::vector<std::pair<std::string, int> > filenames = j->second;
				for (std::vector<std::pair<std::string, int> >::const_iterator
						k = filenames.begin();
						k != filenames.end();
						++k)
				{
					const std::string filename = k->first;

//					const int midiIndex = k->second;
//					LoadMusic(filename, midiIndex):

					bool loaded = false;

					// The file may have already been loaded because of an other assignment.
					if (_musicFile.find(filename) != _musicFile.end())
						loaded = true;

					if (loaded == false) // Try digital tracks.
					{
						// sza_ExtraMusic_BEGIN:

						// kL_note: This section may well be redundant w/ sza_MusicRules!!
						// Load alternative digital track if there is an override.
/*						for (size_t
								l = 0;
								l < sizeof(mus) / sizeof(mus[0]);
								++l)
						{
							bool loaded = false;

							const std::vector<std::pair<std::string, ExtraMusic*> > extraMusic = rules->getExtraMusic();
							for (std::vector<std::pair<std::string, ExtraMusic*> >::const_iterator
									m = extraMusic.begin();
									m != extraMusic.end();
									++m)
							{
								const ExtraMusic* const musicRule = m->second;
								// check if there is an entry which overrides something but does not specify the terrain
								if (musicRule->hasTerrainSpecification() == false)
								{
									const std::string overridden = musicRule->getOverridden();
									if (overridden.empty() == false
										&& overridden.compare(mus[l]) == 0)
									{
										// Found one, let's use it!
										std::ostringstream mediaFilename;
										mediaFilename << "SOUND/" << m->first;
										_musics[mus[l]] = new Music();
										_musics[mus[l]]->load(CrossPlatform::getDataFile(mediaFilename.str()));

										loaded = true;
										break;
									}
								}
							}
						} */

//						if (loaded == false) // sza_End.
//						{
						const std::string exts[] =
						{
							".ogg"
//							".flac",
//							".mp3",
//							".mod",
//							".wav" // kL_add ( also added "." and removed them below )
						};

						for (size_t
								ext = 0;
								ext < sizeof(exts) / sizeof(exts[0]);
								++ext)
						{
							std::ostringstream s;
							s << "SOUND/" << filename << exts[ext];

							if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())) == true)
							{
								Log(LOG_INFO) << "Music: load file " << filename << exts[ext];
								_musicFile[filename] = new Music();
								_musicFile[filename]->load(CrossPlatform::getDataFile(s.str()));

								loaded = true;
								break;
							}
						}
//						}
					}

/*kL: forget about adlib, aintro, gmcat, midi.
					if (loaded == false)
					{
						if (adlibcat != NULL // Try Adlib music
							&& Options::audioBitDepth == 16)
						{
							_musicFile[filename] = new AdlibMusic();

//							if (tracks[i] < adlibcat->getAmount()) // kL_note: tracks[i] -> filename .....
//							{
							_musicFile[filename]->load(
													adlibcat->load(midiIndex, true),
													adlibcat->getObjectSize(midiIndex));
							loaded = true;
//							}
//							else if (aintrocat) // separate intro music
//							{
//								int track = tracks[i] - adlibcat->getAmount();
//								_musics[mus[i]]->load(aintrocat->load(track, true), aintrocat->getObjectSize(track));

//								loaded = true;
//							}
						}
						else if (gmcat != NULL) // Try GM music
						{
							_musicFile[filename] = gmcat->loadMIDI(midiIndex);

							loaded = true;
						}
						else // Try MIDI music
						{
							std::ostringstream s;
							s << "SOUND/" << filename << ".mid";

							if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())) == true)
							{
								_musicFile[filename] = new Music();
								_musicFile[filename]->load(CrossPlatform::getDataFile(s.str()));

								loaded = true;
							}
						}
					} */

					if (loaded == false)
					{
						throw Exception(filename + " music not found");
					}
				}
			}
		}

/*kL		for (size_t // newest: 140513, replaces all below to delete's
				i = 0;
				i < sizeof(mus) / sizeof(mus[0]);
				++i)
		{
			Music *music = 0;
			for (size_t j = 0; j < sizeof(priority)/sizeof(priority[0]) && music == 0; ++j)
			{
				music = loadMusic(priority[j], mus[i], tracks[i], tracks_normalize[i], adlibcat, aintrocat, gmcat);
			}
			if (!music)
			{
				throw Exception(mus[i] + " not found");
			}
			_musics[mus[i]] = music; */


/*			bool loaded = false;

			// sza_ExtraMusic_BEGIN:
			// Load alternative digital track if there is an override
			for (std::vector<std::pair<std::string, ExtraMusic*> >::const_iterator
					j = extraMusic.begin();
					j != extraMusic.end();
					++j)
			{
				ExtraMusic* musicRule = j->second;
				// check if there is an entry which overrides something but does not specify the terrain
				if (!musicRule->hasTerrainSpecification())
				{
					std::string overridden = musicRule->getOverridden();
					if (!overridden.empty()
						&& overridden.compare(mus[i]) == 0)
					{
						// Found one, let's use it!
						std::ostringstream mediaFilename;
						mediaFilename << "SOUND/" << j->first;
						_musics[mus[i]] = new Music();
						_musics[mus[i]]->load(CrossPlatform::getDataFile(mediaFilename.str()));

						loaded = true;

						break;
					}
				}
			}

			if (!loaded) // sza_End.

			for (size_t // Try digital tracks
					j = 0;
					j < sizeof(exts) / sizeof(exts[0];
					++j)
			{
				std::ostringstream s;
				s << "SOUND/" << mus[i] << "." << exts[j];
				if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())))
				{
					_musics[mus[i]] = new Music();
					_musics[mus[i]]->load(CrossPlatform::getDataFile(s.str()));
					loaded = true;

					break;
				}
			}

			if (!loaded)
			{
				if (adlibcat // Try Adlib music
					&& Options::audioBitDepth == 16)
				{
					_musics[mus[i]] = new AdlibMusic(tracks_normalize[i]);
					if (tracks[i] < adlibcat->getAmount())
					{
						_musics[mus[i]]->load(
											adlibcat->load(tracks[i], true),
											adlibcat->getObjectSize(tracks[i]));
						loaded = true;
					}
					else if (aintrocat) // separate intro music
					{
						int track = tracks[i] - adlibcat->getAmount();
						_musics[mus[i]]->load(aintrocat->load(track, true), aintrocat->getObjectSize(track));
						loaded = true;
					}
				}
				else if (gmcat) // Try GM music
				{
					_musics[mus[i]] = gmcat->loadMIDI(tracks[i]);
					loaded = true;
				}
				else // Try MIDI music
				{
					std::ostringstream s;
					s << "SOUND/" << mus[i] << ".mid";

					if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())))
					{
						_musics[mus[i]] = new Music();
						_musics[mus[i]]->load(CrossPlatform::getDataFile(s.str()));
						loaded = true;
					}
				}
			}

			if (!loaded)
			{
				throw Exception(mus[i] + " not found");
			}
		} */

//		delete adlibcat;
//		delete aintrocat;
//		delete gmcat;

/*		std::string musOptional[] = {"GMGEO3",
									 "GMGEO4",
									 "GMGEO5",
									 "GMGEO6",
									 "GMGEO7",
									 "GMGEO8",
									 "GMGEO9",
									 "GMTACTIC1",
									 "GMTACTIC2",
									 "GMTACTIC3",
									 "GMTACTIC4",
									 "GMTACTIC5",
									 "GMTACTIC6",
									 "GMTACTIC7",
									 "GMTACTIC8",
									 "GMTACTIC9"}; */

/*kL		for (size_t // Ok, now try to load the optional musics
				i = 0;
				i < sizeof(musOptional) / sizeof(musOptional[0]);
				++i)
		{
			Music* music = 0;
			for (size_t
					j = 0;
					j < sizeof(priority) / sizeof(priority[0])
						&& music == 0;
					++j)
			{
				music = loadMusic(
								priority[j],
								musOptional[i],
								0, 0, 0, 0, 0);
			}

			if (music)
				_musics[musOptional[i]] = music;
		} */
#endif


		/* SOUNDS fx */
		Log(LOG_INFO) << "Loading sound FX ...";

		if (rules->getSoundDefinitions()->empty() == true) // Load sounds.
		{
			const std::string
				catsId[] =
				{
					"GEO.CAT",
					"BATTLE.CAT"
				},
				catsWin[] =
				{
					"SAMPLE.CAT",
					"SAMPLE2.CAT"
				},
				catsDos[] =
				{
					"SOUND2.CAT",
					"SOUND1.CAT"
				},
				* cats[] = // Try the preferred format first, otherwise use the default priority.
				{
					NULL,
					catsWin,
					catsDos
				};

			if (Options::preferredSound == SOUND_14)
				cats[0] = catsWin;
			else if (Options::preferredSound == SOUND_10)
				cats[1] = catsDos;

			Options::currentSound = SOUND_AUTO;

			for (size_t
					i = 0;
					i < sizeof(catsId) / sizeof(catsId[0]);
					++i)
			{
				SoundSet* sound = NULL;

				for (size_t
						j = 0;
						j < sizeof(cats) / sizeof(cats[0])
							&& sound == NULL;
						++j)
				{
					bool wav = true;

					if (cats[j] == 0)
						continue;
					else if (cats[j] == catsDos)
						wav = false;

					std::ostringstream s;
					s << "SOUND/" << cats[j][i];
					const std::string file = CrossPlatform::getDataFile(s.str());
					if (CrossPlatform::fileExists(file) == true)
					{
						sound = new SoundSet();
						sound->loadCat(file, wav);

						Options::currentSound = (wav)? SOUND_14: SOUND_10;
					}
				}

				if (sound == 0)
				{
					throw Exception(catsWin[i] + " not found");
				}
				else
					_sounds[catsId[i]] = sound;
			}
		}
		else
		{
			for (std::map<std::string, SoundDefinition*>::const_iterator
					i = rules->getSoundDefinitions()->begin();
					i != rules->getSoundDefinitions()->end();
					++i)
			{
				std::ostringstream s;
				s << "SOUND/" << (*i).second->getCATFile();
				const std::string file = CrossPlatform::getDataFile(s.str());

				if (CrossPlatform::fileExists(file) == true)
				{
					if (_sounds.find((*i).first) == _sounds.end())
						_sounds[(*i).first] = new SoundSet();

					for (std::vector<int>::const_iterator
							j = (*i).second->getSoundList().begin();
							j != (*i).second->getSoundList().end();
							++j)
					{
						_sounds[(*i).first]->loadCatbyIndex(file, *j);
					}
				}
				else
				{
					s << " not found";
					throw Exception(s.str());
				}
			}
		}

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile("SOUND/INTRO.CAT")) == true)
		{
			SoundSet* const soundSet = _sounds["INTRO.CAT"] = new SoundSet();
			soundSet->loadCat(CrossPlatform::getDataFile("SOUND/INTRO.CAT"), false);
		}

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile("SOUND/SAMPLE3.CAT")) == true)
		{
			SoundSet* const soundSet = _sounds["SAMPLE3.CAT"] = new SoundSet();
			soundSet->loadCat(CrossPlatform::getDataFile("SOUND/SAMPLE3.CAT"), true);
		}
	}

	// define GUI sound Fx
	TextButton::soundPress		= getSound("GEO.CAT", ResourcePack::BUTTON_PRESS);		// #0 bleep
//	Window::soundPopup[0]		= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// #1 wahahahah
	Window::soundPopup[1]		= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[1]);	// #2 swish1
	Window::soundPopup[2]		= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[2]);	// #3 swish2
//	GeoscapeState::soundPop		= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// wahahahah // kL, used for Geo->Base & Geo->Graphs
//	BasescapeState::soundPop	= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// wahahahah // kL, used for Basescape RMB.
//	GraphsState::soundPop		= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// wahahahah // kL, used for switching Graphs screens. Or just returning to Geoscape.
	kL_soundPop					= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// global.

	/* BATTLESCAPE RESOURCES */
	Log(LOG_INFO) << "Loading battlescape resources ...";
	loadBattlescapeResources(); // TODO load this at battlescape start, unload at battlescape end

	// we create extra rows on the soldier stat screens by shrinking them all down one pixel.
	// this is done after loading them, but BEFORE loading the extraSprites, in case a modder wants to replace them.
	// kL_note: Actually, let's do it *after* loading extraSprites, in case a modder wants to alter the original
	// and still have this stretching happen:

	// first, let's do the base info screen
	// erase the old lines, copying from a +2 offset to account for the dithering
/*	for (int y = 91; y < 199; y += 12)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,
												y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, y + 2));
	// drawn new lines, use the bottom row of pixels as a basis
	for (int y = 89; y < 199; y += 11)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,
												y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, 199));
	// finally, move the top of the graph up by one pixel, offset for the last iteration again due to dithering.
	for (int y = 72; y < 80; ++y)
		for (int x = 0; x < 320; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,
												y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, y + (y == 79? 2: 1))); */

	// Adjust the battlescape unit-info screen:
	// erase the old lines, no need to worry about dithering on this one
	for (int y = 39; y < 199; y += 10)
		for (int x = 0; x < 169; ++x)
			_surfaces["UNIBORD.PCK"]->setPixelColor(
												x,
												y,
												_surfaces["UNIBORD.PCK"]->getPixelColor(x, 30));
	// drawn new lines, use the bottom row of pixels as a basis
	for (int y = 190; y > 37; y -= 9)
		for (int x = 0; x < 169; ++x)
			_surfaces["UNIBORD.PCK"]->setPixelColor(
												x,
												y,
												_surfaces["UNIBORD.PCK"]->getPixelColor(x, 199));
	// move the top of the graph down by eight pixels to erase the row they don't need (we actually created ~1.8 extra rows earlier)
	for (int y = 37; y > 29; --y)
		for (int x = 0; x < 320; ++x)
		{
			_surfaces["UNIBORD.PCK"]->setPixelColor(
												x,
												y,
												_surfaces["UNIBORD.PCK"]->getPixelColor(x, y - 8));
			_surfaces["UNIBORD.PCK"]->setPixelColor(x, y - 8, 0);
		}


	/* EXTRA SPRITES */
	//Log(LOG_DEBUG) << "Loading extra resources from ruleset...";
	Log(LOG_INFO) << "Loading extra sprites ...";

	std::ostringstream s;

	const std::vector<std::pair<std::string, ExtraSprites*> > extraSprites = rules->getExtraSprites();
	for (std::vector<std::pair<std::string, ExtraSprites*> >::const_iterator
			i = extraSprites.begin();
			i != extraSprites.end();
			++i)
	{
		const std::string sheetName = i->first;
		Log(LOG_INFO) << ". sheetName = " << i->first;

		ExtraSprites* const spritePack = i->second;
		const bool subdivision = spritePack->getSubX() != 0
							  && spritePack->getSubY() != 0;

		if (spritePack->getSingleImage() == true)
		{
			if (_surfaces.find(sheetName) == _surfaces.end())
			{
				//Log(LOG_DEBUG) << "Creating new single image: " << sheetName;
				//Log(LOG_INFO) << "Creating new single image: " << sheetName;

				_surfaces[sheetName] = new Surface(
												spritePack->getWidth(),
												spritePack->getHeight());
			}
			else
			{
				//Log(LOG_DEBUG) << "Adding/Replacing single image: " << sheetName;
				//Log(LOG_INFO) << "Adding/Replacing single image: " << sheetName;

				delete _surfaces[sheetName];

				_surfaces[sheetName] = new Surface(
												spritePack->getWidth(),
												spritePack->getHeight());
			}

			s.str("");
			s << CrossPlatform::getDataFile(spritePack->getSprites()->operator[](0));
			_surfaces[sheetName]->loadImage(s.str());
		}
		else // not SingleImage
		{
			bool adding = false;

			if (_sets.find(sheetName) == _sets.end())
			{
				//Log(LOG_DEBUG) << "Creating new surface set: " << sheetName;
				//Log(LOG_INFO) << "Creating new surface set: " << sheetName;

				adding = true;

				if (subdivision == true)
					_sets[sheetName] = new SurfaceSet(
													spritePack->getSubX(),
													spritePack->getSubY());
				else
					_sets[sheetName] = new SurfaceSet(
													spritePack->getWidth(),
													spritePack->getHeight());
			}
			else
			{
				//Log(LOG_DEBUG) << "Adding/Replacing items in surface set: " << sheetName;
				//Log(LOG_INFO) << "Adding/Replacing items in surface set: " << sheetName;
			}

			//if (subdivision == true)
			//{
			//	const int frames = (spritePack->getWidth() / spritePack->getSubX()) * (spritePack->getHeight() / spritePack->getSubY());
				//Log(LOG_DEBUG) << "Subdividing into " << frames << " frames.";
				//Log(LOG_INFO) << "Subdividing into " << frames << " frames.";
			//}

			for (std::map<int, std::string>::const_iterator
					j = spritePack->getSprites()->begin();
					j != spritePack->getSprites()->end();
					++j)
			{
				s.str("");
				const int startFrame = j->first;

				const std::string fileName = j->second;
				if (fileName.substr(fileName.length() - 1, 1) == "/")
				{
					//Log(LOG_DEBUG) << "Loading surface set from folder: " << fileName << " starting at frame: " << startFrame;
					//Log(LOG_INFO) << "Loading surface set from folder: " << fileName << " starting at frame: " << startFrame;

					int offset = startFrame;

					std::ostringstream folder;
					folder << CrossPlatform::getDataFolder(fileName);
					const std::vector<std::string> contents = CrossPlatform::getFolderContents(folder.str());
					for (std::vector<std::string>::const_iterator
							k = contents.begin();
							k != contents.end();
							++k)
					{
						if (isImageFile((*k).substr((*k).length() - 4, (*k).length())) == false)
							continue;

						try
						{
							s.str("");
							s << folder.str() << CrossPlatform::getDataFile(*k);

							if (_sets[sheetName]->getFrame(offset))
							{
								//Log(LOG_DEBUG) << "Replacing frame: " << offset;
								//Log(LOG_INFO) << "Replacing frame: " << offset;

								_sets[sheetName]->getFrame(offset)->loadImage(s.str());
							}
							else
							{
								if (adding == true)
									_sets[sheetName]->addFrame(offset)->loadImage(s.str());
								else
								{
									//Log(LOG_DEBUG) << "Adding frame: " << offset + spritePack->getModIndex();
									//Log(LOG_INFO) << "Adding frame: " << offset + spritePack->getModIndex();

									_sets[sheetName]->addFrame(offset + spritePack->getModIndex())->loadImage(s.str());
								}
							}

							++offset;
						}
						catch (Exception &e)
						{
							Log(LOG_WARNING) << e.what();
						}
					}
				}
				else
				{
					if (   spritePack->getSubX() == 0
						&& spritePack->getSubY() == 0)
					{
						s << CrossPlatform::getDataFile(fileName);

						if (_sets[sheetName]->getFrame(startFrame))
						{
							//Log(LOG_DEBUG) << "Replacing frame: " << startFrame;
							//Log(LOG_INFO) << "Replacing frame: " << startFrame;

							_sets[sheetName]->getFrame(startFrame)->loadImage(s.str());
						}
						else
						{
							//Log(LOG_DEBUG) << "Adding frame: " << startFrame << ", using index: " << startFrame + spritePack->getModIndex();
							//Log(LOG_INFO) << "Adding frame: " << startFrame << ", using index: " << startFrame + spritePack->getModIndex();

							_sets[sheetName]->addFrame(startFrame + spritePack->getModIndex())->loadImage(s.str());
						}
					}
					else
					{
						Surface* const temp = new Surface(
														spritePack->getWidth(),
														spritePack->getHeight());
						s.str("");
						s << CrossPlatform::getDataFile(spritePack->getSprites()->operator[](startFrame));
						temp->loadImage(s.str());
						const int
							xDivision = spritePack->getWidth() / spritePack->getSubX(),
							yDivision = spritePack->getHeight() / spritePack->getSubY();
						int
							offset = startFrame;

						for (int
								y = 0;
								y != yDivision;
								++y)
						{
							for (int
									x = 0;
									x != xDivision;
									++x)
							{
								if (_sets[sheetName]->getFrame(offset))
								{
									//Log(LOG_DEBUG) << "Replacing frame: " << offset;
									//Log(LOG_INFO) << "Replacing frame: " << offset;

									_sets[sheetName]->getFrame(offset)->clear();

									// for some reason regular blit() doesn't work here how i want it, so i use this function instead.
									temp->blitNShade(
												_sets[sheetName]->getFrame(offset),
												0 - (x * spritePack->getSubX()),
												0 - (y * spritePack->getSubY()),
												0);
								}
								else
								{
									if (adding == true)
									{
										// for some reason regular blit() doesn't work here how i want it, so i use this function instead.
										temp->blitNShade(
													_sets[sheetName]->addFrame(offset),
													0 - (x * spritePack->getSubX()),
													0 - (y * spritePack->getSubY()),
													0);
									}
									else
									{
										//Log(LOG_DEBUG) << "Adding frame: " << offset + spritePack->getModIndex();
										//Log(LOG_INFO) << "Adding frame: " << offset + spritePack->getModIndex();

										// for some reason regular blit() doesn't work here how i want it, so i use this function instead.
										temp->blitNShade(
													_sets[sheetName]->addFrame(offset + spritePack->getModIndex()),
													0 - (x * spritePack->getSubX()),
													0 - (y * spritePack->getSubY()),
													0);
									}
								}

								++offset;
							}
						}

						delete temp;
					}
				}
			}
		}
	}

	// kL_begin: from before^ loading extraSprites
	for (int y = 91; y < 199; y += 12)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,
												y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, y + 2));
	for (int y = 89; y < 199; y += 11)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,
												y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, 199));
	for (int y = 72; y < 80; ++y)
		for (int x = 0; x < 320; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,
												y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, y + (y == 79? 2: 1)));

	Surface* const altBack07 = new Surface(320, 200);
	altBack07->copy(_surfaces["BACK07.SCR"]);
	for (int y = 172; y >= 152; --y)
		for (int x = 5; x <= 314; ++x)
			altBack07->setPixelColor(
									x,
									y + 4,
									altBack07->getPixelColor(x, y));
	for (int y = 147; y >= 134; --y)
		for (int x = 5; x <= 314; ++x)
			altBack07->setPixelColor(
									x,
									y + 9,
									altBack07->getPixelColor(x, y));
	for (int y = 132; y >= 109; --y)
		for (int x = 5; x <= 314; ++x)
			altBack07->setPixelColor(
									x,
									y + 10,
									altBack07->getPixelColor(x, y));
	_surfaces["ALTBACK07.SCR"] = altBack07; // kL_end.

	// copy constructor doesn't like doing this directly,
	// so let's make a second handobs file the old fashioned way.
	// handob2 is used for all the left handed sprites.
	_sets["HANDOB2.PCK"] = new SurfaceSet(
									_sets["HANDOB.PCK"]->getWidth(),
									_sets["HANDOB.PCK"]->getHeight());

	const std::map<int, Surface*>* handob = _sets["HANDOB.PCK"]->getFrames();
	for (std::map<int, Surface*>::const_iterator
			i = handob->begin();
			i != handob->end();
			++i)
	{
		Surface
			* const surface1 = _sets["HANDOB2.PCK"]->addFrame(i->first),
			* const surface2 = i->second;
		surface1->setPalette(surface2->getPalette());
		surface2->blit(surface1);
	}


	/* EXTRA SOUNDS */
	Log(LOG_INFO) << "Loading extra sounds ...";

	const std::vector<std::pair<std::string, ExtraSounds*> > extraSounds = rules->getExtraSounds();
	for (std::vector<std::pair<std::string, ExtraSounds*> >::const_iterator
			i = extraSounds.begin();
			i != extraSounds.end();
			++i)
	{
		const std::string setName = i->first;
		ExtraSounds* const soundPack = i->second;

		if (_sounds.find(setName) == _sounds.end())
		{
			Log(LOG_DEBUG) << "Creating new sound set: " << setName << ", this will likely have no in-game use.";
			_sounds[setName] = new SoundSet();
		}
		else
			Log(LOG_DEBUG) << "Adding/Replacing items in sound set: " << setName;

		for (std::map<int, std::string>::const_iterator
				j = soundPack->getSounds()->begin();
				j != soundPack->getSounds()->end();
				++j)
		{
			const int startSound = j->first;
			std::string fileName = j->second;
			s.str("");

			if (fileName.substr(fileName.length() - 1, 1) == "/")
			{
				Log(LOG_DEBUG) << "Loading sound set from folder: " << fileName << " starting at index: " << startSound;

				int offset = startSound;
				std::ostringstream folder;
				folder << CrossPlatform::getDataFolder(fileName);
				std::vector<std::string> contents = CrossPlatform::getFolderContents(folder.str());

				for (std::vector<std::string>::iterator
						k = contents.begin();
						k != contents.end();
						++k)
				{
					try
					{
						s.str("");
						s << folder.str() << CrossPlatform::getDataFile(*k);
						if (_sounds[setName]->getSound(offset))
							_sounds[setName]->getSound(offset)->load(s.str());
						else
							_sounds[setName]->addSound(offset + soundPack->getModIndex())->load(s.str());

						++offset;
					}
					catch (Exception &e)
					{
						Log(LOG_WARNING) << e.what();
					}
				}
			}
			else
			{
				s << CrossPlatform::getDataFile(fileName);
				if (_sounds[setName]->getSound(startSound))
				{
					Log(LOG_DEBUG) << "Replacing index: " << startSound;
					_sounds[setName]->getSound(startSound)->load(s.str());
				}
				else
				{
					Log(LOG_DEBUG) << "Adding index: " << startSound;
					_sounds[setName]->addSound(startSound + soundPack->getModIndex())->load(s.str());
				}
			}
		}
	}
}

/**
 * dTor.
 */
XcomResourcePack::~XcomResourcePack()
{}

/**
 * Loads the resources required by the Battlescape.
 */
void XcomResourcePack::loadBattlescapeResources()
{
	//Log(LOG_INFO) << "XcomResourcePack::loadBattlescapeResources()";

	// Load Battlescape ICONS
	std::ostringstream
		s,
		s2;

	s << "UFOGRAPH/" << "SPICONS.DAT";
	_sets["SPICONS.DAT"] = new SurfaceSet(32, 24);
	_sets["SPICONS.DAT"]->loadDat(CrossPlatform::getDataFile(s.str()));

	s.str("");
	s << "UFOGRAPH/" << "CURSOR.PCK";
	s2 << "UFOGRAPH/" << "CURSOR.TAB";
	_sets["CURSOR.PCK"] = new SurfaceSet(32, 40);
	_sets["CURSOR.PCK"]->loadPck(
							CrossPlatform::getDataFile(s.str()),
							CrossPlatform::getDataFile(s2.str()));

	s.str("");
	s2.str("");
	s << "UFOGRAPH/" << "SMOKE.PCK";
	s2 << "UFOGRAPH/" << "SMOKE.TAB";
	_sets["SMOKE.PCK"] = new SurfaceSet(32, 40);
	_sets["SMOKE.PCK"]->loadPck(
							CrossPlatform::getDataFile(s.str()),
							CrossPlatform::getDataFile(s2.str()));

	s.str("");
	s2.str("");
	s << "UFOGRAPH/" << "HIT.PCK";
	s2 << "UFOGRAPH/" << "HIT.TAB";
	_sets["HIT.PCK"] = new SurfaceSet(32, 40);
	_sets["HIT.PCK"]->loadPck(
							CrossPlatform::getDataFile(s.str()),
							CrossPlatform::getDataFile(s2.str()));

	s.str("");
	s2.str("");
	s << "UFOGRAPH/" << "X1.PCK";
	s2 << "UFOGRAPH/" << "X1.TAB";
	_sets["X1.PCK"] = new SurfaceSet(128, 64);
	_sets["X1.PCK"]->loadPck(
							CrossPlatform::getDataFile(s.str()),
							CrossPlatform::getDataFile(s2.str()));

	s.str("");
	_sets["MEDIBITS.DAT"] = new SurfaceSet(52, 58);
	s << "UFOGRAPH/" << "MEDIBITS.DAT";
	_sets["MEDIBITS.DAT"]->loadDat(CrossPlatform::getDataFile(s.str()));

	s.str("");
	_sets["DETBLOB.DAT"] = new SurfaceSet(16, 16);
	s << "UFOGRAPH/" << "DETBLOB.DAT";
	_sets["DETBLOB.DAT"]->loadDat(CrossPlatform::getDataFile(s.str()));


	// Load Battlescape Terrain (only blanks are loaded, others are loaded just in time)
	const std::string bsets[] =
	{
		"BLANKS.PCK"
	};

	for (size_t
			i = 0;
			i < sizeof(bsets) / sizeof(bsets[0]);
			++i)
	{
		std::ostringstream
			s,
			s2;

		s << "TERRAIN/" << bsets[i];

		const std::string tab = CrossPlatform::noExt(bsets[i]) + ".TAB";
		s2 << "TERRAIN/" << tab;

		//Log(LOG_INFO) << ". bset = " << s;
		_sets[bsets[i]] = new SurfaceSet(32, 40);
		_sets[bsets[i]]->loadPck(
							CrossPlatform::getDataFile(s.str()),
							CrossPlatform::getDataFile(s2.str()));
	}


	// Load Battlescape units
	const std::string units = CrossPlatform::getDataFolder("UNITS/");
	std::vector<std::string> usets = CrossPlatform::getFolderContents(units, "PCK");
	for (std::vector<std::string>::iterator
			i = usets.begin();
			i != usets.end();
			++i)
	{
		//Log(LOG_INFO) << "XcomResourcePack::loadBattlescapeResources() units/ " << *i;

		const std::string path = units + *i;
		const std::string tab = CrossPlatform::getDataFile("UNITS/" + CrossPlatform::noExt(*i) + ".TAB");
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					std::toupper);

		if (*i != "BIGOBS.PCK")
			_sets[*i] = new SurfaceSet(32, 40);
		else
			_sets[*i] = new SurfaceSet(32, 48);

		_sets[*i]->loadPck(path, tab);
	}

	if (!_sets["CHRYS.PCK"]->getFrame(225)) // incomplete chryssalid set: 1.0 data: stop loading.
	{
		Log(LOG_FATAL) << "Version 1.0 data detected";
		throw Exception("Invalid CHRYS.PCK, please patch your X-COM data to the latest version.");
	}

	// TFTD uses the loftemps dat from the terrain folder, but still has enemy unknown's version in the geodata folder, which is short by 2 entries.
	s.str("");
	s << "TERRAIN/" << "LOFTEMPS.DAT";
	if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())) == false)
	{
		s.str("");
		s << "GEODATA/" << "LOFTEMPS.DAT";
	}

	MapDataSet::loadLOFTEMPS(
						CrossPlatform::getDataFile(s.str()),
						&_voxelData);

	const std::string scrs[] =
	{
		"TAC00.SCR"
	};

	for (size_t
			i = 0;
			i < sizeof(scrs) / sizeof(scrs[0]);
			++i)
	{
		std::ostringstream s;
		s << "UFOGRAPH/" << scrs[i];

		_surfaces[scrs[i]] = new Surface(320, 200);
		_surfaces[scrs[i]]->loadScr(CrossPlatform::getDataFile(s.str()));
	}

	const std::string lbms[] =
	{
		"D0.LBM",
		"D1.LBM",
		"D2.LBM",
		"D3.LBM"
	};

	const std::string pals[] =
	{
		"PAL_BATTLESCAPE",
		"PAL_BATTLESCAPE_1",
		"PAL_BATTLESCAPE_2",
		"PAL_BATTLESCAPE_3"
	};

	const SDL_Color backPal[] =
	{
		{0,	 5,  4, 255},
		{0, 10, 34, 255},
		{2,  9, 24, 255},
		{2,  0, 24, 255}
	};

	for (size_t
			i = 0;
			i < sizeof(lbms) / sizeof(lbms[0]);
			++i)
	{
		std::ostringstream s;
		s << "UFOGRAPH/" << lbms[i];
		if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())) == true)
		{
			if (i == 0)
				delete _palettes["PAL_BATTLESCAPE"];

			Surface* tempSurface = new Surface(1, 1);
			tempSurface->loadImage(CrossPlatform::getDataFile(s.str()));

			_palettes[pals[i]] = new Palette();
			SDL_Color* colors = tempSurface->getPalette();
			colors[255] = backPal[i];
			_palettes[pals[i]]->setColors(colors, 256);

			createTransparencyLUT(_palettes[pals[i]]);

			delete tempSurface;
		}
	}

	const std::string spks[] =
	{
		"TAC01.SCR",
		"DETBORD.PCK",
		"DETBORD2.PCK",
		"ICONS.PCK",
		"MEDIBORD.PCK",
		"SCANBORD.PCK",
		"UNIBORD.PCK"
	};

	for (size_t
			i = 0;
			i < sizeof(spks) / sizeof(spks[0]);
			++i)
	{
		std::ostringstream s;
		s << "UFOGRAPH/" << spks[i];
		if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())) == true)
		{
			_surfaces[spks[i]] = new Surface(320, 200);
			_surfaces[spks[i]]->loadSpk(CrossPlatform::getDataFile(s.str()));
		}
	}


	const std::string ufograph = CrossPlatform::getDataFolder("UFOGRAPH/");

	std::vector<std::string> bdys = CrossPlatform::getFolderContents(ufograph, "BDY");
	for (std::vector<std::string>::iterator
			i = bdys.begin();
			i != bdys.end();
			++i)
	{
		std::string path = ufograph + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					std::toupper);
		*i = (*i).substr(0, (*i).length() - 3);
		if ((*i).substr(0, 3) == "MAN")
			*i = *i + "SPK";
		else if (*i == "TAC01.")
			*i = *i + "SCR";
		else
			*i = *i + "PCK";

		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadBdy(path);
	}

	// Load Battlescape inventory
	std::vector<std::string> invs = CrossPlatform::getFolderContents(ufograph, "SPK");
	for (std::vector<std::string>::iterator
			i = invs.begin();
			i != invs.end();
			++i)
	{
		std::string path = ufograph + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					std::toupper);

		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadSpk(path);
	}

	if (Options::battleHairBleach == true) // "fix" of color index in original soldier sprites
	{
		std::string ent ("XCOM_1.PCK"); // init. personal armor

		if (_sets.find(ent) != _sets.end())
		{
			SurfaceSet* const xcom_1 = _sets[ent];
			Surface* srf;

			for (int // chest frame
					i = 0;
					i != 8;
					++i)
			{
				srf = xcom_1->getFrame(4 * 8 + i);
				ShaderMove<Uint8> head = ShaderMove<Uint8>(srf);
				GraphSubset dim = head.getBaseDomain();

				srf->lock();
				dim.beg_y = 6;
				dim.end_y = 9;
				head.setDomain(dim);
				ShaderDraw<HairXCOM1>(head, ShaderScalar<Uint8>(HairXCOM1::Face + 5));
				dim.beg_y = 9;
				dim.end_y = 10;
				head.setDomain(dim);
				ShaderDraw<HairXCOM1>(head, ShaderScalar<Uint8>(HairXCOM1::Face + 6));
				srf->unlock();
			}

			for (int // fall frame
					i = 0;
					i < 3;
					++i)
			{
				srf = xcom_1->getFrame(264 + i);
				ShaderMove<Uint8> head = ShaderMove<Uint8>(srf);
				GraphSubset dim = head.getBaseDomain();

				dim.beg_y = 0;
				dim.end_y = 24;
				dim.beg_x = 11;
				dim.end_x = 20;
				head.setDomain(dim);
				srf->lock();
				ShaderDraw<HairXCOM1>(head, ShaderScalar<Uint8>(HairXCOM1::Face + 6));
				srf->unlock();
			}
		}

		ent = "TDXCOM_?.PCK"; // all TFTD armors
		for (int
				j = 0;
				j != 3;
				++j)
		{
			ent[7] = '0' + static_cast<char>(j);
			if (_sets.find(ent) != _sets.end())
			{
				SurfaceSet* const xcom_2 = _sets[ent];

				for (int
						i = 0;
						i != 16;
						++i)
				{
					Surface* const surf = xcom_2->getFrame(262 + i); // chest frame without helm

					surf->lock();
					if (i < 8)
					{
						ShaderMove<Uint8> head = ShaderMove<Uint8>(surf); // female chest frame
						GraphSubset dim = head.getBaseDomain();
						dim.beg_y = 6;
						dim.end_y = 18;
						head.setDomain(dim);
						ShaderDraw<HairXCOM2>(head);

						if (j == 2) // fix some pixels in ION armor that was overwrite by previous function
						{
							if		(i == 0) surf->setPixelColor(18, 14, 16);
							else if (i == 3) surf->setPixelColor(19, 12, 20);
							else if (i == 6) surf->setPixelColor(13, 14, 16);
						}
					}

					// change face to pink to prevent mixup with ION armor backpack that have same colorgroup.
					ShaderDraw<FaceXCOM2>(ShaderMove<Uint8>(surf));
					surf->unlock();
				}

				for (int
						i = 0;
						i != 2;
						++i)
				{
					Surface* const surf = xcom_2->getFrame(256 + i); // fall frame (first and second)

					surf->lock();
					ShaderMove<Uint8> head = ShaderMove<Uint8>(surf);
					GraphSubset dim = head.getBaseDomain();
					dim.beg_y = 0;

					if (j == 3)
						dim.end_y = 11 + 5 * i;
					else
						dim.end_y = 17;

					head.setDomain(dim);
					ShaderDraw<FallXCOM2>(head);
					// change face to pink, to prevent mixup with ION armor backpack that have same colorgroup.
					ShaderDraw<FaceXCOM2>(ShaderMove<Uint8>(surf));
					surf->unlock();
				}

				if (j == 2) // Palette fix for ION armor
				{
					int frames = xcom_2->getTotalFrames();
					for (int
							i = 0;
							i != frames;
							++i)
					{
						Surface* const surf = xcom_2->getFrame(i);
						surf->lock();
						ShaderDraw<BodyXCOM2>(ShaderMove<Uint8>(surf));
						surf->unlock();
					}
				}
			}
		}
	}
}

/**
 * Determines if an image file is an acceptable format for the game.
 * @param extension - image file extension
 * @return, true if extension is considered valid
 */
bool XcomResourcePack::isImageFile(std::string extension)
{
	std::transform(
				extension.begin(),
				extension.end(),
				extension.begin(),
				std::toupper);

	return extension == ".BMP"
		|| extension == ".LBM"
		|| extension == ".IFF"
		|| extension == ".PCX"
		|| extension == ".GIF"
		|| extension == ".PNG"
		|| extension == ".TGA"
		|| extension == ".TIF";
//		|| extension == "TIFF"; // kL_note: why not .TIFF (prob because only the last 4 chars are passed in)

			/* // arbitrary limitation: let's not use these ones (although they're officially supported by sdl)
			extension == ".ICO" ||
			extension == ".CUR" ||
			extension == ".PNM" ||
			extension == ".PPM" ||
			extension == ".PGM" ||
			extension == ".PBM" ||
			extension == ".XPM" ||
			extension == "ILBM" ||
			// excluding jpeg to avoid inevitable issues due to compression
			extension == ".JPG" ||
			extension == "JPEG" || */
}

/**
 * Loads the specified music file format.
 * @param fmt		- format of the music
 * @param file		- reference the filename of the music
 * @param track		- track number of the music, if stored in a CAT
 * @param volume	- volume modifier of the music, if stored in a CAT
 * @param adlibcat	- pointer to ADLIB.CAT if available
 * @param aintrocat	- pointer to AINTRO.CAT if available
 * @param gmcat		- pointer to GM.CAT if available
 * @return, pointer to the music file, or NULL if it couldn't be loaded
 */
/*
Music* XcomResourcePack::loadMusic(
		MusicFormat fmt,
		const std::string& file,
		int track,
		float volume,
		CatFile* adlibcat,
		CatFile* aintrocat,
		GMCatFile* gmcat)
{
	// MUSIC_AUTO, MUSIC_FLAC, MUSIC_OGG, MUSIC_MP3, MUSIC_MOD, MUSIC_WAV, MUSIC_ADLIB, MUSIC_MIDI
	static const std::string exts[] =
	{
		"",
		"flac",
		"ogg",
		"mp3",
		"mod",
		"wav",
		"",
		"mid"
	};

	Music* music = NULL;

	try
	{
		if (fmt == MUSIC_ADLIB) // Try Adlib music
		{
			if (adlibcat
				&& Options::audioBitDepth == 16)
			{
				music = new AdlibMusic(volume);

				if (track < adlibcat->getAmount())
					music->load(
							adlibcat->load(track, true),
							adlibcat->getObjectSize(track));
				else if (aintrocat) // separate intro music
				{
					track -= adlibcat->getAmount();
					if (track < aintrocat->getAmount())
						music->load(
								aintrocat->load(track, true),
								aintrocat->getObjectSize(track));
					else
					{
						delete music;
						music = NULL;
					}
				}
			}
		}
		else if (fmt == MUSIC_MIDI) // Try MIDI music
		{
			if (gmcat // DOS MIDI
				&& track < gmcat->getAmount())
			{
				music = gmcat->loadMIDI(track);
			}
			else // Windows MIDI
			{
				std::ostringstream s;
				s << "SOUND/" << file << "." << exts[fmt];
				if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())))
				{
					music = new Music();
					music->load(CrossPlatform::getDataFile(s.str()));
				}
			}
		}
		else // Try digital tracks
		{
			std::ostringstream s;
			s << "SOUND/" << file << "." << exts[fmt];
			if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())))
			{
				music = new Music();
				music->load(CrossPlatform::getDataFile(s.str()));
			}
		}
	}
	catch (Exception &e)
	{
		Log(LOG_INFO) << e.what();
		if (music)
			delete music;

		music = NULL;
	}

	return music;
} */

/**
 * Preamble:
 * This is the most horrible function i've ever written, and it makes me sad.
 * This is, however, a necessary evil, in order to save massive amounts of time in the draw function.
 * When used with the default TFTD ruleset, this function loops 4,194,304 times
 * (4 palettes, 4 tints, 4 levels of opacity, 256 colors, 256 comparisons per);
 * each additional tint in the rulesets will result in over a million iterations more.
 *
 * kL_note: hmm, sounds like I'm going to take this out ...
 * but, since it runs only at start ....
 *
 * @param pal - the palette to base the lookup table on
 */
void XcomResourcePack::createTransparencyLUT(Palette* pal)
{
	SDL_Color target;
	std::vector<Uint8> lookUpTable;

	// start with the color sets
	for (std::vector<SDL_Color>::const_iterator
			tint = _ruleset->getTransparencies()->begin();
			tint != _ruleset->getTransparencies()->end();
			++tint)
	{
		// then the opacity levels, using the alpha channel as the step
		for (int
				opacity = 1;
				opacity < 1 + tint->unused * 4;
				opacity += tint->unused)
		{
			// then the palette itself
			for (int
					color = 0;
					color < 256;
					++color)
			{
				// add the RGB values from the ruleset to those of the colors contained
				// in the palette in order to determine the desired color:
				// all this casting and clamping is required, we're dealing with Uint8s here,
				// and there's a lot of potential for values to wrap around.
				target.r = static_cast<Uint8>(std::min(
								255,
								static_cast<int>(pal->getColors(color)->r) + (static_cast<int>(tint->r) * opacity)));
				target.g = static_cast<Uint8>(std::min(
								255,
								static_cast<int>(pal->getColors(color)->g) + (static_cast<int>(tint->g) * opacity)));
				target.b = static_cast<Uint8>(std::min(
								255,
								static_cast<int>(pal->getColors(color)->b) + (static_cast<int>(tint->b) * opacity)));

				Uint8 closest = 0;
				int
					low = std::numeric_limits<int>::max(),
					cur;

				// compare each color in the palette to find the closest match to the desired one
				for (int
						comparoperator = 0;
						comparoperator != 256;
						++comparoperator)
				{
					cur = Sqr(target.r - pal->getColors(comparoperator)->r)
						+ Sqr(target.g - pal->getColors(comparoperator)->g)
						+ Sqr(target.b - pal->getColors(comparoperator)->b);

					if (cur < low)
					{
						closest = static_cast<Uint8>(comparoperator);
						low = cur;
					}
				}

				lookUpTable.push_back(closest);
			}
		}
	}

	_transparencyLUTs.push_back(lookUpTable);
}

}
