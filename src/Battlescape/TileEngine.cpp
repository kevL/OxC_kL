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

#include "TileEngine.h"

//#define _USE_MATH_DEFINES
//#include <climits>
//#include <cmath>
//#include <functional>
//#include <set>
//#include <assert.h>
//#include <SDL.h>

//#include "../fmath.h"

#include "AlienBAIState.h"
#include "BattleAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "ExplosionBState.h"
#include "InfoBoxOKState.h"
#include "Map.h"
//#include "MeleeAttackBState.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "UnitTurnBState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

const int TileEngine::heightFromCenter[11] =
{
		  0,
	 -2,  2,
	 -4,  4,
	 -6,  6,
	 -8,  8,
	-12, 12
};


/**
 * Sets up a TileEngine.
 * @param battleSave	- pointer to SavedBattleGame object
 * @param voxelData		- pointer to a vector of voxel data
 */
TileEngine::TileEngine(
		SavedBattleGame* battleSave,
		std::vector<Uint16>* voxelData)
	:
		_battleSave(battleSave),
		_voxelData(voxelData),
		_unitLighting(true),
		_powerE(-1),
		_powerT(-1),
		_missileDirection(-1)
{}

/**
 * Deletes the TileEngine.
 */
TileEngine::~TileEngine()
{}

/**
 * Calculates sun shading for the whole terrain.
 */
void TileEngine::calculateSunShading()
{
	const int layer = 0; // Ambient lighting layer.

	for (size_t
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->resetLight(layer);
		calculateSunShading(_battleSave->getTiles()[i]);
	}
}

/**
 * Calculates sun shading for 1 tile. Sun comes from above and is blocked by floors or objects.
 * TODO: angle the shadow according to the time - link to Options::globeSeasons (or whatever the realistic lighting thing is)
 * @param tile - a tile to calculate sun shading for
 */
void TileEngine::calculateSunShading(Tile* const tile)
{
	//Log(LOG_INFO) << "TileEngine::calculateSunShading()";
	const size_t layer = 0; // Ambient lighting layer.
	int light = 15 - _battleSave->getGlobalShade();

	// At night/dusk sun isn't dropping shades blocked by roofs
	if (_battleSave->getGlobalShade() < 5)
	{
		// kL: old code
		if (verticalBlockage(
						_battleSave->getTile(Position(
												tile->getPosition().x,
												tile->getPosition().y,
												_battleSave->getMapSizeZ() - 1)),
						tile,
						DT_NONE) != 0)
		// kL_note: new code
/*		int
			block = 0,
			x = tile->getPosition().x,
			y = tile->getPosition().y;

		for (int
				z = _save->getMapSizeZ() - 1;
				z > tile->getPosition().z;
				--z)
		{
			block += blockage(
							_save->getTile(Position(x, y, z)),
							MapData::O_FLOOR,
							DT_NONE);
			block += blockage(
							_save->getTile(Position(x, y, z)),
							MapData::O_OBJECT,
							DT_NONE,
							Pathfinding::DIR_DOWN);
		}

		if (block > 0) */
		{
			light -= 2;
		}
	}

	tile->addLight(
				light,
				layer);
	//Log(LOG_INFO) << "TileEngine::calculateSunShading() EXIT";
}

/**
 * Recalculates lighting for the terrain: objects, items, fire.
 */
void TileEngine::calculateTerrainLighting()
{
	const size_t layer = 1;		// static lighting layer
	const int fireLight = 15;	// amount of light a fire generates

	for (size_t // reset all light to 0 first
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->resetLight(layer);
	}

	for (size_t // add lighting of terrain
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		// only floors and objects can light up
		if (   _battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR) != NULL
			&& _battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getLightSource() != 0)
		{
			addLight(
				_battleSave->getTiles()[i]->getPosition(),
				_battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getLightSource(),
				layer);
		}

		if (   _battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT) != NULL
			&& _battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)->getLightSource() != 0)
		{
			addLight(
				_battleSave->getTiles()[i]->getPosition(),
				_battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)->getLightSource(),
				layer);
		}

		if (_battleSave->getTiles()[i]->getFire() != 0)
			addLight(
				_battleSave->getTiles()[i]->getPosition(),
				fireLight,
				layer);

		for (std::vector<BattleItem*>::const_iterator
				j = _battleSave->getTiles()[i]->getInventory()->begin();
				j != _battleSave->getTiles()[i]->getInventory()->end();
				++j)
		{
			if ((*j)->getRules()->getBattleType() == BT_FLARE)
				addLight(
					_battleSave->getTiles()[i]->getPosition(),
					(*j)->getRules()->getPower(),
					layer);
		}
	}
}

/**
 * Recalculates lighting for the units.
 */
void TileEngine::calculateUnitLighting()
{
	const size_t layer = 2;	// Dynamic lighting layer.
	const int
		unitLight = 12,	// amount of light a unit generates
		fireLight = 15;	// amount of light a fire generates

	for (size_t // reset all light to 0 first
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->resetLight(layer);
	}

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (_unitLighting // add lighting of soldiers
			&& (*i)->getFaction() == FACTION_PLAYER
			&& (*i)->isOut() == false)
		{
			addLight(
				(*i)->getPosition(),
				unitLight,
				layer);
		}

		if ((*i)->getFireOnUnit() != 0) // add lighting of units on fire
			addLight(
				(*i)->getPosition(),
				fireLight,
				layer);
	}
}

/**
 * Toggles personal lighting on/off.
 */
void TileEngine::togglePersonalLighting()
{
	_unitLighting = !_unitLighting;
	calculateUnitLighting();
}

/**
 * Adds circular light pattern starting from pos and losing power with distance travelled.
 * @param pos	- reference center position
 * @param power	- power of light
 * @param layer	- light is separated in 3 layers: Ambient, Static and Dynamic
 */
void TileEngine::addLight(
		const Position& pos,
		int power,
		size_t layer)
{
	int dist;

	for (int // only loop through the positive quadrant.
			x = 0;
			x != power + 1;
			++x)
	{
		for (int
				y = 0;
				y != power + 1;
				++y)
		{
			for (int
					z = 0;
					z != _battleSave->getMapSizeZ();
					++z)
			{
				dist = power
					 - static_cast<int>(Round(std::sqrt(static_cast<double>(x * x + y * y))));

				Tile* tile = _battleSave->getTile(Position(
														pos.x + x,
														pos.y + y,
														z));
				if (tile != NULL)
					tile->addLight(
								dist,
								layer);

				tile = _battleSave->getTile(Position(
												pos.x - x,
												pos.y - y,
												z));
				if (tile != NULL)
					tile->addLight(
								dist,
								layer);

				tile = _battleSave->getTile(Position(
												pos.x + x,
												pos.y - y,
												z));
				if (tile != NULL)
					tile->addLight(
								dist,
								layer);

				tile = _battleSave->getTile(Position(
												pos.x - x,
												pos.y + y,
												z));
				if (tile != NULL)
					tile->addLight(
								dist,
								layer);
			}
		}
	}
}

/**
 * Calculates line of sight of a BattleUnit.
 * @param unit - pointer to a BattleUnit to check field of view for
 * @return, true when previously concealed units are spotted
 */
