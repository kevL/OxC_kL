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

#include "XcomResourcePack.h"

#include <sstream>

#include "../Basescape/BasescapeState.h" // kL: soundPop

#include "../Battlescape/Position.h"

#include "../Engine/AdlibMusic.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Font.h"
#include "../Engine/GMCat.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h" // kL
#include "../Engine/ShaderDraw.h"
#include "../Engine/ShaderMove.h"
#include "../Engine/Sound.h"
#include "../Engine/SoundSet.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Geoscape/GeoscapeState.h" // kL: soundPop
#include "../Geoscape/GraphsState.h" // kL: sound Pop

#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Ruleset/ExtraMusic.h" // sza_ExtraMusic
#include "../Ruleset/ExtraSounds.h"
#include "../Ruleset/ExtraSprites.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleMusic.h" // sza_MusicRules
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

namespace
{

struct HairBleach
{
	static const Uint8 ColorGroup = 15 << 4;
	static const Uint8 ColorShade = 15;

	static const Uint8 Hair = 9 << 4;
	static const Uint8 Face = 6 << 4;
	static inline void func(
			Uint8& src,
			const Uint8& cutoff,
			int,
			int,
			int)
	{
		if (src > cutoff
			&& src <= Face + 15)
		{
			src = Hair + (src & ColorShade) - 6; // make hair color like male in xcom_0.pck
		}
	}
};

}


/**
 * Initializes the resource pack by loading all the
 * resources contained in the original game folder.
 * @param musicRules	- list of ...
 * @param extraSprites	- list of mod extra sprites
 * @param extraSounds	- list of mod extra sounds
 * @param extraMusic	- list of ...
 */
XcomResourcePack::XcomResourcePack(Ruleset* rules)
	:
		ResourcePack()
{
	//Log(LOG_INFO) << "Create XcomResourcePack";

	/* PALETTES */
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
		std::string s = "GEODATA/PALETTES.DAT";
		_palettes[pal[i]] = new Palette();
		_palettes[pal[i]]->loadDat(
								CrossPlatform::getDataFile(s),
								256,
								Palette::palOffset(i));
	}

	{
		std::string s1 = "GEODATA/BACKPALS.DAT";
		std::string s2 = "BACKPALS.DAT";
		_palettes[s2] = new Palette();
		_palettes[s2]->loadDat(
							CrossPlatform::getDataFile(s1),
							128);
	}

	{
		// Correct Battlescape palette
		std::string s1 = "GEODATA/PALETTES.DAT";
		std::string s2 = "PAL_BATTLESCAPE";
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
			SDL_Color* color = _palettes[s2]->getColors(Palette::backPos + 16 + i);
			*color = gradient[i];
		}
	}


	/* FONTS */
	// Load fonts
	YAML::Node doc = YAML::LoadFile(CrossPlatform::getDataFile("Language/Font.dat"));
	Font::setIndex(Language::utf8ToWstr(doc["chars"].as<std::string>()));
	for (YAML::const_iterator
			i = doc["fonts"].begin();
			i != doc["fonts"].end();
			++i)
	{
		std::string id = (*i)["id"].as<std::string>();
		Font *font = new Font();
		font->load(*i);
		_fonts[id] = font;
	}


	/* GRAPHICS */
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

	std::string geograph = CrossPlatform::getDataFolder("GEOGRAPH/");
	std::vector<std::string> scrs = CrossPlatform::getFolderContents(geograph, "SCR");
	for (std::vector<std::string>::iterator
			i = scrs.begin();
			i != scrs.end();
			++i)
	{
		std::string path = geograph + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					toupper);
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadScr(path);
	}

	std::vector<std::string> bdys = CrossPlatform::getFolderContents(geograph, "BDY");
	for (std::vector<std::string>::iterator
			i = bdys.begin();
			i != bdys.end();
			++i)
	{
		std::string path = geograph + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					toupper);
