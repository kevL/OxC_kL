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

#include "MapDataSet.h"

//#include <fstream>
//#include <sstream>

//#include <SDL_endian.h>

#include "MapData.h"

//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Game.h"
//#include "../Engine/Logger.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

MapData
	* MapDataSet::_blankTile	= NULL,
	* MapDataSet::_scorchedTile	= NULL;


/**
 * MapDataSet construction.
 * @param name - reference the name of the data
 * @param game - pointer to Game (default NULL)
 */
MapDataSet::MapDataSet(
		const std::string& name,
		Game* game)
	:
		_name(name),
		_game(game),
		_surfaceSet(NULL),
		_loaded(false)
{
	//Log(LOG_INFO) << "Create MapDataSet";
}

/**
 * MapDataSet destruction.
 */
MapDataSet::~MapDataSet()
{
	unloadData();
}

/**
 * Loads the map data set from a YAML file.
 * @param node - reference a YAML node
 */
void MapDataSet::load(const YAML::Node& node)
{
	for (YAML::const_iterator
			i = node.begin();
			i != node.end();
			++i)
	{
		_name = i->as<std::string>(_name);
	}
}

/**
 * Gets the MapDataSet name string.
 * @return, the MapDataSet name
 */
std::string MapDataSet::getName() const
{
	return _name;
}

/**
 * Gets the MapDataSet size.
 * @return, the size in number of records
 */
size_t MapDataSet::getSize() const
{
	return _objects.size();
}

/**
 * Gets the objects in this dataset.
 * @return, pointer to a vector of pointers to MapData objects
 */
std::vector<MapData*>* MapDataSet::getObjects()
{
	return &_objects;
}

/**
 * Gets the surfaces in this dataset.
 * @return, pointer to the SurfaceSet graphics
 */
SurfaceSet* MapDataSet::getSurfaceset() const
{
	return _surfaceSet;
}

/**
 * Loads terrain data in XCom format - MCD & PCK files.
 * @sa http://www.ufopaedia.org/index.php?title=MCD
 */
void MapDataSet::loadData()
{
	//Log(LOG_INFO) << "MapDataSet::loadData()";
	if (_loaded == true) // prevents loading twice
		return;

	_loaded = true;

	int objNumber = 0;

#pragma pack(push, 1)
	struct MCD // this struct helps to read the .MCD file format
	{
		unsigned char	Frame[8];
		unsigned char	LOFT[12];
		unsigned short	ScanG;
		unsigned char	u23;
		unsigned char	u24;
		unsigned char	u25;
		unsigned char	u26;
		unsigned char	u27;
		unsigned char	u28;
		unsigned char	u29;
		unsigned char	u30;
		unsigned char	UFO_Door;
		unsigned char	Stop_LOS;
		unsigned char	No_Floor;
		unsigned char	Big_Wall;
		unsigned char	Gravlift;
		unsigned char	Door;
		unsigned char	Block_Fire;
		unsigned char	Block_Smoke;
		unsigned char	u39;
		unsigned char	TU_Walk;
//kL	unsigned char	TU_Slide;
		unsigned char	TU_Fly;
		unsigned char	TU_Slide; // kL
		unsigned char	Armor;
		unsigned char	HE_Block;
		unsigned char	Die_MCD;
		unsigned char	Flammable;
		unsigned char	Alt_MCD;
		unsigned char	u48;
		signed char		T_Level;
		unsigned char	P_Level;
		unsigned char	u51;
		unsigned char	Light_Block;
		unsigned char	Footstep;
		unsigned char	Tile_Type;
		unsigned char	HE_Type;
		unsigned char	HE_Strength;
		unsigned char	Smoke_Blockage;
		unsigned char	Fuel;
		unsigned char	Light_Source;
		unsigned char	Target_Type;
		unsigned char	Xcom_Base;
		unsigned char	u62;
	};
#pragma pack(pop)

	MCD mcd;

	// Load Terrain Data from MCD file
	std::ostringstream s;
	s << "TERRAIN/" << _name << ".MCD";

	// Load file
	std::ifstream mapFile ( // init.
						CrossPlatform::getDataFile(s.str()).c_str(),
						std::ios::in | std::ios::binary);
	if (!mapFile)
	{
		throw Exception(s.str() + " not found");
	}

	while (mapFile.read((char*)& mcd, sizeof(MCD)))
	{
		MapData* to = new MapData(this);
		_objects.push_back(to);

		// set all the terrain-object properties:
		for (int
				frame = 0;
				frame < 8;
				++frame)
		{
			to->setSprite(
					frame,
					(int)mcd.Frame[frame]);
		}

		to->setYOffset((int)mcd.P_Level);
		to->setSpecialType(
				(int)mcd.Target_Type,
				(int)mcd.Tile_Type);
		to->setTUCosts(
				(int)mcd.TU_Walk,
				(int)mcd.TU_Fly,
				(int)mcd.TU_Slide);
		to->setFlags(
				mcd.UFO_Door != 0,
				mcd.Stop_LOS != 0,
				mcd.No_Floor != 0,
				(int)mcd.Big_Wall,
				mcd.Gravlift != 0,
				mcd.Door != 0,
				mcd.Block_Fire != 0,
				mcd.Block_Smoke != 0,
				mcd.Xcom_Base != 0);
		to->setTerrainLevel((int)mcd.T_Level);
		to->setFootstepSound((int)mcd.Footstep);
		to->setAltMCD((int)(mcd.Alt_MCD));
		to->setDieMCD((int)(mcd.Die_MCD));
		to->setBlock(
				(int)mcd.Light_Block,
				(int)mcd.Stop_LOS,
				(int)mcd.HE_Block,
				(int)mcd.Block_Smoke,
				(int)mcd.Flammable,
				(int)mcd.HE_Block);
		to->setLightSource((int)mcd.Light_Source);
		to->setArmor((int)mcd.Armor);
		to->setFlammable((int)mcd.Flammable);
		to->setFuel((int)mcd.Fuel);
		to->setExplosiveType((int)mcd.HE_Type);
		to->setExplosive((int)mcd.HE_Strength);

		mcd.ScanG = SDL_SwapLE16(mcd.ScanG);
		to->setMiniMapIndex(mcd.ScanG);

		for (int
				layer = 0;
				layer < 12;
				++layer)
		{
			int loft = (int)mcd.LOFT[layer];
			to->setLoftID(loft, layer);
		}

		// store the 2 tiles of blanks in a static - so they are accessible everywhere
		if (_name.compare("BLANKS") == 0)
		{
			if (objNumber == 0)
				MapDataSet::_blankTile = to;
			else if (objNumber == 1)
				MapDataSet::_scorchedTile = to;
		}

		objNumber++;
	}


	if (mapFile.eof() == false)
	{
		throw Exception("Invalid MCD file");
	}

	mapFile.close();

	// process the MapDataSet to put 'block' values on floortiles (as they don't exist in UFO::Orig)
	for (std::vector<MapData*>::const_iterator
			i = _objects.begin();
			i != _objects.end();
			++i)
	{
		if ((*i)->getObjectType() == MapData::O_FLOOR
			&& (*i)->getBlock(DT_HE) == 0)
		{
			const int armor = (*i)->getArmor();
			(*i)->setBlock(
						1,		// light
						1,		// LoS
						armor,	// HE
						1,		// smoke
						1,		// fire
						1);		// gas, kL
//kL					armor);	// gas

			if ((*i)->getDieMCD())
				_objects.at((*i)->getDieMCD())->setBlock(
														1,
														1,
														armor,
														1,
														1,
														1); // kL
//kL													armor);

			if ((*i)->isGravLift() == false)	// kL
				(*i)->setStopLOS();				// kL
		}
	}

	// Load terrain sprites/surfaces/PCK files into a SurfaceSet.
	// kL_begin: Let extraSprites override terrain sprites.
	//Log(LOG_INFO) << ". terrain_PCK = " << _name;
	std::ostringstream test;
	test << _name << ".PCK";
	SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet(test.str());
	if (srt != NULL)
	{
		//Log(LOG_INFO) << ". . Overriding terrain SurfaceSet:" << _name << ".PCK";
		_surfaceSet = srt;
	}
	else // kL_end.
	{
		//Log(LOG_INFO) << ". . Creating new terrain SurfaceSet:" << _name << ".PCK";
		std::ostringstream
			s1,
			s2;
		s1 << "TERRAIN/" << _name << ".PCK";
		s2 << "TERRAIN/" << _name << ".TAB";

		_surfaceSet = new SurfaceSet(32, 40);
		_surfaceSet->loadPck(
						CrossPlatform::getDataFile(s1.str()),
						CrossPlatform::getDataFile(s2.str()));
	}
}