bool TileEngine::calculateFOV(BattleUnit* const unit)
{
	unit->clearVisibleUnits();
	unit->clearVisibleTiles();

	if (unit->isOut(true, true) == true)
		return false;

//	if (unit->getFaction() == FACTION_PLAYER)
//	{
//		unit->getTile()->setTileVisible();
//		unit->getTile()->setDiscovered(true, 2);
//	}


	bool ret = false;

	const size_t visUnits_pre = unit->getUnitsSpottedThisTurn().size();

	int dir;
	if (Options::strafe == true
		&& unit->getTurretType() > -1)
	{
		dir = unit->getTurretDirection();
	}
	else
		dir = unit->getDirection();

	const bool swapXY = (dir == 0 || dir == 4);

	const int
		sign_x[8] = { 1, 1, 1, 1,-1,-1,-1,-1},
		sign_y[8] = {-1,-1, 1, 1, 1, 1,-1,-1};

	int
		y1 = 0,
		y2 = 0,
		unitSize,
		block;
	size_t trajLength;

	bool diag;
	if (dir %2)
	{
		diag = true;
		y2 = MAX_VIEW_DISTANCE;
	}
	else
		diag = false;

	std::vector<Position> _trajectory;

	Position
		posUnit = unit->getPosition(),
		posTest,
		pos,
		posTraj;

	Tile
		* tile,
		* tileEdge;

	BattleUnit* spottedUnit;

	if (unit->getHeight()
				+ unit->getFloatHeight()
				- _battleSave->getTile(posUnit)->getTerrainLevel() > 27)
	{
		const Tile* const tileAbove = _battleSave->getTile(posUnit + Position(0,0,1));
		if (tileAbove != NULL
			&& tileAbove->hasNoFloor(NULL))
		{
			++posUnit.z;
		}
	}

	for (int
			x = 0; // does the unit itself really need checking ... Yes, marks own Tile as _visible.
			x <= MAX_VIEW_DISTANCE;
			++x)
	{
		if (diag == false)
		{
			y1 = -x;
			y2 = x;
		}

		for (int
				y = y1;
				y <= y2;
				++y)
		{
			for (int
					z = 0;
					z != _battleSave->getMapSizeZ();
					++z)
			{
				posTest.z = z;

				if (x * x + y * y <= MAX_VIEW_DISTANCE_SQR)
				{
					const int
						deltaPos_x = (sign_x[static_cast<size_t>(dir)] * (swapXY ? y : x)),
						deltaPos_y = (sign_y[static_cast<size_t>(dir)] * (swapXY ? x : y));

					posTest.x = posUnit.x + deltaPos_x;
					posTest.y = posUnit.y + deltaPos_y;

					if (_battleSave->getTile(posTest) != NULL)
					{
						spottedUnit = _battleSave->getTile(posTest)->getUnit();
						if (spottedUnit != NULL
							&& spottedUnit->isOut(true, true) == false
							&& visible(
									unit,
									_battleSave->getTile(posTest)) == true)
						{
							if (spottedUnit->getUnitVisible() == false)
								ret = true;

							if (unit->getFaction() == FACTION_PLAYER)
							{
								spottedUnit->setUnitVisible();
								spottedUnit->getTile()->setTileVisible();
							}

							if ((unit->getFaction() == FACTION_PLAYER
									&& spottedUnit->getFaction() == FACTION_HOSTILE)
								|| (unit->getFaction() == FACTION_HOSTILE
									&& spottedUnit->getFaction() != FACTION_HOSTILE))
							{
								// adds spottedUnit to _visibleUnits *and* to _unitsSpottedThisTurn:
								unit->addToVisibleUnits(spottedUnit);
								unit->addToVisibleTiles(spottedUnit->getTile());

								if (_battleSave->getSide() == FACTION_HOSTILE
									&& unit->getFaction() == FACTION_HOSTILE
									&& spottedUnit->getFaction() != FACTION_HOSTILE)
								{
									//Log(LOG_INFO) << "calculateFOV() id " << unit->getId() << " spots " << spottedUnit->getId();
									spottedUnit->setExposed();	// note that xCom agents can be seen by enemies but *not* become Exposed.
																// Only potential reactionFire should set them Exposed during xCom's turn.
								}
							}
						}

						if (unit->getFaction() == FACTION_PLAYER) // reveal extra tiles ->>
						{
							// this sets tiles to discovered if they are in FoV -
							// tile visibility is not calculated in voxelspace but in tilespace;
							// large units have "4 pair of eyes"
							unitSize = unit->getArmor()->getSize();
							for (int
									size_x = 0;
									size_x != unitSize;
									++size_x)
							{
								for (int
										size_y = 0;
										size_y != unitSize;
										++size_y)
								{
									_trajectory.clear();

									pos = posUnit + Position(
															size_x,
															size_y,
															0);
									block = calculateLine(
														pos,
														posTest,
														true,
														&_trajectory,
														unit,
														false);

									trajLength = _trajectory.size();

//kL								if (block > 127) // last tile is blocked thus must be cropped
									if (block > 0)	// kL: -1 - do NOT crop trajectory (ie. hit content-object)
													//		0 - expose Tile ( should never return this, unless out-of-bounds )
													//		1 - crop the trajectory ( hit regular wall )
										--trajLength;

									for (size_t
											i = 0;
											i != trajLength;
											++i)
									{
										posTraj = _trajectory.at(i);

										// mark every tile of line as visible (this is needed because of bresenham narrow stroke).
										tile = _battleSave->getTile(posTraj);
										tile->setTileVisible();
										tile->setDiscovered(true, 2); // sprite caching for floor+content, ergo + west & north walls.

										// walls to the east or south of a visible tile, we see that too
										// kL_note: Yeh, IF there's walls or an appropriate BigWall object!
										/*	parts:
											#0 - floor
											#1 - westwall
											#2 - northwall
											#3 - object (content) */
										/* discovered:
											#0 - westwall
											#1 - northwall
											#2 - floor + content (reveals both walls also) */
										tileEdge = _battleSave->getTile(Position(
																			posTraj.x + 1,
																			posTraj.y,
																			posTraj.z));
										if (tileEdge != NULL) // show Tile EAST
//kL										tileEdge->setDiscovered(true, 0);
										{
											if (tile->getMapData(MapData::O_OBJECT) == NULL
												|| (tile->getMapData(MapData::O_OBJECT)
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_BLOCK
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_EAST
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_E_S))
											{
												for (int
														part = 1;
														part < 4;
														part += 2)
												{
													if (tileEdge->getMapData(part))
													{
														if (tileEdge->getMapData(part)->getObjectType() == MapData::O_WESTWALL) // #1
															tileEdge->setDiscovered(true, 0); // reveal westwall
														else if (tileEdge->getMapData(part)->getObjectType() == MapData::O_OBJECT // #3
															&& tileEdge->getMapData(MapData::O_OBJECT)
															&& (tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
																|| tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_WEST))
														{
															tileEdge->setDiscovered(true, 2); // reveal entire tileEdge
														}
													}
												}
											}
										}

										tileEdge = _battleSave->getTile(Position(
																			posTraj.x,
																			posTraj.y + 1,
																			posTraj.z));
										if (tileEdge != NULL) // show Tile SOUTH
//kL										tileEdge->setDiscovered(true, 1);
										{
											if (tile->getMapData(MapData::O_OBJECT) == NULL
												|| (tile->getMapData(MapData::O_OBJECT)
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_BLOCK
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_SOUTH
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_E_S))
											{
												for (int
														part = 2;
														part < 4;
														++part)
												{
													if (tileEdge->getMapData(part))
													{
														if (tileEdge->getMapData(part)->getObjectType() == MapData::O_NORTHWALL) // #2
															tileEdge->setDiscovered(true, 1); // reveal northwall
														else if (tileEdge->getMapData(part)->getObjectType() == MapData::O_OBJECT // #3
															&& tileEdge->getMapData(MapData::O_OBJECT)
															&& (tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
																|| tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NORTH))
														{
															tileEdge->setDiscovered(true, 2); // reveal entire tileEdge
														}
													}
												}
											}
										}

										tileEdge = _battleSave->getTile(Position(
																			posTraj.x - 1,
																			posTraj.y,
																			posTraj.z));
										if (tileEdge != NULL) // show Tile WEST
										{
											if (tile->getMapData(MapData::O_WESTWALL) == NULL
												|| (tile->getMapData(MapData::O_OBJECT)
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_BLOCK
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_WEST))
											{
												if (tileEdge->getMapData(MapData::O_OBJECT) // #3
													&& (tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
														|| tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_EAST
														|| tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_E_S))
												{
													tileEdge->setDiscovered(true, 2); // reveal entire tileEdge
												}
											}
										}

										tileEdge = _battleSave->getTile(Position(
																			posTraj.x,
																			posTraj.y - 1,
																			posTraj.z));
										if (tileEdge != NULL) // show Tile NORTH
										{
											if (tile->getMapData(MapData::O_NORTHWALL) == NULL
												|| (tile->getMapData(MapData::O_OBJECT)
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_BLOCK
													&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_NORTH))
											{
												if (tileEdge->getMapData(MapData::O_OBJECT) // #3
													&& (tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
														|| tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_SOUTH
														|| tileEdge->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_E_S))
												{
													tileEdge->setDiscovered(true, 2); // reveal entire tileEdge
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (unit->getFaction() == FACTION_PLAYER
		&& ret == true)
	{
		return true;
	}

	if (unit->getFaction() != FACTION_PLAYER
		&& unit->getVisibleUnits()->empty() == false
		&& unit->getUnitsSpottedThisTurn().size() > visUnits_pre)
	{
		return true;
	}

	return false;
}

/**
 * Calculates line of sight of all units within range of the Position.
 * Used when terrain has changed, which can reveal unseen units and/or parts of terrain.
 * @param pos - reference the position of the changed terrain
 */
void TileEngine::calculateFOV(const Position& pos)
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (distanceSq(
					pos,
					(*i)->getPosition(),
					false) < MAX_VIEW_DISTANCE_SQR + 1)
		{
			calculateFOV(*i);
		}
	}
}

/**
 * Recalculates FOV of all conscious units on the battlefield.
 */
void TileEngine::recalculateFOV()
{
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getTile() != NULL)
			calculateFOV(*i);
	}
}

/**
 * Checks for an opposing unit on a tile.
 * @param unit - pointer a BattleUnit that's looking at tile
 * @param tile - pointer to a Tile that unit is looking at
 * @return, true if a unit is on that tile and is seen
 */
bool TileEngine::visible(
		const BattleUnit* const unit,
		const Tile* const tile)
{
	if (tile == NULL
		|| tile->getUnit() == NULL)
	{
		return false;
	}

	const BattleUnit* const targetUnit = tile->getUnit();
	if (targetUnit->isOut(true, true) == true)
		return false;

	if (unit->getFaction() == targetUnit->getFaction())
		return true;


	const int distTiles = distance(
								unit->getPosition(),
								targetUnit->getPosition());
	if (distTiles * distTiles > MAX_VIEW_DISTANCE_SQR)
		return false;

	if (unit->getFaction() == FACTION_PLAYER
		&& tile->getShade() > MAX_SHADE_TO_SEE_UNITS
		&& distTiles > 23 - _battleSave->getGlobalShade())
	{
		return false;
	}


	// for large units origin voxel is in the middle ( not anymore )
	// kL_note: this leads to problems with large units trying to shoot around corners, b.t.w.
	// because it might See with a clear LoS, but the LoF is taken from a different, offset voxel.
	// further, i think Lines of Sight and Fire determinations are getting mixed up somewhere!!!
	const Position originVoxel = getSightOriginVoxel(unit);
	Position scanVoxel;
	std::vector<Position> _trajectory;

	// kL_note: Is an intermediary object *not* obstructing viewing
	// or targetting, when it should be?? Like, around corners?
	// kL_later: it might have something to do with an offset for handedness re. LoF.
	bool isSeen = canTargetUnit(
							&originVoxel,
							tile,
							&scanVoxel,
							unit);
	if (isSeen == true)
	{
		// now check if we really see it taking into account smoke tiles
		// initial smoke "density" of a smoke grenade is around 15 per tile
		// we do density/3 to get the decay of visibility
		// so in fresh smoke we should only have 4 tiles of visibility
		// this is traced in voxel space, with smoke affecting visibility every step of the way
		// kL_note: well not really, not until I floatified it.....
		_trajectory.clear();

		calculateLine(
					originVoxel,
					scanVoxel,
					true,
					&_trajectory,
					unit);


		const Tile* scanTile = _battleSave->getTile(unit->getPosition());
		double distWeighted = static_cast<double>(_trajectory.size());

		for (size_t
				i = 0;
				i != _trajectory.size();
				++i)
		{
			// The 'origin tile' now steps along through voxel/tile-space, picking up extra weight
			// as it goes ( effective distance is reduced by real distance * obscuration-per-voxel ).
			scanTile = _battleSave->getTile(Position(
												_trajectory.at(i).x / 16,
												_trajectory.at(i).y / 16,
												_trajectory.at(i).z / 24));

			distWeighted += static_cast<double>(scanTile->getSmoke()) / 3.;
			distWeighted += static_cast<double>(scanTile->getFire());

			if (static_cast<int>(std::ceil(distWeighted * distWeighted)) > MAX_VOXEL_VIEW_DIST_SQR)
//			if (static_cast<int>(std::ceil(distWeighted)) > MAX_VOXEL_VIEW_DISTANCE)
			{
				isSeen = false;
				break;
			}
		}

		// Check if unitSeen (the unit at the end of calculateLine() trajectory) is really The targetUnit.
		if (isSeen == true)
		{
			// have to check if targetUnit is poking its head up from tileBelow
			const Tile* const tileBelow = _battleSave->getTile(scanTile->getPosition() + Position(0,0,-1));
			if (!
				(scanTile->getUnit() == targetUnit
					|| (tileBelow != NULL // could add a check for && scanTile->hasNoFloor() around here.
						&& tileBelow->getUnit() != NULL
						&& tileBelow->getUnit() == targetUnit)))
			{
				isSeen = false;
			}
		}
	}

	return isSeen;
}

/**
 * Gets the origin voxel of a unit's LoS.
 * @param unit - the watcher
 * @return, approximately an eyeball voxel
 */
Position TileEngine::getSightOriginVoxel(const BattleUnit* const unit)
{
	// determine the origin (and target) voxels for calculations
	Position originVoxel = Position(
								unit->getPosition().x * 16 + 8,
								unit->getPosition().y * 16 + 8,
								unit->getPosition().z * 24);

	originVoxel.z += unit->getHeight()
				   + unit->getFloatHeight()
				   - _battleSave->getTile(unit->getPosition())->getTerrainLevel()
				   - 2; // two voxels lower (nose level)
		// kL_note: Can make this equivalent to LoF origin, perhaps.....
		// hey, here's an idea: make Snaps & Auto shoot from hip, Aimed from shoulders or eyes.

	const Tile* const tileAbove = _battleSave->getTile(unit->getPosition() + Position(0,0,1));

	// kL_note: let's stop this. Tanks appear to make their FoV etc. Checks from all four quadrants anyway.
/*	if (unit->getArmor()->getSize() > 1)
	{
		originVoxel.x += 8;
		originVoxel.y += 8;
		originVoxel.z += 1; // topmost voxel
	} */

	if (originVoxel.z >= (unit->getPosition().z + 1) * 24
		&& (tileAbove == NULL
			|| tileAbove->hasNoFloor(NULL) == false))
	{
		while (originVoxel.z >= (unit->getPosition().z + 1) * 24)
		{
			--originVoxel.z; // careful with that ceiling, Eugene.
		}
	}

	return originVoxel;
}

/**
 * Checks for how exposed unit is for another unit.
 * @param originVoxel	- voxel of trace origin (eye or gun's barrel)
 * @param tile			- pointer to Tile to check for
 * @param excludeUnit	- is self (not to hit self)
 * @param excludeAllBut	- [optional] is unit which is the only one to be considered for ray hits
 * @return, degree of exposure (as percent)
 */
/*kL int TileEngine::checkVoxelExposure(
		Position* originVoxel,
		Tile* tile,
		BattleUnit* excludeUnit,
		BattleUnit* excludeAllBut)
{
	Position targetVoxel = Position(
			tile->getPosition().x * 16 + 7,
			tile->getPosition().y * 16 + 8,
			tile->getPosition().z * 24);
	Position scanVoxel;
	std::vector<Position> _trajectory;

	BattleUnit* otherUnit = tile->getUnit();

	if (otherUnit == 0)
		return 0; // no unit in this tile, even if it elevated and appearing in it.

	if (otherUnit == excludeUnit)
		return 0; // skip self

	int targetMinHeight = targetVoxel.z - tile->getTerrainLevel();
	if (otherUnit)
		 targetMinHeight += otherUnit->getFloatHeight();

	// if there is an other unit on target tile, we assume we want to check against this unit's height
	int heightRange;

	int unitRadius = otherUnit->getLoftemps(); // width == loft in default loftemps set
	if (otherUnit->getArmor()->getSize() > 1)
	{
		unitRadius = 3;
	}

	// vector manipulation to make scan work in view-space
	Position relPos = targetVoxel - *originVoxel;
	float normal = static_cast<float>(unitRadius) / sqrt(static_cast<float>(relPos.x * relPos.x + relPos.y * relPos.y));
	int relX = static_cast<int>(floor(static_cast<float>(relPos.y) * normal + 0.5f));
	int relY = static_cast<int>(floor(static_cast<float>(-relPos.x) * normal + 0.5f));

	int sliceTargets[10] = // looks like [6] to me..
	{
		0,		0,
		relX,	relY,
		-relX,	-relY
	};
/*	int sliceTargets[10] = // taken from "canTargetUnit()"
	{
		0,		0,
		relX,	relY,
		-relX,	-relY,
		relY,	-relX,
		-relY,	relX
	}; */

/*	if (!otherUnit->isOut())
	{
		heightRange = otherUnit->getHeight();
	}
	else
	{
		heightRange = 12;
	}

	int targetMaxHeight = targetMinHeight+heightRange;
	// scan ray from top to bottom plus different parts of target cylinder
	int total = 0;
	int visible = 0;

	for (int i = heightRange; i >= 0; i -= 2)
	{
		++total;

		scanVoxel.z = targetMinHeight + i;
		for (int j = 0; j < 2; ++j)
		{
			scanVoxel.x = targetVoxel.x + sliceTargets[j * 2];
			scanVoxel.y = targetVoxel.y + sliceTargets[j * 2 + 1];

			_trajectory.clear();

			int test = calculateLine(*originVoxel, scanVoxel, false, &_trajectory, excludeUnit, true, false, excludeAllBut);
			if (test == VOXEL_UNIT)
			{
				// voxel of hit must be inside of scanned box
				if (_trajectory.at(0).x / 16 == scanVoxel.x / 16
					&& _trajectory.at(0).y / 16 == scanVoxel.y / 16
					&& _trajectory.at(0).z >= targetMinHeight
					&& _trajectory.at(0).z <= targetMaxHeight)
				{
					++visible;
				}
			}
		}
	}

	return visible * 100 / total;
} */

/**
 * Checks for another unit available for targeting and what particular voxel.
 * @param originVoxel	- pointer to voxel of trace origin (eg. gun's barrel)
 * @param tile			- pointer to Tile to check for
 * @param scanVoxel		- pointer to voxel that is returning coordinate of hit
 * @param excludeUnit	- pointer to unitSelf (to not hit self)
 * @param unit			- pointer to a hypothetical unit to draw a virtual LoF for AI;
							if left NULL this function behaves normally (default NULL)
 * @return, true if a unit can be targeted
 */
bool TileEngine::canTargetUnit(
		const Position* const originVoxel,
		const Tile* const tile,
		Position* const scanVoxel,
		const BattleUnit* const excludeUnit,
		const BattleUnit* unit)
{
	//Log(LOG_INFO) << "TileEngine::canTargetUnit()";
	const bool hypothetical = (unit != NULL);

	if (unit == NULL)
	{
		unit = tile->getUnit();
		if (unit == NULL)
		{
			//Log(LOG_INFO) << ". no Unit, ret FALSE";
			return false; // no unit in this tile, even if it's elevated and appearing in it.
		}
	}

	if (unit == excludeUnit) // skip self
	{
		//Log(LOG_INFO) << ". hit vs Self, ret FALSE";
		return false;
	}

	const Position targetVoxel = Position(
										tile->getPosition().x * 16 + 8,
										tile->getPosition().y * 16 + 8,
										tile->getPosition().z * 24);
	const int
		targetMinHeight = targetVoxel.z
						- tile->getTerrainLevel()
						+ unit->getFloatHeight(),
		// if there is an other unit on target tile, assume check against that unit's height
		xOffset = unit->getPosition().x - tile->getPosition().x,
		yOffset = unit->getPosition().y - tile->getPosition().y,
		targetSize = unit->getArmor()->getSize() - 1;
	int
		unitRadius = unit->getLoftemps(), // width == loft in default loftemps set
		targetMaxHeight = targetMinHeight,
		targetCenterHeight,
		heightRange;

	if (targetSize > 0)
		unitRadius = 3;

	// vector manipulation to make scan work in voxelspace
	const Position relPos = targetVoxel - *originVoxel;
	const float unitCenter = static_cast<float>(unitRadius)
						   / std::sqrt(static_cast<float>((relPos.x * relPos.x) + (relPos.y * relPos.y)));
	const int
//		relX = static_cast<int>(std::floor(static_cast<float>( relPos.y) * unitCenter + 0.5f)),
//		relY = static_cast<int>(std::floor(static_cast<float>(-relPos.x) * unitCenter + 0.5f)),
		relX = static_cast<int>(Round(std::floor(static_cast<float>( relPos.y) * unitCenter))),
		relY = static_cast<int>(Round(std::floor(static_cast<float>(-relPos.x) * unitCenter))),
		sliceTargets[10] =
		{
			 0,		 0,
			 relX,	 relY,
			-relX,	-relY,
			 relY,	-relX,
			-relY,	 relX
		};

	if (unit->isOut() == false)
		heightRange = unit->getHeight();
	else
		heightRange = 12;

	targetMaxHeight += heightRange;
	targetCenterHeight = (targetMaxHeight + targetMinHeight) / 2;
	heightRange /= 2;
	if (heightRange > 10) heightRange = 10;
	if (heightRange < 0) heightRange = 0;

	std::vector<Position> _trajectory;

	for (int // scan ray from top to bottom plus different parts of target cylinder
			i = 0;
			i <= heightRange;
			++i)
	{
		scanVoxel->z = targetCenterHeight + heightFromCenter[static_cast<size_t>(i)];
		//Log(LOG_INFO) << ". iterate heightRange, i = " << i << ", scan.Z = " << scanVoxel->z;

		for (size_t
				j = 0;
				j != 5;
				++j)
		{
			//Log(LOG_INFO) << ". . iterate j = " << j;
			if (i < heightRange - 1
				&& j > 2)
			{
				//Log(LOG_INFO) << ". . . out of bounds, break (j)";
				break; // skip unnecessary checks
			}

			scanVoxel->x = targetVoxel.x + sliceTargets[j * 2];
			scanVoxel->y = targetVoxel.y + sliceTargets[j * 2 + 1];
			//Log(LOG_INFO) << ". . scan.X = " << scanVoxel->x;
			//Log(LOG_INFO) << ". . scan.Y = " << scanVoxel->y;

			_trajectory.clear();

			const int test = calculateLine(
										*originVoxel,
										*scanVoxel,
										false,
										&_trajectory,
										excludeUnit);
			//Log(LOG_INFO) << ". . testLine = " << test;

			if (test == VOXEL_UNIT)
			{
				for (int // voxel of hit must be inside of scanned box
						x = 0;
						x <= targetSize;
						++x)
				{
					//Log(LOG_INFO) << ". . . iterate x-Size";
					for (int
							y = 0;
							y <= targetSize;
							++y)
					{
						//Log(LOG_INFO) << ". . . iterate y-Size";
						if (   _trajectory.at(0).x / 16 == (scanVoxel->x / 16) + x + xOffset
							&& _trajectory.at(0).y / 16 == (scanVoxel->y / 16) + y + yOffset
							&& _trajectory.at(0).z >= targetMinHeight
							&& _trajectory.at(0).z <= targetMaxHeight)
						{
							//Log(LOG_INFO) << ". . . . Unit found @ voxel, ret TRUE";
							return true;
						}
					}
				}
			}
			else if (test == VOXEL_EMPTY
				&& hypothetical == true
				&& _trajectory.empty() == false)
			{
				//Log(LOG_INFO) << ". . hypothetical Found, ret TRUE";
				return true;
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::canTargetUnit() exit FALSE";
	return false;
}

/**
 * Checks for a tile part available for targeting and what particular voxel.
 * @param originVoxel	- pointer to the Position voxel of trace origin (eg. gun's barrel)
 * @param tile			- pointer to the Tile to check for
 * @param part			- tile part to check for
 * @param scanVoxel		- pointer to voxel that is returned coordinate of hit
 * @param excludeUnit	- pointer to unitSelf (to not hit self)
 * @return, true if tile can be targeted
 */
bool TileEngine::canTargetTile(
		const Position* const originVoxel,
		const Tile* const tile,
		const int part,
		Position* const scanVoxel,
		const BattleUnit* const excludeUnit)
{
	static int
		sliceObjectSpiral[82] =
		{
			8,8,  8,6, 10,6, 10,8, 10,10, 8,10,  6,10,  6,8,  6,6,											// first circle
			8,4, 10,4, 12,4, 12,6, 12,8, 12,10, 12,12, 10,12, 8,12, 6,12, 4,12, 4,10, 4,8, 4,6, 4,4, 6,4,	// second circle
			8,1, 12,1, 15,1, 15,4, 15,8, 15,12, 15,15, 12,15, 8,15, 4,15, 1,15, 1,12, 1,8, 1,4, 1,1, 4,1	// third circle
		},
		northWallSpiral[14] =
		{
			7,0, 9,0, 6,0, 11,0, 4,0, 13,0, 2,0
		},
		westWallSpiral[14] =
		{
			0,7, 0,9, 0,6, 0,11, 0,4, 0,13, 0,2
		};

	Position targetVoxel = Position(
								tile->getPosition().x * 16,
								tile->getPosition().y * 16,
								tile->getPosition().z * 24);

	std::vector<Position> _trajectory;

	int
		* spiralArray,
		minZ = 0,
		maxZ = 0;
	size_t spiralCount;
	bool
		minZfound = false,
		maxZfound = false;


	if (part == MapData::O_OBJECT)
	{
		spiralArray = sliceObjectSpiral;
		spiralCount = 41;
	}
	else if (part == MapData::O_NORTHWALL)
	{
		spiralArray = northWallSpiral;
		spiralCount = 7;
	}
	else if (part == MapData::O_WESTWALL)
	{
		spiralArray = westWallSpiral;
		spiralCount = 7;
	}
	else if (part == MapData::O_FLOOR)
	{
		spiralArray = sliceObjectSpiral;
		spiralCount = 41;
		minZfound =
		maxZfound = true;
		minZ =
		maxZ = 0;
	}
	else
	{
		//Log(LOG_INFO) << "TileEngine::canTargetTile() EXIT, ret False (part is not a tileObject)";
		return false;
	}

	if (minZfound == false) // find out height range
	{
		for (int
				j = 1;
				j != 12;
				++j)
		{
			if (minZfound == true)
				break;

			for (size_t
					i = 0;
					i != spiralCount;
					++i)
			{
				int
					tX = spiralArray[i * 2],
					tY = spiralArray[i * 2 + 1];

				if (voxelCheck(
							Position(
									targetVoxel.x + tX,
									targetVoxel.y + tY,
									targetVoxel.z + j * 2),
							NULL,
							true) == part) // bingo
				{
					if (minZfound == false)
					{
						minZ = j * 2;
						minZfound = true;

						break;
					}
				}
			}
		}
	}

	if (minZfound == false)
		return false; // empty object!!!

	if (maxZfound == false)
	{
		for (int
				j = 10;
				j != -1;
				--j)
		{
			if (maxZfound == true)
				break;

			for (size_t
					i = 0;
					i != spiralCount;
					++i)
			{
				int
					tX = spiralArray[i * 2],
					tY = spiralArray[i * 2 + 1];

				if (voxelCheck(
							Position(
									targetVoxel.x + tX,
									targetVoxel.y + tY,
									targetVoxel.z + j * 2),
							NULL,
							true) == part) // bingo
				{
					if (maxZfound == false)
					{
						maxZ = j * 2;
						maxZfound = true;

						break;
					}
				}
			}
		}
	}

	if (maxZfound == false)
		return false; // it's impossible to get there

	if (minZ > maxZ)
		minZ = maxZ;

	const int
		rangeZ = maxZ - minZ,
		centerZ = (maxZ + minZ) / 2;

	for (int
			j = 0;
			j != rangeZ + 1;
			++j)
	{
		scanVoxel->z = targetVoxel.z + centerZ + heightFromCenter[static_cast<size_t>(j)];

		for (size_t
				i = 0;
				i != spiralCount;
				++i)
		{
			scanVoxel->x = targetVoxel.x + spiralArray[i * 2];
			scanVoxel->y = targetVoxel.y + spiralArray[i * 2 + 1];

			_trajectory.clear();

			const int test = calculateLine(
										*originVoxel,
										*scanVoxel,
										false,
										&_trajectory,
										excludeUnit);
			if (test == part) // bingo
			{
				if (   _trajectory.at(0).x / 16 == scanVoxel->x / 16
					&& _trajectory.at(0).y / 16 == scanVoxel->y / 16
					&& _trajectory.at(0).z / 24 == scanVoxel->z / 24)
				{
					return true;
				}
			}
		}
	}

	return false;
}

/**
 * Checks if a 'sniper' from the opposing faction sees this unit.
 * @note The unit with the highest reaction score will be compared with the
 * triggering unit's reaction score. If it's higher a shot is fired when enough
 * time units plus a weapon and ammo are available.
 * @note The tuSpent parameter is needed because popState() doesn't subtract TU
 * until after the Initiative has been calculated or called from
 * ProjectileFlyBState.
 * @param unit		- pointer to a unit to check reaction fire against
 * @param tuSpent	- the unit's triggering expenditure of TU if firing or throwing.
 * @return, true if reaction fire took place
 */
bool TileEngine::checkReactionFire(
		BattleUnit* unit,
		int tuSpent)
{
	//Log(LOG_INFO) << "TileEngine::checkReactionFire() vs targetID " << unit->getId();
	//Log(LOG_INFO) << ". tuSpent = " << tuSpent;
	if (_battleSave->getSide() == FACTION_NEUTRAL) // no reaction on civilian turn.
		return false;


	// trigger reaction fire only when the spotted unit is of the
	// currently playing side, and is still on the map, alive
	if (unit->getFaction() != _battleSave->getSide()
		|| unit->getTile() == NULL
		|| unit->isOut(true, true) == true)
	{
		return false;
	}

	bool ret = false;

	if (unit->getFaction() == unit->getOriginalFaction()
		|| unit->getFaction() == FACTION_PLAYER)
	{
		//Log(LOG_INFO) << ". Target = VALID";
		std::vector<BattleUnit*> spotters = getSpottingUnits(unit);
		//Log(LOG_INFO) << ". # spotters = " << spotters.size();

		BattleUnit* reactor = getReactor( // get the first man up to bat.
									spotters,
									unit,
									tuSpent);
		// start iterating through the possible reactors until
		// the current unit is the one with the highest score.
		while (reactor != unit)
		{
			// !!!!!SHOOT!!!!!!!!
			if (reactionShot(reactor, unit) == false)
			{
				//Log(LOG_INFO) << ". . no Snap by : " << reactor->getId();
				// can't make a reaction snapshot for whatever reason, boot this guy from the vector.
				for (std::vector<BattleUnit*>::const_iterator
						i = spotters.begin();
						i != spotters.end();
						++i)
				{
					if (*i == reactor)
					{
						spotters.erase(i);
						break;
					}
				}
			}
			else
			{
				if (reactor->getGeoscapeSoldier() != NULL
					&& reactor->getFaction() == reactor->getOriginalFaction())
				{
					//Log(LOG_INFO) << ". . reactionXP to " << reactor->getId();
					reactor->addReactionExp();
				}

				//Log(LOG_INFO) << ". . Snap by : " << reactor->getId();
				ret = true;
			}

			reactor = getReactor( // nice shot. Get cocky, kid; you're a neurotic ass. Gratz!
								spotters,
								unit,
								tuSpent);
			//Log(LOG_INFO) << ". . NEXT AT BAT : " << reactor->getId();
		}

		spotters.clear();
	}

	return ret;
}

/**
 * Creates a vector of BattleUnits that can spot 'unit'.
 * @param unit - pointer to a BattleUnit to spot
 * @return, vector of pointers to BattleUnits that can see the triggering unit
 */
std::vector<BattleUnit*> TileEngine::getSpottingUnits(BattleUnit* const unit)
{
	//Log(LOG_INFO) << "TileEngine::getSpottingUnits() vs. ID " << unit->getId();
	const Tile* const tile = unit->getTile();
	std::vector<BattleUnit*> spotters;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getTimeUnits() != 0
			&& (*i)->getFaction() != _battleSave->getSide()
			&& (*i)->getFaction() != FACTION_NEUTRAL
			&& (*i)->isOut(true, true) == false)
		{
			if (((*i)->getFaction() == FACTION_HOSTILE					// Mc'd xCom units will RF on loyal xCom units
					|| ((*i)->getOriginalFaction() == FACTION_PLAYER	// but Mc'd aLiens won't RF on other aLiens ...
						&& (*i)->checkViewSector(unit->getPosition()) == true))
				&& visible(*i, tile) == true)
			{
				spotters.push_back(*i);
			}
		}
	}

	return spotters;
}

/**
 * Gets the unit with the highest reaction score from the spotters vector.
 * NOTE: the tuSpent parameter is needed because popState() doesn't
 * subtract TU until after the Initiative has been calculated or called from ProjectileFlyBState.
 * @param spotters	- vector of the pointers to spotting BattleUnits
 * @param defender	- pointer to the defending BattleUnit to check reaction scores against
 * @param tuSpent	- defending BattleUnit's expenditure of TU that had caused reaction checks
 * @return, pointer to BattleUnit with the initiative (next up!)
 */
BattleUnit* TileEngine::getReactor(
		std::vector<BattleUnit*> spotters,
		BattleUnit* const defender,
		const int tuSpent) const
{
	//Log(LOG_INFO) << "TileEngine::getReactor() vs ID " << defender->getId();
	//Log(LOG_INFO) << ". tuSpent = " << tuSpent;
	BattleUnit* nextReactor = NULL;
	int
		init = -1,
		initTest;

	for (std::vector<BattleUnit*>::const_iterator
			i = spotters.begin();
			i != spotters.end();
			++i)
	{
		//Log(LOG_INFO) << ". . check nextReactor ID " << (*i)->getId();
		if ((*i)->isOut() == false)
		{
			initTest = static_cast<int>((*i)->getInitiative());
			if (initTest > init)
			{
				init = initTest;
				nextReactor = *i;
			}
		}
	}

	//Log(LOG_INFO) << ". ID " << defender->getId() << " initi = " << static_cast<int>(defender->getInitiative(tuSpent));

	// nextReactor has to *best* defender.Init to get initiative
	// Analysis: It appears that defender's tu for firing/throwing
	// are not subtracted before getInitiative() is called.

	if (nextReactor == NULL
		|| init <= static_cast<int>(defender->getInitiative(tuSpent)))
	{
		nextReactor = defender;
	}

	if (nextReactor != defender
		&& nextReactor->getFaction() == FACTION_HOSTILE)
	{
		//Log(LOG_INFO) << "getReactor() id " << nextReactor->getId() << " spots " << defender->getId();
		defender->setExposed(); // defender has been spotted on Player turn.
	}

	//Log(LOG_INFO) << ". init = " << init;
	return nextReactor;
}

/**
 * Fires off a reaction shot.
 * @param unit			- pointer to the spotting unit
 * @param targetUnit	- pointer to the spotted unit
 * @return, true if a shot happens
 */
bool TileEngine::reactionShot(
		BattleUnit* const unit,
		const BattleUnit* const targetUnit)
{
//	if (targetUnit->isOut(true, true) == true) return false; // not gonna stop shit.

	BattleAction action;
	action.actor = unit;

	if (unit->getFaction() == FACTION_PLAYER
		&& unit->getOriginalFaction() == FACTION_PLAYER)
	{
		action.weapon = unit->getItem(unit->getActiveHand());
	}
	else
	{
		action.weapon = unit->getMainHandWeapon();

		if (action.weapon == NULL
			&& action.actor->getUnitRules() != NULL
			&& action.actor->getUnitRules()->getMeleeWeapon() == "STR_FIST")
		{
			action.weapon = _battleSave->getBattleGame()->getFist();
		}
	}

	if (action.weapon == NULL
		|| action.weapon->getRules()->canReactionFire() == false
		|| action.weapon->getAmmoItem() == NULL						// lasers & melee are their own ammo-items
		|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0		// lasers & melee return 255
		|| (_battleSave->getDepth() == 0
			&& action.weapon->getRules()->isWaterOnly() == true)
		|| (action.actor->getOriginalFaction() != FACTION_HOSTILE	// is not an aLien and has unresearched weapon.
			&& _battleSave->getGeoscapeSave()->isResearched(action.weapon->getRules()->getRequirements()) == false))
	{
		return false;
	}


	action.target = targetUnit->getPosition();
	action.TU = 0;

	if (action.weapon->getRules()->getBattleType() == BT_MELEE)
	{
		action.type = BA_HIT;
		action.TU = action.actor->getActionTUs(
											BA_HIT,
											action.weapon);
		if (action.TU == 0
			|| action.TU > action.actor->getTimeUnits())
		{
			return false;
		}

		bool canMelee = false;
		for (int
				i = 0;
				i != 8
					&& canMelee == false;
				++i)
		{
			canMelee = validMeleeRange(			// hopefully this is blocked by walls & bigWalls ...
									unit,		// see also, AI do_grenade_action .....
									targetUnit,	// darn Sectoid tried to hurl a grenade through a northwall .. with *no LoS*
									i);			// cf. ActionMenuState::btnActionMenuItemClick()
		}

		if (canMelee == false)
			return false;
	}
	else
	{
		action.type = BA_NONE;
		selectFireMethod(action); // choose BAT & setTU req'd.

		if (action.type == BA_NONE)
			return false;
	}

	action.targeting = true;

	if (unit->getFaction() == FACTION_HOSTILE)
	{
		AlienBAIState* aggro_AI = dynamic_cast<AlienBAIState*>(unit->getCurrentAIState());
		if (aggro_AI == NULL)
		{
			aggro_AI = new AlienBAIState(
									_battleSave,
									unit,
									NULL);
			unit->setAIState(aggro_AI);
		}

		if (action.weapon->getAmmoItem()->getRules()->getExplosionRadius() > 0
			&& aggro_AI->explosiveEfficacy(
									action.target,
									unit,
									action.weapon->getAmmoItem()->getRules()->getExplosionRadius(),
									_battleSave->getBattleState()->getSavedGame()->getDifficulty()) == false)
		{
			action.targeting = false;
		}
	}

	if (action.targeting == true
		&& action.actor->spendTimeUnits(action.TU) == true)
	{
		action.TU = 0;
		if (action.actor->getFaction() != FACTION_HOSTILE)
			action.cameraPosition = _battleSave->getBattleState()->getMap()->getCamera()->getMapOffset();

		_battleSave->getBattleGame()->statePushBack(new UnitTurnBState( // moved to ProjectileFlyBState, post-cosmetics
																_battleSave->getBattleGame(),
																action,
																false));
//		if (action.type == BA_HIT)
//			_save->getBattleGame()->statePushBack(new MeleeAttackBState(_save->getBattleGame(), action));

		_battleSave->getBattleGame()->statePushBack(new ProjectileFlyBState(
																_battleSave->getBattleGame(),
																action));
		return true;
	}

	return false;
}

/**
 * Selects a fire method based on range & time units.
 * @param action - reference a BattleAction struct
 */
void TileEngine::selectFireMethod(BattleAction& action)
{
	const RuleItem* const rule = action.weapon->getRules();
	const int dist = _battleSave->getTileEngine()->distance(
														action.actor->getPosition(),
														action.target);
	if (dist > rule->getMaxRange()
		|| dist < rule->getMinRange()) // this might not be what I think it is ...
	{
		return;
	}

	const int tu = action.actor->getTimeUnits();

	if (dist <= rule->getAutoRange())
	{
		if (rule->getTUAuto()
			&& tu >= action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))
		{
			action.type = BA_AUTOSHOT;
			action.TU = action.actor->getActionTUs(BA_AUTOSHOT, action.weapon);
		}
		else if (rule->getTUSnap()
			&& tu >= action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
		{
			action.type = BA_SNAPSHOT;
			action.TU = action.actor->getActionTUs(BA_SNAPSHOT, action.weapon);
		}
		else if (rule->getTUAimed()
			&& tu >= action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
		{
			action.type = BA_AIMEDSHOT;
			action.TU = action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon);
		}
	}
	else if (dist <= rule->getSnapRange())
	{
		if (rule->getTUSnap()
			&& tu >= action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
		{
			action.type = BA_SNAPSHOT;
			action.TU = action.actor->getActionTUs(BA_SNAPSHOT, action.weapon);
		}
		else if (rule->getTUAimed()
			&& tu >= action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
		{
			action.type = BA_AIMEDSHOT;
			action.TU = action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon);
		}
		else if (rule->getTUAuto()
			&& tu >= action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))
		{
			action.type = BA_AUTOSHOT;
			action.TU = action.actor->getActionTUs(BA_AUTOSHOT, action.weapon);
		}
	}
	else // if (dist <= rule->getAimRange())
	{
		if (rule->getTUAimed()
			&& tu >= action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
		{
			action.type = BA_AIMEDSHOT;
			action.TU = action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon);
		}
		else if (rule->getTUSnap()
			&& tu >= action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
		{
			action.type = BA_SNAPSHOT;
			action.TU = action.actor->getActionTUs(BA_SNAPSHOT, action.weapon);
		}
		else if (rule->getTUAuto()
			&& tu >= action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))
		{
			action.type = BA_AUTOSHOT;
			action.TU = action.actor->getActionTUs(BA_AUTOSHOT, action.weapon);
		}
	}

	return;
}

/**
 * Handles bullet/weapon hits. A bullet/weapon hits a voxel.
 * kL_note: called from ExplosionBState::explode()
 * @param targetPos_voxel	- reference the center of hit in voxelspace
 * @param power				- power of the hit/explosion
 * @param dType				- damage type of the hit (RuleItem.h)
 * @param attacker			- pointer to BattleUnit that caused the hit
 * @param melee				- true if no projectile, trajectory, etc. is needed
 * @return, pointer to the BattleUnit that got hit
 */
BattleUnit* TileEngine::hit(
		const Position& targetPos_voxel,
		int power,
		ItemDamageType dType,
		BattleUnit* const attacker,
		const bool melee)
{
	//Log(LOG_INFO) << "TileEngine::hit() power = " << power << " dType = " << (int)dType;
	//if (attacker != NULL) Log(LOG_INFO) << ". by ID " << attacker->getId() << " @ " << attacker->getPosition();

	if (dType != DT_NONE) // bypass Psi-attacks. Psi-attacks don't get this far anymore .... But leave it in for safety.
	{
		//Log(LOG_INFO) << "DT_ dType = " << static_cast<int>(dType);
		Position targetPos_tile = Position(
										targetPos_voxel.x / 16,
										targetPos_voxel.y / 16,
										targetPos_voxel.z / 24);
		Tile* const tile = _battleSave->getTile(targetPos_tile);
		//Log(LOG_INFO) << ". targetTile " << tile->getPosition() << " targetVoxel " << targetPos_voxel;
		if (tile == NULL)
		{
			//Log(LOG_INFO) << ". Position& targetPos_voxel : NOT Valid, return NULL";
			return NULL;
		}

		BattleUnit* targetUnit = tile->getUnit();

		int part = VOXEL_UNIT;
		if (melee == false)
		{
			part = voxelCheck(
							targetPos_voxel,
							attacker,
							false,
							false,
							NULL);
			//Log(LOG_INFO) << ". voxelCheck() part = " << part;
		}

		if (part > VOXEL_EMPTY && part < VOXEL_UNIT	// 4 terrain parts ( 0..3 )
			&& dType != DT_STUN						// kL, workaround for Stunrod. ( might include DT_SMOKE & DT_IN )
			&& dType != DT_SMOKE)
		{
			//Log(LOG_INFO) << ". . terrain hit";
			power = RNG::generate( // 25% to 75% linear.
								power / 4,
								power * 3 / 4);
			//Log(LOG_INFO) << ". . RNG::generate(power) = " << power;
			// kL_note: This would be where to adjust damage based on effectiveness of weapon vs Terrain!
			// DT_NONE,		// 0
			// DT_AP,		// 1
			// DT_IN,		// 2
			// DT_HE,		// 3
			// DT_LASER,	// 4
			// DT_PLASMA,	// 5
			// DT_STUN,		// 6
			// DT_MELEE,	// 7
			// DT_ACID,		// 8
			// DT_SMOKE		// 9
			switch (dType) // round up.
			{
				case DT_AP:
					power = ((power * 3) + 19) / 20;	// 15%
				break;

				case DT_LASER:
					if (tile->getMapData(part)->getSpecialType() != ALIEN_ALLOYS)
						power = (power + 4) / 5;		// 20%
				break;

				case DT_IN:
					power = (power + 3) / 4;			// 25%
				break;

				case DT_PLASMA:
					power = (power + 2) / 3;			// 33% // problem: Fusion Torch; fixed, heh.
				break;

				case DT_MELEE:							// TODO: define 2 terrain types, Soft & Hard; so that edged weapons do good vs. Soft, blunt weapons do good vs. Hard
					power = (power + 1) / 2;			// 50%
				break;

				case DT_HE:								// question: do HE & IN ever get in here - hit() or explode() below
					power += power / 10;				// 110%
//				break;

//				case DT_ACID: // 100% damage

//				default: // [DT_NONE],[DT_STUN,DT_SMOKE]
//					return NULL;
			}
			//Log(LOG_INFO) << ". . power by Type = " << power;

			if (part == VOXEL_OBJECT
				&& tile->getMapData(VOXEL_OBJECT)->isBaseModule() == true
				&& power >= tile->getMapData(MapData::O_OBJECT)->getArmor()
				&& _battleSave->getTacticalType() == TCT_BASEDEFENSE)
//				&& _battleSave->getMissionType() == "STR_BASE_DEFENSE")
			{
				//Log(LOG_INFO) << ". . . vs Object hp  = " << tile->getMapData(MapData::O_OBJECT)->getArmor();
				_battleSave->getModuleMap()
									[(targetPos_voxel.x / 16) / 10]
									[(targetPos_voxel.y / 16) / 10].second--;
			}

			if (tile->damage(
						part,
						power) == true)
			{
				_battleSave->addDestroyedObjective();
			}
		}
		else if (part == VOXEL_UNIT) // battleunit part HIT SUCCESS.
		{
			//Log(LOG_INFO) << ". . battleunit hit";
			// power 0 - 200% -> 1 - 200%
//			if (tile->getUnit() != NULL)
//			{
				//Log(LOG_INFO) << ". . targetUnit Valid ID = " << targetUnit->getId();
//			targetUnit = tile->getUnit();
//			}

			int vertOffset = 0;
			if (targetUnit == NULL)
			{
				//Log(LOG_INFO) << ". . . targetUnit NOT Valid, check tileBelow";
				// it's possible we have a unit below the actual tile, when he
				// stands on a stairs and sticks his head up into the above tile.
				// kL_note: yeah, just like in LoS calculations!!!! cf. visible() etc etc .. idiots.
				const Tile* const tileBelow = _battleSave->getTile(targetPos_tile + Position(0, 0,-1));
				if (tileBelow != NULL
					&& tileBelow->getUnit() != NULL)
				{
					targetUnit = tileBelow->getUnit();
					vertOffset = 24;
				}
			}

			if (targetUnit != NULL)
			{
				//Log(LOG_INFO) << ". . . targetUnit Valid ID = " << targetUnit->getId();
				const int wounds = targetUnit->getFatalWounds();

				// kL_begin: TileEngine::hit(), Silacoids can set targets on fire!!
				if (attacker != NULL
					&& (attacker->getSpecialAbility() == SPECAB_BURNFLOOR
						|| attacker->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
				{
					const float vulnerable = targetUnit->getArmor()->getDamageModifier(DT_IN);
					if (vulnerable > 0.f)
					{
						const int
							fire = RNG::generate( // 25% - 75% / 2
											power / 8,
											power * 3 / 8),
							burn = RNG::generate(
											0,
											static_cast<int>(Round(vulnerable * 5.f)));
						//Log(LOG_INFO) << ". . . . DT_IN : fire = " << fire;

						targetUnit->damage(
										Position(0,0,0),
										fire,
										DT_IN,
										true);
						//Log(LOG_INFO) << ". . . . DT_IN : " << targetUnit->getId() << " takes " << check;

						if (targetUnit->getFireOnUnit() < burn)
							targetUnit->setFireOnUnit(burn); // catch fire and burn
					}
				} // kL_end.

				const int unitSize = targetUnit->getArmor()->getSize() * 8;
				const Position
					targetPos = targetUnit->getPosition() * Position(16,16,24) // convert tilespace to voxelspace
							  + Position(
										unitSize,
										unitSize,
										targetUnit->getFloatHeight() - tile->getTerrainLevel()),
					relPos = targetPos_voxel
						   - targetPos
						   - Position(
									0,0,
									vertOffset);
				//Log(LOG_INFO) << "TileEngine::hit() relPos " << relPos;

				double delta;
				if (dType == DT_HE
					|| Options::TFTDDamage == true)
				{
					delta = 50.;
				}
				else
					delta = 100.;

				const int
					low = static_cast<int>(std::floor(static_cast<double>(power) * (100. - delta) / 100.) + 1),
					high = static_cast<int>(std::ceil(static_cast<double>(power) * (100. + delta) / 100.));

				int extraPow = 0;
				if (attacker != NULL) // bonus to damage per Accuracy (TODO: use ranks also for xCom or aLien)
				{
					if (   dType == DT_AP
						|| dType == DT_LASER
						|| dType == DT_PLASMA
						|| dType == DT_ACID)
					{
						extraPow = static_cast<int>(std::ceil(static_cast<double>(power * attacker->getBaseStats()->firing) / 1000.));
					}
					else if (dType == DT_MELEE)
						extraPow = static_cast<int>(std::ceil(static_cast<double>(power * attacker->getBaseStats()->melee) / 1000.));
				}

				power = RNG::generate(low, high) // bell curve
					  + RNG::generate(low, high);
				power /= 2;
				//Log(LOG_INFO) << ". . . RNG::generate(power) = " << power;
				power += extraPow;

				const bool ignoreArmor = dType == DT_STUN	// kL. stun ignores armor... does now! UHM....
									  || dType == DT_SMOKE;	// note it still gets Vuln.modifier, but not armorReduction.

				power = targetUnit->damage(
										relPos,
										power,
										dType,
										ignoreArmor);
				//Log(LOG_INFO) << ". . . power = " << power;

				if (power > 0)
//					&& !targetUnit->isOut() // target will be neither STATUS_DEAD nor STATUS_UNCONSCIOUS here!
				{
					if (attacker != NULL
						&& (wounds < targetUnit->getFatalWounds()
							|| targetUnit->getHealth() == 0)) // .. just do this here and bDone with it. Regularly done in BattlescapeGame::checkForCasualties()
					{
						targetUnit->killedBy(attacker->getFaction());
						Log(LOG_INFO) << "TE::hit() " << targetUnit->getId() << " killedBy = " << (int)attacker->getFaction();
					}
					// kL_note: Not so sure that's been setup right (cf. other kill-credit code as well as DebriefingState)
					// I mean, shouldn't that be checking that the thing actually DIES?
					// And, probly don't have to state if killed by aLiens: probly assumed in DebriefingState.

					if (targetUnit->getHealth() > 0)
					{
						const int loss = (110 - targetUnit->getBaseStats()->bravery) / 10;
						if (loss > 0)
						{
							int leadership = 100;
							if (targetUnit->getOriginalFaction() == FACTION_PLAYER)
								leadership = _battleSave->getMoraleModifier();
							else if (targetUnit->getOriginalFaction() == FACTION_HOSTILE)
								leadership = _battleSave->getMoraleModifier(NULL, false);

							const int moraleLoss = -(power * loss * 10 / leadership);
							//Log(LOG_INFO) << ". . . . moraleLoss = " << moraleLoss;
							targetUnit->moraleChange(moraleLoss);
						}
					}

					//Log(LOG_INFO) << ". . check for Cyberdisc expl.";
					//Log(LOG_INFO) << ". . health = " << targetUnit->getHealth();
					//Log(LOG_INFO) << ". . stunLevel = " << targetUnit->getStun();
					if ((targetUnit->getSpecialAbility() == SPECAB_EXPLODEONDEATH // cyberdiscs
							|| targetUnit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE)
						&& (targetUnit->getHealth() == 0
							|| targetUnit->getStun() >= targetUnit->getHealth()))
//						&& !targetUnit->isOut(false, true))	// don't explode if stunned. Maybe... wrong!!!
					{
						//Log(LOG_INFO) << ". . . Cyberdisc down!!";
						if (   dType != DT_STUN		// don't explode if stunned. Maybe... see above.
							&& dType != DT_SMOKE
							&& dType != DT_HE)	// don't explode if taken down w/ explosives -> wait a sec, this is hit() not explode() ...
						{
							//Log(LOG_INFO) << ". . . . new ExplosionBState(), !DT_STUN & !DT_HE";
							// kL_note: wait a second. hit() creates an ExplosionBState,
							// but ExplosionBState::explode() creates a hit() ! -> terrain..

							const Position unitPos = Position(
//kL													targetUnit->getPosition().x * 16,
//kL													targetUnit->getPosition().y * 16,
//kL													targetUnit->getPosition().z * 24);
														targetUnit->getPosition().x * 16 + 16,	// kL, cyberdisc a big unit.
														targetUnit->getPosition().y * 16 + 16,	// kL
														targetUnit->getPosition().z * 24 + 12);	// kL
							_battleSave->getBattleGame()->statePushNext(new ExplosionBState(
																						_battleSave->getBattleGame(),
																						unitPos,
																						NULL,
																						targetUnit));
						}
					}
				}


				if (melee == false
					&& _battleSave->getBattleGame()->getCurrentAction()->takenXP == false
					&& targetUnit->getOriginalFaction() == FACTION_HOSTILE
					&& attacker != NULL
					&& attacker->getGeoscapeSoldier() != NULL
					&& attacker->getFaction() == attacker->getOriginalFaction())
				{
					_battleSave->getBattleGame()->getCurrentAction()->takenXP = true;
					attacker->addFiringExp();
				}
			}
		}

		applyGravity(tile);

		calculateSunShading();		// roofs could have been destroyed
		calculateTerrainLighting();	// fires could have been started
//		calculateUnitLighting();	// units could have collapsed <- done in UnitDieBState

		calculateFOV(targetPos_tile);

		//if (targetUnit) Log(LOG_INFO) << "TileEngine::hit() EXIT, return targetUnit";
		//else Log(LOG_INFO) << "TileEngine::hit() EXIT, return NULL[0]";
		return targetUnit;
	}
	//else Log(LOG_INFO) << ". DT_ = " << static_cast<int>(dType);

	//Log(LOG_INFO) << "TileEngine::hit() EXIT, return NULL[1]";
	return NULL;
}

/**
 * Handles explosions.
 * kL_note: called from ExplosionBState
 *
 * HE, smoke and fire explodes in a circular pattern on 1 level only.
 * HE however damages floor tiles of the above level. Not the units on it.
 * HE destroys an object if its armor is lower than the explosive power,
 * then its HE blockage is applied for further propagation.
 * See http://www.ufopaedia.org/index.php?title=Explosions for more info.
 * @param voxelTarget	- reference to the center of explosion in voxelspace
 * @param power			- power of explosion
 * @param dType			- damage type of explosion (enum ItemDamageType)
 * @param maxRadius		- maximum radius of explosion
 * @param attacker		- pointer to a unit that caused explosion (default NULL)
 * @param grenade		- true if explosion is caused by a grenade for throwing XP (default false)
 */
void TileEngine::explode(
			const Position& voxelTarget,
			int power,
			ItemDamageType dType,
			int maxRadius,
			BattleUnit* const attacker,
			bool grenade)
{
/*	int iFalse = 0;
	for (int
			i = 0;
			i < 1000;
			++i)
	{
		int iTest = RNG::generate(0, 1);
		if (iTest == 0)
			++iFalse;
	}
	Log(LOG_INFO) << "RNG:TEST = " << iFalse; */

	//Log(LOG_INFO) << "TileEngine::explode() power = " << power << ", dType = " << (int)dType << ", maxRadius = " << maxRadius;
	//Log(LOG_INFO) << "missileDir = " << _missileDirection;
	if (dType == DT_IN)
	{
		power /= 2;
		//Log(LOG_INFO) << ". DT_IN power = " << power;
	}

	if (power < 1) // quick out.
		return;

	BattleUnit* targetUnit = NULL;

	std::set<Tile*> tilesAffected;
	std::pair<std::set<Tile*>::iterator, bool> tilePair;

	int z_Dec; // = 1000; // default flat explosion

	switch (Options::battleExplosionHeight)
	{
/*		case 1: z_Dec = 10; break;
		case 2: z_Dec = 20; break;
		case 3: z_Dec = 30; break;
		case 4: z_Dec = 40; break;
		case 5: z_Dec = 50; break;

		case 0: // default flat explosion
		default:
			z_Dec = 1000;
		break; */
		case 3: z_Dec = 10; break; // makes things easy for AlienBAIState::explosiveEfficacy()
		case 2: z_Dec = 20; break;
		case 1: z_Dec = 30; break;

		default:
			z_Dec = 1000;
	}

	Tile
		* origin = NULL,
		* destTile = NULL;

	int // convert voxel-space to tile-space
		centerX = voxelTarget.x / 16,
		centerY = voxelTarget.y / 16,
		centerZ = voxelTarget.z / 24,

		tileX,
		tileY,
		tileZ;

	double
		r,
		r_Max = static_cast<double>(maxRadius),

		vect_x,
		vect_y,
		vect_z,

		sin_te,
		cos_te,
		sin_fi,
		cos_fi;

	bool takenXP = false;

//	int testIter = 0; // TEST.
	//Log(LOG_INFO) << ". r_Max = " << r_Max;

//	for (int fi = 0; fi == 0; ++fi) // kL_note: Looks like a TEST ray. ( 0 == horizontal )
//	for (int fi = 90; fi == 90; ++fi) // vertical: UP
	for (int
			fi = -90;
			fi < 91;
			fi += 5)
	{
//		for (int te = 180; te == 180; ++te) // kL_note: Looks like a TEST ray. ( 0 == south, 180 == north, goes CounterClock-wise )
//		for (int te = 90; te < 360; te += 180) // E & W
//		for (int te = 45; te < 360; te += 180) // SE & NW
//		for (int te = 225; te < 420; te += 180) // NW & SE
		for (int // ray-tracing every 3 degrees makes sure we cover all tiles in a circle.
				te = 0;
				te < 361;
				te += 3)
		{
			sin_te = std::sin(static_cast<double>(te) * M_PI / 180.);
			cos_te = std::cos(static_cast<double>(te) * M_PI / 180.);
			sin_fi = std::sin(static_cast<double>(fi) * M_PI / 180.);
			cos_fi = std::cos(static_cast<double>(fi) * M_PI / 180.);

			origin = _battleSave->getTile(Position(
												centerX,
												centerY,
												centerZ));

			_powerE =
			_powerT = power;	// initialize _powerE & _powerT for each ray.
			r = 0.;				// initialize radial length, also.

			while (_powerE > 0
				&& r - 0.5 < r_Max) // kL_note: Allows explosions of 0 radius(!), single tile only hypothetically.
									// the idea is to show an explosion animation but affect only that one tile.
			{
				//Log(LOG_INFO) << ". . . . .";
				//++testIter;
				//Log(LOG_INFO) << ". i = " << testIter;
				//Log(LOG_INFO) << ". _powerE = " << _powerE;
				//Log(LOG_INFO) << ". r = " << r << " _powerE = " << _powerE; // << ", r_Max = " << r_Max;

				vect_x = static_cast<double>(centerX) + r * sin_te * cos_fi;
				vect_y = static_cast<double>(centerY) + r * cos_te * cos_fi;
				vect_z = static_cast<double>(centerZ) + r * sin_fi;

				tileX = static_cast<int>(std::floor(vect_x));
				tileY = static_cast<int>(std::floor(vect_y));
				tileZ = static_cast<int>(std::floor(vect_z));

				destTile = _battleSave->getTile(Position(
														tileX,
														tileY,
														tileZ));
				//Log(LOG_INFO) << ". . test : origin " << origin->getPosition() << " dest " << destTile->getPosition(); //<< ". _powerE = " << _powerE << ". r = " << r;

				if (destTile == NULL) // out of map!
				{
					//Log(LOG_INFO) << ". destTile NOT Valid " << Position(tileX, tileY, tileZ);
					break;
				}


				if (r > 0.5					// don't block epicentrum.
					&& origin != destTile)	// don't double blockage from the same tiles (when diagonal that happens).
				{
					if (dType == DT_IN)
					{
						int dir;
						Pathfinding::vectorToDirection(
													destTile->getPosition() - origin->getPosition(),
													dir);
						if (dir != -1
							&& dir %2 != 0)
						{
							_powerE -= 5; // diagonal costs an extra 50% for fire.
						}
					}

//					_powerE -= 10;
					if (maxRadius > 0)
					{
//						if (power / maxRadius < 1) // do min -1 per tile. Nah ...
//							_powerE -= 1;
//						else

//						_powerE -= (power / maxRadius) + 1; // per RSSwizard, http://openxcom.org/forum/index.php?topic=2927.msg32061#msg32061

						_powerE -= ((power + maxRadius - 1) / maxRadius); // round up.
						//Log(LOG_INFO) << "maxRadius > 0, " << power << "/" << maxRadius << "=" << _powerE;
					}
					//else Log(LOG_INFO) << "maxRadius <= 0";


					if (_powerE < 1)
					{
						//Log(LOG_INFO) << ". _powerE < 1 BREAK[hori] " << Position(tileX, tileY, tileZ) << "\n";
						break;
					}

					if (origin->getPosition().z != tileZ) // up/down explosion decrease
					{
						_powerE -= z_Dec;
						if (_powerE < 1)
						{
							//Log(LOG_INFO) << ". _powerE < 1 BREAK[vert] " << Position(tileX, tileY, tileZ) << "\n";
							break;
						}
					}

					_powerT = _powerE;

					const int horiBlock = horizontalBlockage(
														origin,
														destTile,
														dType);
					//if (horiBlock != 0) Log(LOG_INFO) << ". horiBlock = " << horiBlock;

					const int vertBlock = verticalBlockage(
														origin,
														destTile,
														dType);
					//if (vertBlock != 0) Log(LOG_INFO) << ". vertBlock = " << vertBlock;

					if (horiBlock < 0 // only visLike will return < 0 for this break here.
						&& vertBlock < 0)
					{
						break; // WAIT A SECOND ... oh, Stun &tc.
					}
					else
					{
						if (horiBlock > 0) // only !visLike will return > 0 for these breaks here.
						{
							_powerT -= horiBlock; // terrain takes 200% power to destruct. <- But this isn't for destruction.
							if (_powerT < 1)
							{
								//Log(LOG_INFO) << ". horiBlock BREAK " << Position(tileX, tileY, tileZ) << "\n";
								break;
							}
						}

						if (vertBlock > 0) // only !visLike will return > 0 for these breaks here.
						{
							_powerT -= vertBlock; // terrain takes 200% power to destruct. <- But this isn't for destruction.
							if (_powerT < 1)
							{
								//Log(LOG_INFO) << ". vertBlock BREAK " << Position(tileX, tileY, tileZ) << "\n";
								break;
							}
						}
					}
				}

				// set this to the power-value *before* BLOCK reduces it, and *after* distance is accounted for!
				// ..... not necessarily.
				if (dType == DT_HE) // explosions do 50% damage to terrain and 50% to 150% damage to units
				{
					//Log(LOG_INFO) << ". setExplosive() _powerE = " << _powerE;
					destTile->setExplosive(
										_powerE,	// try powerT to prevent smoke/fire appearing behind intact walls etc.
										0);			// although that might gimp true damage vs parts calculations .... NOPE.
				}

				_powerE = _powerT; // note: These two are becoming increasingly redundant !!!


				// ** DAMAGE begins w/ _powerE ***

				//Log(LOG_INFO) << ". pre insert Tile " << Position(tileX, tileY, tileZ);
				tilePair = tilesAffected.insert(destTile); // check if this tile was hit already
				//Log(LOG_INFO) << ". post insert Tile";
				if (tilePair.second == true) // true if a new tile was inserted.
				{
					//Log(LOG_INFO) << ". > tile TRUE : origin " << origin->getPosition() << " dest " << destTile->getPosition(); //<< ". _powerE = " << _powerE << ". r = " << r;
					//Log(LOG_INFO) << ". > _powerE = " << _powerE;

					targetUnit = destTile->getUnit();
					if (targetUnit != NULL
						&& targetUnit->getTakenExpl() == true) // hit large units only once ... stop experience exploitation near the end of this loop, also. Lulz
					{
						//Log(LOG_INFO) << ". . targetUnit ID " << targetUnit->getId() << ", set Unit = NULL";
						targetUnit = NULL;
					}

					int
						powerUnit = 0,
						wounds = 0;

					if (attacker != NULL
						&& targetUnit != NULL)
					{
						wounds = targetUnit->getFatalWounds();
					}

					switch (dType)
					{
						case DT_STUN: // power 0 - 200%
						{
							if (targetUnit != NULL)
							{
								powerUnit = RNG::generate(1, _powerE * 2) // bell curve
										  + RNG::generate(1, _powerE * 2);
								powerUnit /= 2;
								//Log(LOG_INFO) << ". . . powerUnit = " << powerUnit << " DT_STUN";
								targetUnit->damage(
												Position(0,0,0),
												powerUnit,
												DT_STUN,
												true);
							}

							BattleUnit* bu;
							for (std::vector<BattleItem*>::const_iterator
									i = destTile->getInventory()->begin();
									i != destTile->getInventory()->end();
									++i)
							{
								bu = (*i)->getUnit();
								if (bu != NULL
									&& bu->getStatus() == STATUS_UNCONSCIOUS
									&& bu->getTakenExpl() == false)
								{
									bu->setTakenExpl();
									powerUnit = RNG::generate(1, _powerE * 2) // bell curve
											  + RNG::generate(1, _powerE * 2);
									powerUnit /= 2;
									//Log(LOG_INFO) << ". . . . powerUnit (corpse) = " << powerUnit << " DT_STUN";
									bu->damage(
											Position(0,0,0),
											powerUnit,
											DT_STUN,
											true);
								}
							}
						}
						break;

						case DT_HE: // power 50 - 150%. 70% of that if kneeled, 85% if kneeled @ GZ
						{
							//Log(LOG_INFO) << ". . dType == DT_HE";
							if (targetUnit != NULL)
							{
								//Log(LOG_INFO) << ". . powerE = " << _powerE << " vs. " << targetUnit->getId();
								const double
									power0 = static_cast<double>(_powerE),
									power1 = power0 * 0.5,
									power2 = power0 * 1.5;

								powerUnit = static_cast<int>(RNG::generate(power1, power2)) // bell curve
										  + static_cast<int>(RNG::generate(power1, power2));
								powerUnit /= 2;
								//Log(LOG_INFO) << ". . DT_HE = " << powerUnit; // << ", vs ID " << targetUnit->getId();

								if (powerUnit > 0)
								{
									Position pos;

									// units above the explosion will be hit in the legs, units lateral to or below will be hit in the torso
									if (distance(
												destTile->getPosition(),
												Position(
														centerX,
														centerY,
														centerZ)) < 2)
									{
										//Log(LOG_INFO) << ". . . powerUnit = " << powerUnit << " DT_HE, GZ";
										pos = Position(0,0,0); // Ground zero effect is in effect
										if (targetUnit->isKneeled() == true)
										{
											powerUnit = powerUnit * 17 / 20; // 85% damage
											//Log(LOG_INFO) << ". . . powerUnit(kneeled) = " << powerUnit << " DT_HE, GZ";
										}
									}
									else
									{
										//Log(LOG_INFO) << ". . . powerUnit = " << powerUnit << " DT_HE, not GZ";
										pos = Position( // Directional damage relative to explosion position.
													centerX * 16 - destTile->getPosition().x * 16,
													centerY * 16 - destTile->getPosition().y * 16,
													centerZ * 24 - destTile->getPosition().z * 24);
										if (targetUnit->isKneeled() == true)
										{
											powerUnit = powerUnit * 7 / 10; // 70% damage
											//Log(LOG_INFO) << ". . . powerUnit(kneeled) = " << powerUnit << " DT_HE, not GZ";
										}
									}

									if (powerUnit > 0)
									{
										targetUnit->damage(
														pos,
														powerUnit,
														DT_HE);
										//Log(LOG_INFO) << ". . . realDamage = " << damage << " DT_HE";
									}
								}
							}

							BattleUnit* bu;
							bool done = false;
							while (done == false)
							{
								//Log(LOG_INFO) << ". INVENTORY: iterate done=False";
//								done = destTile->getInventory()->empty();
								if (destTile->getInventory()->empty() == true)
									break;

								for (std::vector<BattleItem*>::const_iterator
										i = destTile->getInventory()->begin();
										i != destTile->getInventory()->end();
										)
								{
									//Log(LOG_INFO) << ". . INVENTORY: Item = " << (*i)->getRules()->getType();
									bu = (*i)->getUnit();
									if (bu != NULL
										&& bu->getStatus() == STATUS_UNCONSCIOUS
										&& bu->getTakenExpl() == false)
									{
										bu->setTakenExpl();

										const double
											power0 = static_cast<double>(_powerE),
											power1 = power0 * 0.5,
											power2 = power0 * 1.5;

										powerUnit = static_cast<int>(RNG::generate(power1, power2)) // bell curve
												  + static_cast<int>(RNG::generate(power1, power2));
										powerUnit /= 2;
										//Log(LOG_INFO) << ". . . INVENTORY: power = " << powerUnit;
										bu->damage(
												Position(0,0,0),
												powerUnit,
												DT_HE);
										//Log(LOG_INFO) << ". . . INVENTORY: damage = " << damage;

										if (bu->getHealth() == 0)
										{
											//Log(LOG_INFO) << ". . . . INVENTORY: instaKill";
											bu->instaKill();

											if (attacker != NULL)
											{
												bu->killedBy(attacker->getFaction());
												Log(LOG_INFO) << "TE::explode() " << bu->getId() << " killedBy = " << (int)attacker->getFaction();
											}

											if (Options::battleNotifyDeath // send Death notice.
												&& bu->getGeoscapeSoldier() != NULL)
											{
												Game* const game = _battleSave->getBattleState()->getGame();
												game->pushState(new InfoboxOKState(game->getLanguage()->getString( // "has exploded ..."
																											"STR_HAS_BEEN_KILLED",
																											bu->getGender())
																										.arg(bu->getName(game->getLanguage()))));
											}
										}

										++i;
										done = (i == destTile->getInventory()->end());
									}
									else if (_powerE > (*i)->getRules()->getArmor()
										&& (bu == NULL
											|| (bu->getStatus() == STATUS_DEAD
												&& bu->getTakenExpl() == false)))
									{
										//Log(LOG_INFO) << ". . . INVENTORY: removeItem = " << (*i)->getRules()->getType();
										_battleSave->removeItem(*i);
										break;
									}
									else
									{
										//Log(LOG_INFO) << ". . . INVENTORY: bypass item = " << (*i)->getRules()->getType();
										++i;
										done = (i == destTile->getInventory()->end());
									}
								}
							}
						}
						break;

						case DT_SMOKE:
							if (destTile->getTerrainLevel() > -24)
							{
								const int smokePow = static_cast<int>(std::ceil(
													(static_cast<double>(_powerE) / static_cast<double>(power)) * 10.));
								destTile->addSmoke(smokePow + RNG::generate(0,7));
							}

							if (targetUnit != NULL)
							{
								powerUnit = RNG::generate( // 10% to 20%
														_powerE / 10,
														_powerE / 5);
								targetUnit->damage(
												Position(0,0,0),
												powerUnit,
												DT_SMOKE,
												true);
								//Log(LOG_INFO) << ". . DT_IN : " << targetUnit->getId() << " takes " << firePower << " firePower";
							}

							BattleUnit* bu;
							for (std::vector<BattleItem*>::const_iterator
									i = destTile->getInventory()->begin();
									i != destTile->getInventory()->end();
									++i)
							{
								bu = (*i)->getUnit();
								if (bu != NULL
									&& bu->getStatus() == STATUS_UNCONSCIOUS
									&& bu->getTakenExpl() == false)
								{
									bu->setTakenExpl();
									powerUnit = RNG::generate( // 10% to 20%
															_powerE / 10,
															_powerE / 5);
									bu->damage(
											Position(0,0,0),
											powerUnit,
											DT_SMOKE,
											true);
								}
							}
						break;

						case DT_IN:
						{
							if (targetUnit != NULL)
							{
								targetUnit->setTakenExpl();
								powerUnit = RNG::generate( // 25% - 75%
														_powerE / 4,
														_powerE * 3 / 4);
								targetUnit->damage(
												Position(0,0,0),
												powerUnit,
												DT_IN,
												true);
								//Log(LOG_INFO) << ". . DT_IN : " << targetUnit->getId() << " takes " << firePower << " firePower";

								float vuln = targetUnit->getArmor()->getDamageModifier(DT_IN);
								if (vuln > 0.f)
								{
									const int burn = RNG::generate(
																0,
																static_cast<int>(Round(5.f * vuln)));
									if (targetUnit->getFireOnUnit() < burn)
										targetUnit->setFireOnUnit(burn); // catch fire and burn!!
								}
							}

							Tile // NOTE: Should check if tileBelow's have already had napalm drop on them from this explosion ....
								* fireTile = destTile,
								* tileBelow = _battleSave->getTile(fireTile->getPosition() + Position(0,0,-1));

							while (fireTile != NULL // safety.
								&& fireTile->getPosition().z > 0
								&& fireTile->getMapData(MapData::O_OBJECT) == NULL
								&& fireTile->getMapData(MapData::O_FLOOR) == NULL
								&& fireTile->hasNoFloor(tileBelow) == true)
							{
								fireTile = tileBelow;
								tileBelow = _battleSave->getTile(fireTile->getPosition() + Position(0,0,-1));
							}

//							if (fireTile->isVoid() == false)
//							{
								// kL_note: So, this just sets a tile on fire/smoking regardless of its content.
								// cf. Tile::ignite() -> well, not regardless, but automatically. That is,
								// ignite() checks for Flammability first: if (getFlammability() == 255) don't do it.
								// So this is, like, napalm from an incendiary round, while ignite() is for parts
								// of the tile itself self-igniting.

							if (fireTile != NULL) // safety.
							{
								const int firePow = static_cast<int>(std::ceil(
												   (static_cast<double>(_powerE) / static_cast<double>(power)) * 10.));
								fireTile->addFire(firePow + fireTile->getFuel());
								fireTile->addSmoke(std::max(
														firePow + fireTile->getFuel(),
														firePow + ((fireTile->getFlammability() + 9) / 10)));
							}
//							}

							targetUnit = fireTile->getUnit();
							if (targetUnit != NULL
								&& targetUnit->getTakenExpl() == false)
							{
								targetUnit->setTakenExpl();
								powerUnit = RNG::generate( // 25% - 75%
														_powerE / 4,
														_powerE * 3 / 4);
								targetUnit->damage(
												Position(0,0,0),
												powerUnit,
												DT_IN,
												true);
								//Log(LOG_INFO) << ". . DT_IN : " << targetUnit->getId() << " takes " << firePower << " firePower";

								float vulnerable = targetUnit->getArmor()->getDamageModifier(DT_IN);
								if (vulnerable > 0.f)
								{
									const int burn = RNG::generate(
																0,
																static_cast<int>(Round(5.f * vulnerable)));
									if (targetUnit->getFireOnUnit() < burn)
										targetUnit->setFireOnUnit(burn); // catch fire and burn!!
								}
							}

							BattleUnit* bu;
							bool done = false;
							while (done == false)
							{
								if (fireTile->getInventory()->empty() == true)
									break;

								for (std::vector<BattleItem*>::const_iterator
										i = fireTile->getInventory()->begin();
										i != fireTile->getInventory()->end();
										)
								{
									bu = (*i)->getUnit();
									if (bu != NULL
										&& bu->getStatus() == STATUS_UNCONSCIOUS
										&& bu->getTakenExpl() == false)
									{
										bu->setTakenExpl();
										powerUnit = RNG::generate( // 25% - 75%
																_powerE / 4,
																_powerE * 3 / 4);
										bu->damage(
												Position(0,0,0),
												powerUnit,
												DT_IN,
												true);

										if (bu->getHealth() == 0)
										{
											bu->instaKill();

											if (attacker != NULL)
											{
												bu->killedBy(attacker->getFaction());
												Log(LOG_INFO) << "TE::explode() " << bu->getId() << " killedBy = " << (int)attacker->getFaction();
											}

											if (Options::battleNotifyDeath == true // send Death notice.
												&& bu->getGeoscapeSoldier() != NULL)
											{
												Game* const game = _battleSave->getBattleState()->getGame();
												game->pushState(new InfoboxOKState(game->getLanguage()->getString( // "has been killed with Fire ..."
																											"STR_HAS_BEEN_KILLED",
																											bu->getGender())
																										.arg(bu->getName(game->getLanguage()))));
											}
										}

										++i;
										done = (i == fireTile->getInventory()->end());
									}
									else if (_powerE > (*i)->getRules()->getArmor()
										&& (bu == NULL
											|| (bu->getStatus() == STATUS_DEAD
												&& bu->getTakenExpl() == false)))
									{
										_battleSave->removeItem(*i);
										break;
									}
									else
									{
										++i;
										done = (i == fireTile->getInventory()->end());
									}
								}
							}
						}
					} // End switch().


					if (targetUnit != NULL)
					{
						//Log(LOG_INFO) << ". . targetUnit ID " << targetUnit->getId() << ", setTaken TRUE";
						targetUnit->setTakenExpl();

						// if it's going to bleed to death and it's not a player, give credit for the kill.
						// kL_note: See Above^
						if (attacker != NULL)
						{
							if (wounds < targetUnit->getFatalWounds()
								|| targetUnit->getHealth() == 0) // kL .. just do this here and bDone with it. Regularly done in BattlescapeGame::checkForCasualties()
							{
								targetUnit->killedBy(attacker->getFaction());
								Log(LOG_INFO) << "TE::explode() " << targetUnit->getId() << " killedBy = " << (int)attacker->getFaction();
							}

							if (takenXP == false
								&& attacker->getGeoscapeSoldier() != NULL
								&& attacker->getFaction() == attacker->getOriginalFaction()
								&& targetUnit->getOriginalFaction() == FACTION_HOSTILE
								&& dType != DT_SMOKE)
							{
								takenXP = true;

								if (grenade == false)
									attacker->addFiringExp();
								else
									attacker->addThrowingExp();
							}
						}
					}
				}

				origin = destTile;
				r += 1.;
				//Log(LOG_INFO) << " ";
			}
		}
	}

	_powerE =
	_powerT = -1;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		//Log(LOG_INFO) << ". . unitTaken ID " << (*i)->getId() << ", reset Taken";
		(*i)->setTakenExpl(false);
	}


	if (dType == DT_HE) // detonate tiles affected with HE
	{
		//Log(LOG_INFO) << ". explode Tiles, size = " << tilesAffected.size();
		for (std::set<Tile*>::const_iterator
				i = tilesAffected.begin();
				i != tilesAffected.end();
				++i)
		{
			if (detonate(*i) == true)
				_battleSave->addDestroyedObjective();

			applyGravity(*i);

			Tile* const tileAbove = _battleSave->getTile((*i)->getPosition() + Position(0,0,1));
			if (tileAbove != NULL)
				applyGravity(tileAbove);
		}
		//Log(LOG_INFO) << ". explode Tiles DONE";
	}


	calculateSunShading();		// roofs could have been destroyed
	calculateTerrainLighting();	// fires could have been started
//	calculateUnitLighting();	// units could have collapsed <- done in UnitDieBState

	recalculateFOV();
	//Log(LOG_INFO) << "TileEngine::explode() EXIT";
}

/**
 * Calculates the amount of power that is blocked as it passes
 * from one tile to another on the same z-level.
 * @param startTile	- pointer to tile where the power starts
 * @param endTile	- pointer to adjacent tile where the power ends
 * @param dType		- type of power (DT_* RuleItem.h)
 * @return, -99 special case for Content & bigWalls to block vision and still get revealed, and for invalid tiles also
 *			-1 hardblock power / vision (can be less than -1)
 *			 0 no block
 *			 1+ variable power blocked
 */
int TileEngine::horizontalBlockage(
		const Tile* const startTile,
		const Tile* const endTile,
		const ItemDamageType dType)
{
	//Log(LOG_INFO) << "TileEngine::horizontalBlockage()";
/*	DT_NONE,	// 0
	DT_AP,		// 1
	DT_IN,		// 2
	DT_HE,		// 3
	DT_LASER,	// 4
	DT_PLASMA,	// 5
	DT_STUN,	// 6
	DT_MELEE,	// 7
	DT_ACID,	// 8
	DT_SMOKE	// 9 */
	bool visLike = dType == DT_NONE
				|| dType == DT_IN
				|| dType == DT_STUN
				|| dType == DT_SMOKE;

	if (startTile == NULL // safety checks
		|| endTile == NULL
		|| startTile->getPosition().z != endTile->getPosition().z)
	{
		return 0;
	}

	// debug:
	//bool debug = dType == DT_HE;

	int dir;
	Pathfinding::vectorToDirection( // Set&Get direction by reference
							endTile->getPosition() - startTile->getPosition(),
							dir);
	if (dir == -1) // startTile == endTile
		return 0;


	static const Position
		tileNorth	= Position( 0,-1, 0),
		tileEast	= Position( 1, 0, 0),
		tileSouth	= Position( 0, 1, 0),
		tileWest	= Position(-1, 0, 0);

	int block = 0;
//	Tile *tmpTile; // new.

	switch (dir)
	{
		case 0:	// north
			block = blockage(
							startTile,
							MapData::O_NORTHWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
				block += blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								0,
								true)
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								4);
		break;
		case 1: // north east
//			if (dType == DT_NONE) // this is two-way diagonal visibility check
			if (visLike)
			{
				block = blockage( // up+right
								startTile,
								MapData::O_NORTHWALL,
								dType)
						+ blockage(
								endTile,
								MapData::O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								dType,
								3); // checks Content/bigWalls

				// instead of last blockage() above, check this:
/*				tmpTile = _save->getTile(startTile->getPosition() + oneTileNorth);
				if (tmpTile && tmpTile->getMapData(MapData::O_OBJECT) && tmpTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNESW)
					block += blockage(tmpTile, MapData::O_OBJECT, dType, 3); */

				if (block == 0) break; // this way is opened

				block = blockage( // right+up
//				block += blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								dType,
								7); // checks Content/bigWalls

				// instead of last blockage() above, check this:
/*				tmpTile = _save->getTile(startTile->getPosition() + oneTileEast);
				if (tmpTile && tmpTile->getMapData(MapData::O_OBJECT) && tmpTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALLNESW)
					block += blockage(tmpTile, MapData::O_OBJECT, dType, 7); */
				// etc. on down through non-cardinal dir's; see 'wayboys' source for details (or not)

			}
			else // dt_NE
			{
				block = blockage(
								startTile,
								MapData::O_NORTHWALL,
								dType) / 2
						+ blockage(
								endTile,
								MapData::O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_NORTHWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_WESTWALL,
								dType) / 2

						+ blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								0,
								true) / 2
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								2,
								true) / 2

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								dType,
								2) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								dType,
								4) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								dType,
								6) / 2 // checks Content/bigWalls

						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								4) / 2 // checks Content/bigWalls
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								6) / 2; // checks Content/bigWalls
			}
		break;
		case 2: // east
			block = blockage(
							endTile,
							MapData::O_WESTWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
				block += blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								6)
						+  blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								2,
								true);
		break;
		case 3: // south east
//			if (dType == DT_NONE) // this is two-way diagonal visibility check
			if (visLike)
			{
				block = blockage( // down+right
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_NORTHWALL,
								dType)
						+ blockage(
								endTile,
								MapData::O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								dType,
								1); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // right+down
//				block += blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_WESTWALL,
								dType)
						+ blockage(
								endTile,
								MapData::O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								dType,
								5); // checks Content/bigWalls
			}
			else // dt_SE
			{
				block = blockage(
								endTile,
								MapData::O_WESTWALL,
								dType) / 2
						+ blockage(
								endTile,
								MapData::O_NORTHWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_NORTHWALL,
								dType) / 2

						+ blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								2,
								true) / 2 // checks Content/bigWalls
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								4,
								true) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								dType,
								2) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								dType,
								4) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								dType,
								6) / 2 // checks Content/bigWalls

						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								6) / 2; // checks Content/bigWalls
			}
		break;
		case 4: // south
			block = blockage(
							endTile,
							MapData::O_NORTHWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
				block += blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								0)
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								4,
								true);
		break;
		case 5: // south west