//		*i = (*i).substr(0, (*i).length() - 3);
//		*i = *i + "PCK";
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadBdy(path);
	}

	// kL_begin:
	Surface* kL_Geo = new Surface(
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
	_surfaces["LGEOBORD.SCR"] = kL_Geo; // kL_end.

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

	// here we create an "alternate" background surface for the base info screen.
	_surfaces["ALTBACK07.SCR"] = new Surface(320, 200);
	_surfaces["ALTBACK07.SCR"]->loadScr(CrossPlatform::getDataFile("GEOGRAPH/BACK07.SCR"));
	for (int
			y = 172;
			y >= 152;
			--y)
		for (int
				x = 5;
				x <= 314;
				++x)
			_surfaces["ALTBACK07.SCR"]->setPixelColor(
												x,
												y + 4,
												_surfaces["ALTBACK07.SCR"]->getPixelColor(x, y));

	for (int
			y = 147;
			y >= 134;
			--y)
		for (int
				x = 5;
				x <= 314;
				++x)
			_surfaces["ALTBACK07.SCR"]->setPixelColor(
												x,
												y + 9,
												_surfaces["ALTBACK07.SCR"]->getPixelColor(x, y));

	for (int
			y = 132;
			y >= 109;
			--y)
		for (int
				x = 5;
				x <= 314;
				++x)
			_surfaces["ALTBACK07.SCR"]->setPixelColor(
												x,
												y + 10,
												_surfaces["ALTBACK07.SCR"]->getPixelColor(x, y));

	std::vector<std::string> spks = CrossPlatform::getFolderContents(geograph, "SPK");
	for (std::vector<std::string>::iterator
			i = spks.begin();
			i != spks.end();
			++i)
	{
		std::string path = geograph + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					toupper);
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadSpk(path);
	}

	std::string ufointro = CrossPlatform::getDataFolder("UFOINTRO/"); // Load intro
	std::vector<std::string> lbms = CrossPlatform::getFolderContents(ufointro, "LBM");
	for (std::vector<std::string>::iterator
			i = lbms.begin();
			i != lbms.end();
			++i)
	{
		std::string path = ufointro + *i;
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					toupper);
		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadImage(path);
	}

	std::string sets[] = // Load surface sets
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

		std::string ext = sets[i].substr(sets[i].find_last_of('.') + 1, sets[i].length());
		if (ext == "PCK")
		{
			std::string tab = CrossPlatform::noExt(sets[i]) + ".TAB";
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

/*
MOVED TO Ruleset !

	std::ostringstream s; // Load polygons
	s << "GEODATA/" << "WORLD.DAT";
	Globe::loadDat(
				CrossPlatform::getDataFile(s.str()),
				&_polygons);


	// LINES /
	// Load polylines (extracted from game)
	// -10 = Start of line
	// -20 = End of data
	double lines[] =
	{
		-10,
			1.18901, -0.412334, 1.23918, -0.425424, 1.213, -0.471239, 1.22828, -0.490874, 1.23482, -0.482147, 1.30245, -0.541052, 1.29373,
			-0.608684, 1.35918, -0.61741, 1.38099, -0.53887, 1.41154, -0.530144, 1.39626, -0.503964, 1.53153, -0.460331, 1.54025, -0.488692,
			1.55116, -0.490874, 1.55334, -0.466876, 1.60352, -0.469057, 1.59916, -0.488692, 1.67552, -0.517054, 1.69515, -0.475602, 1.61661,
			-0.386154, 1.61225, -0.436332, 1.56861, -0.440696, 1.56425, -0.460331, 1.54243, -0.462512, 1.53589, -0.449422, 1.55552, -0.373064,
		-10,
			6.13047, -0.726493, 6.17628, -0.726493, 6.1501, -0.645772, -10, 6.25264, -0.759218, 0.0109083, -0.73522, 0.0567232, -0.741765,
		-10,
			0.128718, -0.7614, 0.122173, -0.80067, 0.102538, -0.807215, 0.1309, -0.829031, 0.14399, -0.85303, 0.111265, -0.863938, 0.0719948,
			-0.870483, 0.0501782, -0.885755, -10, 0.122173, -0.80067, 0.148353, -0.811578, 0.159261, -0.80067, 0.211621, -0.820305, 0.239983,
			-0.811578, 0.239983, -0.794125, -10, 0.111265, -0.863938, 0.102538, -0.907571, 0.11781, -0.90539, 0.122173, -0.938114, -10, 0.139626,
			-0.959931, 0.181078, -0.953386, -10, 0.248709, -0.942478, 0.261799, -0.887936, 0.213803, -0.877028, 0.242164, -0.85303, 0.229074,
			-0.829031, 0.1309, -0.829031, -10, 0.0458149, -0.109083, 0.0479966, -0.148353, 0.0654498, -0.185441, 0.0698132, -0.237801, 0.0981748,
			-0.244346, 0.122173, -0.224711, 0.17017, -0.222529, 0.231256, -0.235619, 0.257436, -0.211621, 0.19635, -0.113446, 0.176715, -0.126536,
			0.148353, -0.0763582,
		-10,
			0.438514, -0.554142, 0.436332, -0.383972, 0.595594, -0.383972, 0.628319, -0.410152,
		-10,
			0.59123, -0.547597, 0.619592, -0.493056, -10, 0.283616, 0.4996, 0.349066, 0.495237, 0.349066, 0.434151, 0.362156, 0.469057, 0.407971,
			0.440696, 0.447241, 0.449422, 0.510509, 0.386154, 0.545415, 0.390517, 0.558505, 0.469057, 0.575959, 0.464694,
		-10,
			5.36252, 0.580322, 5.27962, 0.523599, 5.34071, 0.449422, 5.27089, 0.386154, 5.26653, 0.283616, 5.14436, 0.174533, 5.05491, 0.194168,
			4.996, 0.14399, 5.01564, 0.0872665, 5.06364, 0.0763582, 5.06582, -0.0305433, 5.18145, -0.0370882, 5.15527, -0.0698132, 5.2229, -0.0938114,
			5.2578, -0.019635, 5.35816, -0.0327249, 5.38652, -0.0741765, -10, 4.10152, -0.85303, 4.45059, -0.85303, 4.62512, -0.855211, 4.71893,
			-0.837758,
		-10,
			5.116, -0.776672, 5.08545, -0.824668, 5.03309, -0.785398, 4.97419, -0.785398, 4.95019, -0.770127,
		-10,
			3.82227, -1.21519, 3.82227, -1.05374, 4.01426, -0.977384, 3.95972, -0.949023,
		-10,
			4.23897, -0.569414, 4.42659, -0.554142, 4.48113, -0.503964, 4.51386, -0.519235, 4.55531, -0.460331, 4.59022, -0.455967,
		-10,
			4.82584, -0.728675, 4.84983, -0.750492,
		-10,
			4.8062, -0.81376, 4.82802, -0.80067,
		-10,
			0.545415, -1.21955, 0.549779, -1.09738, 0.490874, -1.05156,
		-10,
			0.488692, -1.04283, 0.490874, -0.981748, 0.569414, -0.933751, 0.554142, -0.909753, 0.698132, -0.863938, 0.665407, -0.818123,
		-10,
			0.693768, -0.763582, 0.857393, -0.730857,
		-10,
			0.861756, -0.805033, 0.831213, -0.87921, 1.0472, -0.885755, 1.0712, -0.944659, 1.2021, -0.966476, 1.34172, -0.951204, 1.39626, -0.885755,
			1.53589, -0.857393, 1.71042, -0.872665, 1.72569, -0.909753, 1.91986, -0.859575, 2.03767, -0.870483, 2.08131, -0.872665, 2.09658, -0.922843,
			2.19693, -0.925025, 2.23184, -0.86612, 2.34747, -0.842121, 2.32129, -0.785398, 2.28638, -0.783217, 2.27984, -0.73522, 2.16857, -0.698132,
		-10,
			1.88277, -0.375246, 1.8435, -0.407971, 1.77587, -0.370882, 1.73006, -0.386154, 1.72569, -0.423242, 1.7017, -0.418879, 1.72569, -0.477784,
			1.69515, -0.475602,
		-10,
			1.59916, -0.488692, 1.55116, -0.490874,
		-10,
			1.54025, -0.488692, 1.41154, -0.530144,
		-10,
			1.35918, -0.61741, 1.28064, -0.687223, 1.40499, -0.737402, 1.39626, -0.785398, 1.4399, -0.78758, 1.44644, -0.824668, 1.49662, -0.822486,
			1.50753, -0.857393, 1.53589, -0.857393, 1.5817, -0.789761, 1.67988, -0.746128, 1.8326, -0.724312, 1.95477, -0.7614, 1.95695, -0.785398,
			2.09221, -0.815941, 2.02022, -0.833395, 2.03767, -0.870483,
		-20
	};

	Polyline* line = 0;

	int start = 0;
	for (size_t
			i = 0;
			lines[i] > -19.999;
			++i)
	{
		if (lines[i] < -9.999
			&& lines[i] > -10.001)
		{
			if (line != 0)
				_polylines.push_back(line);

			int points = 0;
			for (int
					j = i + 1;
					lines[j] > -9.999;
					++j)
			{
				points++;
			}

			points /= 2;
			line = new Polyline(points);

			start = i + 1;
		}
		else
		{
			if ((i - start) %2 == 0)
				line->setLongitude((i - start) / 2, lines[i]);
			else
				line->setLatitude((i - start) / 2, lines[i]);
		}
	}

	_polylines.push_back(line); */


	/* MUSICS */
	if (!Options::mute)
	{
#ifndef __NO_MUSIC // sza_MusicRules
		// Load musics
		// gather the assignments first.
		std::vector<std::pair<std::string, RuleMusic*> > musicRules = rules->getMusic();
		for (std::vector<std::pair<std::string, RuleMusic*> >::const_iterator
				i = musicRules.begin();
				i != musicRules.end();
				++i)
		{
			std::string type = i->first;
			RuleMusic* musicRule = i->second;

			std::vector<std::string> terrains = musicRule->getTerrains();
			std::string mode = musicRule->getMode();
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

		std::string mus[] =
		{
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
			"GMWIN"
		};

/*		int tracks[] = {3, 6, 0, 18, 2, 19, 20, 21, 10, 9, 8, 12, 17, 11};
		float tracks_normalize[] = {0.76f, 0.83f, 1.19f, 1.0f, 0.74f, 0.8f, 0.8f, 0.8f, 1.0f, 0.92f, 0.81f, 1.0f, 1.14f, 0.84f}; */

		// Check which music version is available
		// Check if GM.CAT data is available // sza_MusicRules
		CatFile
			* adlibcat = 0,
			* aintrocat = 0;
		GMCatFile* gmcat = 0;

		std::string
			musicAdlib	= "SOUND/ADLIB.CAT",
			musicIntro	= "SOUND/AINTRO.CAT",
			musicGM		= "SOUND/GM.CAT";

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile(musicAdlib)))
		{
			adlibcat = new CatFile(CrossPlatform::getDataFile(musicAdlib).c_str());

			if (CrossPlatform::fileExists(CrossPlatform::getDataFile(musicIntro)))
				aintrocat = new CatFile(CrossPlatform::getDataFile(musicIntro).c_str());
		}

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile(musicGM)))
			gmcat = new GMCatFile(CrossPlatform::getDataFile(musicGM).c_str());

		// We have the assignments, we only need to load the required files now.
		for (std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, int> > > >::const_iterator
				i = _musicAssignment.begin();
				i != _musicAssignment.end();
				++i)
		{
			std::string type = i->first;
			std::map<std::string, std::vector<std::pair<std::string, int> > > assignment = i->second;
			for (std::map<std::string, std::vector<std::pair<std::string, int> > >::const_iterator
					j = assignment.begin();
					j != assignment.end();
					++j)
			{
				std::vector<std::pair<std::string, int> > filenames = j->second;
				for (std::vector<std::pair<std::string, int> >::const_iterator
						k = filenames.begin();
						k != filenames.end();
						++k)
				{
					std::string filename = k->first;
					int midiIndex = k->second;

//					LoadMusic(filename, midiIndex):
/*kL, moved below...
					std::string exts[] =
					{
						".flac",
						".ogg",
						".mp3",
						".mod",
						".wav" // kL_add ( also add "." and remove them below )
					}; */

					bool loaded = false;

					// The file may have already been loaded because of an other assignment
					if (_musicFile.find(filename) != _musicFile.end())
						loaded = true;

					if (!loaded) // Try digital tracks
					{


						// sza_ExtraMusic_BEGIN:

						// kL_note: This section may well be redundant w/ sza_MusicRules!!
						// Load alternative digital track if there is an override
						for (size_t
								l = 0;
								l < sizeof(mus) / sizeof(mus[0]);
								++l)
						{
							bool loaded = false;

							std::vector<std::pair<std::string, ExtraMusic*> > extraMusic = rules->getExtraMusic();
							for (std::vector<std::pair<std::string, ExtraMusic*> >::const_iterator
									m = extraMusic.begin();
									m != extraMusic.end();
									++m)
							{
								ExtraMusic* musicRule = m->second;
								// check if there is an entry which overrides something but does not specify the terrain
								if (!musicRule->hasTerrainSpecification())
								{
									std::string overridden = musicRule->getOverridden();
									if (!overridden.empty()
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
						}

						if (!loaded) // sza_End.
						{ // sza_Add.

							// kL_begin: moved here from above.
							std::string exts[] =
							{
								".flac",
								".ogg",
								".mp3",
								".mod",
								".wav" // kL_add ( also add "." and remove them below )
							}; // kL_end.

							for (size_t
									exti = 0;
									exti < 3;
									++exti)
							{
								std::ostringstream s;
								s << "SOUND/" << filename << exts[exti];

								if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())))
								{
									_musicFile[filename] = new Music();
									_musicFile[filename]->load(CrossPlatform::getDataFile(s.str()));

									loaded = true;

									break;
								}
							}
						} // sza_Add.
					}

					if (!loaded)
					{
						if (adlibcat // Try Adlib music
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
						else if (gmcat) // Try GM music
						{
							_musicFile[filename] = gmcat->loadMIDI(midiIndex);

							loaded = true;
						}
						else // Try MIDI music
						{
							std::ostringstream s;
							s << "SOUND/" << filename << ".mid";

							if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())))
							{
								_musicFile[filename] = new Music();
								_musicFile[filename]->load(CrossPlatform::getDataFile(s.str()));

								loaded = true;
							}
						}
					}

					if (!loaded)
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

		delete adlibcat;
		delete aintrocat;
		delete gmcat;

/*		std::string musOptional[] = {"GMGEO3",
									 "GMGEO4",
									 "GMGEO5",
									 "GMGEO6",
									 "GMGEO7",
									 "GMGEO8",
									 "GMGEO9",
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
		std::string catsId[] = // Load sounds
		{
			"GEO.CAT",
			"BATTLE.CAT"
		};
		std::string catsDos[] =
		{
			"SOUND2.CAT",
			"SOUND1.CAT"
		};
		std::string catsWin[] =
		{
			"SAMPLE.CAT",
			"SAMPLE2.CAT"
		};

		// Try the preferred format first, otherwise use the default priority
		std::string* cats[] =
		{
			0,
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
			SoundSet* sound = 0;

			for (size_t
					j = 0;
					j < sizeof(cats) / sizeof(cats[0])
						&& sound == 0;
					++j)
			{
				bool wav = true;

				if (cats[j] == 0)
					continue;
				else if (cats[j] == catsDos)
					wav = false;

				std::ostringstream s;
				s << "SOUND/" << cats[j][i];
				std::string file = CrossPlatform::getDataFile(s.str());
				if (CrossPlatform::fileExists(file))
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
			{
				_sounds[catsId[i]] = sound;
			}
		}

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile("SOUND/INTRO.CAT")))
		{
			SoundSet* s = _sounds["INTRO.CAT"] = new SoundSet();
			s->loadCat(CrossPlatform::getDataFile("SOUND/INTRO.CAT"), false);
		}

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile("SOUND/SAMPLE3.CAT")))
		{
			SoundSet* s = _sounds["SAMPLE3.CAT"] = new SoundSet();
			s->loadCat(CrossPlatform::getDataFile("SOUND/SAMPLE3.CAT"), true);
		}
	}

	// define GUI sound Fx
	TextButton::soundPress		= getSound("GEO.CAT", ResourcePack::BUTTON_PRESS);		// #0 bleep
//kL	Window::soundPopup[0]	= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// #1 wahahahah
	Window::soundPopup[1]		= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[1]);	// #2 swish1
	Window::soundPopup[2]		= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[2]);	// #3 swish2
	GeoscapeState::soundPop		= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// wahahahah // kL, used for Geo->Base & Geo->Graphs
	BasescapeState::soundPop	= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// wahahahah // kL, used for Basescape RMB.
	GraphsState::soundPop		= getSound("GEO.CAT", ResourcePack::WINDOW_POPUP[0]);	// wahahahah // kL, used for switching Graphs screens. Or just returning to Geoscape.

	/* BATTLESCAPE RESOURCES */
	loadBattlescapeResources(); // TODO load this at battlescape start, unload at battlescape end?

	// we create extra rows on the soldier stat screens by shrinking them all down one pixel.
	// this is done after loading them, but BEFORE loading the extraSprites, in case a modder wants to replace them.

	// first, let's do the base info screen
	// erase the old lines, copying from a +2 offset to account for the dithering
	for (int y = 91; y < 199; y += 12)
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
												_surfaces["BACK06.SCR"]->getPixelColor(x, y + (y == 79? 2: 1)));

	// now, let's adjust the battlescape info screen.
	// erase the old lines, no need to worry about dithering on this one.
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
	// move the top of the graph down by eight pixels to erase the row we don't need (we actually created ~1.8 extra rows earlier)
	for (int y = 37; y > 29; --y)
		for (int x = 0; x < 320; ++x)
		{
			_surfaces["UNIBORD.PCK"]->setPixelColor(
												x,
												y,
												_surfaces["UNIBORD.PCK"]->getPixelColor(x, y - 8));
			_surfaces["UNIBORD.PCK"]->setPixelColor(x, y - 8, 0);
		}


	Log(LOG_DEBUG) << "Loading extra resources from ruleset...";
	Log(LOG_INFO) << "Loading extra resources from ruleset...";

	/* EXTRA SPRITES */
	std::ostringstream s;

	std::vector<std::pair<std::string, ExtraSprites*> > extraSprites = rules->getExtraSprites();
	for (std::vector<std::pair<std::string, ExtraSprites*> >::const_iterator
			i = extraSprites.begin();
			i != extraSprites.end();
			++i)
	{
		std::string sheetName = i->first;
		Log(LOG_INFO) << ". sheetName = " << i->first;

		ExtraSprites* spritePack = i->second;
		bool subdivision = spritePack->getSubX() != 0
						&& spritePack->getSubY() != 0;

		if (spritePack->getSingleImage())
		{
			if (_surfaces.find(sheetName) == _surfaces.end())
			{
				Log(LOG_DEBUG) << "Creating new single image: " << sheetName;
				Log(LOG_INFO) << "Creating new single image: " << sheetName;

				_surfaces[sheetName] = new Surface(
												spritePack->getWidth(),
												spritePack->getHeight());
			}
			else
			{
				Log(LOG_DEBUG) << "Adding/Replacing single image: " << sheetName;
				Log(LOG_INFO) << "Adding/Replacing single image: " << sheetName;

				delete _surfaces[sheetName];

				_surfaces[sheetName] = new Surface(
												spritePack->getWidth(),
												spritePack->getHeight());
			}

			s.str("");
			s << CrossPlatform::getDataFile(spritePack->getSprites()->operator[](0));
			_surfaces[sheetName]->loadImage(s.str());
		}
		else
		{
			bool adding = false;

			if (_sets.find(sheetName) == _sets.end())
			{
				Log(LOG_DEBUG) << "Creating new surface set: " << sheetName;
				Log(LOG_INFO) << "Creating new surface set: " << sheetName;

				adding = true;

				if (subdivision)
					_sets[sheetName] = new SurfaceSet(
													spritePack->getSubX(),
													spritePack->getSubY());
				else
					_sets[sheetName] = new SurfaceSet(
													spritePack->getWidth(),
													spritePack->getHeight());
			}
			else
				Log(LOG_DEBUG) << "Adding/Replacing items in surface set: " << sheetName;
				Log(LOG_INFO) << "Adding/Replacing items in surface set: " << sheetName;

			if (subdivision)
			{
				int frames = (spritePack->getWidth() / spritePack->getSubX()) * (spritePack->getHeight() / spritePack->getSubY());

				Log(LOG_DEBUG) << "Subdividing into " << frames << " frames.";
				Log(LOG_INFO) << "Subdividing into " << frames << " frames.";
			}

			for (std::map<int, std::string>::iterator
					j = spritePack->getSprites()->begin();
					j != spritePack->getSprites()->end();
					++j)
			{
				s.str("");
				int startFrame = j->first;

				std::string fileName = j->second;
				if (fileName.substr(fileName.length() - 1, 1) == "/")
				{
					Log(LOG_DEBUG) << "Loading surface set from folder: " << fileName << " starting at frame: " << startFrame;
					Log(LOG_INFO) << "Loading surface set from folder: " << fileName << " starting at frame: " << startFrame;

					int offset = startFrame;

					std::ostringstream folder;
					folder << CrossPlatform::getDataFolder(fileName);
					std::vector<std::string> contents = CrossPlatform::getFolderContents(folder.str());
					for (std::vector<std::string>::iterator
							k = contents.begin();
							k != contents.end();
							++k)
					{
						if (!isImageFile((*k).substr((*k).length() - 4, (*k).length())))
							continue;

						try
						{
							s.str("");
							s << folder.str() << CrossPlatform::getDataFile(*k);

							if (_sets[sheetName]->getFrame(offset))
							{
								Log(LOG_DEBUG) << "Replacing frame: " << offset;
								Log(LOG_INFO) << "Replacing frame: " << offset;

								_sets[sheetName]->getFrame(offset)->loadImage(s.str());
							}
							else
							{
								if (adding)
									_sets[sheetName]->addFrame(offset)->loadImage(s.str());
								else
								{
									Log(LOG_DEBUG) << "Adding frame: " << offset + spritePack->getModIndex();
									Log(LOG_INFO) << "Adding frame: " << offset + spritePack->getModIndex();

									_sets[sheetName]->addFrame(offset + spritePack->getModIndex())->loadImage(s.str());
								}
							}

							offset++;
						}
						catch (Exception &e)
						{
							Log(LOG_WARNING) << e.what();
						}
					}
				}
				else
				{
					if (spritePack->getSubX() == 0
						&& spritePack->getSubY() == 0)
					{
						s << CrossPlatform::getDataFile(fileName);

						if (_sets[sheetName]->getFrame(startFrame))
						{
							Log(LOG_DEBUG) << "Replacing frame: " << startFrame;
							Log(LOG_INFO) << "Replacing frame: " << startFrame;

							_sets[sheetName]->getFrame(startFrame)->loadImage(s.str());
						}
						else
						{
							Log(LOG_DEBUG) << "Adding frame: " << startFrame << ", using index: " << startFrame + spritePack->getModIndex();
							Log(LOG_INFO) << "Adding frame: " << startFrame << ", using index: " << startFrame + spritePack->getModIndex();

							_sets[sheetName]->addFrame(startFrame + spritePack->getModIndex())->loadImage(s.str());
						}
					}
					else
					{
						Surface* temp = new Surface(
												spritePack->getWidth(),
												spritePack->getHeight());
						s.str("");
						s << CrossPlatform::getDataFile(spritePack->getSprites()->operator[](startFrame));
						temp->loadImage(s.str());
						int xDivision = spritePack->getWidth() / spritePack->getSubX();
						int yDivision = spritePack->getHeight() / spritePack->getSubY();
						int offset = startFrame;

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
									Log(LOG_DEBUG) << "Replacing frame: " << offset;
									Log(LOG_INFO) << "Replacing frame: " << offset;

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
									if (adding)
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
										Log(LOG_DEBUG) << "Adding frame: " << offset + spritePack->getModIndex();
										Log(LOG_INFO) << "Adding frame: " << offset + spritePack->getModIndex();

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

	// copy constructor doesn't like doing this directly,
	// so let's make a second handobs file the old fashioned way.
	// handob2 is used for all the left handed sprites.
	_sets["HANDOB2.PCK"] = new SurfaceSet(
									_sets["HANDOB.PCK"]->getWidth(),
									_sets["HANDOB.PCK"]->getHeight());
	std::map<int, Surface*>* handob = _sets["HANDOB.PCK"]->getFrames();

	for (std::map<int, Surface*>::const_iterator
			i = handob->begin();
			i != handob->end();
			++i)
	{
		Surface* surface1 = _sets["HANDOB2.PCK"]->addFrame(i->first);
		Surface* surface2 = i->second;
		surface1->setPalette(surface2->getPalette());
		surface2->blit(surface1);
	}


	/* EXTRA SOUNDS */
	std::vector<std::pair<std::string, ExtraSounds*> > extraSounds = rules->getExtraSounds();
	for (std::vector<std::pair<std::string, ExtraSounds*> >::const_iterator
			i = extraSounds.begin();
			i != extraSounds.end();
			++i)
	{
		std::string setName = i->first;
		ExtraSounds* soundPack = i->second;

		if (_sounds.find(setName) == _sounds.end())
		{
			Log(LOG_DEBUG) << "Creating new sound set: " << setName << ", this will likely have no in-game use.";

			_sounds[setName] = new SoundSet();
		}
		else
			Log(LOG_DEBUG) << "Adding/Replacing items in sound set: " << setName;

		for (std::map<int, std::string>::iterator
				j = soundPack->getSounds()->begin();
				j != soundPack->getSounds()->end();
				++j)
		{
			int startSound = j->first;
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

						offset++;
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
{
}

/**
 * Loads the resources required by the Battlescape.
 */
void XcomResourcePack::loadBattlescapeResources()
{
	//Log(LOG_INFO) << "XcomResourcePack::loadBattlescapeResources()";

	// Load Battlescape ICONS
	std::ostringstream s;
	s << "UFOGRAPH/" << "SPICONS.DAT";
	_sets["SPICONS.DAT"] = new SurfaceSet(32, 24);
	_sets["SPICONS.DAT"]->loadDat(CrossPlatform::getDataFile(s.str()));

	s.str("");
	std::ostringstream s2;
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
	std::string bsets[] =
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

		std::string tab = CrossPlatform::noExt(bsets[i]) + ".TAB";
		s2 << "TERRAIN/" << tab;

		//Log(LOG_INFO) << ". bset = " << s;
		_sets[bsets[i]] = new SurfaceSet(32, 40);
		_sets[bsets[i]]->loadPck(
							CrossPlatform::getDataFile(s.str()),
							CrossPlatform::getDataFile(s2.str()));
	}


	// Load Battlescape units
	std::string units = CrossPlatform::getDataFolder("UNITS/");
	std::vector<std::string> usets = CrossPlatform::getFolderContents(units, "PCK");
	for (std::vector<std::string>::iterator
			i = usets.begin();
			i != usets.end();
			++i)
	{
		Log(LOG_INFO) << "XcomResourcePack::loadBattlescapeResources() units/ " << *i;

		std::string path = units + *i;
		std::string tab = CrossPlatform::getDataFile("UNITS/" + CrossPlatform::noExt(*i) + ".TAB");
		std::transform(
					i->begin(),
					i->end(),
					i->begin(),
					toupper);

		if (*i != "BIGOBS.PCK")
			_sets[*i] = new SurfaceSet(32, 40);
		else
			_sets[*i] = new SurfaceSet(32, 48);

		_sets[*i]->loadPck(path, tab);
	}

	if (!_sets["CHRYS.PCK"]->getFrame(225)) // incomplete chryssalid set: 1.0 data: stop loading.
	{
		Log(LOG_FATAL) << "Version 1.0 data detected";
		throw Exception("Invalid CHRYS.PCK, please patch your X-COM data to the latest version");
	}

	s.str("");
	s << "GEODATA/" << "LOFTEMPS.DAT";
	MapDataSet::loadLOFTEMPS(
						CrossPlatform::getDataFile(s.str()),
						&_voxelData);

	std::string scrs[] =
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

	std::string lbms[] =
	{
		"D0.LBM",
		"D1.LBM",
		"D2.LBM",
		"D2.LBM"
	};

	std::string pals[] =
	{
		"PAL_BATTLESCAPE",
		"PAL_BATTLESCAPE_1",
		"PAL_BATTLESCAPE_2",
		"PAL_BATTLESCAPE_3"
	};

	for (size_t
			i = 0;
			i < sizeof(lbms) / sizeof(lbms[0]);
			++i)
	{
		std::ostringstream s;
		s << "UFOGRAPH/" << lbms[i];
		if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())))
		{
			if (!i)
				delete _palettes["PAL_BATTLESCAPE"];

			Surface* tempSurface = new Surface(1, 1);
			tempSurface->loadImage(CrossPlatform::getDataFile(s.str()));

			_palettes[pals[i]] = new Palette();
			_palettes[pals[i]]->setColors(tempSurface->getPalette(), 256);

			delete tempSurface;
		}
	}

	std::string spks[] =
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
		if (CrossPlatform::fileExists(CrossPlatform::getDataFile(s.str())))
		{
			_surfaces[spks[i]] = new Surface(320, 200);
			_surfaces[spks[i]]->loadSpk(CrossPlatform::getDataFile(s.str()));
		}
	}


	std::string ufograph = CrossPlatform::getDataFolder("UFOGRAPH/");

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
					toupper);
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
					toupper);

		_surfaces[*i] = new Surface(320, 200);
		_surfaces[*i]->loadSpk(path);
	}

	if (Options::battleHairBleach) // "fix" of hair color of male personal armor
	{
		if (_sets.find("XCOM_1.PCK") != _sets.end())
		{
			SurfaceSet* xcom_1 = _sets["XCOM_1.PCK"];

			for (int // chest frame
					i = 0;
					i < 16;
					++i)
			{
				Surface* surf = xcom_1->getFrame(4 * 8 + i);
				ShaderMove<Uint8> head = ShaderMove<Uint8>(surf);
				GraphSubset dim = head.getBaseDomain();
				surf->lock();
				dim.beg_y = 6;
				dim.end_y = 9;
				head.setDomain(dim);
				ShaderDraw<HairBleach>(head, ShaderScalar<Uint8>(HairBleach::Face + 5));
				dim.beg_y = 9;
				dim.end_y = 10;
				head.setDomain(dim);
				ShaderDraw<HairBleach>(head, ShaderScalar<Uint8>(HairBleach::Face + 6));
				surf->unlock();
			}

			for (int // fall frame
					i = 0;
					i < 3;
					++i)
			{
				Surface* surf = xcom_1->getFrame(264 + i);
				ShaderMove<Uint8> head = ShaderMove<Uint8>(surf);
				GraphSubset dim = head.getBaseDomain();
				dim.beg_y = 0;
				dim.end_y = 24;
				dim.beg_x = 11;
				dim.end_x = 20;
				head.setDomain(dim);
				surf->lock();
				ShaderDraw<HairBleach>(head, ShaderScalar<Uint8>(HairBleach::Face + 6));
				surf->unlock();
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
				toupper);

	return extension == ".BMP"
			|| extension == ".LBM"
			|| extension == ".IFF"
			|| extension == ".PCX"
			|| extension == ".GIF"
			|| extension == ".PNG"
			|| extension == ".TGA"
			|| extension == ".TIF";
//			|| extension == "TIFF"; // kL_note: why not .TIFF (prob because only the last 4 chars are passed in)

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
 * @param fmt Format of the music.
 * @param file Filename of the music.
 * @param track Track number of the music, if stored in a CAT.
 * @param volume Volume modifier of the music, if stored in a CAT.
 * @param adlibcat Pointer to ADLIB.CAT if available.
 * @param aintrocat Pointer to AINTRO.CAT if available.
 * @param gmcat Pointer to GM.CAT if available.
 * @return, Pointer to the music file, or NULL if it couldn't be loaded.
 */
Music* XcomResourcePack::loadMusic(
		MusicFormat fmt,
		const std::string& file,
		int track,
		float volume,
		CatFile* adlibcat,
		CatFile* aintrocat,
		GMCatFile* gmcat)
{
/* MUSIC_AUTO, MUSIC_FLAC, MUSIC_OGG, MUSIC_MP3, MUSIC_MOD, MUSIC_WAV, MUSIC_ADLIB, MUSIC_MIDI */
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
					music->load(adlibcat->load(track, true), adlibcat->getObjectSize(track));
				else if (aintrocat) // separate intro music
				{
					track -= adlibcat->getAmount();
					music->load(
							aintrocat->load(track, true),
							aintrocat->getObjectSize(track));
				}
			}
		}
		else if (fmt == MUSIC_MIDI) // Try MIDI music
		{
			if (gmcat) // DOS MIDI
				music = gmcat->loadMIDI(track);
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
}

}