/**
 * Unloads the terrain data.
 */
void MapDataSet::unloadData()
{
	//Log(LOG_INFO) << "MapDataSet::unloadData()";
	if (_loaded == true)
	{
		for (std::vector<MapData*>::const_iterator
				i = _objects.begin();
				i != _objects.end();
				)
		{
			delete *i;
			i = _objects.erase(i);
		}

		// kL_begin: but don't delete the extraSprites for terrain!!!
		// hm what about deleting only the Pointer?? ie, leave the ResourcePack's surfaceSet intact?
		// Conclusion, seems to work ... but gives an assertion failure in VC++ when exiting game after debugging.
		std::ostringstream test;
		test << _name << ".PCK";
		const SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet(test.str());
		if (srt == NULL)
			delete _surfaceSet;

/*		if (srt != NULL)
		{
			//Log(LOG_INFO) << ". Deleting terrain SurfaceSet POINTER:" << _name << ".PCK";
			delete &_surfaceSet; // Asserion Failure!
		}
		else // kL_end.
		{
			//Log(LOG_INFO) << ". Deleting terrain SurfaceSet:" << _name << ".PCK";
			delete _surfaceSet;
		} */

		_loaded = false;
	}
	//Log(LOG_INFO) << "MapDataSet::unloadData() EXIT";
}

/**
 * Loads the LOFTEMPS.DAT into the ruleset voxeldata.
 * @param filename	- reference the filename of the DAT file
 * @param voxelData	- pointer to a vector of voxelDatums
 */
void MapDataSet::loadLOFTEMPS(
		const std::string& filename,
		std::vector<Uint16>* voxelData)
{
	std::ifstream mapFile( // Load file
						filename.c_str(),
						std::ios::in | std::ios::binary);
	if (!mapFile)
	{
		throw Exception(filename + " not found");
	}

	Uint16 value;

	while (mapFile.read(
						(char*)& value,
						sizeof(value)))
	{
		value = SDL_SwapLE16(value);
		voxelData->push_back(value);
	}

	if (!mapFile.eof())
	{
		throw Exception("Invalid LOFTEMPS");
	}

	mapFile.close();
}

/**
 * Gets a blank floor tile.
 * @return, pointer to blank tile MapData
 */
MapData* MapDataSet::getBlankFloorTile()
{
	return MapDataSet::_blankTile;
}

/**
 * Gets a scorched earth tile.
 * @return, pointer to scorched earth tile MapData
 */
MapData* MapDataSet::getScorchedEarthTile()
{
	return MapDataSet::_scorchedTile;
}

}