//			if (dType == DT_NONE) // this is two-way diagonal visibility check
			if (visLike)
			{
				block = blockage( // down+left
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								dType,
								7); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // left+down
//				block += blockage(
								startTile,
								MapData::O_WESTWALL,
								dType)
						+ blockage(
								endTile,
								MapData::O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								dType,
								3);
			}
			else // dt_SW
			{
				block = blockage(
								endTile,
								MapData::O_NORTHWALL,
								dType) / 2
						+ blockage(
								startTile,
								MapData::O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_NORTHWALL,
								dType) / 2

						+ blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								4,
								true) / 2 // checks Content/bigWalls
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								6,
								true) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								dType,
								6) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								dType,
								2) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								dType,
								4) / 2 // checks Content/bigWalls

						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								2) / 2; // checks Content/bigWalls
			}
		break;
		case 6: // west
			block = blockage(
							startTile,
							MapData::O_WESTWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
			{
				block += blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								6,
								true)
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								2);
			}
		break;
		case 7: // north west
//			if (dType == DT_NONE) // this is two-way diagonal visibility check
			if (visLike)
			{
				block = blockage( // up+left
								startTile,
								MapData::O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								dType,
								5);

				if (block == 0) break; // this way is opened

				block = blockage( // left+up
//				block += blockage(
								startTile,
								MapData::O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								dType,
								1);
			}
			else // dt_NW
			{
				block = blockage(
								startTile,
								MapData::O_WESTWALL,
								dType) / 2
						+ blockage(
								startTile,
								MapData::O_NORTHWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_NORTHWALL,
								dType) / 2

						+ blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								0,
								true) / 2
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								dType,
								6,
								true) / 2

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								dType,
								4) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								dType,
								6) / 2

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								dType,
								0) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								dType,
								2) / 2

						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								2) / 2
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								dType,
								4) / 2;
			}
		break;

		default:
		break;
	}

	if (visLike == true)
	{
		block += blockage(
						startTile,
						MapData::O_OBJECT,
						dType,
						dir, // checks Content/bigWalls
						true,
						true);

		if (block == 0		// if, no visBlock yet ...
			&& blockage(	// so check for content @endTile & reveal it by not cutting trajectory.
					endTile,
					MapData::O_OBJECT,
					dType,
					(dir + 4) %8) // opposite direction
				!= 0) // should always be, < 1; ie. this conditions checks [if -1]
		{
			if (dType == DT_NONE)
				return -1;
			else
			{
				//Log(LOG_INFO) << "explode End: hardblock 1000";
				return 1000; // this is a hardblock and should be greater than the most powerful explosions.
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::horizontalBlockage() EXIT, ret = " << block;
	return block;
}

/**
 * Calculates the amount of power that is blocked going from one tile to another on a different z-level.
 * Can cross more than one level (used for lighting). Only floor & object tiles are taken into account ... not really!
 * @param startTile	- pointer to Tile where the power starts
 * @param endTile	- pointer to adjacent Tile where the power ends
 * @param dType		- ItemDamageType of power (RuleItem.h)
 * @return, (int)block	-99 special case for Content-objects to block vision, and for invalid tiles
 *						-1 hardblock power / vision (can be less than -1)
 *						 0 no block
 *						 1+ variable power blocked
 */
int TileEngine::verticalBlockage(
		const Tile* const startTile,
		const Tile* const endTile,
		const ItemDamageType dType)
{
	//Log(LOG_INFO) << "TileEngine::verticalBlockage()";
/*	bool visLike = dType == DT_NONE
				|| dType == DT_IN
				|| dType == DT_STUN
				|| dType == DT_SMOKE; */

	if (startTile == NULL // safety check
		|| endTile == NULL
		|| startTile == endTile)
	{
		return 0;
	}

	int dirZ = endTile->getPosition().z - startTile->getPosition().z;
	if (dirZ == 0)
		return 0;

	int
		x = startTile->getPosition().x,
		y = startTile->getPosition().y,
		z = startTile->getPosition().z,

		block = 0;

	if (dirZ > 0) // up
	{
		if (x == endTile->getPosition().x
			&& y == endTile->getPosition().y)
		{
			for ( // this checks directly up.
					z += 1;
					z <= endTile->getPosition().z;
					++z)
			{
				block += blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								dType)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_OBJECT,
								dType,
								Pathfinding::DIR_UP);
			}

			return block;
		}
		else
//		if (x != endTile->getPosition().x // if endTile is offset on x/y-plane
//			|| y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			z = startTile->getPosition().z;

			block += horizontalBlockage( // this checks for ANY Block horizontally to a tile beneath the endTile
									startTile,
									_battleSave->getTile(Position(x, y, z)),
									dType);

			for (
					z += 1;
					z <= endTile->getPosition().z;
					++z)
			{
				block += blockage( // these check the endTile
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								dType)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_OBJECT,
								dType); // note: no Dir vs typeOBJECT
			}
		}
	}
	else // if (dirZ < 0) // down
	{
		if (x == endTile->getPosition().x
			&& y == endTile->getPosition().y)
		{
			for ( // this checks directly down.
					;
					z > endTile->getPosition().z;
					--z)
			{
				block += blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								dType)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_OBJECT,
								dType,
								Pathfinding::DIR_DOWN,
								true); // kL_add. ( should be false for LoS, btw )
			}

			return block;
		}
		else
