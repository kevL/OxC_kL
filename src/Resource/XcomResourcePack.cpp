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

//#include "../Engine/AdlibMusic.h"
//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
//#include "../Engine/Font.h"
//#include "../Engine/GMCat.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Music.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h" // kL
//#include "../Engine/ShaderDraw.h"
//#include "../Engine/ShaderMove.h"
#include "../Engine/Sound.h"
#include "../Engine/SoundSet.h"
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
 * Recolor class used in TFTD.
 */
struct FallXCOM2
{
	static const Uint8 RoguePixel = 151;

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
 * Initializes the resource pack by loading all the resources contained in the
 * original game folder.
 * @param rules - pointer to the Ruleset
 */
XcomResourcePack::XcomResourcePack(const Ruleset* const rules)
	:
		ResourcePack()
{
	/* PALETTES */
	Log(LOG_INFO) << "Loading palettes ...";

	std::string st = "BACKPALS.DAT";

	_palettes[st] = new Palette();
	_palettes[st]->loadDat(
						CrossPlatform::getDataFile("GEODATA/" + st),
						128);

	const char* const pal[] = // Load palettes
	{
		"PAL_GEOSCAPE",		// geoscape
		"PAL_BASESCAPE",	// basescape
		"PAL_GRAPHS",		// graphs
		"PAL_UFOPAEDIA",	// research
		"PAL_BATTLEPEDIA"	// tactical
	};

	for (size_t
			i = 0;
			i != sizeof(pal) / sizeof(pal[0]);
			++i)
	{
		_palettes[pal[i]] = new Palette();
		_palettes[pal[i]]->loadDat(
								CrossPlatform::getDataFile("GEODATA/PALETTES.DAT"),
								256,
								Palette::palOffset(i));
	}

	st = "PAL_BATTLESCAPE"; // correct the Tactical palette
	_palettes[st] = new Palette();
	_palettes[st]->loadDat(
						CrossPlatform::getDataFile("GEODATA/PALETTES.DAT"),
						256,
						Palette::palOffset(4));

	SDL_Color gradient[] = // last 16 colors are a steel-grayish gradient
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
			i != sizeof(gradient) / sizeof(gradient[0]);
			++i)
	{
		SDL_Color* const colors = _palettes[st]->getColors(Palette::PAL_bgID + 16 + i);
		*colors = gradient[i];
	}


	/* FONT */
	st = rules->getFontName(); // Font.dat
	YAML::Node doc = YAML::LoadFile(CrossPlatform::getDataFile("Language/" + st));
	Log(LOG_INFO) << "Loading font: " << st;

	Font::setIndex(Language::utf8ToWstr(doc["chars"].as<std::string>()));
	for (YAML::const_iterator
			i = doc["fonts"].begin();
			i != doc["fonts"].end();
			++i)
	{
		st = (*i)["id"].as<std::string>();
		Font* const font = new Font();
		font->load(*i);
		_fonts[st] = font;
	}


	/* GRAPHICS */
	Log(LOG_INFO) << "Loading graphics ...";

	// Load surfaces
/*	std::ostringstream oststr;
	oststr << "GEODATA/" << "INTERWIN.DAT";
	_surfaces["INTERWIN.DAT"] = new Surface(160, 556);
	_surfaces["INTERWIN.DAT"]->loadScr(CrossPlatform::getDataFile(oststr.str())); */