//		if (x != endTile->getPosition().x // if endTile is offset on x/y-plane
//			|| y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			z = startTile->getPosition().z;

			block += horizontalBlockage( // this checks for ANY Block horizontally to a tile above the endTile
									startTile,
									_battleSave->getTile(Position(x, y, z)),
									dType);

			for (
					;
					z > endTile->getPosition().z;
					--z)
			{
				block += blockage( // these check the endTile
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								dType)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_OBJECT,
								dType); // note: no Dir vs typeOBJECT
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::verticalBlockage() EXIT ret = " << block;
	return block;
}

/**
 * Calculates the amount of power that is blocked going from one tile to another on a different level.
 * @param startTile The tile where the power starts.
 * @param endTile The adjacent tile where the power ends.
 * @param dType The type of power/damage.
 * @param skipObject
 * @return Amount of blockage of this power.
 */
/* int TileEngine::verticalBlockage(Tile *startTile, Tile *endTile, ItemDamageType dType, bool skipObject)
{
	int block = 0;

	// safety check
	if (startTile == 0 || endTile == 0) return 0;
	int direction = endTile->getPosition().z - startTile->getPosition().z;

	if (direction == 0 ) return 0;

	int x = startTile->getPosition().x;
	int y = startTile->getPosition().y;
	int z = startTile->getPosition().z;

	if (direction < 0) // down
	{
		block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, dType);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, dType, Pathfinding::DIR_DOWN);
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), dType, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, dType);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, dType, Pathfinding::DIR_DOWN);
		}
	}
	else if (direction > 0) // up
	{
		block += blockage(_save->getTile(Position(x, y, z+1)), MapData::O_FLOOR, dType);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z+1)), MapData::O_OBJECT, dType, Pathfinding::DIR_UP);
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z+1;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), dType, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, dType);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, dType, Pathfinding::DIR_UP);
		}
	}

	return block;
} */

/**
 * Calculates the amount of power or LoS/FoV/LoF that various types of
 * walls/bigwalls or floors or object parts of a tile blocks.
 * @param startTile		- pointer to tile where the power starts
 * @param part			- the part of the tile that the power tries to go through
 * @param dType			- the type of power (RuleItem.h) DT_NONE if line-of-vision
 * @param dir			- direction the power travels	-1	walls & floors (default)
 *														 0+	big-walls & content
 * @param originTest	- true if the origin tile is being examined for bigWalls;
 *							used only when dir is specified (default: false)
 * @param trueDir		- for checking if dir is *really* from the direction of sight (true)
 *							or, in the case of some bigWall determinations, perpendicular to it (false);
 *							used only when dir is specified (default: false)
 * @return, (int)block	-99 special case for invalid tiles
 *						-1 hardblock power / vision
 *						 0 no block
 *						 1+ variable power blocked
 */
int TileEngine::blockage(
		const Tile* const tile,
		const int part,
		const ItemDamageType dType,
		const int dir,
		const bool originTest,
		const bool trueDir)
{
	//Log(LOG_INFO) << "TileEngine::blockage() dir " << dir;
	const bool visLike = dType == DT_NONE
					  || dType == DT_SMOKE
					  || dType == DT_STUN
					  || dType == DT_IN;

	if (tile == NULL							// probably outside the map here
		|| tile->isUfoDoorOpen(part) == true)	// open ufo doors are actually still closed behind the scenes
	{
		//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret ( no tile OR ufo-door open )";
		return 0;
	}

	if (tile->getMapData(part) != NULL)
	{
		//Log(LOG_INFO) << ". getMapData(part) stopLOS() = " << tile->getMapData(part)->stopLOS();
		if (dir == -1) // regular north/west wall (not BigWall), or it's a floor, or a Content-object (incl. BigWall) vs upward-diagonal.
		{
			if (visLike == true)
			{
				if ((tile->getMapData(part)->stopLOS() == true // stopLOS() should join w/ DT_NONE ...
							|| (dType == DT_SMOKE
								&& tile->getMapData(part)->getBlock(DT_SMOKE) == 1)
							|| (dType == DT_IN
								&& tile->getMapData(part)->blockFire() == true))
						&& (tile->getMapData(part)->getObjectType() == MapData::O_OBJECT // this one is for verticalBlockage() only.
							|| tile->getMapData(part)->getObjectType() == MapData::O_NORTHWALL
							|| tile->getMapData(part)->getObjectType() == MapData::O_WESTWALL)
					|| tile->getMapData(part)->getObjectType() == MapData::O_FLOOR)	// all floors that block LoS should have their stopLOS flag set true, if not gravLift floor.
																					// Might want to check hasNoFloor() flag.
				{
					//Log(LOG_INFO) << ". . . . Ret 1000[0] part = " << part << " " << tile->getPosition();
					return 1000;
				}
			}
			else if (tile->getMapData(part)->stopLOS() == true // stopLOS() should join w/ DT_NONE ...
				&& _powerE > -1
				&& _powerE < tile->getMapData(part)->getArmor() * 2) // terrain absorbs 200% damage from DT_HE!
			{
				//Log(LOG_INFO) << ". . . . Ret 1000[1] part = " << part << " " << tile->getPosition();
				return 1000; // this is a hardblock for HE; hence it has to be higher than the highest HE power in the Rulesets.
			}
		}
		else // dir > -1 -> OBJECT part. ( BigWalls & content ) *always* an OBJECT-part gets passed in through here, and *with* a direction.
		{
			const Pathfinding::bigWallTypes bigWall = static_cast<Pathfinding::bigWallTypes>(tile->getMapData(MapData::O_OBJECT)->getBigWall()); // 0..9 or, per MCD.
			//Log(LOG_INFO) << ". bigWall = " << bigWall;


			if (originTest == true)	// the ContentOBJECT already got hit as the previous endTile... but can still block LoS when looking down ...
			{
				bool diagStop = true;
				if (dType == DT_HE
					&& _missileDirection != -1)
				{
					const int dirDelta = std::abs(8 + _missileDirection - dir) %8;
					diagStop = (dirDelta < 2 || dirDelta > 6);
				}

				// this needs to check which side the *missile* is coming from,
				// although grenades that land on a diagonal bigWall are exempt regardless!!!
				if (bigWall == Pathfinding::BIGWALL_NONE // !visLike, if (only Content-part == true) -> all DamageTypes ok here (because, origin).
					|| (diagStop == false
						&& (bigWall == Pathfinding::BIGWALL_NESW
							|| bigWall == Pathfinding::BIGWALL_NWSE))
					|| (dir == Pathfinding::DIR_DOWN
						&& tile->getMapData(MapData::O_OBJECT)->stopLOS() == false // stopLOS() should join w/ DT_NONE ...
						&& !(
							dType == DT_SMOKE
							&& tile->getMapData(MapData::O_OBJECT)->getBlock(DT_SMOKE) == 1)
						&& !(
							dType == DT_IN
							&& tile->getMapData(MapData::O_OBJECT)->blockFire() == true)))
				{
					return 0;
				}
				else if (visLike == false // diagonal BigWall blockage ...
					&& (bigWall == Pathfinding::BIGWALL_NESW
						|| bigWall == Pathfinding::BIGWALL_NWSE)
					&& tile->getMapData(MapData::O_OBJECT)->stopLOS() == true // stopLOS() should join w/ DT_NONE ...
					&& _powerE > -1
					&& _powerE < tile->getMapData(MapData::O_OBJECT)->getArmor() * 2)
				{
					//Log(LOG_INFO) << ". . . . Ret 1000[2] part = " << part << " " << tile->getPosition();
					return 1000;
				}
			}

			if (visLike == true // hardblock for visLike against non-bigWall content-object.
				&& bigWall == Pathfinding::BIGWALL_NONE
				&& (tile->getMapData(MapData::O_OBJECT)->stopLOS() == true // stopLOS() should join w/ DT_NONE ...
					|| (dType == DT_SMOKE
						&& tile->getMapData(MapData::O_OBJECT)->getBlock(DT_SMOKE) == 1)
					|| (dType == DT_IN
						&& tile->getMapData(MapData::O_OBJECT)->blockFire() == true)))
			{
				//Log(LOG_INFO) << ". . . . Ret 1000[3] part = " << part << " " << tile->getPosition();
				return 1000;
			}


			switch (dir) // -> OBJECT part. ( BigWalls & content )
			{
				case 0: // north
					if (bigWall == Pathfinding::BIGWALL_WEST
						|| bigWall == Pathfinding::BIGWALL_EAST
						|| bigWall == Pathfinding::BIGWALL_SOUTH
						|| bigWall == Pathfinding::BIGWALL_E_S)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 0 north )";
						return 0; // part By-passed.
					}
				break;

				case 1: // north east
					if (bigWall == Pathfinding::BIGWALL_WEST
						|| bigWall == Pathfinding::BIGWALL_SOUTH
						|| ( //visLike &&
							bigWall == Pathfinding::BIGWALL_NWSE
							&& trueDir == false))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 1 northeast )";
						return 0;
					}
				break;

				case 2: // east
					if (bigWall == Pathfinding::BIGWALL_NORTH
						|| bigWall == Pathfinding::BIGWALL_SOUTH
						|| bigWall == Pathfinding::BIGWALL_WEST
						|| bigWall == Pathfinding::BIGWALL_W_N)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 2 east )";
						return 0;
					}
				break;

				case 3: // south east
					if (bigWall == Pathfinding::BIGWALL_NORTH
						|| bigWall == Pathfinding::BIGWALL_WEST
						|| ( //visLike &&
							bigWall == Pathfinding::BIGWALL_NESW
							&& trueDir == false)
						|| bigWall == Pathfinding::BIGWALL_W_N)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 3 southeast )";
						return 0;
					}
				break;

				case 4: // south
					if (bigWall == Pathfinding::BIGWALL_WEST
						|| bigWall == Pathfinding::BIGWALL_EAST
						|| bigWall == Pathfinding::BIGWALL_NORTH
						|| bigWall == Pathfinding::BIGWALL_W_N)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 4 south )";
						return 0;
					}
				break;

				case 5: // south west
					if (bigWall == Pathfinding::BIGWALL_NORTH
						|| bigWall == Pathfinding::BIGWALL_EAST
						|| ( //visLike &&
							bigWall == Pathfinding::BIGWALL_NWSE
							&& trueDir == false))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 5 southwest )";
						return 0;
					}
				break;

				case 6: // west
					if (bigWall == Pathfinding::BIGWALL_NORTH
						|| bigWall == Pathfinding::BIGWALL_SOUTH
						|| bigWall == Pathfinding::BIGWALL_EAST
						|| bigWall == Pathfinding::BIGWALL_E_S)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 6 west )";
						return 0;
					}
				break;

				case 7: // north west
					if (bigWall == Pathfinding::BIGWALL_SOUTH
						|| bigWall == Pathfinding::BIGWALL_EAST
						|| bigWall == Pathfinding::BIGWALL_E_S
						|| ( //visLike &&
							bigWall == Pathfinding::BIGWALL_NESW
							&& trueDir == false))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 7 northwest )";
						return 0;
					}
				break;

				case 8: // up
				case 9: // down
					if ((bigWall != Pathfinding::BIGWALL_NONE			// lets content-objects Block explosions
							&& bigWall != Pathfinding::BIGWALL_BLOCK)	// includes stopLoS (floors handled above under non-directional condition)
						|| (visLike == true
							&& tile->getMapData(MapData::O_OBJECT)->stopLOS() == false // stopLOS() should join w/ DT_NONE ...
							&& !(
								dType == DT_SMOKE
								&& tile->getMapData(MapData::O_OBJECT)->getBlock(DT_SMOKE) == 1)
							&& !(
								dType == DT_IN
								&& tile->getMapData(MapData::O_OBJECT)->blockFire() == true)))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 8,9 up,down )";
						return 0;
					}
				break;


				default:
					return 0; // .....
//				break;
			}


			// might be Content-part or remaining-bigWalls block here
			if (tile->getMapData(MapData::O_OBJECT)->stopLOS() == true // use stopLOS to hinder explosions from propagating through bigWalls freely. // stopLOS() should join w/ DT_NONE ...
				|| (dType == DT_SMOKE
					&& tile->getMapData(MapData::O_OBJECT)->getBlock(DT_SMOKE) == 1)
				|| (dType == DT_IN
					&& tile->getMapData(MapData::O_OBJECT)->blockFire() == true))
			{
				if (visLike == true
					|| (_powerE > -1
						&& _powerE < tile->getMapData(MapData::O_OBJECT)->getArmor() * 2)) // terrain absorbs 200% damage from DT_HE!
				{
					//Log(LOG_INFO) << ". . . . Ret 1000[4] part = " << part << " " << tile->getPosition();
					return 1000; // this is a hardblock for HE; hence it has to be higher than the highest HE power in the Rulesets.
				}
			}
		}


		if (visLike == false) // only non-visLike can get partly blocked; other damage-types are either completely blocked above or get a pass here
		{
			//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret = " << tile->getMapData(part)->getBlock(dType);
			return tile->getMapData(part)->getBlock(dType);
		}
	}

	//Log(LOG_INFO) << "TileEngine::blockage() EXIT, (no valid part) ret 0";
	return 0; // no Valid [part].
}

/**
 * Sets the final direction from which a missile or thrown-object came;
 * for use in determining blast propagation against diagonal BigWalls.
 * This needs to be stored because the Projectile itself is long gone
 * once ExplosionBState starts.
 * @param dir - the direction as calculated in Projectile
 */
void TileEngine::setProjectileDirection(const int dir)
{
	_missileDirection = dir;
}

/**
 * Applies the explosive power to tile parts.
 * This is where the actual destruction takes place; 9 parts are affected:
 * 2 walls, floors top & bottom, up to 4 bigWalls around the perimeter,
 * plus the content-object in the center.
 * @param tile - pointer to Tile affected
 * @return, true if an objective was destroyed
 */