	std::string folder = CrossPlatform::getDataFolder("GEOGRAPH/");
	std::vector<std::string> files = CrossPlatform::getFolderContents(folder, "SCR");
	for (std::vector<std::string>::iterator
			i = files.begin();
			i != files.end();
			++i)
	{
		st = folder + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					std::toupper);
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadScr(st);
	}

	files = CrossPlatform::getFolderContents(folder, "BDY");
	for (std::vector<std::string>::iterator
			i = files.begin();
			i != files.end();
			++i)
	{
		st = folder + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					std::toupper);
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadBdy(st);
	}

	files = CrossPlatform::getFolderContents(folder, "SPK");
	for (std::vector<std::string>::iterator
			i = files.begin();
			i != files.end();
			++i)
	{
		st = folder + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					std::toupper);
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadSpk(st);
	}

	folder = CrossPlatform::getDataFolder("UFOINTRO/"); // Load intro
	if (CrossPlatform::folderExists(folder) == true)
	{
		files = CrossPlatform::getFolderContents(folder, "LBM");
		for (std::vector<std::string>::iterator
				i = files.begin();
				i != files.end();
				++i)
		{
			st = folder + *i;
			std::transform(
						i->begin(),
						i->end(),
						i->begin(),
						std::toupper);
			_surfaces[*i] = new Surface(320, 200);
			_surfaces[*i]->loadImage(st);
		}
	}

	const std::string sets[] = // Load surface sets
	{
		"BASEBITS.PCK",
		"INTICON.PCK",
		"TEXTURE.DAT"
	};

	std::ostringstream
		oststr,
		oststr2;

	for (size_t
			i = 0;
			i != sizeof(sets) / sizeof(sets[0]);
			++i)
	{
		oststr.str("");
		oststr << "GEOGRAPH/" << sets[i];

		if (sets[i].substr(
						sets[i].find_last_of('.') + 1,
						sets[i].length()) == "PCK")
		{
			st = CrossPlatform::noExt(sets[i]) + ".TAB";
			oststr2.str("");
			oststr2 << "GEOGRAPH/" << st;
			_sets[sets[i]] = new SurfaceSet(32,40);
			_sets[sets[i]]->loadPck(
								CrossPlatform::getDataFile(oststr.str()),
								CrossPlatform::getDataFile(oststr2.str()));
		}
		else
		{
			_sets[sets[i]] = new SurfaceSet(32,32);
			_sets[sets[i]]->loadDat(CrossPlatform::getDataFile(oststr.str()));
		}
	}

	_sets["SCANG.DAT"] = new SurfaceSet(4,4);
	oststr.str("");
	oststr << "GEODATA/" << "SCANG.DAT";
	_sets["SCANG.DAT"]->loadDat(CrossPlatform::getDataFile(oststr.str()));


	/* MUSICS */
	if (Options::mute == false)
	{
#ifndef __NO_MUSIC // sza_MusicRules
		Log(LOG_INFO) << "Loading music ...";
		// Load musics!

		// gather the assignments first.
		const RuleMusic* musicRule;
		std::vector<std::string> terrains;
		std::string
			type,
			mode;

		const std::vector<std::pair<std::string, RuleMusic*> > musicRules = rules->getMusic();
		for (std::vector<std::pair<std::string, RuleMusic*> >::const_iterator
				i = musicRules.begin();
				i != musicRules.end();
				++i)
		{
			type = i->first;
			musicRule = i->second;

			terrains = musicRule->getMusicalTerrains();
			mode = musicRule->getMode();
			if (mode == "replace")
			{
				for (std::vector<std::string>::const_iterator
						j = terrains.begin();
						j != terrains.end();
						++j)
				{
					ClearMusicAssignment(type, *j);
				}
			}

			for (std::vector<std::string>::const_iterator
					j = terrains.begin();
					j != terrains.end();
					++j)
			{
				MakeMusicAssignment(
								type,
								*j,
								musicRule->getFiles(),
								musicRule->getIndexes());
			}
		}

		// have the assignments, load the required files.
		const std::string exts[] =
		{
			".ogg" //,".flac",".mp3",".mod",".wav"
		};

		std::map<std::string, std::vector<std::pair<std::string, int> > > assignment;
		std::vector<std::pair<std::string, int> > files;

		bool loaded;

		for (std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, int> > > >::const_iterator
				i = _musicAssignment.begin();
				i != _musicAssignment.end();
				++i)
		{
			assignment = i->second;
			for (std::map<std::string, std::vector<std::pair<std::string, int> > >::const_iterator
					j = assignment.begin();
					j != assignment.end();
					++j)
			{
				files = j->second;
				for (std::vector<std::pair<std::string, int> >::const_iterator
						k = files.begin();
						k != files.end();
						++k)
				{
					loaded = false;
					st = k->first;

					// The file may have already been loaded because of another assignment.
					if (_musicFile.find(st) != _musicFile.end())
						loaded = true;

					if (loaded == false) // Try digital tracks.
					{
						for (size_t
								l = 0;
								l != sizeof(exts) / sizeof(exts[0]);
								++l)
						{
							oststr.str("");
							oststr << "SOUND/" << st << exts[l];

							if (CrossPlatform::fileExists(CrossPlatform::getDataFile(oststr.str())) == true)
							{
								Log(LOG_INFO) << "Music: load file " << st << exts[l];
								_musicFile[st] = new Music();
								_musicFile[st]->load(CrossPlatform::getDataFile(oststr.str()));

								loaded = true;
								break;
							}
						}
					}

					if (loaded == false)
					{
						throw Exception(st + " music not found");
					}
				}
			}
		}
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
					i != sizeof(catsId) / sizeof(catsId[0]);
					++i)
			{
				SoundSet* sound = NULL;

				for (size_t
						j = 0;
						j != sizeof(cats) / sizeof(cats[0])
							&& sound == NULL;
						++j)
				{
					if (cats[j] != NULL)
					{
						bool wav;
						if (cats[j] == catsDos)
							wav = false;
						else
							wav = true;

						oststr.str("");
						oststr << "SOUND/" << cats[j][i];
						st = CrossPlatform::getDataFile(oststr.str());

						if (CrossPlatform::fileExists(st) == true)
						{
							sound = new SoundSet();
							sound->loadCat(st, wav);

							Options::currentSound = (wav) ? SOUND_14 : SOUND_10;
						}
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
				oststr.str("");
				oststr << "SOUND/" << (*i).second->getCATFile();
				st = CrossPlatform::getDataFile(oststr.str());

				if (CrossPlatform::fileExists(st) == true)
				{
					if (_sounds.find((*i).first) == _sounds.end())
						_sounds[(*i).first] = new SoundSet();

					for (std::vector<int>::const_iterator
							j = (*i).second->getSoundList().begin();
							j != (*i).second->getSoundList().end();
							++j)
					{
						_sounds[(*i).first]->loadCatbyIndex(st, *j);
					}
				}
				else
				{
					oststr << " not found";
					throw Exception(oststr.str());
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
	kL_soundPop					= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// global.

	/* BATTLESCAPE RESOURCES */
	Log(LOG_INFO) << "Loading battlescape resources ...";
	loadBattlescapeResources(); // TODO load this at battlescape start, unload at battlescape end

	// create extra rows on the soldier stat screens by shrinking them all down one pixel.
	// this is done after loading them, but BEFORE loading the extraSprites, in case a modder wants to replace them.
	// Actually, do it *after* loading extraSprites, in case a modder wants to alter the original
	// and still have this stretching happen:

	// first, do the base info screen
	// erase the old lines, copying from a +2 offset to account for the dithering
/*	for (int y = 91; y < 199; y += 12)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, y + 2));
	// drawn new lines, use the bottom row of pixels as a basis
	for (int y = 89; y < 199; y += 11)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, 199));
	// finally, move the top of the graph up by one pixel, offset for the last iteration again due to dithering.
	for (int y = 72; y < 80; ++y)
		for (int x = 0; x < 320; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, y + (y == 79? 2: 1))); */

	// Adjust the battlescape unit-info screen:
	// erase the old lines, no need to worry about dithering on this one
	for (int y = 39; y < 199; y += 10)
		for (int x = 0; x < 169; ++x)
			_surfaces["UNIBORD.PCK"]->setPixelColor(
												x,y,
												_surfaces["UNIBORD.PCK"]->getPixelColor(x, 30));
	// drawn new lines, use the bottom row of pixels as a basis
	for (int y = 190; y > 37; y -= 9)
		for (int x = 0; x < 169; ++x)
			_surfaces["UNIBORD.PCK"]->setPixelColor(
												x,y,
												_surfaces["UNIBORD.PCK"]->getPixelColor(x, 199));
	// move the top of the graph down by eight pixels to erase the row not needed (actually created ~1.8 extra rows earlier)
	for (int y = 37; y > 29; --y)
		for (int x = 0; x < 320; ++x)
		{
			_surfaces["UNIBORD.PCK"]->setPixelColor(
												x,y,
												_surfaces["UNIBORD.PCK"]->getPixelColor(x, y - 8));
			_surfaces["UNIBORD.PCK"]->setPixelColor(x, y - 8, 0);
		}


	/* EXTRA SPRITES */
	//Log(LOG_DEBUG) << "Loading extra resources from ruleset...";
	Log(LOG_INFO) << "Loading extra sprites ...";

	std::string st2;
	int
		start,
		offset;

	const std::vector<std::pair<std::string, ExtraSprites*> > extraSprites = rules->getExtraSprites();
	for (std::vector<std::pair<std::string, ExtraSprites*> >::const_iterator
			i = extraSprites.begin();
			i != extraSprites.end();
			++i)
	{
		st = i->first;
		Log(LOG_INFO) << ". sheetName = " << i->first;

		ExtraSprites* const spritePack = i->second;
		const bool subdivision = spritePack->getSubX() != 0
							  && spritePack->getSubY() != 0;

		if (spritePack->getSingleImage() == true)
		{
			if (_surfaces.find(st) == _surfaces.end())
			{
				//Log(LOG_DEBUG) << "Creating new single image: " << st;
				//Log(LOG_INFO) << "Creating new single image: " << st;
				_surfaces[st] = new Surface(
										spritePack->getWidth(),
										spritePack->getHeight());
			}
			else
			{
				//Log(LOG_DEBUG) << "Adding/Replacing single image: " << st;
				//Log(LOG_INFO) << "Adding/Replacing single image: " << st;
				delete _surfaces[st];
				_surfaces[st] = new Surface(
										spritePack->getWidth(),
										spritePack->getHeight());
			}

			oststr.str("");
			oststr << CrossPlatform::getDataFile(spritePack->getSprites()->operator[](0));
			_surfaces[st]->loadImage(oststr.str());
		}
		else // not SingleImage
		{
			bool adding = false;

			if (_sets.find(st) == _sets.end())
			{
				//Log(LOG_DEBUG) << "Creating new surface set: " << st;
				//Log(LOG_INFO) << "Creating new surface set: " << st;
				adding = true;
				if (subdivision == true)
					_sets[st] = new SurfaceSet(
											spritePack->getSubX(),
											spritePack->getSubY());
				else
					_sets[st] = new SurfaceSet(
											spritePack->getWidth(),
											spritePack->getHeight());
			}
			//else
			//{
				//Log(LOG_DEBUG) << "Adding/Replacing items in surface set: " << st;
				//Log(LOG_INFO) << "Adding/Replacing items in surface set: " << st;
			//}

			//if (subdivision == true)
			//{
				//const int frames = (spritePack->getWidth() / spritePack->getSubX()) * (spritePack->getHeight() / spritePack->getSubY());
				//Log(LOG_DEBUG) << "Subdividing into " << frames << " frames.";
				//Log(LOG_INFO) << "Subdividing into " << frames << " frames.";
			//}

			for (std::map<int, std::string>::const_iterator
					j = spritePack->getSprites()->begin();
					j != spritePack->getSprites()->end();
					++j)
			{
				start = j->first;

				st2 = j->second;
				if (st2.substr(st2.length() - 1, 1) == "/")
				{
					//Log(LOG_DEBUG) << "Loading surface set from folder: " << st2 << " starting at frame: " << start;
					//Log(LOG_INFO) << "Loading surface set from folder: " << st2 << " starting at frame: " << start;
					offset = start;

					oststr2.str("");
					oststr2 << CrossPlatform::getDataFolder(st2);
					const std::vector<std::string> contents = CrossPlatform::getFolderContents(oststr2.str());
					for (std::vector<std::string>::const_iterator
							k = contents.begin();
							k != contents.end();
							++k)
					{
						if (isImageFile((*k).substr(
												(*k).length() - 4,
												(*k).length())) == true)
						{
							try
							{
								oststr.str("");
								oststr << oststr2.str() << CrossPlatform::getDataFile(*k);

								if (_sets[st]->getFrame(offset))
								{
									//Log(LOG_DEBUG) << "Replacing frame: " << offset;
									//Log(LOG_INFO) << "Replacing frame: " << offset;
									_sets[st]->getFrame(offset)->loadImage(oststr.str());
								}
								else
								{
									if (adding == true)
										_sets[st]->addFrame(offset)->loadImage(oststr.str());
									else
									{
										//Log(LOG_DEBUG) << "Adding frame: " << offset + spritePack->getModIndex();
										//Log(LOG_INFO) << "Adding frame: " << offset + spritePack->getModIndex();
										_sets[st]->addFrame(offset + spritePack->getModIndex())->loadImage(oststr.str());
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
				}
				else
				{
					oststr.str("");

					if (spritePack->getSubX() == 0
						&& spritePack->getSubY() == 0)
					{
						oststr << CrossPlatform::getDataFile(st2);

						if (_sets[st]->getFrame(start))
						{
							//Log(LOG_DEBUG) << "Replacing frame: " << start;
							//Log(LOG_INFO) << "Replacing frame: " << start;
							_sets[st]->getFrame(start)->loadImage(oststr.str());
						}
						else
						{
							//Log(LOG_DEBUG) << "Adding frame: " << start << ", using index: " << start + spritePack->getModIndex();
							//Log(LOG_INFO) << "Adding frame: " << start << ", using index: " << start + spritePack->getModIndex();
							_sets[st]->addFrame(start + spritePack->getModIndex())->loadImage(oststr.str());
						}
					}
					else
					{
						Surface* const blank = new Surface(
														spritePack->getWidth(),
														spritePack->getHeight());
						oststr << CrossPlatform::getDataFile(spritePack->getSprites()->operator[](start));
						blank->loadImage(oststr.str());
						const int
							xDivision = spritePack->getWidth() / spritePack->getSubX(),
							yDivision = spritePack->getHeight() / spritePack->getSubY();

						offset = start;

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
								// joyDivision
								if (_sets[st]->getFrame(offset))
								{
									//Log(LOG_DEBUG) << "Replacing frame: " << offset;
									//Log(LOG_INFO) << "Replacing frame: " << offset;
									_sets[st]->getFrame(offset)->clear();
									// for some reason regular blit() doesn't work here how i want it, so use this function instead.
									blank->blitNShade(
												_sets[st]->getFrame(offset),
												0 - (x * spritePack->getSubX()),
												0 - (y * spritePack->getSubY()),
												0);
								}
								else
								{
									if (adding == true)
									{
										//Log(LOG_DEBUG) << "Adding frame: " << offset;
										//Log(LOG_INFO) << "Adding frame: " << offset;
										// for some reason regular blit() doesn't work here how i want it, so use this function instead.
										blank->blitNShade(
													_sets[st]->addFrame(offset),
													0 - (x * spritePack->getSubX()),
													0 - (y * spritePack->getSubY()),
													0);
									}
									else
									{
										//Log(LOG_DEBUG) << "Adding custom frame: " << offset + spritePack->getModIndex();
										//Log(LOG_INFO) << "Adding custom frame: " << offset + spritePack->getModIndex();
										// for some reason regular blit() doesn't work here how i want it, so use this function instead.
										blank->blitNShade(
													_sets[st]->addFrame(offset + spritePack->getModIndex()),
													0 - (x * spritePack->getSubX()),
													0 - (y * spritePack->getSubY()),
													0);
									}
								}

								++offset;
							}
						}

						delete blank;
					}
				}
			}
		}
	}

	// kL_begin: from before^ loading extraSprites
	for (int y = 91; y < 199; y += 12)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, y + 2));
	for (int y = 89; y < 199; y += 11)
		for (int x = 0; x < 149; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, 199));
	for (int y = 72; y < 80; ++y)
		for (int x = 0; x < 320; ++x)
			_surfaces["BACK06.SCR"]->setPixelColor(
												x,y,
												_surfaces["BACK06.SCR"]->getPixelColor(x, y + (y == 79 ? 2 : 1)));

	Surface* const altBack07 = new Surface(320,200);
	altBack07->copy(_surfaces["BACK07.SCR"]);
	for (int y = 172; y >= 152; --y)
		for (int x = 5; x <= 314; ++x)
			altBack07->setPixelColor(
									x, y + 4,
									altBack07->getPixelColor(x,y));
	for (int y = 147; y >= 134; --y)
		for (int x = 5; x <= 314; ++x)
			altBack07->setPixelColor(
									x, y + 9,
									altBack07->getPixelColor(x,y));
	for (int y = 132; y >= 109; --y)
		for (int x = 5; x <= 314; ++x)
			altBack07->setPixelColor(
									x, y + 10,
									altBack07->getPixelColor(x,y));
	_surfaces["ALTBACK07.SCR"] = altBack07; // kL_end.

	// copy constructor doesn't like doing this directly,
	// so make a second handobs file the old fashioned way.
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
			* const srf1 = _sets["HANDOB2.PCK"]->addFrame(i->first),
			* const srf2 = i->second;
		srf1->setPalette(srf2->getPalette());
		srf2->blit(srf1);
	}


	/* EXTRA SOUNDS */
	Log(LOG_INFO) << "Loading extra sounds ...";

	ExtraSounds* soundPack;

	const std::vector<std::pair<std::string, ExtraSounds*> > extraSounds = rules->getExtraSounds();
	for (std::vector<std::pair<std::string, ExtraSounds*> >::const_iterator
			i = extraSounds.begin();
			i != extraSounds.end();
			++i)
	{
		st = i->first;
		soundPack = i->second;

		if (_sounds.find(st) == _sounds.end())
		{
			Log(LOG_DEBUG) << "Creating new sound set: " << st << ", this will likely have no in-game use.";
			_sounds[st] = new SoundSet();
		}
		else
			Log(LOG_DEBUG) << "Adding/Replacing items in sound set: " << st;

		for (std::map<int, std::string>::const_iterator
				j = soundPack->getSounds()->begin();
				j != soundPack->getSounds()->end();
				++j)
		{
			start = j->first;
			st2 = j->second;

			if (st2.substr(st2.length() - 1, 1) == "/")
			{
				Log(LOG_DEBUG) << "Loading sound set from folder: " << st2 << " starting at index: " << start;

				offset = start;
				oststr.str("");
				oststr << CrossPlatform::getDataFolder(st2);
				files = CrossPlatform::getFolderContents(oststr.str());

				for (std::vector<std::string>::const_iterator
						k = files.begin();
						k != files.end();
						++k)
				{
					try
					{
						oststr2.str("");
						oststr2 << oststr.str() << CrossPlatform::getDataFile(*k);
						if (_sounds[st]->getSound(offset))
							_sounds[st]->getSound(offset)->load(oststr2.str());
						else
							_sounds[st]->addSound(offset + soundPack->getModIndex())->load(oststr2.str());

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
				oststr2.str("");
				oststr2 << CrossPlatform::getDataFile(st2);
				if (_sounds[st]->getSound(start))
				{
					Log(LOG_DEBUG) << "Replacing index: " << start;
					_sounds[st]->getSound(start)->load(oststr2.str());
				}
				else
				{
					Log(LOG_DEBUG) << "Adding index: " << start;
					_sounds[st]->addSound(start + soundPack->getModIndex())->load(oststr2.str());
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
	std::ostringstream // Load Battlescape ICONS
		oststr1,
		oststr2;

	oststr1 << "UFOGRAPH/" << "SPICONS.DAT";
	_sets["SPICONS.DAT"] = new SurfaceSet(32, 24);
	_sets["SPICONS.DAT"]->loadDat(CrossPlatform::getDataFile(oststr1.str()));

	oststr1.str("");
	oststr1 << "UFOGRAPH/" << "CURSOR.PCK";
	oststr2 << "UFOGRAPH/" << "CURSOR.TAB";
	_sets["CURSOR.PCK"] = new SurfaceSet(32, 40);
	_sets["CURSOR.PCK"]->loadPck(
							CrossPlatform::getDataFile(oststr1.str()),
							CrossPlatform::getDataFile(oststr2.str()));

	oststr1.str("");
	oststr2.str("");
	oststr1 << "UFOGRAPH/" << "SMOKE.PCK";
	oststr2 << "UFOGRAPH/" << "SMOKE.TAB";
	_sets["SMOKE.PCK"] = new SurfaceSet(32, 40);
	_sets["SMOKE.PCK"]->loadPck(
							CrossPlatform::getDataFile(oststr1.str()),
							CrossPlatform::getDataFile(oststr2.str()));

	oststr1.str("");
	oststr2.str("");
	oststr1 << "UFOGRAPH/" << "HIT.PCK";
	oststr2 << "UFOGRAPH/" << "HIT.TAB";
	_sets["HIT.PCK"] = new SurfaceSet(32, 40);
	_sets["HIT.PCK"]->loadPck(
							CrossPlatform::getDataFile(oststr1.str()),
							CrossPlatform::getDataFile(oststr2.str()));

	oststr1.str("");
	oststr2.str("");
	oststr1 << "UFOGRAPH/" << "X1.PCK";
	oststr2 << "UFOGRAPH/" << "X1.TAB";
	_sets["X1.PCK"] = new SurfaceSet(128, 64);
	_sets["X1.PCK"]->loadPck(
							CrossPlatform::getDataFile(oststr1.str()),
							CrossPlatform::getDataFile(oststr2.str()));

	oststr1.str("");
	_sets["MEDIBITS.DAT"] = new SurfaceSet(52, 58);
	oststr1 << "UFOGRAPH/" << "MEDIBITS.DAT";
	_sets["MEDIBITS.DAT"]->loadDat(CrossPlatform::getDataFile(oststr1.str()));

	oststr1.str("");
	_sets["DETBLOB.DAT"] = new SurfaceSet(16, 16);
	oststr1 << "UFOGRAPH/" << "DETBLOB.DAT";
	_sets["DETBLOB.DAT"]->loadDat(CrossPlatform::getDataFile(oststr1.str()));


	// Load Battlescape Terrain (only blanks are loaded, others are loaded just in time)
	const std::string blanks[] =
	{
		"BLANKS.PCK"
	};

	for (size_t
			i = 0;
			i != sizeof(blanks) / sizeof(blanks[0]);
			++i)
	{
		std::ostringstream
			oststr3,
			oststr4;

		oststr3 << "TERRAIN/" << blanks[i];

		const std::string tab = CrossPlatform::noExt(blanks[i]) + ".TAB";
		oststr4 << "TERRAIN/" << tab;

		//Log(LOG_INFO) << ". bset = " << oststr3;
		_sets[blanks[i]] = new SurfaceSet(32, 40);
		_sets[blanks[i]]->loadPck(
							CrossPlatform::getDataFile(oststr3.str()),
							CrossPlatform::getDataFile(oststr4.str()));
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
			_sets[*i] = new SurfaceSet(32,40);
		else
			_sets[*i] = new SurfaceSet(32,48);

		_sets[*i]->loadPck(path,tab);
	}

//	if (!_sets["CHRYS.PCK"]->getFrame(225)) // incomplete chryssalid set: 1.0 data: stop loading.
//	{
//		Log(LOG_FATAL) << "Version 1.0 data detected";
//		throw Exception("Invalid CHRYS.PCK, please patch your X-COM data to the latest version.");
//	}

	oststr1.str("");
	oststr1 << "GEODATA/LOFTEMPS.DAT";
	MapDataSet::loadLoft(
					CrossPlatform::getDataFile(oststr1.str()),
					&_voxelData);

	const std::string scrs[] =
	{
		"TAC00.SCR"
	};

	for (size_t
			i = 0;
			i != sizeof(scrs) / sizeof(scrs[0]);
			++i)
	{
		std::ostringstream oststr;
		oststr << "UFOGRAPH/" << scrs[i];

		_surfaces[scrs[i]] = new Surface(320,200);
		_surfaces[scrs[i]]->loadScr(CrossPlatform::getDataFile(oststr.str()));
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
			i != sizeof(spks) / sizeof(spks[0]);
			++i)
	{
		std::ostringstream oststr;
		oststr << "UFOGRAPH/" << spks[i];
		if (CrossPlatform::fileExists(CrossPlatform::getDataFile(oststr.str())) == true)
		{
			_surfaces[spks[i]] = new Surface(320, 200);
			_surfaces[spks[i]]->loadSpk(CrossPlatform::getDataFile(oststr.str()));
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

		_surfaces[*i] = new Surface(320,200);
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

		_surfaces[*i] = new Surface(320,200);
		_surfaces[*i]->loadSpk(path);
	}

	if (Options::battleHairBleach == true) // "fix" of color index in original soldier sprites
	{
		const std::string armorSheet ("XCOM_1.PCK"); // init. personal armor

		if (_sets.find(armorSheet) != _sets.end())
		{
			SurfaceSet* const xcom_1 = _sets[armorSheet];
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
					i != 3;
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

//		|| extension == "TIFF"; // why not .TIFF (because only the last 4 chars are passed in)
	/* // arbitrary limitation: let's not use these ones (although they're officially supported by sdl)
	extension == ".ICO"
	extension == ".CUR"
	extension == ".PNM"
	extension == ".PPM"
	extension == ".PGM"
	extension == ".PBM"
	extension == ".XPM"
	extension == "ILBM"
	// excluding jpeg to avoid inevitable issues due to compression
	extension == ".JPG"
	extension == "JPEG" */
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

}