bool TileEngine::detonate(Tile* const tile)
{
	int expl = tile->getExplosive();
	if (expl == 0) // no explosive applied to the Tile
		return false;

	//Log(LOG_INFO) << "TileEngine::detonate() " << tile->getPosition();
	tile->setExplosive(0,0, true); // reset Tile's '_explosive' value to 0


	const Position pos = tile->getPosition();

	Tile* tiles[9];
	tiles[0] = _battleSave->getTile(Position(		// tileAbove, do floor
										pos.x,
										pos.y,
										pos.z + 1));
	tiles[1] = _battleSave->getTile(Position(		// tileEast, do westwall
										pos.x + 1,
										pos.y,
										pos.z));
	tiles[2] = _battleSave->getTile(Position(		// tileSouth, do northwall
										pos.x,
										pos.y + 1,
										pos.z));
	tiles[3] =										// do floor
	tiles[4] =										// do westwall
	tiles[5] =										// do northwall
	tiles[6] = tile;								// do object

	tiles[7] = _battleSave->getTile(Position(		// tileNorth, do bigwall south
										pos.x,
										pos.y - 1,
										pos.z));
	tiles[8] = _battleSave->getTile(Position(		// tileWest, do bigwall east
										pos.x - 1,
										pos.y,
										pos.z));
	static const int parts[9] =
	{
		MapData::O_FLOOR,		// 0 - in tileAbove
		MapData::O_WESTWALL,	// 1 - in tileEast
		MapData::O_NORTHWALL,	// 2 - in tileSouth
		MapData::O_FLOOR,		// 3 - in tile
		MapData::O_WESTWALL,	// 4 - in tile
		MapData::O_NORTHWALL,	// 5 - in tile
		MapData::O_OBJECT,		// 6 - in tile
		MapData::O_OBJECT,		// 7 - in tileNorth
		MapData::O_OBJECT		// 8 - in tileWest
	};

	int
		explTest,
//		burn,
//		fuel,
		vol = 0,
		part,
		partTemp,
		dieMCD;
	bool
		objectiveDestroyed = false,
		diagWallDestroyed = true;
//		partDestroyed;

	for (size_t
			i = 8;
			i != std::numeric_limits<size_t>::max();
			--i)
	{
		if (tiles[i] == NULL
			|| tiles[i]->getMapData(parts[i]) == NULL)
		{
			continue; // no tile or no tile-part
		}

		const int bigWall = tiles[i]->getMapData(parts[i])->getBigWall();

		if (i > 6
			&& (!
				(bigWall == Pathfinding::BIGWALL_BLOCK
					|| bigWall == Pathfinding::BIGWALL_E_S
					|| (i == 8
						&& bigWall == Pathfinding::BIGWALL_EAST)
					|| (i == 7
						&& bigWall == Pathfinding::BIGWALL_SOUTH))))
		{
			continue;
		}

		if (diagWallDestroyed == false
			&& parts[i] == MapData::O_FLOOR)
		{
			continue; // when ground shouldn't be destroyed
		}

		// kL_begin:
		if (tile->getMapData(MapData::O_OBJECT) != NULL														// if tile has object
			&& ((i == 1																						// don't hit tileEast westwall
					&& tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_EAST)			// if eastern bigWall not destroyed
				|| (i == 2																					// don't hit tileSouth northwall
					&& tile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_SOUTH)))		// if southern bigWall not destroyed
		{
			continue;
		} // kL_end.


		explTest = expl;

//		partDestroyed = false;
//		vol = 0;
		part = parts[i];

//		burn = tiles[i]->getFlammability(part);
//		fuel = tiles[i]->getFuel(part) + 1;

/*		for (int // get a yes/no volume for the object by checking its loftemps objects.
				j = 0;
				j != 12;
				++j)
		{
			if (tiles[i]->getMapData(part)->getLoftID(j) != 0)
				++vol;
		} */ // moved below_

		if (i == 6
			&& (bigWall == Pathfinding::BIGWALL_NESW
				|| bigWall == Pathfinding::BIGWALL_NWSE)  // diagonals
			&& tiles[i]->getMapData(part)->getArmor() * 2 > explTest) // not enough to destroy
		{
			diagWallDestroyed = false;
		}

		// iterate through tile-part armor and destroy all deathtiles if enough explTest
		while (tiles[i]->getMapData(part) != NULL
			&& tiles[i]->getMapData(part)->getArmor() != 255
			&& tiles[i]->getMapData(part)->getArmor() * 2 < explTest + 1)
		{
			if (explTest == expl) // only once per initial part destroyed.
			{
				for (int // get a yes/no volume for the object by checking its loftemps objects.
						j = 0;
						j != 12;
						++j)
				{
					if (tiles[i]->getMapData(part)->getLoftID(j) != 0)
						++vol;
				}
			}

			explTest -= tiles[i]->getMapData(part)->getArmor() * 2;
//			partDestroyed = true;

			if (i == 6
				&& (bigWall == Pathfinding::BIGWALL_NESW // diagonals for the current tile
					|| bigWall == Pathfinding::BIGWALL_NWSE))
			{
				diagWallDestroyed = true;
			}

//			if (_battleSave->getMissionType() == "STR_BASE_DEFENSE"
			if (_battleSave->getTacticalType() == TCT_BASEDEFENSE
				&& tiles[i]->getMapData(part)->isBaseModule() == true)
			{
				_battleSave->getModuleMap()[tile->getPosition().x / 10]
										   [tile->getPosition().y / 10].second--;
			}

			// this follows transformed object parts (object can become a ground - unless your MCDs are correct)
			dieMCD = tiles[i]->getMapData(part)->getDieMCD();
			if (dieMCD != 0)
				partTemp = tiles[i]->getMapData(part)->getDataset()->getObjects()->at(dieMCD)->getObjectType();
			else
				partTemp = part;

			if (tiles[i]->destroy(part) == true) // DESTROY HERE <-|
				objectiveDestroyed = true;

			part = partTemp;
			if (tiles[i]->getMapData(part) != NULL) // update values
			{
//				burn = tiles[i]->getFlammability(part);
//				fuel = tiles[i]->getFuel(part) + 1;
			}
		}


/*		if (i > 2 && i < 7) // hit only tile itself w/ Smoke & Fire
		{
			if (burn != 0
				&& RNG::percent(((burn + 3) / 4) + ((explTest + 23) / 24)) == true
				&& (tile->getMapData(MapData::O_OBJECT) == NULL
					|| tile->getMapData(MapData::O_OBJECT)->getBlock(DT_IN) != 0))
			{
				if (tile->getMapData(MapData::O_FLOOR) != NULL // fire needs to 'set' on something solid (see 'drop' routine elsewhere)
					|| tile->getMapData(MapData::O_OBJECT) != NULL)
				{
					tile->addFire((explTest / 36) + fuel); // use Tile::ignite()
					tile->addSmoke((explTest / 36) + fuel + RNG::generate(1,3));

					Tile* const tileAbove = _battleSave->getTile(tile->getPosition() + Position(0,0,1));
					if (tileAbove != NULL
						&& tileAbove->hasNoFloor(tile) == true
						&& RNG::percent(tile->getSmoke() * 5) == true)
					{
						tileAbove->addSmoke((tile->getSmoke() + 2) / 3);
					}
				}
			}
			else if (tile->getMapData(MapData::O_OBJECT) == NULL
				|| tile->getMapData(MapData::O_OBJECT)->getBlock(DT_SMOKE) != 0)
			{
				int smoke = (explTest / 73) + RNG::generate(0,2);
				if (partDestroyed == true)
					smoke += RNG::generate(
									1,
									(vol / 2) + 3)
						   + (vol / 2);

				tile->addSmoke(smoke);

				Tile* const tileAbove = _battleSave->getTile(tile->getPosition() + Position(0,0,1));
				if (tileAbove != NULL
					&& tileAbove->hasNoFloor(tile) == true
					&& RNG::percent(tile->getSmoke() * 5) == true)
				{
					tileAbove->addSmoke((tile->getSmoke() + 2) / 3);
				}
			}
		} */
	}


	expl = ((expl + 29) / 30);

	if (tile->ignite((expl + 1) / 2) == false)
		tile->addSmoke(expl + vol);

	if (tile->getSmoke() != 0)
	{
		Tile* const tileAbove = _battleSave->getTile(tile->getPosition() + Position(0,0,1));
		if (tileAbove != NULL
			&& tileAbove->hasNoFloor(tile) == true
			&& RNG::percent(tile->getSmoke() * 8) == true)
		{
			tileAbove->addSmoke((tile->getSmoke()) / 3);
		}
	}

	return objectiveDestroyed;
}

/**
 * Checks for chained explosions.
 * Chained explosions are explosions which occur after an explosive map object is destroyed.
 * May be due a direct hit, other explosion or fire.
 * @return, tile on which an explosion occurred
 */
Tile* TileEngine::checkForTerrainExplosions()
{
	for (size_t
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		if (_battleSave->getTiles()[i]->getExplosive() != 0)
			return _battleSave->getTiles()[i];
	}

	return NULL;
}

/**
 * Opens a door (if any) by rightclick or by walking through it.
 * @note The unit has to face in the right direction.
 * @param unit		- pointer to a BattleUnit trying the door
 * @param rtClick	- true if the player right-clicked (default false)
 * @param dir		- direction to check for a door (default -1)
 * @return, -1 there is no door, you can walk through; or you're a tank and can't do sweet shit with a door except blast the fuck out of it.
 *			 0 normal door opened, make a squeaky sound and you can walk through;
 *			 1 ufo door is starting to open, make a whoosh sound, don't walk through;
 *			 3 ufo door is still opening, don't walk through it yet. (have patience, futuristic technology...)
 *			 4 not enough TUs
 *			 5 would contravene fire reserve
 */
int TileEngine::unitOpensDoor(
		BattleUnit* const unit,
		const bool rtClick,
		int dir)
{
	//Log(LOG_INFO) << "unitOpensDoor()";
	int door = -1;

	if (dir == -1)
		dir = unit->getDirection();

	if (rtClick == true
		&& ((unit->getUnitRules() != NULL
				&& unit->getUnitRules()->isMechanical() == true)
			|| unit->getArmor()->getSize() != 1
			|| dir % 2 == 1)) // RMB works only for cardinal directions
	{
		return door;
	}

	Tile* tile;
	int
		tuCost = 0,
		z = 0;

	if (unit->getTile()->getTerrainLevel() < -12)
		z = 1; // if standing on stairs, check the tile above instead

	const int unitSize = unit->getArmor()->getSize();
	for (int
			x = 0;
			x != unitSize
				&& door == -1;
			++x)
	{
		for (int
				y = 0;
				y != unitSize
					&& door == -1;
				++y)
		{
			std::vector<std::pair<Position, int> > checkPos;
			tile = _battleSave->getTile(unit->getPosition() + Position(x,y,z));

			if (tile == NULL)
				continue;

			const Position posUnit = unit->getPosition();

			switch (dir)
			{
				case 0: // north
						checkPos.push_back(std::make_pair(Position(0, 0, 0), MapData::O_NORTHWALL));	// origin
					if (x != 0)
						checkPos.push_back(std::make_pair(Position(0,-1, 0), MapData::O_WESTWALL));		// one tile north
				break;

				case 1: // north east
						checkPos.push_back(std::make_pair(Position(0, 0, 0), MapData::O_NORTHWALL));	// origin
						checkPos.push_back(std::make_pair(Position(1,-1, 0), MapData::O_WESTWALL));		// one tile north-east
					if (rtClick == true)
					{
						checkPos.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));		// one tile east
						checkPos.push_back(std::make_pair(Position(1, 0, 0), MapData::O_NORTHWALL));	// one tile east
					}
/*					if (rtClick
						|| testAdjacentDoor(posUnit, MapData::O_NORTHWALL, 1)) // kL
					{
						checkPos.push_back(std::make_pair(Position(0, 0, 0), MapData::O_NORTHWALL));	// origin
						checkPos.push_back(std::make_pair(Position(1,-1, 0), MapData::O_WESTWALL));		// one tile north-east
						checkPos.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));		// one tile east
						checkPos.push_back(std::make_pair(Position(1, 0, 0), MapData::O_NORTHWALL));	// one tile east
					} */
				break;

				case 2: // east
						checkPos.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));		// one tile east
				break;

				case 3: // south-east
					if (y == 0)
						checkPos.push_back(std::make_pair(Position(1, 1, 0), MapData::O_WESTWALL));		// one tile south-east
					if (x == 0)
						checkPos.push_back(std::make_pair(Position(1, 1, 0), MapData::O_NORTHWALL));	// one tile south-east
					if (rtClick == true)
					{
						checkPos.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));		// one tile east
						checkPos.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
					}
/*					if (rtClick
						|| testAdjacentDoor(posUnit, MapData::O_NORTHWALL, 3)) // kL
					{
						checkPos.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));		// one tile east
						checkPos.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
						checkPos.push_back(std::make_pair(Position(1, 1, 0), MapData::O_WESTWALL));		// one tile south-east
						checkPos.push_back(std::make_pair(Position(1, 1, 0), MapData::O_NORTHWALL));	// one tile south-east
					} */
				break;

				case 4: // south
						checkPos.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
				break;

				case 5: // south-west
						checkPos.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
						checkPos.push_back(std::make_pair(Position(-1, 1, 0), MapData::O_NORTHWALL));	// one tile south-west
					if (rtClick == true)
					{
						checkPos.push_back(std::make_pair(Position(0, 1, 0), MapData::O_WESTWALL));		// one tile south
						checkPos.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
					}
/*					if (rtClick
						|| testAdjacentDoor(posUnit, MapData::O_NORTHWALL, 5)) // kL
					{
						checkPos.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
						checkPos.push_back(std::make_pair(Position( 0, 1, 0), MapData::O_WESTWALL));	// one tile south
						checkPos.push_back(std::make_pair(Position( 0, 1, 0), MapData::O_NORTHWALL));	// one tile south
						checkPos.push_back(std::make_pair(Position(-1, 1, 0), MapData::O_NORTHWALL));	// one tile south-west
					} */
				break;

				case 6: // west
						checkPos.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
					if (y != 0)
						checkPos.push_back(std::make_pair(Position(-1, 0, 0), MapData::O_NORTHWALL));	// one tile west
				break;

				case 7: // north-west
						checkPos.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
						checkPos.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_NORTHWALL));	// origin
					if (x != 0)
						checkPos.push_back(std::make_pair(Position(-1,-1, 0), MapData::O_WESTWALL));	// one tile north
					if (y != 0)
						checkPos.push_back(std::make_pair(Position(-1,-1, 0), MapData::O_NORTHWALL));	// one tile north
					if (rtClick == true)
					{
						checkPos.push_back(std::make_pair(Position( 0,-1, 0), MapData::O_WESTWALL));	// one tile north
						checkPos.push_back(std::make_pair(Position(-1, 0, 0), MapData::O_NORTHWALL));	// one tile west
					}
/*					if (rtClick
						|| testAdjacentDoor(posUnit, MapData::O_NORTHWALL, 7)) // kL
					{
						//Log(LOG_INFO) << ". north-west";
						checkPos.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
						checkPos.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_NORTHWALL));	// origin
						checkPos.push_back(std::make_pair(Position( 0,-1, 0), MapData::O_WESTWALL));	// one tile north
						checkPos.push_back(std::make_pair(Position(-1, 0, 0), MapData::O_NORTHWALL));	// one tile west
					} */
			}


			int part = 0;

			for (std::vector<std::pair<Position, int> >::const_iterator
					i = checkPos.begin();
					i != checkPos.end()
						&& door == -1;
					++i)
			{
				tile = _battleSave->getTile(
										posUnit
											+ Position(x, y, z)
											+ i->first);
				if (tile != NULL)
				{
					door = tile->openDoor(
										i->second,
										unit,
										_battleSave->getBattleGame()->getReservedAction());
					if (door != -1)
					{
						part = i->second;
						if (door == 1)
							openAdjacentDoors(
											posUnit
												+ Position(x, y, z)
												+ i->first,
											i->second);
					}
				}
			}

			if (door == 0
				&& rtClick == true)
			{
				if (part == MapData::O_WESTWALL)
					part = MapData::O_NORTHWALL;
				else
					part = MapData::O_WESTWALL;

				tuCost = tile->getTUCostTile(
										part,
										unit->getMoveTypeUnit());
				//Log(LOG_INFO) << ". normal door, RMB, part = " << part << ", TUcost = " << tuCost;
			}
			else if (door == 1
				|| door == 4)
			{
				tuCost = tile->getTUCostTile(
										part,
										unit->getMoveTypeUnit());
				//Log(LOG_INFO) << ". UFO door, part = " << part << ", TUcost = " << tuCost;
			}
		}
	}

	if (tuCost != 0)
	{
		if (_battleSave->getBattleGame()->checkReservedTU(
														unit,
														tuCost) == true)
		{
			if (unit->spendTimeUnits(tuCost) == true)
			{
//				tile->animate(); // ensures frame advances for ufo doors to update TU cost

				if (rtClick == true) // kL: try this one ...... <--- let UnitWalkBState handle FoV & new unit visibility, when walking (ie, not RMB).
				{
					_battleSave->getBattleGame()->checkForProximityGrenades(unit); // kL

					calculateFOV(unit->getPosition()); // calculate FoV for everyone within sight-range, incl. unit.

					// look from the other side (may need check reaction fire)
					// kL_note: This seems redundant, but hey maybe it removes now-unseen units from a unit's visible-units vector ....
					const std::vector<BattleUnit*>* const visibleUnits = unit->getVisibleUnits();
					for (size_t
							i = 0;
							i != visibleUnits->size();
							++i)
					{
						calculateFOV(visibleUnits->at(i)); // calculate FoV for all units that are visible to this unit.
					}
				}
			}
			else // not enough TU
				return 4;
		}
		else // reserved TU
			return 5;
	}

// -1 there is no door, you can walk through; or you're a tank and can't do sweet shit with a door except blast the fuck out of it.
//	0 normal door opened, make a squeaky sound and you can walk through;
//	1 ufo door is starting to open, make a whoosh sound, don't walk through;
//	3 ufo door is still opening, don't walk through it yet. (have patience, futuristic technology...)
//	4 not enough TUs
//	5 would contravene fire reserve

	return door;
}

/**
 * Checks for a door connected to a wall at this position,
 * so that units can open double doors diagonally.
 * @param pos	- the starting position
 * @param part	- the wall to test
 * @param dir	- the direction to check out
 */
/* bool TileEngine::testAdjacentDoor(
		Position pos,
		int part,
		int dir)
{
	Position offset;
	switch (dir)
	{
		// only Northwall-doors are handled at present
		case 1: offset = Position( 1, 0, 0);	break;	// northwall in tile to east
		case 3: offset = Position( 1, 1, 0);	break;	// northwall in tile to south-east
		case 5: offset = Position(-1, 1, 0);	break;	// northwall in tile to south-west
		case 7: offset = Position(-1, 0, 0);			// northwall in tile to west
	}

	const Tile* const tile = _battleSave->getTile(pos + offset);
	if (tile != NULL
		&& tile->getMapData(part) != NULL
		&& tile->getMapData(part)->isUFODoor() == true)
	{
		return true;
	}

	return false;
} */

/**
 * Opens any doors connected to this wall at this position,
 * Keeps processing till it hits a non-ufo-door.
 * @param pos	- the starting position
 * @param part	- the wall to open ( defines which direction to check )
 */
void TileEngine::openAdjacentDoors(
		Position pos,
		int part)
{
	Position offset;
	const bool westSide = (part == 1);

	for (int
			i = 1;
			;
			++i)
	{
		offset = westSide? Position(0, i, 0): Position(i, 0, 0);
		Tile* const tile = _battleSave->getTile(pos + offset);
		if (tile != NULL
			&& tile->getMapData(part) != NULL
			&& tile->getMapData(part)->isUFODoor() == true)
		{
			tile->openDoor(part);
		}
		else
			break;
	}

	for (int
			i = -1;
			;
			--i)
	{
		offset = westSide? Position(0, i, 0): Position(i, 0, 0);
		Tile* const tile = _battleSave->getTile(pos + offset);
		if (tile != NULL
			&& tile->getMapData(part) != NULL
			&& tile->getMapData(part)->isUFODoor() == true)
		{
			tile->openDoor(part);
		}
		else
			break;
	}
}

/**
 * Closes ufo doors.
 * @return, # of doors that get closed
 */
int TileEngine::closeUfoDoors()
{
	int closed = 0;

	for (size_t // prepare a list of tiles on fire/smoke & close any ufo doors
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		if (_battleSave->getTiles()[i]->getUnit()
			&& _battleSave->getTiles()[i]->getUnit()->getArmor()->getSize() > 1)
		{
			const BattleUnit* const bu = _battleSave->getTiles()[i]->getUnit();

			const Tile
				* const tile = _battleSave->getTiles()[i],
				* const tileNorth = _battleSave->getTile(tile->getPosition() + Position(0,-1, 0)),
				* const tileWest = _battleSave->getTile(tile->getPosition() + Position(-1, 0, 0));
			if ((tile->isUfoDoorOpen(MapData::O_NORTHWALL) == true
					&& tileNorth != NULL
					&& tileNorth->getUnit() != NULL // probly not needed.
					&& tileNorth->getUnit() == bu)
				|| (tile->isUfoDoorOpen(MapData::O_WESTWALL) == true
					&& tileWest != NULL
					&& tileWest->getUnit() != NULL // probly not needed.
					&& tileWest->getUnit() == bu))
			{
				continue;
			}
		}

		closed += _battleSave->getTiles()[i]->closeUfoDoor();
	}

	return closed;
}

/**
 * Calculates a line trajectory, using bresenham algorithm in 3D.
 * @param origin			- reference the origin (voxelspace for 'doVoxelCheck'; tilespace otherwise)
 * @param target			- reference the target (voxelspace for 'doVoxelCheck'; tilespace otherwise)
 * @param storeTrajectory	- true will store the whole trajectory; otherwise only the last position gets stored
 * @param trajectory		- pointer to a vector of Positions in which the trajectory will be stored
 * @param excludeUnit		- pointer to a BattleUnit to be excluded from collision detection
 * @param doVoxelCheck		- true to check against a voxel; false to check tile blocking for FoV (true for unit visibility and line of fire, false for terrain visibility) (default true)
 * @param onlyVisible		- true to skip invisible units (default false) [used in FPS view]
 * @param excludeAllBut		- pointer to a unit that's to be considered exclusively for ray hits [optional] (default NULL)
 * @return,  -1 hit nothing
 *			0-3 tile-part
 *			  4 unit
 *			  5 out-of-map
 *			-99 special case for calculateFOV() so as not to remove the last tile of the trajectory
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
int TileEngine::calculateLine(
		const Position& origin,
		const Position& target,
		const bool storeTrajectory,
		std::vector<Position>* const trajectory,
		const BattleUnit* const excludeUnit,
		const bool doVoxelCheck, // false is used only for calculateFOV()
		const bool onlyVisible,
		const BattleUnit* const excludeAllBut)
{
	bool
		swap_xy,
		swap_xz;
	int
		x,x0,x1, delta_x,step_x,
		y,y0,y1, delta_y,step_y,
		z,z0,z1, delta_z,step_z,

		drift_xy,drift_xz,

		cx,cy,cz,

		horiBlock,vertBlock,
		ret;

	Position lastPoint (origin); // init.

	x0 = origin.x; // start & end points
	x1 = target.x;

	y0 = origin.y;
	y1 = target.y;

	z0 = origin.z;
	z1 = target.z;

	swap_xy = std::abs(y1 - y0) > std::abs(x1 - x0); // 'steep' xy Line, make longest delta x plane
	if (swap_xy == true)
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
	}

	swap_xz = std::abs(z1 - z0) > std::abs(x1 - x0); // do same for xz
	if (swap_xz == true)
	{
		std::swap(x0, z0);
		std::swap(x1, z1);
	}

	delta_x = std::abs(x1 - x0); // delta is Length in each plane
	delta_y = std::abs(y1 - y0);
	delta_z = std::abs(z1 - z0);

	drift_xy = drift_xz = delta_x / 2;	// drift controls when to step in 'shallow' planes;
//	drift_xz = delta_x / 2;				// starting value keeps Line centered

	step_x = step_y = step_z = 1; // direction of Line
	if (x0 > x1) step_x = -1;
	if (y0 > y1) step_y = -1;
	if (z0 > z1) step_z = -1;

	y = y0; // starting point
	z = z0;
	for (
			x = x0; // step through longest delta (which has been swapped to x)
			x != x1 + step_x;
			x += step_x)
	{
/*		if (x < 0 || y < 0 || z < 0 // kL->
			|| x > _battleSave->getMapSizeX() * 16
			|| y > _battleSave->getMapSizeY() * 16
			|| z > _battleSave->getMapSizeZ() * 24)
		{
			break;
		} */

		cx = x; cy = y; cz = z; // copy position
		if (swap_xz == true) std::swap(cx,cz); // unswap (in reverse)
		if (swap_xy == true) std::swap(cx,cy);

		if (storeTrajectory == true
			&& trajectory != NULL)
		{
			trajectory->push_back(Position(cx,cy,cz));
		}

		if (doVoxelCheck == true) // passes through this voxel, for Unit visibility & LoS/LoF
		{
			ret = voxelCheck(
						Position(cx,cy,cz),
						excludeUnit,
						false,
						onlyVisible,
						excludeAllBut);

			if (ret != VOXEL_EMPTY) // hit.
			{
				if (trajectory != NULL) // store the position of impact
					trajectory->push_back(Position(cx,cy,cz));

				return ret;
			}
		}
		else // for Terrain visibility, ie. FoV / Fog of War.
		{
			Tile
				* const startTile = _battleSave->getTile(lastPoint),
				* const endTile = _battleSave->getTile(Position(cx,cy,cz));

//			if (_battleSave->getSelectedUnit()->getId() == 389)
//			{
//				int dist = distance(origin, Position(cx, cy, cz));
//				Log(LOG_INFO) << "unitID = " << _battleSave->getSelectedUnit()->getId() << " dist = " << dist;
//			}

			horiBlock = horizontalBlockage(
										startTile,
										endTile,
										DT_NONE);
			vertBlock = verticalBlockage(
										startTile,
										endTile,
										DT_NONE);
			// kL_TEST:
/*			BattleUnit* selUnit = _battleSave->getSelectedUnit();
			if (selUnit
//				&& selUnit->getFaction() == FACTION_PLAYER
				&& selUnit->getId() == 389
				&& startTile != endTile)
//				&& _battleSave->getDebugMode())
//				&& _battleSave->getTurn() > 1)
			{
//				Position posUnit = selUnit->getPosition();
//				if ( //(posUnit.x == cx))
//						&& std::abs(posUnit.y - cy) > 4) ||
//					(posUnit.y == cy))
//						&& std::abs(posUnit.x - cx) > 4))
				{
					//kL_debug = true;

					Log(LOG_INFO) << ". start " << lastPoint << " hori = " << horiBlock;
					Log(LOG_INFO) << ". . end " << Position(cx,cy,cz) << " vert = " << vertBlock;
				}
			} */ // kL_TEST_end.

			if (horiBlock < 0) // hit content-object
			{
				if (vertBlock < 1)
					return horiBlock;
				else
					horiBlock = 0;
			}

			horiBlock += vertBlock;
//			if (horiBlock > 0)
			if (horiBlock != 0)
				return horiBlock;

			lastPoint = Position(cx,cy,cz);
		}

		drift_xy = drift_xy - delta_y; // update progress in other planes
		drift_xz = drift_xz - delta_z;

		if (drift_xy < 0) // step in y plane
		{
			y = y + step_y;
			drift_xy = drift_xy + delta_x;

			if (doVoxelCheck == true) // check for xy diagonal intermediate voxel step, for Unit visibility
			{
				cx = x; cy = y; cz = z;
				if (swap_xz == true) std::swap(cx,cz);
				if (swap_xy == true) std::swap(cx,cy);

				ret = voxelCheck(
							Position(cx,cy,cz),
							excludeUnit,
							false,
							onlyVisible,
							excludeAllBut);

				if (ret != VOXEL_EMPTY)
				{
					if (trajectory != NULL)
						trajectory->push_back(Position(cx,cy,cz)); // store the position of impact

					return ret;
				}
			}
		}

		if (drift_xz < 0) // same in z
		{
			z = z + step_z;
			drift_xz = drift_xz + delta_x;

			if (doVoxelCheck == true) // check for xz diagonal intermediate voxel step
			{
				cx = x; cy = y; cz = z;
				if (swap_xz == true) std::swap(cx,cz);
				if (swap_xy == true) std::swap(cx,cy);

				ret = voxelCheck(
							Position(cx,cy,cz),
							excludeUnit,
							false,
							onlyVisible,
							excludeAllBut);

				if (ret != VOXEL_EMPTY)
				{
					if (trajectory != NULL) // store the position of impact
						trajectory->push_back(Position(cx,cy,cz));

					return ret;
				}
			}
		}
	}

	return VOXEL_EMPTY;
}

/**
 * Calculates a parabola trajectory, used for throwing items.
 * @param origin			- reference the origin in voxelspace
 * @param target			- reference the target in voxelspace
 * @param storeTrajectory	- true will store the whole trajectory - otherwise it stores the last position only
 * @param trajectory		- pointer to a vector of Positions in which the trajectory will be stored
 * @param excludeUnit		- pointer to a unit to exclude - makes sure the trajectory does not hit the shooter itself
 * @param arc				- how high the parabola goes: 1.0 is almost straight throw, 3.0 is a very high throw, to throw over a fence for example
 * @param acu				- the deviation of the angles that should be taken into account. 1.0 is perfection. // now superceded by @param delta...
 * @param delta				- reference the deviation of the angles that should be taken into account, (0,0,0) is perfection (default Position(0,0,0))
 * @return,  -1 hit nothing
 *			0-3 tile-part (floor / westwall / northwall / content)
 *			  4 unit
 *			  5 out-of-map
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
int TileEngine::calculateParabola(
		const Position& origin,
		const Position& target,
		const bool storeTrajectory,
		std::vector<Position>* const trajectory,
		const BattleUnit* const excludeUnit,
		const double arc,
		const Position& delta)
{
	//Log(LOG_INFO) << "TileEngine::calculateParabola()";
	const double
		ro = std::sqrt(static_cast<double>(
				  (target.x - origin.x) * (target.x - origin.x)
				+ (target.y - origin.y) * (target.y - origin.y)
				+ (target.z - origin.z) * (target.z - origin.z)));
	double
		fi = std::acos(static_cast<double>(target.z - origin.z) / ro),
		te = std::atan2(
				static_cast<double>(target.y - origin.y),
				static_cast<double>(target.x - origin.x));

	if (AreSame(ro, 0.)) // just in case.
		return VOXEL_EMPTY;

//	te *= acu;
//	fi *= acu;
	te += (delta.x / ro) / 2. * M_PI;						// horizontal magic value
	fi += ((delta.z + delta.y) / ro) / 14. * M_PI * arc;	// another magic value (vertical), to make it in line with fire spread

	const double
		zA = std::sqrt(ro) * arc,
		zK = (4. * zA) / (ro * ro);

	int
		x = origin.x,
		y = origin.y,
		z = origin.z,
		i = 8;

	Position lastPosition = Position(x, y, z);

	while (z > 0)
	{
		x = static_cast<int>(static_cast<double>(origin.x) + static_cast<double>(i) * std::cos(te) * std::sin(fi));
		y = static_cast<int>(static_cast<double>(origin.y) + static_cast<double>(i) * std::sin(te) * std::sin(fi));
		z = static_cast<int>(static_cast<double>(origin.z) + static_cast<double>(i) * std::cos(fi)
				- zK * (static_cast<double>(i) - ro / 2.) * (static_cast<double>(i) - ro / 2.)
				+ zA);

		if (storeTrajectory == true
			&& trajectory != NULL)
		{
			trajectory->push_back(Position(x,y,z));
		}

		const Position nextPosition = Position(x,y,z);
		int test = calculateLine(
							lastPosition,
							nextPosition,
							false,
							NULL,
							excludeUnit);
//		int test = voxelCheck(
//							Position(x,y,z),
//							excludeUnit);
		if (test != VOXEL_EMPTY)
		{
			if (lastPosition.z < nextPosition.z)
				test = VOXEL_OUTOFBOUNDS;

			if (storeTrajectory == false // store only the position of impact
				&& trajectory != NULL)
			{
				trajectory->push_back(nextPosition);
			}

			//Log(LOG_INFO) << ". cP() ret = " << test;
			return test;
		}

		lastPosition = Position(x,y,z);
		++i;
	}

	if (storeTrajectory == false // store only the position of impact
		&& trajectory != NULL)
	{
		trajectory->push_back(Position(x,y,z));
	}

	//Log(LOG_INFO) << ". cP() ret VOXEL_EMTPY";
	return VOXEL_EMPTY;
}

/**
 * Validates a throw action.
 * @param action		- reference the action to validate
 * @param originVoxel	- reference the origin point of the action
 * @param targetVoxel	- reference the target point of the action
 * @param arc			- pointer to a curvature of the throw (default NULL)
 * @param voxelType		- pointer to a type of voxel at which this parabola terminates (default NULL)
 * @return, true if throw is valid
 */
bool TileEngine::validateThrow(
						const BattleAction& action,
						const Position& originVoxel,
						const Position& targetVoxel,
						double* const arc,
						int* const voxelType)
{
	//Log(LOG_INFO) << "\nTileEngine::validateThrow()"; //, cf Projectile::calculateThrow()";
	double parabolicCoefficient = 0.; // higher arc means lower arc IG. eh ......

	const Position targetPos = targetVoxel / Position(16, 16, 24);
	if (targetPos != originVoxel / Position(16, 16, 24))
	{
		parabolicCoefficient = 0.78;

		// kL_note: Unfortunately, this prevents weak units from throwing heavy
		// objects at their own feet. ( needs starting arc ~0.8, less if kneeled )
/*		if (action.type == BA_THROW)
		{
			parabolicCoefficient += 0.8;
//			parabolicCoefficient += std::max(
//						0.48,
//						1.73 / sqrt(
//									sqrt(
//										static_cast<double>(action.actor->getBaseStats()->strength) * (action.actor->getAccuracyModifier() / 2.0 + 0.5)
//										/ static_cast<double>(action.weapon->getRules()->getWeight()))));
		}
		// kL_note: And arcing shots from targeting their origin tile. ( needs starting arc ~0.1 )
		else // spit, etc
		{
			// arcing projectile weapons assume a fixed strength and weight.(70 and 10 respectively)
			// curvature should be approximately 1.06358350461 at this point.
//			parabolicCoefficient = 1.73 / sqrt(sqrt(70.0 / 10.0)) + kneel; // OR ...
//			parabolicCoefficient += 1.0635835046056873518242669985672;
			parabolicCoefficient += 1.0;
		}

		if (action.actor->isKneeled() == true)
			parabolicCoefficient -= 0.5; // stock: 0.1 */
	}
	//Log(LOG_INFO) << ". starting arc = " << parabolicCoefficient;

	const Tile* const targetTile = _battleSave->getTile(action.target);

	if (ProjectileFlyBState::validThrowRange(
										&action,
										originVoxel,
										targetTile) == false)
	{
		//Log(LOG_INFO) << ". vT() ret FALSE, ThrowRange not valid";
		return false;
	}

	if (action.type == BA_THROW
		&& targetTile != NULL
		&& targetTile->getMapData(MapData::O_OBJECT) != NULL
		&& (targetTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NESW
			|| targetTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NWSE))
//		&& (action.weapon->getRules()->getBattleType() == BT_GRENADE
//			|| action.weapon->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
//		&& targetTile->getMapData(MapData::O_OBJECT)->getTUCostObject(MT_WALK) == 255)
	{
		return false; // prevent Grenades from landing on diagonal BigWalls.
	}


	// check for test up from the lowest trajectory
	bool found = false;
	while (found == false // try several different curvatures
		&& parabolicCoefficient < 8.3)
	{
		//Log(LOG_INFO) << ". . arc = " << parabolicCoefficient;
		std::vector<Position> trajectory;
		const int test = calculateParabola(
										originVoxel,
										targetVoxel,
										false,
										&trajectory,
										action.actor,
										parabolicCoefficient);
		//Log(LOG_INFO) << ". . calculateParabola() = " << test;

		if (test != VOXEL_OUTOFBOUNDS
			&& test != VOXEL_WESTWALL
			&& test != VOXEL_NORTHWALL
			&& (trajectory.at(0) / Position(16, 16, 24)) == targetPos)
		{
			//Log(LOG_INFO) << ". . . found TRUE";
			found = true;

			if (voxelType != NULL)
				*voxelType = test;
		}
		else
			parabolicCoefficient += 0.3;
	}

	if (parabolicCoefficient >= 8.3)
	{
		//Log(LOG_INFO) << ". vT() ret FALSE, arc > 8.3";
		return false;
	}


	// now test continues rising to find upper limit
	double parabolicCoefficient2 = parabolicCoefficient;

	found = false;
	while (found == false // try several different curvatures
		&& parabolicCoefficient2 < 8.3)
	{
		//Log(LOG_INFO) << ". . arc = " << parabolicCoefficient2;
		std::vector<Position> trajectory;
		const int test = calculateParabola(
										originVoxel,
										targetVoxel,
										false,
										&trajectory,
										action.actor,
										parabolicCoefficient2);
		//Log(LOG_INFO) << ". . calculateParabola() = " << test;

		if (test == VOXEL_OUTOFBOUNDS
			|| test == VOXEL_WESTWALL
			|| test == VOXEL_NORTHWALL
			|| (trajectory.at(0) / Position(16, 16, 24)) != targetPos)
		{
			//Log(LOG_INFO) << ". . . found TRUE";
			found = true;
			parabolicCoefficient2 -= 0.3; // back off from the upper lip.
		}
		else
			parabolicCoefficient2 += 0.3;
	}

	// use the average of upper & lower limits:
	// Lessens chance of unit bouncing a thrown item back off a wall by
	// barely skimming it overtop, after accuracy is applied.
	parabolicCoefficient = (parabolicCoefficient + parabolicCoefficient2) / 2.;

	if (arc != NULL)
		*arc = parabolicCoefficient;

	//Log(LOG_INFO) << ". vT() ret TRUE";
	return true;
}

/**
 * Calculates 'ground' z-value for a particular voxel - used for projectile shadow.
 * @param voxel - reference the voxel to trace down
 * @return, z-coord of 'ground'
 */
int TileEngine::castedShade(const Position& voxel) const
{
	int start_z = voxel.z;
	Position testCoord = voxel / Position(16,16,24);

	const Tile* tile = _battleSave->getTile(testCoord);
	while (tile != NULL
		&& tile->isVoid(false, false) == true
		&& tile->getUnit() == NULL)
	{
		start_z = testCoord.z * 24;
		--testCoord.z;

		tile = _battleSave->getTile(testCoord);
	}

	Position testVoxel = voxel;

	int z;
	for (
			z = start_z;
			z > 0;
			--z)
	{
		testVoxel.z = z;

		if (voxelCheck(testVoxel, NULL) != VOXEL_EMPTY)
			break;
	}

	return z;
}

/**
 * Traces voxel visibility.
 * @param voxel - reference the voxel coordinates
 * @return, true if visible
 */
/* bool TileEngine::isVoxelVisible(const Position& voxel) const
{
	const int start_z = voxel.z + 3; // slight Z adjust
	if (start_z / 24 != voxel.z / 24)
		return true; // visible!

	Position testVoxel = voxel;

	const int end_z = (start_z / 24) * 24 + 24;
	for (int // only OBJECT can cause additional occlusion (because of any shape)
			z = start_z;
			z < end_z;
			++z)
	{
		testVoxel.z = z;
		if (voxelCheck(testVoxel, NULL) == VOXEL_OBJECT)
			return false;

		++testVoxel.x;
		if (voxelCheck(testVoxel, NULL) == VOXEL_OBJECT)
			return false;

		++testVoxel.y;
		if (voxelCheck(testVoxel, NULL) == VOXEL_OBJECT)
			return false;
	}

	return true;
} */

/**
 * Checks if we hit a targetPos in voxel space.
 * @param targetPos			- reference the Position to check in voxelspace
 * @param excludeUnit		- pointer to unit NOT to do checks for (default NULL)
 * @param excludeAllUnits	- true to NOT do checks on any unit (default false)
 * @param onlyVisible		- true to consider only visible units (default false)
 * @param excludeAllBut		- pointer to an only unit to be considered (default NULL)
 * @return,  -1 hit nothing
 *			0-3 tile-part (floor / westwall / northwall / content)
 *			  4 unit
 *			  5 out-of-map
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
int TileEngine::voxelCheck(
		const Position& targetPos,
		const BattleUnit* const excludeUnit,
		const bool excludeAllUnits,
		const bool onlyVisible,
		const BattleUnit* const excludeAllBut) const
{
	//Log(LOG_INFO) << "TileEngine::voxelCheck()"; // massive lag-to-file, Do not use.
	const Tile* targetTile = _battleSave->getTile(targetPos / Position(16,16,24)); // converts to tilespace -> Tile
	//Log(LOG_INFO) << ". targetTile " << targetTile->getPosition();
	// check if we are out of the map
	if (targetTile == NULL
		|| targetPos.x < 0
		|| targetPos.y < 0
		|| targetPos.z < 0)
	{
		//Log(LOG_INFO) << ". vC() ret VOXEL_OUTOFBOUNDS";
		return VOXEL_OUTOFBOUNDS;
	}

	const Tile* const tileBelow = _battleSave->getTile(targetTile->getPosition() + Position(0,0,-1));
	if (targetTile->isVoid(false, false) == true
		&& targetTile->getUnit() == NULL
		&& (tileBelow == NULL
			|| tileBelow->getUnit() == NULL))
	{
		//Log(LOG_INFO) << ". vC() ret VOXEL_EMPTY";
		return VOXEL_EMPTY;
	}

	// kL_note: should allow items to be thrown through a gravLift down to the floor below
	if (targetPos.z %24 < 2
		&& targetTile->getMapData(MapData::O_FLOOR) != NULL
		&& targetTile->getMapData(MapData::O_FLOOR)->isGravLift() == true)
	{
		//Log(LOG_INFO) << "voxelCheck() isGravLift";
		//Log(LOG_INFO) << ". level = " << targetTile->getPosition().z;
		if (targetTile->getPosition().z == 0
			|| (tileBelow != NULL
				&& tileBelow->getMapData(MapData::O_FLOOR) != NULL
				&& tileBelow->getMapData(MapData::O_FLOOR)->isGravLift() == false))
		{
			//Log(LOG_INFO) << ". vC() ret VOXEL_FLOOR";
			return VOXEL_FLOOR;
		}
	}

	// first check TERRAIN tile/voxel data
	// not to allow 2x2 units to stick through walls
	for (int // terrain parts [0=floor, 1/2=walls, 3=content-object]
			i = 0;
			i < 4;
			++i)
	{
		if (targetTile->isUfoDoorOpen(i) == true)
			continue;

		const MapData* const dataTarget = targetTile->getMapData(i);
		if (dataTarget != NULL)
		{
			const int
				x = 15 - targetPos.x %16,	// x-direction is reversed
				y = targetPos.y %16;		// y-direction is standard

			const int loftIdx = ((dataTarget->getLoftID((targetPos.z %24) / 2) * 16) + y);
			if (loftIdx < static_cast<int>(_voxelData->size()) // davide, http://openxcom.org/forum/index.php?topic=2934.msg32146#msg32146 (x2 _below)
				&& _voxelData->at(loftIdx) & (1 << x))
			{
				//Log(LOG_INFO) << ". vC() ret = " << i;
				return i;
			}
		}
	}

	if (excludeAllUnits == false)
	{
		const BattleUnit* targetUnit = targetTile->getUnit();
		// sometimes there is unit on the tile below, but sticks up into this tile with its head.
		if (targetUnit == NULL
			&& targetTile->hasNoFloor(NULL) == true)
		{
			targetTile = _battleSave->getTile(Position( // tileBelow
													targetPos.x / 16,
													targetPos.y / 16,
													targetPos.z / 24 - 1));
			if (targetTile != NULL)
				targetUnit = targetTile->getUnit();
		}

		if (targetUnit != NULL
			&& targetUnit != excludeUnit
			&& (excludeAllBut == NULL
				|| targetUnit == excludeAllBut)
			&& (onlyVisible == false
				|| targetUnit->getUnitVisible() == true))
		{
			const Position unitPos = targetUnit->getPosition();
			const int target_z = unitPos.z * 24 + targetUnit->getFloatHeight() - targetTile->getTerrainLevel(); // floor-level voxel

			if (targetPos.z > target_z
				&& targetPos.z <= target_z + targetUnit->getHeight()) // if hit is between foot- and hair-level voxel layers (z-axis)
			{
				int ent = 0;
				const int
					x = targetPos.x %16, // where on the x-axis
					y = targetPos.y %16; // where on the y-axis
				// That should be (8,8,10) as per BattlescapeGame::handleNonTargetAction(), if (_currentAction.type == BA_HIT)

				if (targetUnit->getArmor()->getSize() > 1) // for large units...
				{
					const Position tilePos = targetTile->getPosition();
					ent = (tilePos.x - unitPos.x) + ((tilePos.y - unitPos.y) * 2);
					//Log(LOG_INFO) << ". vC, large unit, LoFT entry = " << ent;
				}

//				if (ent > -1)
//				{
				const int loftIdx = (targetUnit->getLoftemps(ent) * 16) + y;
				//Log(LOG_INFO) << "loftIdx = " << loftIdx << " vD-size = " << (int)_voxelData->size();
				if (loftIdx < static_cast<int>(_voxelData->size()) // davide, http://openxcom.org/forum/index.php?topic=2934.msg32146#msg32146 (x2 ^above)
					&& _voxelData->at(loftIdx) & (1 << x)) // if the voxelData at loftIdx is "1" solid:
				{
					//Log(LOG_INFO) << ". vC() ret VOXEL_UNIT";
					return VOXEL_UNIT;
				}
//				}
//				else Log(LOG_INFO) << "ERROR TileEngine::voxelCheck() LoFT entry = " << ent;
			}
		}
	}

	//Log(LOG_INFO) << ". vC() ret VOXEL_EMPTY"; // massive lag-to-file, Do not use.
	return VOXEL_EMPTY;
}

/**
 * Calculates the distance between 2 points - rounded to nearest INT.
 * @param pos1 - reference the first Position
 * @param pos2 - reference the second Position
 * @return, distance
 */
int TileEngine::distance(
		const Position& pos1,
		const Position& pos2) const
{
	const int
		x = pos1.x - pos2.x,
		y = pos1.y - pos2.y,
		z = pos1.z - pos2.z; // kL

//	return static_cast<int>(Round(
	return static_cast<int>(std::ceil(
		   std::sqrt(static_cast<double>(x * x + y * y + z * z)))); // kL: 3-d
}

/**
 * Calculates the distance squared between 2 points.
 * No sqrt() no floating point math and sometimes it's all you need.
 * @param pos1		- reference the first Position
 * @param pos2		- reference the second Position
 * @param considerZ	- true to consider the z coordinate (default true)
 * @return, distance
 */
int TileEngine::distanceSq(
		const Position& pos1,
		const Position& pos2,
		bool considerZ) const
{
	const int
		x = pos1.x - pos2.x,
		y = pos1.y - pos2.y;

	int z;
	if (considerZ == true)
		z = pos1.z - pos2.z;
	else
		z = 0;

	return x * x + y * y + z * z;
}

/**
 * Attempts a panic or mind-control BattleAction.
 * @param action - pointer to a BattleAction (BattlescapeGame.h)
 * @return, true if attack succeeds
 */
bool TileEngine::psiAttack(BattleAction* action)
{
	//Log(LOG_INFO) << "\nTileEngine::psiAttack() attackerID " << action->actor->getId();
	const Tile* const tile = _battleSave->getTile(action->target);
	if (tile == NULL)
		return false;
	//Log(LOG_INFO) << ". . target(pos) " << action->target;

	BattleUnit* const victim = tile->getUnit();
	if (victim == NULL)
		return false;
	//Log(LOG_INFO) << "psiAttack: vs ID " << victim->getId();

	const bool psiImmune = victim->getUnitRules() != NULL
						&& victim->getUnitRules()->isPsiImmune() == true;
	if (psiImmune == false)
	{
		if (action->actor->getOriginalFaction() == FACTION_PLAYER)
			action->actor->addPsiSkillExp();
		else if (victim->getOriginalFaction() == FACTION_PLAYER
			&& Options::allowPsiStrengthImprovement == true)
		{
			victim->addPsiStrengthExp();
		}

		const UnitStats
			* const statsActor = action->actor->getBaseStats(),
			* const statsVictim = victim->getBaseStats();

		const double
			defense = static_cast<double>(statsVictim->psiStrength) + (static_cast<double>(statsVictim->psiSkill) / 5.),
			dist = static_cast<double>(distance(
											action->actor->getPosition(),
											action->target));

		int bonusSkill; // add to psiSkill when using aLien to Panic another aLien ....
		if (action->actor->getFaction() == FACTION_PLAYER
			&& action->actor->getOriginalFaction() == FACTION_HOSTILE)
		{
			bonusSkill = 21; // ... arbitrary kL
		}
		else
			bonusSkill = 0;

		double attack = static_cast<double>(statsActor->psiStrength * (statsActor->psiSkill + bonusSkill)) / 50.;
		//Log(LOG_INFO) << ". . . defense = " << (int)defense;
		//Log(LOG_INFO) << ". . . attack = " << (int)attack;
		//Log(LOG_INFO) << ". . . dist = " << (int)dist;

		attack -= dist * 2.;
		attack -= defense;

		if (action->type == BA_MINDCONTROL)
			attack += 15.;
		else
			attack += 45.;

		attack *= 100.;
		attack /= 56.;

		const int success = static_cast<int>(attack);

		std::string info;
		if (action->type == BA_PANIC)
			info = "STR_PANIC";
		else
			info = "STR_CONTROL";

		_battleSave->getBattleState()->warning(
											info,
											true,
											success);

		//Log(LOG_INFO) << ". . . attack Success @ " << success;
		if (RNG::percent(success) == true)
		{
			//Log(LOG_INFO) << ". . Success";
			if (action->actor->getOriginalFaction() == FACTION_PLAYER)
			{
				if (victim->getOriginalFaction() == FACTION_HOSTILE) // no extra-XP for re-controlling Soldiers.
					action->actor->addPsiSkillExp(2);
				else if (victim->getOriginalFaction() == FACTION_NEUTRAL) // only 1 extra-XP for controlling Civies.
					action->actor->addPsiSkillExp();
			}

			//Log(LOG_INFO) << ". . victim morale[0] = " << victim->getMorale();
			if (action->type == BA_PANIC)
			{
				//Log(LOG_INFO) << ". . . action->type == BA_PANIC";
				const int moraleLoss = 105
								 - statsVictim->bravery * 3 / 2
								 + statsActor->psiStrength * 2 / 3;
				//Log(LOG_INFO) << ". . . moraleLoss reduction = " << moraleLoss;
				if (moraleLoss > 0)
					victim->moraleChange(-moraleLoss);

				//Log(LOG_INFO) << ". . . victim morale[1] = " << victim->getMorale();
			}
			else // BA_MINDCONTROL
			{
				//Log(LOG_INFO) << ". . . action->type == BA_MINDCONTROL";
//				if (action->actor->getFaction() == FACTION_PLAYER &&
				if (victim->getOriginalFaction() == FACTION_HOSTILE // aLiens should be reduced to 50- Morale before MC.
//					&& victim->getMorale() > 50)
					&& RNG::percent(victim->getMorale() - 50) == true)
				{
					//Log(LOG_INFO) << ". . . . RESIST vs " << (victim->getMorale() - 50);
					_battleSave->getBattleState()->warning("STR_PSI_RESIST");
					return false;
				}

				int courage = statsVictim->bravery;
				if (action->actor->getFaction() == FACTION_HOSTILE)
				{
					courage = std::min( // xCom Morale loss for getting Mc'd.
									0,
									(_battleSave->getMoraleModifier() / 10) + (courage / 2) - 110);
				}
				else //if (action->actor->getFaction() == FACTION_PLAYER)
				{
					if (victim->getOriginalFaction() == FACTION_HOSTILE)
						courage = std::min( // aLien Morale loss for getting Mc'd.
										0,
										(_battleSave->getMoraleModifier(NULL, false) / 10) + (courage * 3 / 4) - 110);
					else
						courage /= 2; // xCom Morale gain for getting Mc'd back to xCom.
				}
				victim->moraleChange(courage);
				//Log(LOG_INFO) << ". . . victim morale[2] = " << victim->getMorale();

				victim->convertToFaction(action->actor->getFaction());
				victim->initTU();
				victim->allowReselect();
				victim->setStatus(STATUS_STANDING);

				calculateUnitLighting();
				calculateFOV(victim->getPosition());

				// if all units from either faction are mind controlled - auto-end the mission.
				if (Options::allowPsionicCapture == true
					&& Options::battleAutoEnd == true
					&& _battleSave->getSide() == FACTION_PLAYER)
				{
					//Log(LOG_INFO) << ". . . . inside tallyUnits";
					int
						liveAliens,
						liveSoldiers;
					_battleSave->getBattleGame()->tallyUnits(
														liveAliens,
														liveSoldiers);

					if (liveAliens == 0
						|| liveSoldiers == 0)
					{
						_battleSave->setSelectedUnit(NULL);
						_battleSave->getBattleGame()->cancelCurrentAction(true);

						_battleSave->getBattleGame()->requestEndTurn();
					}
				}
				//Log(LOG_INFO) << ". . . tallyUnits DONE";
			}
			//Log(LOG_INFO) << "TileEngine::psiAttack() ret TRUE";
			return true;
		}
		else if (Options::allowPsiStrengthImprovement == true
			&& victim->getOriginalFaction() == FACTION_PLAYER)
		{
			int resistXP = 1; // xCom resisted an xCom attempt
			if (action->actor->getFaction() == FACTION_HOSTILE)
				++resistXP; // xCom resisted an aLien

			victim->addPsiStrengthExp(resistXP);
		}
	}
	else if (action->actor->getFaction() == FACTION_PLAYER)
		_battleSave->getBattleState()->warning("STR_ACTION_NOT_ALLOWED_PSIONIC");

	//Log(LOG_INFO) << "TileEngine::psiAttack() ret FALSE";
	return false;
}

/**
 * Applies gravity to a tile - causes items and units to drop.
 * @param tile - pointer to a Tile to check
 * @return, pointer to the Tile where stuff ends up
 */
Tile* TileEngine::applyGravity(Tile* const tile)
{
	if (tile == NULL)
		return NULL;

	Position pos = tile->getPosition();
	if (pos.z == 0)
		return tile;


	const bool hasNoItems = tile->getInventory()->empty();
	BattleUnit* const unit = tile->getUnit();

	if (unit == NULL
		&& hasNoItems == true)
	{
		return tile;
	}

	Tile
		* dt = tile,
		* dtb = NULL;
	Position posBelow = pos;

	if (unit != NULL)
	{
		const int unitSize = unit->getArmor()->getSize();

		while (posBelow.z > 0)
		{
			bool canFall = true;

			for (int
					y = 0;
					y != unitSize
						&& canFall == true;
					++y)
			{
				for (int
						x = 0;
						x != unitSize
							&& canFall == true;
						++x)
				{
					dt = _battleSave->getTile(Position(
													posBelow.x + x,
													posBelow.y + y,
													posBelow.z));
					dtb = _battleSave->getTile(Position(
													posBelow.x + x,
													posBelow.y + y,
													posBelow.z - 1));
					if (dt->hasNoFloor(dtb) == false)	// note: polar water has no floor, so units that die on them ... uh, sink.
						canFall = false;				// ... before I changed the loop condition to > 0, that is
				}
			}

			if (canFall == false)
				break;

			--posBelow.z;
		}

		if (posBelow != pos)
		{
			if (unit->isOut() == true)
			{
				for (int
						y = unitSize - 1;
						y != -1;
						--y)
				{
					for (int
							x = unitSize - 1;
							x != -1;
							--x)
					{
						_battleSave->getTile(pos + Position(x,y,0))->setUnit(NULL);
					}
				}

				unit->setPosition(posBelow);
			}
			else // if (!unit->isOut(true, true))
			{
				if (unit->getMoveTypeUnit() == MT_FLY)
				{
					// move to the position you're already in. this will unset the kneeling flag, set the floating flag, etc.
					unit->startWalking(
									unit->getDirection(),
									unit->getPosition(),
									_battleSave->getTile(unit->getPosition() + Position(0,0,-1)));
//									true);
					// and set our status to standing (rather than walking or flying) to avoid weirdness.
					unit->setStatus(STATUS_STANDING);
				}
				else
				{
					unit->startWalking(
									Pathfinding::DIR_DOWN,
									unit->getPosition() + Position(0,0,-1),
									_battleSave->getTile(unit->getPosition() + Position(0,0,-1)));
//									true);

					//Log(LOG_INFO) << "TileEngine::applyGravity(), addFallingUnit() ID " << unit->getId();
					_battleSave->addFallingUnit(unit);
				}
			}
		}
	}

	dt = tile;
	posBelow = pos;

	while (posBelow.z > 0)
	{
		dt = _battleSave->getTile(posBelow);
		dtb = _battleSave->getTile(Position(
										posBelow.x,
										posBelow.y,
										posBelow.z - 1));

		if (dt->hasNoFloor(dtb) == false)
			break;

		--posBelow.z;
	}

	if (posBelow != pos)
	{
		dt = _battleSave->getTile(posBelow);

		if (hasNoItems == false)
		{
			for (std::vector<BattleItem*>::const_iterator
					i = tile->getInventory()->begin();
					i != tile->getInventory()->end();
					++i)
			{
				if ((*i)->getUnit() != NULL // corpse
					&& tile->getPosition() == (*i)->getUnit()->getPosition())
				{
					(*i)->getUnit()->setPosition(dt->getPosition());
				}

//				if (dt != tile)
				dt->addItem(
						*i,
						(*i)->getSlot());
			}

//			if (tile != dt) // clear tile
			tile->getInventory()->clear();
		}
	}

	return dt;
}

/**
 * Validates the melee range between two units.
 * @param attacker	- pointer to an attacking unit
 * @param target	- pointer to the unit to attack
 * @param dir		- direction to check
 * @return, true if range is valid
 */
bool TileEngine::validMeleeRange(
		const BattleUnit* const attacker,
		const BattleUnit* const target,
		const int dir)
{
	return validMeleeRange(
						attacker->getPosition(),
						dir,
						attacker,
						target,
						NULL);
}

/**
 * Validates the melee range between a tile and a unit.
 * @param origin		- reference the Position to check from
 * @param dir			- direction to check
 * @param attacker		- pointer to an attacking BattleUnit
 * @param targetUnit	- pointer to the BattleUnit to attack
 *							- NULL any unit
 *							- medikit usage requires a valid BattleUnit for MediTargetState list
 * @param dest			- pointer to destination Position; will be set according to where targetUnit actually is
// * @param preferEnemy	- true if attacking a hostile unit; false if using medi-kit (default true)
 * @return, true if range is valid
 */
bool TileEngine::validMeleeRange(
		const Position& origin,
		const int dir,
		const BattleUnit* const attacker,
		const BattleUnit* const targetUnit,
		Position* const dest)
//		const bool preferEnemy) // <- obsolete.
{
	//Log(LOG_INFO) << "TileEngine::validMeleeRange()";
	if (dir < 0 || dir > 7)
		return false;

	Position posTarget;
	Pathfinding::directionToVector(
								dir,
								&posTarget);
	const Tile
		* tileOrigin,
		* tileTarget,
		* tileTarget_above,
		* tileTarget_below;

	std::vector<BattleUnit*> targetUnits;

	const int unitSize = attacker->getArmor()->getSize() - 1;
	for (int
			x = 0;
			x != unitSize + 1;
			++x)
	{
		for (int
				y = 0;
				y != unitSize + 1;
				++y)
		{
			//Log(LOG_INFO) << ". . . iterate Size";
			tileOrigin = _battleSave->getTile(Position(origin + Position(x,y, 0)));
			tileTarget = _battleSave->getTile(Position(origin + Position(x,y, 0) + posTarget));

			if (tileOrigin != NULL
				&& tileTarget != NULL)
			{
				//Log(LOG_INFO) << ". . . . origin & target VALID";
				if (tileTarget->getUnit() == NULL)
				{
					//Log(LOG_INFO) << ". . . . . tileTarget->unit NOT Valid";
					tileTarget_above = _battleSave->getTile(Position(origin + Position(x,y, 1) + posTarget));
					tileTarget_below = _battleSave->getTile(Position(origin + Position(x,y,-1) + posTarget));

					if (tileTarget_above != NULL // kL_note: standing on a rise only 1/3 up z-axis reaches adjacent tileAbove.
//						&& tileOrigin->getTerrainLevel() < -7)
						&& std::abs(tileTarget_above->getTerrainLevel() - (tileOrigin->getTerrainLevel() + 24)) < 9)
					{
						tileTarget = tileTarget_above;
					}
					else if (tileTarget_below != NULL // kL_note: can reach targetUnit standing on a rise only 1/3 up z-axis on adjacent tileBelow.
//						&& tileTarget->hasNoFloor(tileTarget_below) == true
//						&& tileTarget_below->getTerrainLevel() < -7)
						&& std::abs((tileTarget_below->getTerrainLevel() + 24) + tileOrigin->getTerrainLevel()) < 9)
					{
						tileTarget = tileTarget_below;
					}
				}

				//Log(LOG_INFO) << ". . . . recheck for tileTarget->unit";
				if (tileTarget->getUnit() != NULL)
				{
					//Log(LOG_INFO) << ". . . . . tileTarget->unit VALID";
					if (targetUnit == NULL
						|| targetUnit == tileTarget->getUnit())
					{
						//Log(LOG_INFO) << ". . . . . . targetUnit NOT Valid or targetUnit==tileTarget->unit";
						const Position voxelOrigin = Position(tileOrigin->getPosition() * Position(16,16,24))
												   + Position(
															8,8,
															attacker->getHeight()
																+ attacker->getFloatHeight()
																- tileOrigin->getTerrainLevel()
																- 5);

						Position voxelTarget;
						if (canTargetUnit(
										&voxelOrigin,
										tileTarget,
										&voxelTarget,
										attacker) == true)
						{
							//Log(LOG_INFO) << ". . . . . . canTargetUnit TRUE";
							if (targetUnit != NULL) // medikit check always returns here!
							{
								if (dest != NULL)
									*dest = tileTarget->getPosition();

								//Log(LOG_INFO) << ". . . . . . . targetUnit found, ret TRUE";
								return true;
							}
							else
							{
								//Log(LOG_INFO) << ". . . . . . . add targetUnit to target vector";
								targetUnits.push_back(tileTarget->getUnit());
							}
						}
					}
				}
			}
		}
	}

	if (targetUnits.size() == 0)
		return false;


	const size_t pick = static_cast<int>(RNG::generate(
													0,
													targetUnits.size() - 1));

	if (dest != NULL)
	{
		//Log(LOG_INFO) << ". . set defender Pos " << defender->getPosition();
		*dest = targetUnits[pick]->getPosition();
	}

	//Log(LOG_INFO) << ". ret = " << (int)(defender != NULL);
	return true;
}
/*	const BattleUnit* defender = NULL;
//	if (preferEnemy == true)
//	{
	std::vector<BattleUnit*> randTarget;

	for (std::vector<BattleUnit*>::const_iterator
			i = targetUnits.begin();
			i != targetUnits.end();
			++i)
	{
		if ((*i)->getFaction() != attacker->getFaction())
			randTarget.push_back(*i);
	}

	if (randTarget.empty() == false) // safety.
	{
		const size_t pick = RNG::generate(
									0,
									randTarget.size() - 1);
		//Log(LOG_INFO) << ". . pick = " << pick;
		defender = randTarget[pick];
	} */
//	}
//	else // medikit use - allow 'defender' into MediTargetState list; see above^
//	{}

/* note: This is superceded by changes for MediTargetState.
	for (std::vector<BattleUnit*>::const_iterator
			i = targetUnits.begin();
			i != targetUnits.end();
			++i)
	{
		if (defender == NULL
			|| (preferEnemy == true
				&& (*i)->getFaction() != attacker->getFaction())
			|| (preferEnemy == false
				&& (*i)->getFaction() == attacker->getFaction()
				&& (*i)->getFatalWounds() > defender->getFatalWounds()))
		{
			defender = *i;
		}
	} */

/*	if (dest != NULL
		&& defender != NULL)
	{
		//Log(LOG_INFO) << ". . set defender Pos " << defender->getPosition();
		*dest = defender->getPosition();
	}

	//Log(LOG_INFO) << ". ret = " << (int)(defender != NULL);
	return (defender != NULL); */

/**
 * Gets the AI to look through a window.
 * @param position - reference the current Position
 * @return, direction or -1 if no window found
 */
int TileEngine::faceWindow(const Position& position)
{
	int ret = -1;

	const Tile* tile = _battleSave->getTile(position);
	if (tile != NULL)
	{
		if (tile->getMapData(MapData::O_NORTHWALL) != NULL
			&& tile->getMapData(MapData::O_NORTHWALL)->stopLOS() == false)
		{
			ret = 0;
		}
		else if (tile->getMapData(MapData::O_WESTWALL) != NULL
			&& tile->getMapData(MapData::O_WESTWALL)->stopLOS() == false)
		{
			ret = 6;
		}
	}

	tile = _battleSave->getTile(position + Position(1, 0, 0));
	if (tile != NULL
		&& tile->getMapData(MapData::O_WESTWALL) != NULL
		&& tile->getMapData(MapData::O_WESTWALL)->stopLOS() == false)
	{
		ret = 2;
	}

	tile = _battleSave->getTile(position + Position(0, 1, 0));
	if (tile != NULL
		&& tile->getMapData(MapData::O_NORTHWALL) != NULL
		&& tile->getMapData(MapData::O_NORTHWALL)->stopLOS() == false)
	{
		ret = 4;
	}


	if ((ret == 0
			&& position.y == 0)
		|| (ret == 2
			&& position.x == _battleSave->getMapSizeX() - 1)
		|| (ret == 4
			&& position.y == _battleSave->getMapSizeY() - 1)
		|| (ret == 6
			&& position.x == 0))
	{
		ret = -1;
	}

	return ret;
}

/**
 * Returns the direction from origin to target.
 * kL_note: This function is almost identical to BattleUnit::directionTo().
 * @param origin - reference to the origin point of the action
 * @param target - reference to the target point of the action
 * @return, direction
 */
int TileEngine::getDirectionTo(
		const Position& origin,
		const Position& target) const
{
	if (origin == target) // kL. safety
		return 0;


	double
		offset_x = target.x - origin.x,
		offset_y = target.y - origin.y,

	// kL_note: atan2() usually takes the y-value first;
	// and that's why things may seem so fucked up.
		theta = std::atan2( // radians: + = y > 0; - = y < 0;
						-offset_y,
						offset_x),

	// divide the pie in 4 thetas, each at 1/8th before each quarter
		m_pi_8 = M_PI / 8.,	// a circle divided into 16 sections (rads) -> 22.5 deg
		d = 0.1,			// kL, a bias toward cardinal directions. (0.1..0.12)
		pie[4] =
		{
			M_PI - m_pi_8 - d,				// 2.7488935718910690836548129603696	-> 157.5 deg
			M_PI * 3. / 4. - m_pi_8 + d,	// 1.9634954084936207740391521145497	-> 112.5 deg
			M_PI_2 - m_pi_8 - d,			// 1.1780972450961724644234912687298	-> 67.5 deg
			m_pi_8 + d						// 0.39269908169872415480783042290994	-> 22.5 deg
		};

	int dir;
	if (theta > pie[0] || theta < -pie[0])
		dir = 6;
	else if (theta > pie[1])
		dir = 7;
	else if (theta > pie[2])
		dir = 0;
	else if (theta > pie[3])
		dir = 1;
	else if (theta < -pie[1])
		dir = 5;
	else if (theta < -pie[2])
		dir = 4;
	else if (theta < -pie[3])
		dir = 3;
	else
		dir = 2;

	return dir;
}

/**
 * Gets the origin-voxel of a shot or missile.
 * @param action	- reference the BattleAction
 * @param tile		- pointer to a start tile
 * @return, position of the origin in voxel-space
 */
Position TileEngine::getOriginVoxel(
		const BattleAction& action,
		const Tile* tile)
{
//	const int dirYshift[8] = {1, 1, 8,15,15,15, 8, 1};
//	const int dirXshift[8] = {8,14,15,15, 8, 1, 1, 1};

	if (tile == NULL)
		tile = action.actor->getTile();

	Position
		originTile = tile->getPosition(),
		originVoxel = Position(
							originTile.x * 16,
							originTile.y * 16,
							originTile.z * 24);

	// take into account soldier height and terrain level if the projectile is launched from a soldier
	if (action.actor->getPosition() == originTile
		|| action.type != BA_LAUNCH)
	{
		// calculate vertical offset of the starting point of the projectile
		originVoxel.z += action.actor->getHeight()
					  + action.actor->getFloatHeight()
					  - tile->getTerrainLevel();
//					  - 4; // for good luck. (kL_note: looks like 2 voxels lower than LoS origin or something like it.)
			// Ps. don't need luck - need precision.

//		if (action.type == BA_THROW)	// kL
//			originVoxel.z -= 4;			// kL
/*		if (action.type == BA_THROW)
			originVoxel.z -= 3;
		else
			originVoxel.z -= 4; */

		if (originVoxel.z >= (originTile.z + 1) * 24)
		{
			const Tile* const tileAbove = _battleSave->getTile(originTile + Position(0,0,1));
			if (tileAbove != NULL
				&& tileAbove->hasNoFloor(NULL) == true)
			{
				++originTile.z;
			}
			else
			{
				while (originVoxel.z >= (originTile.z + 1) * 24)
				{
					--originVoxel.z;
				}

				originVoxel.z -= 4; // keep originVoxel 4 voxels below any ceiling.
			}
		}

		// kL_note: This is the old code that does not use the dirX/Yshift stuff...
		//
		// Originally used the dirXShift and dirYShift as detailed above;
		// this however results in MUCH more predictable results.
		// center Origin in the originTile (or the center of all four tiles for large units):
		const int offset = action.actor->getArmor()->getSize() * 8;
		originVoxel.x += offset;
		originVoxel.y += offset;
			// screw Warboy's obscurantist glamor-driven elitist campaign!!!! Have fun with that!!
			// MUCH more predictable results. <- I didn't write that; just iterating it.
			// ... better now but still waiting on it.
/*
		int direction = getDirectionTo(
									originTile,
									action.target);
		originVoxel.x += dirXshift[direction]*action.actor->getArmor()->getSize();
		originVoxel.y += dirYshift[direction]*action.actor->getArmor()->getSize(); */
	}
	else // action.type == BA_LAUNCH
	{
		// don't take into account soldier height and terrain level if the
		// projectile is not launched from a soldier (ie. from a waypoint)
		originVoxel.x += 8;
		originVoxel.y += 8;
		originVoxel.z += 16;
	}

	return originVoxel;
}

/**
 * Marks a region of the map as "dangerous" for a turn.
 * @param pos		- reference the epicenter of the explosion in tilespace
 * @param radius	- how far to spread out
 * @param unit		- pointer to the BattleUnit that is triggering this action
 */
void TileEngine::setDangerZone(
		const Position& pos,
		const int radius,
		const BattleUnit* const unit)
{
	Tile* tile = _battleSave->getTile(pos);
	if (tile == NULL)
		return;

	tile->setDangerous(); // set the epicenter as dangerous

	const Position originVoxel = pos * Position(16,16,24)
							   + Position(
										8,8,
										12 - tile->getTerrainLevel());
	Position targetVoxel;

	for (int
			x = -radius;
			x != radius;
			++x)
	{
		for (int
				y = -radius;
				y != radius;
				++y)
		{
			if (x != 0 || y != 0) // skip the epicenter
			{
				if ((x * x) + (y * y) <= radius * radius) // make sure it's within the radius
				{
					tile = _battleSave->getTile(pos + Position(x,y,0));
					if (tile != NULL)
					{
						targetVoxel = ((pos + Position(x,y,0)) * Position(16,16,24))
									+ Position(
											8,8,
											12 - tile->getTerrainLevel());

						std::vector<Position> trajectory;
						// we'll trace a line here, ignoring all units, to check if the explosion will reach this point;
						// granted this won't properly account for explosions tearing through walls, but then we can't really
						// know that kind of information before the fact, so let's have the AI assume that the wall (or tree)
						// is enough to protect them.
						if (calculateLine(
										originVoxel,
										targetVoxel,
										false,
										&trajectory,
										unit,
										true,
										false,
										unit) == VOXEL_EMPTY)
						{
							if (trajectory.size() != 0
								&& (trajectory.back() / Position(16,16,24)) == pos + Position(x,y,0))
							{
								tile->setDangerous();
							}
						}
					}
				}
			}
		}
	}
}

}
