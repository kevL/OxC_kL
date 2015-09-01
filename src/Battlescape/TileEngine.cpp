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
//#include <set>
//#include <assert.h>
//#include <SDL.h>
//#include "../fmath.h"

#include "AlienBAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "ExplosionBState.h"
#include "InfoBoxOKState.h"
#include "Map.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "UnitTurnBState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

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
		SavedBattleGame* const battleSave,
		const std::vector<Uint16>* const voxelData)
	:
		_battleSave(battleSave),
		_voxelData(voxelData),
		_unitLighting(true),
		_powerE(-1),
		_powerT(-1),
		_spotSound(true),
		_trueTile(NULL)
//		_missileDirection(-1)
{
	_rfAction = new BattleAction();
}

/**
 * Deletes the TileEngine.
 */
TileEngine::~TileEngine()
{
	delete _rfAction;
}

/**
 * Calculates sun shading for the whole terrain.
 */
void TileEngine::calculateSunShading() const
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
void TileEngine::calculateSunShading(Tile* const tile) const
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
							O_FLOOR,
							DT_NONE);
			block += blockage(
							_save->getTile(Position(x, y, z)),
							O_OBJECT,
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
void TileEngine::calculateTerrainLighting() const
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
		if (_battleSave->getTiles()[i]->getMapData(O_FLOOR) != NULL
			&& _battleSave->getTiles()[i]->getMapData(O_FLOOR)->getLightSource() != 0)
		{
			addLight(
				_battleSave->getTiles()[i]->getPosition(),
				_battleSave->getTiles()[i]->getMapData(O_FLOOR)->getLightSource(),
				layer);
		}

		if (_battleSave->getTiles()[i]->getMapData(O_OBJECT) != NULL
			&& _battleSave->getTiles()[i]->getMapData(O_OBJECT)->getLightSource() != 0)
		{
			addLight(
				_battleSave->getTiles()[i]->getPosition(),
				_battleSave->getTiles()[i]->getMapData(O_OBJECT)->getLightSource(),
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
void TileEngine::calculateUnitLighting() const
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
		if (_unitLighting == true // add lighting of soldiers
			&& (*i)->getFaction() == FACTION_PLAYER
			&& (*i)->isOut_t(OUT_STAT) == false)
//			&& (*i)->isOut() == false)
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
		size_t layer) const
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
bool TileEngine::calculateFOV(BattleUnit* const unit) const
{
	unit->clearHostileUnits();
//	unit->clearVisibleTiles();

//	if (unit->isOut(true, true) == true)
	if (unit->isOut_t() == true)
		return false;

//	if (unit->getFaction() == FACTION_PLAYER)
//	{
//		unit->getTile()->setTileVisible();
//		unit->getTile()->setDiscovered(true, 2);
//	}


	bool ret = false;

	const size_t hostiles_pre = unit->getHostileUnitsThisTurn().size();

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
		unitSize;
	size_t trjLength;

	bool diag;
	if (dir % 2)
	{
		diag = true;
		y2 = MAX_VIEW_DISTANCE;
	}
	else
		diag = false;

	VoxelType block;

	std::vector<Position> trj;

	Position
		posUnit = unit->getPosition(),
		posTest,
		pos,
		posTrj;

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

	const int mapSize_z = _battleSave->getMapSizeZ();
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
					z != mapSize_z;
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
						const bool preBattle = _battleSave->getBattleGame() == NULL;

						if (preBattle == true
							|| _battleSave->getBattleGame()->getPanicHandled() == true) // spot units ->>
						{
							spottedUnit = _battleSave->getTile(posTest)->getUnit();

							if (spottedUnit != NULL
//								&& spottedUnit->isOut() == false
								&& spottedUnit->isOut_t(OUT_STAT) == false
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

									if (preBattle == false
										&& _spotSound == true
										&& ret == true // play aggro sound if non-MC'd xCom unit spots a not-previously-visible hostile.
										&& unit->getOriginalFaction() == FACTION_PLAYER
										&& spottedUnit->getFaction() == FACTION_HOSTILE)
									{
										const std::vector<BattleUnit*> spottedUnits = unit->getHostileUnitsThisTurn();
										if (std::find(
													spottedUnits.begin(),
													spottedUnits.end(),
													spottedUnit) == spottedUnits.end())
										{
											const int sound = unit->getAggroSound();
											if (sound != -1)
											{
												const BattlescapeGame* const battle = _battleSave->getBattleGame();
												battle->getResourcePack()->getSound(
																				"BATTLE.CAT",
																				sound)
																			->play(
																				-1,
																				battle->getMap()->getSoundAngle(unit->getPosition()));
											}
										}
									}
								}

								if ((unit->getFaction() == FACTION_PLAYER
										&& spottedUnit->getFaction() == FACTION_HOSTILE)
									|| (unit->getFaction() == FACTION_HOSTILE
										&& spottedUnit->getFaction() != FACTION_HOSTILE))
								{
									// adds spottedUnit to _hostileUnits *and* to _hostileUnitsThisTurn:
									unit->addToHostileUnits(spottedUnit);
//									unit->addToVisibleTiles(spottedUnit->getTile());

									if (_battleSave->getSide() == FACTION_HOSTILE
										&& unit->getFaction() == FACTION_HOSTILE)
									{
										//Log(LOG_INFO) << "calculateFOV() id " << unit->getId() << " spots " << spottedUnit->getId();
										spottedUnit->setExposed();	// note that xCom agents can be seen by enemies but *not* become Exposed.
																	// Only potential reactionFire should set them Exposed during xCom's turn.
									}
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
									trj.clear();

									pos = posUnit + Position(
															size_x,
															size_y,
															0);
									block = plotLine(
													pos,
													posTest,
													true,
													&trj,
													unit,
													false);

									trjLength = trj.size();

//									if (block > 127)	// last tile is blocked thus must be cropped
//									if (block > 0)		// kL: -1 - do NOT crop trajectory (ie. hit content-object)
														//		0 - expose Tile (should never return this, unless out-of-bounds)
														//		1 - crop the trajectory (hit regular wall)
									if (block > VOXEL_FLOOR) // 0
										--trjLength;

									for (size_t
											i = 0;
											i != trjLength;
											++i)
									{
										posTrj = trj.at(i);

										// mark every tile of line as visible (this is needed because of bresenham narrow stroke).
										tile = _battleSave->getTile(posTrj);
										tile->setTileVisible();
										tile->setDiscovered(true, 2); // sprite caching for floor+content, ergo + west & north walls.

										// walls to the east or south of a visible tile, we see that too
										// note: Yeh, IF there's walls or an appropriate BigWall object!
										/* parts:
											#0 - floor
											#1 - westwall
											#2 - northwall
											#3 - object (content) */
										/* discovered:
											#0 - westwall
											#1 - northwall
											#2 - floor + content (reveals both walls also) */
										if (tile->getMapData(O_OBJECT) == NULL
											|| (tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_BLOCK
												&& tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_EAST
												&& tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_E_S))
										{
											tileEdge = _battleSave->getTile(Position( // show Tile EAST
																				posTrj.x + 1,
																				posTrj.y,
																				posTrj.z));
											if (tileEdge != NULL)
											{
												if (tileEdge->getMapData(O_OBJECT) != NULL
													&& (tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK
														|| tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_WEST))
												{
													tileEdge->setDiscovered(true, 2); // reveal entire TileEast
												}
												else if (tileEdge->getMapData(O_WESTWALL) != NULL)
													tileEdge->setDiscovered(true, 0); // reveal only westwall
											}
										}

										if (tile->getMapData(O_OBJECT) == NULL
											|| (tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_BLOCK
												&& tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_SOUTH
												&& tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_E_S))
										{
											tileEdge = _battleSave->getTile(Position( // show Tile SOUTH
																				posTrj.x,
																				posTrj.y + 1,
																				posTrj.z));
											if (tileEdge != NULL)
											{
												if (tileEdge->getMapData(O_OBJECT) != NULL
													&& (tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK
														|| tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NORTH))
												{
													tileEdge->setDiscovered(true, 2); // reveal entire TileSouth
												}
												else if (tileEdge->getMapData(O_NORTHWALL) != NULL)
													tileEdge->setDiscovered(true, 1); // reveal only northwall
											}
										}

										if (tile->getMapData(O_WESTWALL) == NULL
											&& (tile->getMapData(O_OBJECT) == NULL
												|| (tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_BLOCK
													&& tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_WEST)))
										{
											tileEdge = _battleSave->getTile(Position( // show Tile WEST
																				posTrj.x - 1,
																				posTrj.y,
																				posTrj.z));
											if (tileEdge != NULL
												&& tileEdge->getMapData(O_OBJECT) != NULL
												&& (tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK
													|| tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_EAST
													|| tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_E_S))
											{
												tileEdge->setDiscovered(true, 2); // reveal entire TileWest
											}
										}

										if (tile->getMapData(O_NORTHWALL) == NULL
											&& (tile->getMapData(O_OBJECT) == NULL
												|| (tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_BLOCK
													&& tile->getMapData(O_OBJECT)->getBigWall() != BIGWALL_NORTH)))
										{
											tileEdge = _battleSave->getTile(Position( // show Tile NORTH
																				posTrj.x,
																				posTrj.y - 1,
																				posTrj.z));
											if (tileEdge != NULL
												&& tileEdge->getMapData(O_OBJECT) != NULL
												&& (tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_BLOCK
													|| tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_SOUTH
													|| tileEdge->getMapData(O_OBJECT)->getBigWall() == BIGWALL_E_S))
											{
												tileEdge->setDiscovered(true, 2); // reveal entire TileNorth
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
		&& unit->getHostileUnits()->empty() == false
		&& unit->getHostileUnitsThisTurn().size() > hostiles_pre)
	{
		return true;
	}

	return false;
}

/**
 * Calculates line of sight of all units within range of the Position.
 * @note Used when a unit is walking or terrain has changed which can reveal
 * unseen units and/or parts of terrain.
 * @param pos		- reference the position of the changed terrain
 * @param spotSound	- true to play aggro sound (default false)
 */
void TileEngine::calculateFOV(
		const Position& pos,
		bool spotSound)
{
	_spotSound = spotSound;
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (distanceSq(
					pos,
					(*i)->getPosition(),
					false) <= MAX_VIEW_DISTANCE_SQR)
		{
			//Log(LOG_INFO) << "calcFoV for " << (*i)->getId();
			calculateFOV(*i);
		}
	}
	_spotSound = true;
}

/**
 * Recalculates FOV of all conscious units on the battlefield.
 * @param spotSound - true to play aggro sound (default false)
 */
void TileEngine::recalculateFOV(bool spotSound)
{
	_spotSound = spotSound;
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getTile() != NULL)
			calculateFOV(*i);
	}
	_spotSound = true;
}

/**
 * Checks visibility of a unit to a tile.
 * @param unit - pointer to a BattleUnit that's looking at @a tile
 * @param tile - pointer to a Tile that @a unit is looking at
 * @return, true if the unit on @a tile is seen
 */
bool TileEngine::visible(
		const BattleUnit* const unit,
		const Tile* const tile) const
{
	if (tile == NULL)
		return false;

	const BattleUnit* const targetUnit = tile->getUnit();
	if (targetUnit == NULL
		|| targetUnit->isOut_t() == true)
//		|| targetUnit->isOut(true, true) == true)
	{
		return false;
	}

	if (unit->getFaction() == targetUnit->getFaction())
		return true;


	const int dist = distance(
							unit->getPosition(),
							targetUnit->getPosition());
	if (dist * dist > MAX_VIEW_DISTANCE_SQR)
		return false;

	if (unit->getFaction() == FACTION_PLAYER
		&& tile->getShade() > MAX_SHADE_TO_SEE_UNITS
		&& dist > 23 - _battleSave->getGlobalShade())
	{
		return false;
	}


	const Position originVoxel = getSightOriginVoxel(unit);
	Position scanVoxel;
	std::vector<Position> _trajectory;

	if (canTargetUnit(
					&originVoxel,
					tile,
					&scanVoxel,
					unit) == true)
	{
		_trajectory.clear();

		plotLine(
				originVoxel,
				scanVoxel,
				true,
				&_trajectory,
				unit);


		double distWeighted = static_cast<double>(_trajectory.size());
		const Tile* scanTile = _battleSave->getTile(unit->getPosition());

		for (size_t
				i = 0;
				i != _trajectory.size();
				++i)
		{
			scanTile = _battleSave->getTile(Position(
												_trajectory.at(i).x / 16,
												_trajectory.at(i).y / 16,
												_trajectory.at(i).z / 24));

			distWeighted += static_cast<double>(scanTile->getSmoke()) / 3.;
			distWeighted += static_cast<double>(scanTile->getFire()) / 3.;

			if (static_cast<int>(std::ceil(distWeighted * distWeighted)) > MAX_VOXEL_VIEW_DIST_SQR)
				return false;
		}

		if (scanTile->getUnit() == targetUnit)
			return true;

		const Tile* const tileBelow = _battleSave->getTile(scanTile->getPosition() + Position(0,0,-1));
		if (tileBelow != NULL
			&& scanTile->hasNoFloor(tileBelow) == true
			&& tileBelow->getUnit() == targetUnit)
		{
			return true;
		}
	}

	return false;
}

/**
 * Gets the origin voxel of a unit's LoS.
 * @param unit - the watcher
 * @return, approximately an eyeball voxel
 */
Position TileEngine::getSightOriginVoxel(const BattleUnit* const unit) const
{
	// determine the origin (and target) voxels for calculations
	Position posOrigin_voxel = Position(
									unit->getPosition().x * 16,
									unit->getPosition().y * 16,
									unit->getPosition().z * 24);
	posOrigin_voxel.z += unit->getHeight(true)
					   - _battleSave->getTile(unit->getPosition())->getTerrainLevel()
					   - 4; // kL_note: now equal to getOriginVoxel()

	const int offset = unit->getArmor()->getSize() * 8;
	posOrigin_voxel.x += offset;
	posOrigin_voxel.y += offset;

	const int heightLimit = ((unit->getPosition().z + 1) * 24) - 1;
	if (posOrigin_voxel.z > heightLimit)
	{
		const Tile* const tileAbove = _battleSave->getTile(unit->getPosition() + Position(0,0,1));
		if (tileAbove == NULL
			|| tileAbove->hasNoFloor(NULL) == false)
		{
			while (posOrigin_voxel.z > heightLimit)
				--posOrigin_voxel.z; // careful with that ceiling, Eugene.
		}
	}

	return posOrigin_voxel;
}

/**
 * Gets the origin-voxel of a shot or missile.
 * @param action	- reference the BattleAction
 * @param tile		- pointer to a start tile (default NULL)
 * @return, position of the origin in voxel-space
 */
Position TileEngine::getOriginVoxel(
		const BattleAction& action,
		const Tile* tile) const
{
//	const int dirYshift[8] = {1, 1, 8,15,15,15, 8, 1};
//	const int dirXshift[8] = {8,14,15,15, 8, 1, 1, 1};

	if (tile == NULL)
		tile = action.actor->getTile();

	Position
		posOrigin_tile = tile->getPosition(),
		posOrigin_voxel = Position(
								posOrigin_tile.x * 16,
								posOrigin_tile.y * 16,
								posOrigin_tile.z * 24);

	// take into account soldier height and terrain level if the projectile is launched from a soldier
	if (action.type != BA_LAUNCH
		|| action.actor->getPosition() == posOrigin_tile)
	{
		// calculate vertical offset of the starting point of the projectile
		posOrigin_voxel.z += action.actor->getHeight(true)
						   - tile->getTerrainLevel();
						   - 4; // for good luck. kL_note: now equal to getSightOriginVoxel()
		// Ps. don't need luck - need precision.

		// hey, here's an idea: make Autos shoot from hip, Snaps shoot from chest, & Aimed from shoulders or eyes.
//		if (action.type == BA_THROW)	// kL
//			posOrigin_voxel.z -= 4;		// kL
/*		if (action.type == BA_THROW)
			posOrigin_voxel.z -= 3;
		else
			posOrigin_voxel.z -= 4; */

		const int heightLimit = ((posOrigin_tile.z + 1) * 24) - 1;
		if (posOrigin_voxel.z > heightLimit)
		{
			const Tile* const tileAbove = _battleSave->getTile(posOrigin_tile + Position(0,0,1));
			if (tileAbove == NULL
				|| tileAbove->hasNoFloor(NULL) == false)
			{
				while (posOrigin_voxel.z > heightLimit)
					--posOrigin_voxel.z;
			}
		}
/*		const int heightLimit = ((posOrigin_tile.z + 1) * 24) - 1;
		if (posOrigin_voxel.z > heightLimit)
		{
			const Tile* const tileAbove = _battleSave->getTile(posOrigin_tile + Position(0,0,1));
			if (tileAbove != NULL
				&& tileAbove->hasNoFloor(NULL) == true)
			{
				++posOrigin_tile.z;
			}
			else
			{
				while (posOrigin_voxel.z > heightLimit)
					--posOrigin_voxel.z;

				posOrigin_voxel.z -= 4; // keep posOrigin_voxel 4 voxels below any ceiling.
			}
		} */

		// kL_note: This is the old code that does not use the dirX/Yshift stuff...
		//
		// Originally used the dirXShift and dirYShift as detailed above;
		// this however results in MUCH more predictable results.
		// center Origin in the posOrigin_tile (or the center of all four tiles for large units):
		const int offset = action.actor->getArmor()->getSize() * 8;
		posOrigin_voxel.x += offset;
		posOrigin_voxel.y += offset;
		// screw Warboy's obscurantist glamor-driven elitist campaign!!!! Have fun with that!!
		// MUCH more predictable results. <- I didn't write that; just iterating it.
		// ... better now but still waiting on it. Eg, the AI needs to account for it.
/*
		int direction = getDirectionTo(
									posOrigin_tile,
									action.target);
		posOrigin_voxel.x += dirXshift[direction]*action.actor->getArmor()->getSize();
		posOrigin_voxel.y += dirYshift[direction]*action.actor->getArmor()->getSize(); */
	}
	else // action.type == BA_LAUNCH
	{
		// don't take into account soldier height and terrain level if the
		// projectile is not launched from a soldier (ie. is from a waypoint)
		posOrigin_voxel.x += 8;
		posOrigin_voxel.y += 8;
		posOrigin_voxel.z += 16;
	}

	return posOrigin_voxel;
}

/**
 * Checks for another unit available for targeting and what particular voxel.
 * @param originVoxel	- pointer to voxel of trace origin (eg. gun's barrel)
 * @param targetTile	- pointer to Tile to check against
 * @param scanVoxel		- pointer to voxel that is returned coordinate of hit
 * @param excludeUnit	- pointer to unitSelf (to not hit self)
 * @param targetUnit	- pointer to a hypothetical unit to draw a virtual LoF for AI;
						  if left NULL this function behaves normally (default NULL)
 * @return, true if a unit can be targeted
 */
bool TileEngine::canTargetUnit(
		const Position* const originVoxel,
		const Tile* const targetTile,
		Position* const scanVoxel,
		const BattleUnit* const excludeUnit,
		const BattleUnit* targetUnit) const
{
	//Log(LOG_INFO) << "TileEngine::canTargetUnit()";
	const bool hypothetical = (targetUnit != NULL);

	if (targetUnit == NULL)
	{
		targetUnit = targetTile->getUnit();
		if (targetUnit == NULL)
		{
			//Log(LOG_INFO) << ". no Unit, ret FALSE";
			return false; // no unit in this targetTile, even if it's elevated and appearing in it.
		}
	}

	if (targetUnit == excludeUnit) // skip self
	{
		//Log(LOG_INFO) << ". hit vs Self, ret FALSE";
		return false;
	}

	const Position targetVoxel = Position(
										targetTile->getPosition().x * 16 + 8,
										targetTile->getPosition().y * 16 + 8,
										targetTile->getPosition().z * 24);
	const int
		targetMinHeight = targetVoxel.z
						- targetTile->getTerrainLevel()
						+ targetUnit->getFloatHeight(),
		// if there is a unit on targetTile, assume check against that unit's height
		xOffset = targetUnit->getPosition().x - targetTile->getPosition().x,
		yOffset = targetUnit->getPosition().y - targetTile->getPosition().y,
		targetSize = targetUnit->getArmor()->getSize() - 1;
	int
		targetMaxHeight = targetMinHeight,
		targetCenterHeight,
		heightRange;

	size_t unitRadius;
	if (targetSize > 0)
		unitRadius = 3; // width = LoFT in default LoFTemps set
	else
		unitRadius = targetUnit->getLoft();

	// origin-to-target vector manipulation to make scan work in voxelspace
	const Position relPos = targetVoxel - *originVoxel;
	const float unitCenter = static_cast<float>(unitRadius)
						   / std::sqrt(static_cast<float>((relPos.x * relPos.x) + (relPos.y * relPos.y)));
	const int
//		relX = static_cast<int>(std::floor(static_cast<float>( relPos.y) * unitCenter + 0.5f)),
//		relY = static_cast<int>(std::floor(static_cast<float>(-relPos.x) * unitCenter + 0.5f)),
		relX = static_cast<int>(Round(std::floor(static_cast<float>( relPos.y) * unitCenter))),
		relY = static_cast<int>(Round(std::floor(static_cast<float>(-relPos.x) * unitCenter))),
		targetSlices[10] =
		{
			 0,		 0,
			 relX,	 relY,
			-relX,	-relY,
			 relY,	-relX,
			-relY,	 relX
		};

//	if (targetUnit->isOut() == false)
	if (targetUnit->isOut_t(OUT_STAT) == false)
		heightRange = targetUnit->getHeight();
	else
		heightRange = 12;

	targetMaxHeight += heightRange;
	targetCenterHeight = (targetMaxHeight + targetMinHeight) / 2;
	heightRange /= 2;
	if (heightRange > 10) heightRange = 10;
	if (heightRange < 0) heightRange = 0;

	std::vector<Position> trajectory;
	std::vector<int>
		targetable_x,
		targetable_y,
		targetable_z;

	for (int // scan from center point up and down using heightFromCenter array
			i = 0;
			i <= heightRange;
			++i)
	{
		scanVoxel->z = targetCenterHeight + heightFromCenter[static_cast<size_t>(i)];
		//Log(LOG_INFO) << ". iterate heightRange, i = " << i << ", scan.Z = " << scanVoxel->z;

		for (size_t // scan from vertical centerline outwards left and right using targetSlices array
				j = 0;
				j != 5;
				++j)
		{
			//Log(LOG_INFO) << ". . iterate j = " << j;
			if (i < heightRange - 1 // looks like this gives an oval shape to target instead of an upright rectangle
				&& j > 2)
			{
				//Log(LOG_INFO) << ". . . out of bounds, break (j)";
				break; // skip unnecessary checks
			}

			scanVoxel->x = targetVoxel.x + targetSlices[j * 2];
			scanVoxel->y = targetVoxel.y + targetSlices[j * 2 + 1];
			//Log(LOG_INFO) << ". . scan.X = " << scanVoxel->x;
			//Log(LOG_INFO) << ". . scan.Y = " << scanVoxel->y;

			trajectory.clear();

			const VoxelType test = plotLine(
										*originVoxel,
										*scanVoxel,
										false,
										&trajectory,
										excludeUnit);
			//Log(LOG_INFO) << ". . testLine = " << test;

			if (test == VOXEL_UNIT)
			{
				for (int // voxel of hit must be inside of scanned targetTile(s)
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
						if (   trajectory.at(0).x / 16 == (scanVoxel->x / 16) + x + xOffset
							&& trajectory.at(0).y / 16 == (scanVoxel->y / 16) + y + yOffset
							&& trajectory.at(0).z >= targetMinHeight
							&& trajectory.at(0).z <= targetMaxHeight)
						{
							//Log(LOG_INFO) << ". . . . Unit found @ voxel, ret TRUE";
							//Log(LOG_INFO) << ". . . . scanVoxel " << (*scanVoxel);
							targetable_x.push_back(scanVoxel->x);
							targetable_y.push_back(scanVoxel->y);
							targetable_z.push_back(scanVoxel->z);
						}
					}
				}
			}
			else if (test == VOXEL_EMPTY
				&& hypothetical == true
				&& trajectory.empty() == false)
			{
				//Log(LOG_INFO) << ". . hypothetical Found, ret TRUE";
				return true;
			}
		}
	}

	if (targetable_x.empty() == false)
	{
		int
			minVal = *std::min_element(targetable_x.begin(), targetable_x.end()),
			maxVal = *std::max_element(targetable_x.begin(), targetable_x.end()),
			target_x = (minVal + maxVal) / 2,
			target_y,
			target_z;

		minVal = *std::min_element(targetable_y.begin(), targetable_y.end()),
		maxVal = *std::max_element(targetable_y.begin(), targetable_y.end());
		target_y = (minVal + maxVal) / 2;

		minVal = *std::min_element(targetable_z.begin(), targetable_z.end()),
		maxVal = *std::max_element(targetable_z.begin(), targetable_z.end());
		target_z = (minVal + maxVal) / 2;

		*scanVoxel = Position(
						target_x,
						target_y,
						target_z);

		//Log(LOG_INFO) << ". scanVoxel RET " << (*scanVoxel);
		return true;
	}

	//Log(LOG_INFO) << "TileEngine::canTargetUnit() exit FALSE";
	return false;
}
/*
bool TileEngine::canTargetUnit(
		const Position* const originVoxel,
		const Tile* const targetTile,
		Position* const scanVoxel,
		const BattleUnit* const excludeUnit,
		const BattleUnit* targetUnit)
{
	//Log(LOG_INFO) << "TileEngine::canTargetUnit()";
	const bool hypothetical = (targetUnit != NULL);

	if (targetUnit == NULL)
	{
		targetUnit = targetTile->getUnit();
		if (targetUnit == NULL)
		{
			//Log(LOG_INFO) << ". no Unit, ret FALSE";
			return false; // no unit in this targetTile, even if it's elevated and appearing in it.
		}
	}

	if (targetUnit == excludeUnit) // skip self
	{
		//Log(LOG_INFO) << ". hit vs Self, ret FALSE";
		return false;
	}

	const Position targetVoxel = Position(
										targetTile->getPosition().x * 16 + 8,
										targetTile->getPosition().y * 16 + 8,
										targetTile->getPosition().z * 24);
	const int
		targetMinHeight = targetVoxel.z
							- targetTile->getTerrainLevel()
							+ targetUnit->getFloatHeight(),
		// if there is a unit on targetTile, assume check against that unit's height
		xOffset = targetUnit->getPosition().x - targetTile->getPosition().x,
		yOffset = targetUnit->getPosition().y - targetTile->getPosition().y,
		targetSize = targetUnit->getArmor()->getSize() - 1;
	int
		unitRadius = targetUnit->getLoft(), // width = LoFT in default LoFTemps set
		targetMaxHeight = targetMinHeight,
		targetCenterHeight,
		heightRange;

	if (targetSize > 0)
		unitRadius = 3;

	// origin-to-target vector manipulation to make scan work in voxelspace
	const Position relPos = targetVoxel - *originVoxel;
	const float unitCenter = static_cast<float>(unitRadius)
						   / std::sqrt(static_cast<float>((relPos.x * relPos.x) + (relPos.y * relPos.y)));
	const int
//		relX = static_cast<int>(std::floor(static_cast<float>( relPos.y) * unitCenter + 0.5f)),
//		relY = static_cast<int>(std::floor(static_cast<float>(-relPos.x) * unitCenter + 0.5f)),
		relX = static_cast<int>(Round(std::floor(static_cast<float>( relPos.y) * unitCenter))),
		relY = static_cast<int>(Round(std::floor(static_cast<float>(-relPos.x) * unitCenter))),
		targetSlices[10] =
		{
			 0,		 0,
			 relX,	 relY,
			-relX,	-relY,
			 relY,	-relX,
			-relY,	 relX
		};

	if (targetUnit->isOut() == false)
		heightRange = targetUnit->getHeight();
	else
		heightRange = 12;

	targetMaxHeight += heightRange;
	targetCenterHeight = (targetMaxHeight + targetMinHeight) / 2;
	heightRange /= 2;
	if (heightRange > 10) heightRange = 10;
	if (heightRange < 0) heightRange = 0;

	std::vector<Position> trajectory;

	for (int // scan from center point up and down using heightFromCenter array
			i = 0;
			i <= heightRange;
			++i)
	{
		scanVoxel->z = targetCenterHeight + heightFromCenter[static_cast<size_t>(i)];
		//Log(LOG_INFO) << ". iterate heightRange, i = " << i << ", scan.Z = " << scanVoxel->z;

		for (size_t // scan from vertical centerline outwards left and right using targetSlices array
				j = 0;
				j != 5;
				++j)
		{
			//Log(LOG_INFO) << ". . iterate j = " << j;
			if (i < heightRange - 1 // looks like this gives an oval shape to target instead of an upright rectangle
				&& j > 2)
			{
				//Log(LOG_INFO) << ". . . out of bounds, break (j)";
				break; // skip unnecessary checks
			}

			scanVoxel->x = targetVoxel.x + targetSlices[j * 2];
			scanVoxel->y = targetVoxel.y + targetSlices[j * 2 + 1];
			//Log(LOG_INFO) << ". . scan.X = " << scanVoxel->x;
			//Log(LOG_INFO) << ". . scan.Y = " << scanVoxel->y;

			trajectory.clear();

			const int test = plotLine(
									*originVoxel,
									*scanVoxel,
									false,
									&trajectory,
									excludeUnit);
			//Log(LOG_INFO) << ". . testLine = " << test;

			if (test == VOXEL_UNIT)
			{
				for (int // voxel of hit must be inside of scanned targetTile(s)
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
						if (   trajectory.at(0).x / 16 == (scanVoxel->x / 16) + x + xOffset
							&& trajectory.at(0).y / 16 == (scanVoxel->y / 16) + y + yOffset
							&& trajectory.at(0).z >= targetMinHeight
							&& trajectory.at(0).z <= targetMaxHeight)
						{
							//Log(LOG_INFO) << ". . . . Unit found @ voxel, ret TRUE";
							return true;
						}
					}
				}
			}
			else if (test == VOXEL_EMPTY
				&& hypothetical == true
				&& trajectory.empty() == false)
			{
				//Log(LOG_INFO) << ". . hypothetical Found, ret TRUE";
				return true;
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::canTargetUnit() exit FALSE";
	return false;
} */

/**
 * Checks for a tile part available for targeting and what particular voxel.
 * @param originVoxel	- pointer to the Position voxel of trace origin (eg. gun's barrel)
 * @param targetTile	- pointer to the Tile to check against
 * @param tilePart		- tile part to check for (MapData.h)
 * @param scanVoxel		- pointer to voxel that is returned coordinate of hit
 * @param excludeUnit	- pointer to unitSelf (to not hit self)
 * @return, true if targetTile can be targeted
 */
bool TileEngine::canTargetTile(
		const Position* const originVoxel,
		const Tile* const targetTile,
		const MapDataType tilePart,
		Position* const scanVoxel,
		const BattleUnit* const excludeUnit) const
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
								targetTile->getPosition().x * 16,
								targetTile->getPosition().y * 16,
								targetTile->getPosition().z * 24);

	std::vector<Position> _trajectory;

	int
		* spiralArray,
		minZ = 0,
		maxZ = 0;
	size_t spiralCount;
	bool
		minZfound = false,
		maxZfound = false;


	if (tilePart == O_OBJECT)
	{
		spiralArray = sliceObjectSpiral;
		spiralCount = 41;
	}
	else if (tilePart == O_NORTHWALL)
	{
		spiralArray = northWallSpiral;
		spiralCount = 7;
	}
	else if (tilePart == O_WESTWALL)
	{
		spiralArray = westWallSpiral;
		spiralCount = 7;
	}
	else if (tilePart == O_FLOOR)
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
		//Log(LOG_INFO) << "TileEngine::canTargetTile() EXIT, ret False (tilePart is not a tileObject)";
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
							true) == tilePart) // bingo. Note MapDataType & VoxelType correspond.
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
							true) == tilePart) // bingo. Note MapDataType & VoxelType correspond.
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

			const VoxelType test = plotLine(
										*originVoxel,
										*scanVoxel,
										false,
										&_trajectory,
										excludeUnit);
			if (test == tilePart) // bingo. Note MapDataType & VoxelType correspond.
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

/*
 * Checks for how exposed unit is to another unit.
 * @param originVoxel	- voxel of trace origin (eye or gun's barrel)
 * @param tile			- pointer to Tile to check against
 * @param excludeUnit	- is self (not to hit self)
 * @param excludeAllBut	- [optional] is unit which is the only one to be considered for ray hits
 * @return, degree of exposure (as percent)
 */
/*	int TileEngine::checkVoxelExposure(
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

	int unitRadius = otherUnit->getLoft(); // width == loft in default loftemps set
	if (otherUnit->getArmor()->getSize() > 1)
	{
		unitRadius = 3;
	}

	// vector manipulation to make scan work in view-space
	Position relPos = targetVoxel - *originVoxel;
	float normal = static_cast<float>(unitRadius) / sqrt(static_cast<float>(relPos.x * relPos.x + relPos.y * relPos.y));
	int relX = static_cast<int>(floor(static_cast<float>(relPos.y) * normal + 0.5f));
	int relY = static_cast<int>(floor(static_cast<float>(-relPos.x) * normal + 0.5f));

	int targetSlices[10] = // looks like [6] to me..
	{
		0,		0,
		relX,	relY,
		-relX,	-relY
	};
/*	int targetSlices[10] = // taken from "canTargetUnit()"
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
			scanVoxel.x = targetVoxel.x + targetSlices[j * 2];
			scanVoxel.y = targetVoxel.y + targetSlices[j * 2 + 1];

			_trajectory.clear();

			int test = plotLine(*originVoxel, scanVoxel, false, &_trajectory, excludeUnit, true, false, excludeAllBut);
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
 * Checks if a 'sniper' from the opposing faction sees this unit.
 * @note The unit with the highest reaction score will be compared with the
 * triggering unit's reaction score. If it's higher a shot is fired when enough
 * time units plus a weapon and ammo are available.
 * @note The tuSpent parameter is needed because popState() doesn't subtract TU
 * until after the Initiative has been calculated or called from
 * ProjectileFlyBState.
 * @param triggerUnit	- pointer to a unit to check reaction fire against
 * @param tuSpent		- the unit's triggering expenditure of TU if firing or throwing
 * @param autoSpot		- true if RF was not triggered by a Melee atk (default true)
 * @return, true if reaction fire took place
 */
bool TileEngine::checkReactionFire(
		BattleUnit* const triggerUnit,
		int tuSpent,
		bool autoSpot)
{
	//Log(LOG_INFO) << "TileEngine::checkReactionFire() vs id-" << triggerUnit->getId();
	//Log(LOG_INFO) << ". tuSpent = " << tuSpent;
	if (_battleSave->getSide() == FACTION_NEUTRAL						// no reaction on civilian turn.
		|| triggerUnit->getFaction() != _battleSave->getSide()			// spotted unit must be current side's faction
		|| triggerUnit->getTile() == NULL								// and must be on map
		|| triggerUnit->isOut(true, true) == true)						// and must be conscious
//		|| _battleSave->getBattleGame()->getPanicHandled() == false)	// and ... nahhh. Note that doesn't affect aLien RF anyway.
	{
		return false;
	}

	// NOTE: If RF is triggered by melee (or walking/kneeling), a target that is
	// a potential RF-spotter will not be damaged yet and hence the damage +
	// checkForCasualties() happens later; but if RF is triggered by a firearm,
	// a target that is a potential RF-spotter *will* be damaged when this runs
	// since damage + checkForCasualties() has already been called. fucko*

	bool ret = false;

	if (triggerUnit->getFaction() == triggerUnit->getOriginalFaction()	// not MC'd
		|| triggerUnit->getFaction() == FACTION_PLAYER)					// or is xCom agent - MC'd aLiens do not RF.
	{
		//Log(LOG_INFO) << ". Target = VALID";
		std::vector<BattleUnit*> spotters = getSpottingUnits(triggerUnit);
		//Log(LOG_INFO) << ". # spotters = " << spotters.size();

		BattleUnit* reactorUnit = getReactor( // get the first man up to bat.
										spotters,
										triggerUnit,
										tuSpent,
										autoSpot);
		// start iterating through the possible reactors until
		// the current unit is the one with the highest score.
		while (reactorUnit != triggerUnit)
		{
			// !!!!!SHOOT!!!!!!!!
			if (reactionShot(reactorUnit, triggerUnit) == false)
			{
				//Log(LOG_INFO) << ". . no Snap by id-" << reactorUnit->getId();
				// can't make a reaction snapshot for whatever reason then boot this guy from the vector.
				for (std::vector<BattleUnit*>::const_iterator
						i = spotters.begin();
						i != spotters.end();
						++i)
				{
					if (*i == reactorUnit)
					{
						spotters.erase(i);
						break;
					}
				}
			}
			else
			{
				if (reactorUnit->getGeoscapeSoldier() != NULL
					&& reactorUnit->getFaction() == reactorUnit->getOriginalFaction())
				{
					//Log(LOG_INFO) << ". . reactionXP to " << reactorUnit->getId();
					reactorUnit->addReactionExp();
				}

				//Log(LOG_INFO) << ". . Snap by id-" << reactorUnit->getId();
				ret = true;
			}

			reactorUnit = getReactor( // nice shot.
								spotters,
								triggerUnit,
								tuSpent,
								autoSpot);
			//Log(LOG_INFO) << ". . NEXT AT BAT id-" << reactorUnit->getId();
		}

		spotters.clear();
	}

	return ret;
}

/**
 * Creates a vector of BattleUnits that can spot @a unit.
 * @param unit - pointer to a BattleUnit to spot
 * @return, vector of pointers to BattleUnits that can see the triggering unit
 */
std::vector<BattleUnit*> TileEngine::getSpottingUnits(const BattleUnit* const unit)
{
	//Log(LOG_INFO) << "TileEngine::getSpottingUnits() vs. id-" << unit->getId();
	const Tile* const tile = unit->getTile();
	std::vector<BattleUnit*> spotters;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		//Log(LOG_INFO) << ". spotCheck id-" << (*i)->getId();
		if ((*i)->getTimeUnits() != 0
			&& (*i)->getFaction() != _battleSave->getSide()
			&& (*i)->getFaction() != FACTION_NEUTRAL
			&& (*i)->isOut_t() == false)
//			&& (*i)->isOut(true, true) == false)
//			&& (*i)->getSpawnUnit().empty() == true)
		{
			if ((((*i)->getFaction() == FACTION_HOSTILE							// Mc'd xCom units will RF on loyal xCom units
						&& (*i)->getOriginalFaction() != FACTION_PLAYER			// but Mc'd aLiens won't RF on other aLiens ...
						&& (*i)->isZombie() == false)
					|| (((*i)->getOriginalFaction() == FACTION_PLAYER			// Also - aLiens get to see in all directions
							|| (*i)->isZombie() == true)
						&& (*i)->checkViewSector(unit->getPosition()) == true))	// but xCom must checkViewSector() even when MC'd
				&& visible(
						*i,
						tile) == true)
			{
				//Log(LOG_INFO) << ". . Add spotter.";
				spotters.push_back(*i);
			}
		}
	}

	return spotters;
}

/**
 * Gets the unit with the highest reaction score from the spotters vector.
 * @note The tuSpent parameter is needed because popState() doesn't
 * subtract TU until after the Initiative has been calculated or called from
 * ProjectileFlyBState.
 * @param spotters	- vector of the pointers to spotting BattleUnits
 * @param defender	- pointer to the defending BattleUnit to check reaction scores against
 * @param tuSpent	- defending BattleUnit's expenditure of TU that had caused reaction checks
 * @param autoSpot	- true if RF was not triggered by a Melee atk (default true)
 * @return, pointer to the BattleUnit with initiative (next up!)
 */
BattleUnit* TileEngine::getReactor(
		std::vector<BattleUnit*> spotters,
		BattleUnit* const defender,
		const int tuSpent,
		bool autoSpot) const
{
	//Log(LOG_INFO) << "TileEngine::getReactor() vs id-" << defender->getId();
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
		//Log(LOG_INFO) << ". . check nextReactor id-" << (*i)->getId();
		if ((*i)->isOut() == false)
		{
			initTest = (*i)->getInitiative();
			if (initTest > init)
			{
				init = initTest;
				nextReactor = *i;
			}
		}
	}

	//Log(LOG_INFO) << ". id-" << defender->getId() << " initi = " << defender->getInitiative(tuSpent);

	// nextReactor has to *best* defender.Init to get initiative
	// Analysis: It appears that defender's tu for firing/throwing
	// are not subtracted before getInitiative() is called.

	if (nextReactor == NULL
		|| init <= defender->getInitiative(tuSpent))
	{
		nextReactor = defender;
	}

	if (nextReactor != defender
		&& nextReactor->getFaction() == FACTION_HOSTILE)
	{
		//Log(LOG_INFO) << "getReactor() id-" << nextReactor->getId() << " spots " << defender->getId();
		if (autoSpot == true)
			defender->setExposed();								// defender has been spotted on Player turn.
		else
			defender->getRfSpotters()->push_back(nextReactor);	// let BG::checkForCasualties() figure it out
	}															// this is so that if an aLien in the spotters-vector gets put down by the trigger-shot it won't tell its buds.

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
	//Log(LOG_INFO) << "TileEngine::reactionShot() " << unit->getId();
	_rfAction->actor = unit;

	if (unit->getFaction() == FACTION_PLAYER
		&& unit->getOriginalFaction() == FACTION_PLAYER)
	{
		_rfAction->weapon = unit->getItem(unit->getActiveHand());
	}
	else
	{
		_rfAction->weapon = unit->getMainHandWeapon();

		if (_rfAction->weapon == NULL
			&& _rfAction->actor->getUnitRules() != NULL
			&& _rfAction->actor->getUnitRules()->getMeleeWeapon() == "STR_FIST")
		{
			_rfAction->weapon = _battleSave->getBattleGame()->getFist();
		}
	}

	if (_rfAction->weapon == NULL
		|| _rfAction->weapon->getRules()->canReactionFire() == false
		|| _rfAction->weapon->getAmmoItem() == NULL						// lasers & melee are their own ammo-items
		|| _rfAction->weapon->getAmmoItem()->getAmmoQuantity() == 0		// lasers & melee return INT_MAX
		|| (_rfAction->actor->getFaction() != FACTION_HOSTILE			// is not an aLien and has unresearched weapon.
			&& _battleSave->getGeoscapeSave()->isResearched(_rfAction->weapon->getRules()->getRequirements()) == false))
	{
		return false;
	}


	_rfAction->target = targetUnit->getPosition();
	_rfAction->TU = 0;

	if (_rfAction->weapon->getRules()->getBattleType() == BT_MELEE)
	{
		_rfAction->type = BA_HIT;
		_rfAction->TU = _rfAction->actor->getActionTUs(
											BA_HIT,
											_rfAction->weapon);
		if (_rfAction->TU == 0
			|| _rfAction->TU > _rfAction->actor->getTimeUnits())
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
			canMelee = validMeleeRange(				// hopefully this is blocked by walls & bigWalls ...
									unit,			// see also, AI do_grenade_action .....
									i,				// darn Sectoid tried to hurl a grenade through a northwall .. with *no LoS*
									targetUnit);	// cf. ActionMenuState::btnActionMenuItemClick()
		}

		if (canMelee == false)
			return false;
	}
	else
	{
		_rfAction->type = BA_NONE;
		selectFireMethod(*_rfAction); // choose BAT & setTU req'd.

		if (_rfAction->type == BA_NONE)
			return false;
	}

	_rfAction->targeting = true;

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

		if (_rfAction->weapon->getAmmoItem()->getRules()->getExplosionRadius() > 0
			&& aggro_AI->explosiveEfficacy(
									_rfAction->target,
									unit,
									_rfAction->weapon->getAmmoItem()->getRules()->getExplosionRadius(),
									_battleSave->getBattleState()->getSavedGame()->getDifficulty()) == false)
		{
			_rfAction->targeting = false;
		}
	}

	if (_rfAction->targeting == true
		&& _rfAction->actor->spendTimeUnits(_rfAction->TU) == true)
	{
//		if (_rfAction->actor->getFaction() != FACTION_HOSTILE)
//		{
//		Camera* const rfCam = _battleSave->getBattleState()->getMap()->getCamera();	// this is getting done in Map.
//		rfCam->centerOnPosition(_rfAction->actor->getPosition());					// The stored shot-position is more accurate from there.
//		_rfAction->cameraPosition = rfCam->getMapOffset();							// Unfortunately I don't know how to pre-calculate the voxelPosition that the shot will be
//		}																			// fired from (which is done in Map) and convert it to a mapOffset so I can't store it here.

		_rfAction->TU = 0;

		_battleSave->getBattleGame()->statePushBack(new UnitTurnBState(
																_battleSave->getBattleGame(),
																*_rfAction,
																false));
		_battleSave->getBattleGame()->statePushBack(new ProjectileFlyBState(
																	_battleSave->getBattleGame(),
																	*_rfAction));
		return true;
	}

	return false;
}

/**
 * Selects a fire method based on range & time units.
 * @param action - reference a BattleAction struct
 */
void TileEngine::selectFireMethod(BattleAction& action) // <- TODO: this action needs to be replaced w/ _rfAction, i think.
{
	const RuleItem* const itRule = action.weapon->getRules();
	const int dist = _battleSave->getTileEngine()->distance(
														action.actor->getPosition(),
														action.target);
	if (dist > itRule->getMaxRange()
		|| dist < itRule->getMinRange()) // this might not be what I think it is ...
	{
		return;
	}

	const int tu = action.actor->getTimeUnits();

	if (dist <= itRule->getAutoRange())
	{
		if (itRule->getTUAuto() != 0
			&& tu >= action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))
		{
			action.type = BA_AUTOSHOT;
			action.TU = action.actor->getActionTUs(BA_AUTOSHOT, action.weapon);
		}
		else if (itRule->getTUSnap() != 0
			&& tu >= action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
		{
			action.type = BA_SNAPSHOT;
			action.TU = action.actor->getActionTUs(BA_SNAPSHOT, action.weapon);
		}
		else if (itRule->getTUAimed() != 0
			&& tu >= action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
		{
			action.type = BA_AIMEDSHOT;
			action.TU = action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon);
		}
	}
	else if (dist <= itRule->getSnapRange())
	{
		if (itRule->getTUSnap() != 0
			&& tu >= action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
		{
			action.type = BA_SNAPSHOT;
			action.TU = action.actor->getActionTUs(BA_SNAPSHOT, action.weapon);
		}
		else if (itRule->getTUAimed() != 0
			&& tu >= action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
		{
			action.type = BA_AIMEDSHOT;
			action.TU = action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon);
		}
		else if (itRule->getTUAuto() != 0
			&& tu >= action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))
		{
			action.type = BA_AUTOSHOT;
			action.TU = action.actor->getActionTUs(BA_AUTOSHOT, action.weapon);
		}
	}
	else // if (dist <= itRule->getAimRange())
	{
		if (itRule->getTUAimed() != 0
			&& tu >= action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
		{
			action.type = BA_AIMEDSHOT;
			action.TU = action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon);
		}
		else if (itRule->getTUSnap() != 0
			&& tu >= action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
		{
			action.type = BA_SNAPSHOT;
			action.TU = action.actor->getActionTUs(BA_SNAPSHOT, action.weapon);
		}
		else if (itRule->getTUAuto() != 0
			&& tu >= action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))
		{
			action.type = BA_AUTOSHOT;
			action.TU = action.actor->getActionTUs(BA_AUTOSHOT, action.weapon);
		}
	}

	return;
}

/**
 * Gets the unique reaction fire BattleAction struct.
 * @return, rf action struct
 */
BattleAction* TileEngine::getRfAction()
{
	return _rfAction;
}

/**
 * Gets the reaction fire shot list.
 * @return, pointer to a map of unit-IDs & Positions
 */
std::map<int, Position>* TileEngine::getRfShotList()
{
	return &_rfShotList;
}

/**
 * Handles bullet/weapon hits. A bullet/weapon hits a voxel.
 * @note Called from ExplosionBState::explode().
 * @param targetVoxel	- reference the center of hit in voxelspace
 * @param power			- power of the hit/explosion
 * @param dType			- damage type of the hit (RuleItem.h)
 * @param attacker		- pointer to BattleUnit that caused the hit
 * @param melee			- true if no projectile, trajectory, etc. is needed
 * @return, pointer to the BattleUnit that got hit
 */
BattleUnit* TileEngine::hit(
		const Position& targetVoxel,
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
		Position posTarget = Position(
									targetVoxel.x / 16,
									targetVoxel.y / 16,
									targetVoxel.z / 24);
		Tile* const tile = _battleSave->getTile(posTarget);
		//Log(LOG_INFO) << ". targetTile " << tile->getPosition() << " targetVoxel " << targetVoxel;
		if (tile == NULL)
		{
			//Log(LOG_INFO) << ". Position& targetVoxel : NOT Valid, return NULL";
			return NULL;
		}

		BattleUnit* targetUnit = tile->getUnit();

		VoxelType voxelType;
		if (melee == true)
			voxelType = VOXEL_UNIT;
		else
		{
			voxelType = voxelCheck(
								targetVoxel,
								attacker,
								false,
								false,
								NULL);
			//Log(LOG_INFO) << ". voxelCheck() voxelType = " << voxelType;
		}

		if (voxelType > VOXEL_EMPTY && voxelType < VOXEL_UNIT	// 4 terrain parts (0..3)
			&& dType != DT_STUN									// kL, workaround for Stunrod. (might include DT_SMOKE & DT_IN)
			&& dType != DT_SMOKE)
		{
			//Log(LOG_INFO) << ". . terrain hit";
			power = RNG::generate( // 25% to 75% linear.
								power / 4,
								power * 3 / 4);
			//Log(LOG_INFO) << ". . RNG::generate(power) = " << power;
			// kL_note: This is where to adjust damage based on effectiveness of weapon vs Terrain!
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
			MapDataType partType = static_cast<MapDataType>(voxelType); // Note that MapDataType & VoxelType correspond.

			switch (dType) // round up.
			{
				case DT_AP:
					power = ((power * 3) + 19) / 20;	// 15%
				break;

				case DT_LASER:
					if (tile->getMapData(partType)->getSpecialType() != ALIEN_ALLOYS)
						power = (power + 4) / 5;		// 20% // problem: Fusion Torch; fixed, heh.
				break;

				case DT_IN:
					power = (power + 3) / 4;			// 25%
				break;

				case DT_PLASMA:
					power = (power + 2) / 3;			// 33%
				break;

				case DT_MELEE:							// TODO: define 2 terrain types, Soft & Hard; so that edged weapons do good vs. Soft, blunt weapons do good vs. Hard
					power = (power + 1) / 2;			// 50% TODO: allow melee attacks vs. objects.
				break;

				case DT_HE:								// question: do HE & IN ever get in here - hit() or explode() below
					power += power / 10;				// 110%
//				break;

//				case DT_ACID: // 100% damage

//				default: // [DT_NONE],[DT_STUN,DT_SMOKE]
//					return NULL;
			}
			//Log(LOG_INFO) << ". . power by Type = " << power;

			if (power > 0)
			{
				if (partType == O_OBJECT
					&& _battleSave->getTacticalType() == TCT_BASEDEFENSE
					&& tile->getMapData(O_OBJECT)->isBaseModule() == true
					&& tile->getMapData(O_OBJECT)->getArmor() <= power)
				{
					//Log(LOG_INFO) << ". . . vs Object hp  = " << tile->getMapData(O_OBJECT)->getArmor();
					_battleSave->getModuleMap()
										[(targetVoxel.x / 16) / 10]
										[(targetVoxel.y / 16) / 10].second--;
				}

				if (tile->damage(
							partType,
							power,
							_battleSave->getObjectiveType()) == true)
				{
					_battleSave->addDestroyedObjective();
				}
			}
		}
		else if (voxelType == VOXEL_UNIT) // battleunit voxelType HIT SUCCESS.
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
				// it's possible we have a unit below the actual tile, when it
				// stands on a stairs and sticks its head up into the above tile.
				// kL_note: yeah, just like in LoS calculations!!!! cf. visible() etc etc .. idiots.
				const Tile* const tileBelow = _battleSave->getTile(posTarget + Position(0,0,-1));
				if (tileBelow != NULL
					&& tileBelow->getUnit() != NULL)
				{
					targetUnit = tileBelow->getUnit();
					vertOffset = 24;
				}
			}

			if (targetUnit != NULL)
//				&& targetUnit->isOut_t(OUT_DEAD_STUN) == false)
			{
				//Log(LOG_INFO) << ". . . targetUnit Valid ID = " << targetUnit->getId();
				const int wounds = targetUnit->getFatalWounds();

				// kL_begin: TileEngine::hit(), Silacoids can set targets on fire!!
				if (attacker != NULL
					&& attacker->getSpecialAbility() == SPECAB_BURN)
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
					pos = targetUnit->getPosition() * Position(16,16,24) // convert tilespace to voxelspace
							  + Position(
										unitSize,
										unitSize,
										targetUnit->getFloatHeight() - tile->getTerrainLevel()),
					relPos = targetVoxel
						   - pos
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
					power1 = static_cast<int>(std::floor(static_cast<double>(power) * (100. - delta) / 100.)) + 1,
					power2 = static_cast<int>(std::ceil(static_cast<double>(power) * (100. + delta) / 100.));

				int extraPower = 0;
				if (attacker != NULL) // bonus to damage per Accuracy (TODO: use ranks also for xCom or aLien)
				{
					if (dType == DT_AP
						|| dType == DT_LASER
						|| dType == DT_PLASMA
						|| dType == DT_ACID)
					{
						extraPower = attacker->getBaseStats()->firing;
					}
					else if (dType == DT_MELEE)
						extraPower = attacker->getBaseStats()->melee;

					extraPower = static_cast<int>(Round(static_cast<double>(power * extraPower) / 1000.));
				}

				power = RNG::generate(power1, power2) // bell curve
					  + RNG::generate(power1, power2);
				power /= 2;
				//Log(LOG_INFO) << ". . . RNG::generate(power) = " << power;
				power += extraPower;

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
							|| targetUnit->isOut_t(OUT_DEAD) == true))
//							|| targetUnit->getHealth() == 0)) // .. just do this here and bDone with it. Regularly done in BattlescapeGame::checkForCasualties()
					{
						targetUnit->killedBy(attacker->getFaction());
						//Log(LOG_INFO) << "TE::hit() " << targetUnit->getId() << " killedBy = " << (int)attacker->getFaction();
					}
					// kL_note: Not so sure that's been setup right (cf. other kill-credit code as well as DebriefingState)
					// I mean, shouldn't that be checking that the thing actually DIES?
					// And, probly don't have to state if killed by aLiens: probly assumed in DebriefingState.

//					if (targetUnit->getHealth() > 0)
/*					if (targetUnit->isOut_t(OUT_DEAD) == false // -> moved to BattleUnit::damage()
						&& targetUnit->isFearable() == true)
					{
						int moraleLoss = (110 - targetUnit->getBaseStats()->bravery) / 10;
						if (moraleLoss > 0)
						{
							int leadership = 100; // <- for civilians
							if (targetUnit->getOriginalFaction() == FACTION_PLAYER)
								leadership = _battleSave->getMoraleModifier();
							else if (targetUnit->getOriginalFaction() == FACTION_HOSTILE)
								leadership = _battleSave->getMoraleModifier(NULL, false);

							moraleLoss = moraleLoss * power * 10 / leadership;
							//Log(LOG_INFO) << ". . . . moraleLoss = " << moraleLoss;
							targetUnit->moraleChange(-moraleLoss);
						}
					} */

					//Log(LOG_INFO) << ". . check for Cyberdisc expl.";
					//Log(LOG_INFO) << ". . health = " << targetUnit->getHealth();
					//Log(LOG_INFO) << ". . stunLevel = " << targetUnit->getStun();
					if (targetUnit->getSpecialAbility() == SPECAB_EXPLODE // cyberdiscs
//						&& targetUnit->isOut_t(OUT_STAT) == false
						&& targetUnit->isOut_t(OUT_DEAD_STUN) == true)
//						&& (targetUnit->getHealth() == 0
//							|| targetUnit->getStun() >= targetUnit->getHealth()))
					{
						//Log(LOG_INFO) << ". . . Cyberdisc down!!";
						if (dType != DT_STUN	// don't explode if stunned. Maybe... see above.
							&& dType != DT_SMOKE
							&& dType != DT_HE	// don't explode if taken down w/ explosives -> wait a sec, this is hit() not explode() ...
							&& dType != DT_IN)
//							&& dType != DT_MELEE)
						{
							//Log(LOG_INFO) << ". . . . new ExplosionBState(), !DT_STUN & !DT_HE";
							// kL_note: wait a second. hit() creates an ExplosionBState,
							// but ExplosionBState::explode() creates a hit() ! -> terrain..

							const Position posUnit = Position(
//														targetUnit->getPosition().x * 16,
//														targetUnit->getPosition().y * 16,
//														targetUnit->getPosition().z * 24);
														targetUnit->getPosition().x * 16 + 16,	// kL, cyberdisc a big unit.
														targetUnit->getPosition().y * 16 + 16,	// kL
														targetUnit->getPosition().z * 24 + 12);	// kL
							_battleSave->getBattleGame()->statePushNext(new ExplosionBState(
																						_battleSave->getBattleGame(),
																						posUnit,
																						NULL,
																						targetUnit));
						}
					}
				}


				if (melee == false
					&& _battleSave->getBattleGame()->getCurrentAction()->takenXp == false
					&& targetUnit->getOriginalFaction() == FACTION_HOSTILE
					&& attacker != NULL
					&& attacker->getGeoscapeSoldier() != NULL
					&& attacker->getFaction() == attacker->getOriginalFaction()
					&& _battleSave->getBattleGame()->getPanicHandled() == true)
				{
					_battleSave->getBattleGame()->getCurrentAction()->takenXp = true;
					attacker->addFiringExp();
				}
			}
		}


		applyGravity(tile);
		calculateSunShading();		// roofs could have been destroyed
		calculateTerrainLighting();	// fires could have been started
//		calculateUnitLighting();	// units could have collapsed <- done in UnitDieBState
		calculateFOV(
				posTarget,
				true);

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
 * @note Called from ExplosionBState.
 * @note
 * HE/ smoke/ fire explodes in a circular pattern on 1 level only.
 * HE however damages floor tiles of the above level. Not the units on it.
 * HE destroys an object if its power is higher than the object's armor
 * then HE blockage is applied for further propagation.
 * See http://www.ufopaedia.org/index.php?title=Explosions for more info.
 * @param targetVoxel	- reference to the center of explosion in voxelspace
 * @param power			- power of explosion
 * @param dType			- damage type of explosion (RuleItem.h)
 * @param maxRadius		- maximum radius of explosion
 * @param attacker		- pointer to a unit that caused explosion (default NULL)
 * @param grenade		- true if explosion is caused by a grenade for throwing XP (default false)
 * @param defusePulse	- true if explosion item caused an electo-magnetic pulse that defuses primed grenades (default false)
 */
void TileEngine::explode(
			const Position& targetVoxel,
			int power,
			ItemDamageType dType,
			int maxRadius,
			BattleUnit* const attacker,
			bool grenade,
			bool defusePulse)
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
		* tileOrigin = NULL,
		* destTile = NULL;

	int // convert voxel-space to tile-space
		centerX = targetVoxel.x / 16,
		centerY = targetVoxel.y / 16,
		centerZ = targetVoxel.z / 24,

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

	bool takenXp = false;

//	int testIter = 0; // TEST.
	//Log(LOG_INFO) << ". r_Max = " << r_Max;

//	for (int fi = 0; fi == 0; ++fi) // kL_note: Looks like a TEST ray. ( 0 == horizontal )
//	for (int fi = 90; fi == 90; ++fi) // vertical: UP
	for (int
			fi = -90;
			fi < 91;
			fi += 5) // ray-tracing every 5 degrees makes sure all tiles are covered within a sphere.
	{
//		for (int te = 180; te == 180; ++te) // kL_note: Looks like a TEST ray. ( 0 == south, 180 == north, goes CounterClock-wise )
//		for (int te = 90; te < 360; te += 180) // E & W
//		for (int te = 45; te < 360; te += 180) // SE & NW
//		for (int te = 225; te < 420; te += 180) // NW & SE
		for (int
				te = 0;
				te < 361;
				te += 3) // ray-tracing every 3 degrees makes sure all tiles are covered within a circle.
		{
			sin_te = std::sin(static_cast<double>(te) * M_PI / 180.);
			cos_te = std::cos(static_cast<double>(te) * M_PI / 180.);
			sin_fi = std::sin(static_cast<double>(fi) * M_PI / 180.);
			cos_fi = std::cos(static_cast<double>(fi) * M_PI / 180.);

			tileOrigin = _battleSave->getTile(Position(
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

				vect_x = static_cast<double>(centerX) + (r * sin_te * cos_fi);
				vect_y = static_cast<double>(centerY) + (r * cos_te * cos_fi);
				vect_z = static_cast<double>(centerZ) + (r * sin_fi);

				tileX = static_cast<int>(std::floor(vect_x));
				tileY = static_cast<int>(std::floor(vect_y));
				tileZ = static_cast<int>(std::floor(vect_z));

				destTile = _battleSave->getTile(Position(
														tileX,
														tileY,
														tileZ));
				//Log(LOG_INFO) << ". . test : tileOrigin " << tileOrigin->getPosition() << " dest " << destTile->getPosition(); //<< ". _powerE = " << _powerE << ". r = " << r;

				if (destTile == NULL) // out of map!
				{
					//Log(LOG_INFO) << ". destTile NOT Valid " << Position(tileX, tileY, tileZ);
					break;
				}


				if (r > 0.5						// don't block epicentrum.
					&& tileOrigin != destTile)	// don't double blockage from the same tiles (when diagonal that happens).
				{
					int dir;
					Pathfinding::vectorToDirection(
												destTile->getPosition() - tileOrigin->getPosition(),
												dir);
					if (dir != -1
						&& (dir & 1) == 1)
					{
//						_powerE = static_cast<int>(static_cast<float>(_powerE) * 0.70710678f);
						_powerE = static_cast<int>(static_cast<double>(_powerE) * RNG::generate(0.895,0.935));
					}

					if (maxRadius > 0)
					{
						_powerE -= ((power + maxRadius - 1) / maxRadius); // round up.
						//Log(LOG_INFO) << "maxRadius > 0, " << power << "/" << maxRadius << "=" << _powerE;
					}
					//else Log(LOG_INFO) << "maxRadius <= 0";


					if (_powerE < 1)
					{
						//Log(LOG_INFO) << ". _powerE < 1 BREAK[hori] " << Position(tileX, tileY, tileZ) << "\n";
						break;
					}

					if (tileOrigin->getPosition().z != tileZ) // up/down explosion decrease
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
														tileOrigin,
														destTile,
														dType);
					//if (horiBlock != 0) Log(LOG_INFO) << ". horiBlock = " << horiBlock;

					const int vertBlock = verticalBlockage(
														tileOrigin,
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
					//Log(LOG_INFO) << ". > tile TRUE : tileOrigin " << tileOrigin->getPosition() << " dest " << destTile->getPosition(); //<< ". _powerE = " << _powerE << ". r = " << r;
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

							//Log(LOG_INFO) << "\n";
							BattleUnit* bu;
							bool done = false;

							while (done == false
								&& destTile->getInventory()->empty() == false)
							{
								for (std::vector<BattleItem*>::const_iterator
										i = destTile->getInventory()->begin();
										i != destTile->getInventory()->end();
										)
								{
									//Log(LOG_INFO) << "pos " << destTile->getPosition();
									//Log(LOG_INFO) << ". . INVENTORY: Item = " << (*i)->getRules()->getType();
									bu = (*i)->getUnit();

									if (bu != NULL
										&& bu->getStatus() == STATUS_UNCONSCIOUS
										&& bu->getTakenExpl() == false)
									{
										//Log(LOG_INFO) << ". . . vs Unit unconscious";
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
										//Log(LOG_INFO) << ". . . INVENTORY: damage = " << dam;

//										if (bu->getHealth() == 0)
										if (bu->isOut_t(OUT_DEAD) == true)
										{
											//Log(LOG_INFO) << ". . . . INVENTORY: instaKill";
											bu->instaKill();

											if (attacker != NULL)
											{
												bu->killedBy(attacker->getFaction()); // TODO: log the kill in Soldier's Diary.
												//Log(LOG_INFO) << "TE::explode() " << bu->getId() << " killedBy = " << (int)attacker->getFaction();
											}

											if (bu->getGeoscapeSoldier() != NULL // send Death notice.
												&& Options::battleNotifyDeath == true)
											{
												Game* const game = _battleSave->getBattleState()->getGame();
												game->pushState(new InfoboxOKState(game->getLanguage()->getString( // "has exploded ..."
																											"STR_HAS_BEEN_KILLED",
																											bu->getGender())
																										.arg(bu->getName(game->getLanguage()))));
											}
										}
									}
									else if (_powerE > (*i)->getRules()->getArmor()
										&& (bu == NULL
											|| (bu->getStatus() == STATUS_DEAD
												&& bu->getTakenExpl() == false)))
									{
										//Log(LOG_INFO) << ". . . vs Item armor";
										if ((*i)->getFuse() > -1)
										{
											//Log(LOG_INFO) << ". . . . INVENTORY: primed grenade";
											(*i)->setFuse(-2);

											const Position pos = Position(
																		destTile->getPosition().x * 16 + 8,
																		destTile->getPosition().y * 16 + 8,
																		destTile->getPosition().z * 24 + 2);
											_battleSave->getBattleGame()->statePushNext(new ExplosionBState(
																										_battleSave->getBattleGame(),
																										pos,
																										*i,
																										attacker));
										}
										else if ((*i)->getFuse() != -2)
										{
											//Log(LOG_INFO) << ". . . . INVENTORY: removeItem = " << (*i)->getRules()->getType();
											_battleSave->removeItem(*i);
											break;
//											--i; // "vector iterator not decrementable" - yeesh.
										}
									}
									//else Log(LOG_INFO) << ". . . INVENTORY: bypass item = " << (*i)->getRules()->getType();

									++i; // "vector iterator not incrementable" - yeesh.
									done = (i == destTile->getInventory()->end());
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

								const float vulnr = targetUnit->getArmor()->getDamageModifier(DT_IN);
								if (vulnr > 0.f)
								{
									const int burn = RNG::generate(
																0,
																static_cast<int>(Round(5.f * vulnr)));
									if (targetUnit->getFireOnUnit() < burn)
										targetUnit->setFireOnUnit(burn); // catch fire and burn!!
								}
							}

							Tile // NOTE: Should check if tileBelow's have already had napalm drop on them from this explosion ....
								* fireTile = destTile,
								* tileBelow = _battleSave->getTile(fireTile->getPosition() + Position(0,0,-1));

							while (fireTile != NULL // safety.
								&& fireTile->getPosition().z > 0
								&& fireTile->getMapData(O_OBJECT) == NULL
								&& fireTile->getMapData(O_FLOOR) == NULL
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
								const int firePower = std::max(
															1,
															static_cast<int>(std::ceil(
														   (static_cast<double>(_powerE) / static_cast<double>(power)) * 11.)));
								fireTile->addFire(firePower + fireTile->getFuel() + 3 / 4);
								fireTile->addSmoke(std::max(
														firePower + fireTile->getFuel(),
														firePower + ((fireTile->getFlammability() + 9) / 10)));
							}
//							}

							targetUnit = fireTile->getUnit();
							if (targetUnit != NULL
								&& targetUnit->getTakenExpl() == false)
							{
								powerUnit = RNG::generate( // 25% - 75%
														_powerE / 4,
														_powerE * 3 / 4);
								targetUnit->damage(
												Position(0,0,0),
												powerUnit,
												DT_IN,
												true);
								//Log(LOG_INFO) << ". . DT_IN : " << targetUnit->getId() << " takes " << firePower << " firePower";

								const float vulnr = targetUnit->getArmor()->getDamageModifier(DT_IN);
								if (vulnr > 0.f)
								{
									const int burn = RNG::generate(
																0,
																static_cast<int>(Round(5.f * vulnr)));
									if (targetUnit->getFireOnUnit() < burn)
										targetUnit->setFireOnUnit(burn); // catch fire and burn!!
								}
							}

							BattleUnit* bu;
							bool done = false;

							while (done == false
								&& destTile->getInventory()->empty() == false)
							{
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

//										if (bu->getHealth() == 0)
										if (bu->isOut_t(OUT_DEAD) == true)
										{
											bu->instaKill();

											if (attacker != NULL)
											{
												bu->killedBy(attacker->getFaction()); // TODO: log the kill in Soldier's Diary.
												//Log(LOG_INFO) << "TE::explode() " << bu->getId() << " killedBy = " << (int)attacker->getFaction();
											}

											if (bu->getGeoscapeSoldier() != NULL // send Death notice.
												&& Options::battleNotifyDeath == true)
											{
												Game* const game = _battleSave->getBattleState()->getGame();
												game->pushState(new InfoboxOKState(game->getLanguage()->getString( // "has been killed with Fire ..."
																											"STR_HAS_BEEN_KILLED",
																											bu->getGender())
																										.arg(bu->getName(game->getLanguage()))));
											}
										}
									}
									else if (_powerE > (*i)->getRules()->getArmor()
										&& (bu == NULL
											|| (bu->getStatus() == STATUS_DEAD
												&& bu->getTakenExpl() == false)))
									{
										if ((*i)->getFuse() > -1)
										{
											(*i)->setFuse(-2);

											const Position pos = Position(
																		destTile->getPosition().x * 16 + 8,
																		destTile->getPosition().y * 16 + 8,
																		destTile->getPosition().z * 24 + 2);
											_battleSave->getBattleGame()->statePushNext(new ExplosionBState(
																										_battleSave->getBattleGame(),
																										pos,
																										*i,
																										attacker));
										}
										else if ((*i)->getFuse() != -2)
										{
											_battleSave->removeItem(*i);
											break;
//											--i; // "vector iterator not decrementable" - yeesh.
										}
									}

									++i; // "vector iterator not incrementable" - yeesh.
									done = (i == destTile->getInventory()->end());
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
//							if (targetUnit->getHealth() == 0
							if (targetUnit->isOut_t(OUT_DEAD) == true
								|| wounds < targetUnit->getFatalWounds())
							{
								targetUnit->killedBy(attacker->getFaction()); // kL .. just do this here and bDone with it. Normally done in BattlescapeGame::checkForCasualties()
								//Log(LOG_INFO) << "TE::explode() " << targetUnit->getId() << " killedBy = " << (int)attacker->getFaction();
							}

							if (takenXp == false
								&& attacker->getGeoscapeSoldier() != NULL
								&& attacker->getFaction() == attacker->getOriginalFaction()
								&& targetUnit->getOriginalFaction() == FACTION_HOSTILE
								&& dType != DT_SMOKE
								&& _battleSave->getBattleGame()->getPanicHandled() == true)
							{
								takenXp = true;

								if (grenade == false)
									attacker->addFiringExp();
								else
									attacker->addThrowingExp();
							}
						}
					}
				}

				tileOrigin = destTile;
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
		if (_trueTile != NULL)	// special case for when a diagonal bigwall is directly targetted.
		{						// The explosion is moved out a tile so give a full-power hit to the true target-tile.
			_trueTile->setExplosive(
								power,
								0);
			detonate(_trueTile);	// I doubt this needs any *further* consideration ...
		}							// although it would be nice to have the explosion 'kick in' a bit.

		//Log(LOG_INFO) << ". explode Tiles, size = " << tilesAffected.size();
		for (std::set<Tile*>::const_iterator
				i = tilesAffected.begin();
				i != tilesAffected.end();
				++i)
		{
			if (*i != _trueTile)
			{
				if (detonate(*i) == true)
					_battleSave->addDestroyedObjective();

				applyGravity(*i);

				Tile* const tileAbove = _battleSave->getTile((*i)->getPosition() + Position(0,0,1));
				if (tileAbove != NULL)
					applyGravity(tileAbove);
			}
		}
		//Log(LOG_INFO) << ". explode Tiles DONE";
	}
	_trueTile = NULL;


	if (defusePulse == true)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _battleSave->getItems()->begin();
				i != _battleSave->getItems()->end();
				++i)
		{
			(*i)->setFuse(-1);
		}
	}


	calculateSunShading();		// roofs could have been destroyed
	calculateTerrainLighting();	// fires could have been started
//	calculateUnitLighting();	// units could have collapsed <- done in UnitDieBState
	recalculateFOV(true);
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
		const ItemDamageType dType) const
{
	//Log(LOG_INFO) << "TileEngine::horizontalBlockage()";
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

	int dir;
	Pathfinding::vectorToDirection(
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
							O_NORTHWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
				block += blockage(
								startTile,
								O_OBJECT,
								dType,
								0,
								true)
						+ blockage(
								endTile,
								O_OBJECT,
								dType,
								4);
		break;
		case 1: // north east
//			if (dType == DT_NONE) // this is two-way diagonal visibility check
			if (visLike)
			{
				block = blockage( // up+right
								startTile,
								O_NORTHWALL,
								dType)
						+ blockage(
								endTile,
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								O_OBJECT,
								dType,
								3); // checks Content/bigWalls

				// instead of last blockage() above, check this:
/*				tmpTile = _save->getTile(startTile->getPosition() + oneTileNorth);
				if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != BIGWALLNESW)
					block += blockage(tmpTile, O_OBJECT, dType, 3); */

				if (block == 0) break; // this way is opened

				block = blockage( // right+up
//				block += blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_OBJECT,
								dType,
								7); // checks Content/bigWalls

				// instead of last blockage() above, check this:
/*				tmpTile = _save->getTile(startTile->getPosition() + oneTileEast);
				if (tmpTile && tmpTile->getMapData(O_OBJECT) && tmpTile->getMapData(O_OBJECT)->getBigWall() != BIGWALLNESW)
					block += blockage(tmpTile, O_OBJECT, dType, 7); */
				// etc. on down through non-cardinal dir's; see 'wayboys' source for details (or not)

			}
			else // dt_NE
			{
				block = blockage(
								startTile,
								O_NORTHWALL,
								dType) / 2
						+ blockage(
								endTile,
								O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_NORTHWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_WESTWALL,
								dType) / 2

						+ blockage(
								startTile,
								O_OBJECT,
								dType,
								0,
								true) / 2
						+ blockage(
								startTile,
								O_OBJECT,
								dType,
								2,
								true) / 2

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								O_OBJECT,
								dType,
								2) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								O_OBJECT,
								dType,
								4) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_OBJECT,
								dType,
								6) / 2 // checks Content/bigWalls

						+ blockage(
								endTile,
								O_OBJECT,
								dType,
								4) / 2 // checks Content/bigWalls
						+ blockage(
								endTile,
								O_OBJECT,
								dType,
								6) / 2; // checks Content/bigWalls
			}
		break;
		case 2: // east
			block = blockage(
							endTile,
							O_WESTWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
				block += blockage(
								endTile,
								O_OBJECT,
								dType,
								6)
						+  blockage(
								startTile,
								O_OBJECT,
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
								O_NORTHWALL,
								dType)
						+ blockage(
								endTile,
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_OBJECT,
								dType,
								1); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // right+down
//				block += blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_WESTWALL,
								dType)
						+ blockage(
								endTile,
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_OBJECT,
								dType,
								5); // checks Content/bigWalls
			}
			else // dt_SE
			{
				block = blockage(
								endTile,
								O_WESTWALL,
								dType) / 2
						+ blockage(
								endTile,
								O_NORTHWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_NORTHWALL,
								dType) / 2

						+ blockage(
								startTile,
								O_OBJECT,
								dType,
								2,
								true) / 2 // checks Content/bigWalls
						+ blockage(
								startTile,
								O_OBJECT,
								dType,
								4,
								true) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_OBJECT,
								dType,
								2) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_OBJECT,
								dType,
								4) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								O_OBJECT,
								dType,
								6) / 2 // checks Content/bigWalls

						+ blockage(
								endTile,
								O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								endTile,
								O_OBJECT,
								dType,
								6) / 2; // checks Content/bigWalls
			}
		break;
		case 4: // south
			block = blockage(
							endTile,
							O_NORTHWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
				block += blockage(
								endTile,
								O_OBJECT,
								dType,
								0)
						+ blockage(
								startTile,
								O_OBJECT,
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
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_OBJECT,
								dType,
								7); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // left+down
//				block += blockage(
								startTile,
								O_WESTWALL,
								dType)
						+ blockage(
								endTile,
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								O_OBJECT,
								dType,
								3);
			}
			else // dt_SW
			{
				block = blockage(
								endTile,
								O_NORTHWALL,
								dType) / 2
						+ blockage(
								startTile,
								O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_NORTHWALL,
								dType) / 2

						+ blockage(
								startTile,
								O_OBJECT,
								dType,
								4,
								true) / 2 // checks Content/bigWalls
						+ blockage(
								startTile,
								O_OBJECT,
								dType,
								6,
								true) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								O_OBJECT,
								dType,
								6) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								O_OBJECT,
								dType,
								2) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								O_OBJECT,
								dType,
								4) / 2 // checks Content/bigWalls

						+ blockage(
								endTile,
								O_OBJECT,
								dType,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								endTile,
								O_OBJECT,
								dType,
								2) / 2; // checks Content/bigWalls
			}
		break;
		case 6: // west
			block = blockage(
							startTile,
							O_WESTWALL,
							dType);

			if (visLike == false) // visLike does this after the switch()
			{
				block += blockage(
								startTile,
								O_OBJECT,
								dType,
								6,
								true)
						+ blockage(
								endTile,
								O_OBJECT,
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
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								O_OBJECT,
								dType,
								5);

				if (block == 0) break; // this way is opened

				block = blockage( // left+up
//				block += blockage(
								startTile,
								O_WESTWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								O_NORTHWALL,
								dType)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								O_OBJECT,
								dType,
								1);
			}
			else // dt_NW
			{
				block = blockage(
								startTile,
								O_WESTWALL,
								dType) / 2
						+ blockage(
								startTile,
								O_NORTHWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								O_WESTWALL,
								dType) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								O_NORTHWALL,
								dType) / 2

						+ blockage(
								startTile,
								O_OBJECT,
								dType,
								0,
								true) / 2
						+ blockage(
								startTile,
								O_OBJECT,
								dType,
								6,
								true) / 2

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								O_OBJECT,
								dType,
								4) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								O_OBJECT,
								dType,
								6) / 2

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								O_OBJECT,
								dType,
								0) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								O_OBJECT,
								dType,
								2) / 2

						+ blockage(
								endTile,
								O_OBJECT,
								dType,
								2) / 2
						+ blockage(
								endTile,
								O_OBJECT,
								dType,
								4) / 2;
			}
		break;
	}

	if (visLike == true)
	{
		block += blockage(
						startTile,
						O_OBJECT,
						dType,
						dir, // checks Content/bigWalls
						true,
						true);

		if (block == 0		// if, no visBlock yet ...
			&& blockage(	// so check for content @endTile & reveal it by not cutting trajectory.
					endTile,
					O_OBJECT,
					dType,
					(dir + 4) % 8) // opposite direction
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
		const ItemDamageType dType) const
{
	//Log(LOG_INFO) << "TileEngine::verticalBlockage()";
	if (startTile == NULL // safety check
		|| endTile == NULL
		|| startTile == endTile)
	{
		return 0;
	}

	const int dirZ = endTile->getPosition().z - startTile->getPosition().z;
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
								O_FLOOR,
								dType)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								O_OBJECT,
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
								O_FLOOR,
								dType)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								O_OBJECT,
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
								O_FLOOR,
								dType)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								O_OBJECT,
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
								O_FLOOR,
								dType)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								O_OBJECT,
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
		block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, dType);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, dType, Pathfinding::DIR_DOWN);
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), dType, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, dType);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, dType, Pathfinding::DIR_DOWN);
		}
	}
	else if (direction > 0) // up
	{
		block += blockage(_save->getTile(Position(x, y, z+1)), O_FLOOR, dType);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z+1)), O_OBJECT, dType, Pathfinding::DIR_UP);
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z+1;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), dType, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), O_FLOOR, dType);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), O_OBJECT, dType, Pathfinding::DIR_UP);
		}
	}

	return block;
} */

/**
 * Calculates the amount of power or LoS/FoV/LoF that various types of
 * walls/bigwalls or floors or object parts of a tile blocks.
 * @param startTile		- pointer to tile where the power starts
 * @param part			- the part of the tile that the power tries to go through (MapData.h)
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
		const MapDataType part,
		const ItemDamageType dType,
		const int dir,
		const bool originTest,
		const bool trueDir) const
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
						&& (tile->getMapData(part)->getPartType() == O_OBJECT // this one is for verticalBlockage() only.
							|| tile->getMapData(part)->getPartType() == O_NORTHWALL
							|| tile->getMapData(part)->getPartType() == O_WESTWALL)
					|| tile->getMapData(part)->getPartType() == O_FLOOR)	// all floors that block LoS should have their stopLOS flag set true, if not gravLift floor.
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
			const int bigWall = tile->getMapData(O_OBJECT)->getBigWall(); // 0..9 or, per MCD.
			//Log(LOG_INFO) << ". bigWall = " << bigWall;

			if (originTest == true)	// the ContentOBJECT already got hit as the previous endTile... but can still block LoS when looking down ...
			{
/*				bool diagStop = true; // <- superceded by ProjectileFlyBState::_prjVector ->
				if (dType == DT_HE
					&& _missileDirection != -1)
				{
					const int dirDelta = std::abs(8 + _missileDirection - dir) % 8;
					diagStop = (dirDelta < 2 || dirDelta > 6);
				}
				else
					diagStop = true; */

				// this needs to check which side the *missile* is coming from,
				// although grenades that land on a diagonal bigWall are exempt regardless!!!
				if (bigWall == BIGWALL_NONE // !visLike, if (only Content-part == true) -> all DamageTypes ok here (because, origin).
/*					|| (diagStop == false
						&& (bigWall == BIGWALL_NESW
							|| bigWall == BIGWALL_NWSE)) */
					|| (dir == Pathfinding::DIR_DOWN
						&& tile->getMapData(O_OBJECT)->stopLOS() == false // stopLOS() should join w/ DT_NONE ...
						&& !(
							dType == DT_SMOKE
							&& tile->getMapData(O_OBJECT)->getBlock(DT_SMOKE) == 1)
						&& !(
							dType == DT_IN
							&& tile->getMapData(O_OBJECT)->blockFire() == true)))
				{
					return 0;
				}
				else if (visLike == false // diagonal BigWall blockage ...
					&& (bigWall == BIGWALL_NESW
						|| bigWall == BIGWALL_NWSE)
					&& tile->getMapData(O_OBJECT)->stopLOS() == true // stopLOS() should join w/ DT_NONE ...
					&& _powerE > -1
					&& _powerE < tile->getMapData(O_OBJECT)->getArmor() * 2)
				{
					//Log(LOG_INFO) << ". . . . Ret 1000[2] part = " << part << " " << tile->getPosition();
					return 1000;
				}
			}

			if (visLike == true // hardblock for visLike against non-bigWall content-object.
				&& bigWall == BIGWALL_NONE
				&& (tile->getMapData(O_OBJECT)->stopLOS() == true // stopLOS() should join w/ DT_NONE ...
					|| (dType == DT_SMOKE
						&& tile->getMapData(O_OBJECT)->getBlock(DT_SMOKE) == 1)
					|| (dType == DT_IN
						&& tile->getMapData(O_OBJECT)->blockFire() == true)))
			{
				//Log(LOG_INFO) << ". . . . Ret 1000[3] part = " << part << " " << tile->getPosition();
				return 1000;
			}


			switch (dir) // -> OBJECT part. ( BigWalls & content )
			{
				case 0: // north
					if (bigWall == BIGWALL_WEST
						|| bigWall == BIGWALL_EAST
						|| bigWall == BIGWALL_SOUTH
						|| bigWall == BIGWALL_E_S)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 0 north )";
						return 0; // part By-passed.
					}
				break;

				case 1: // north east
					if (bigWall == BIGWALL_WEST
						|| bigWall == BIGWALL_SOUTH
						|| ( //visLike &&
							bigWall == BIGWALL_NWSE
							&& trueDir == false))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 1 northeast )";
						return 0;
					}
				break;

				case 2: // east
					if (bigWall == BIGWALL_NORTH
						|| bigWall == BIGWALL_SOUTH
						|| bigWall == BIGWALL_WEST
						|| bigWall == BIGWALL_W_N)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 2 east )";
						return 0;
					}
				break;

				case 3: // south east
					if (bigWall == BIGWALL_NORTH
						|| bigWall == BIGWALL_WEST
						|| ( //visLike &&
							bigWall == BIGWALL_NESW
							&& trueDir == false)
						|| bigWall == BIGWALL_W_N)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 3 southeast )";
						return 0;
					}
				break;

				case 4: // south
					if (bigWall == BIGWALL_WEST
						|| bigWall == BIGWALL_EAST
						|| bigWall == BIGWALL_NORTH
						|| bigWall == BIGWALL_W_N)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 4 south )";
						return 0;
					}
				break;

				case 5: // south west
					if (bigWall == BIGWALL_NORTH
						|| bigWall == BIGWALL_EAST
						|| ( //visLike &&
							bigWall == BIGWALL_NWSE
							&& trueDir == false))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 5 southwest )";
						return 0;
					}
				break;

				case 6: // west
					if (bigWall == BIGWALL_NORTH
						|| bigWall == BIGWALL_SOUTH
						|| bigWall == BIGWALL_EAST
						|| bigWall == BIGWALL_E_S)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 6 west )";
						return 0;
					}
				break;

				case 7: // north west
					if (bigWall == BIGWALL_SOUTH
						|| bigWall == BIGWALL_EAST
						|| bigWall == BIGWALL_E_S
						|| ( //visLike &&
							bigWall == BIGWALL_NESW
							&& trueDir == false))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 7 northwest )";
						return 0;
					}
				break;

				case 8: // up
				case 9: // down
					if ((bigWall != BIGWALL_NONE			// lets content-objects Block explosions
							&& bigWall != BIGWALL_BLOCK)	// includes stopLoS (floors handled above under non-directional condition)
						|| (visLike == true
							&& tile->getMapData(O_OBJECT)->stopLOS() == false // stopLOS() should join w/ DT_NONE ...
							&& !(
								dType == DT_SMOKE
								&& tile->getMapData(O_OBJECT)->getBlock(DT_SMOKE) == 1)
							&& !(
								dType == DT_IN
								&& tile->getMapData(O_OBJECT)->blockFire() == true)))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 8,9 up,down )";
						return 0;
					}
				break;

				default:
					return 0; // .....
			}


			// might be Content-part or remaining-bigWalls block here
			if (tile->getMapData(O_OBJECT)->stopLOS() == true // use stopLOS to hinder explosions from propagating through bigWalls freely. // stopLOS() should join w/ DT_NONE ...
				|| (dType == DT_SMOKE
					&& tile->getMapData(O_OBJECT)->getBlock(DT_SMOKE) == 1)
				|| (dType == DT_IN
					&& tile->getMapData(O_OBJECT)->blockFire() == true))
			{
				if (visLike == true
					|| (_powerE > -1
						&& _powerE < tile->getMapData(O_OBJECT)->getArmor() * 2)) // terrain absorbs 200% damage from DT_HE!
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
/* void TileEngine::setProjectileDirection(const int dir)
{
	_missileDirection = dir;
} */

/**
 * Applies the explosive power to tile parts.
 * @note This is where the actual destruction takes place; 9 parts are affected:
 * 2 walls, floors top & bottom, up to 4 bigWalls around the perimeter, plus the
 * content-object in the center.
 * @param tile - pointer to Tile affected
 * @return, true if an objective was destroyed
 */
bool TileEngine::detonate(Tile* const tile) const
{
	int expl = tile->getExplosive(); // <- power that hit the Tile.
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
	static const MapDataType parts[9] =
	{
		O_FLOOR,		// 0 - in tileAbove
		O_WESTWALL,		// 1 - in tileEast
		O_NORTHWALL,	// 2 - in tileSouth
		O_FLOOR,		// 3 - in tile
		O_WESTWALL,		// 4 - in tile
		O_NORTHWALL,	// 5 - in tile
		O_OBJECT,		// 6 - in tile
		O_OBJECT,		// 7 - in tileNorth
		O_OBJECT		// 8 - in tileWest
	};

	int
		explTest,
		vol = 0,
		dieMCD;

	MapDataType
		part,
		partTemp;

	bool
		objectiveDestroyed = false,
		diagWallDestroyed = true;

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
				(bigWall == BIGWALL_BLOCK
					|| bigWall == BIGWALL_E_S
					|| (i == 8 && bigWall == BIGWALL_EAST)
					|| (i == 7 && bigWall == BIGWALL_SOUTH))))
		{
			continue;
		}

		if (diagWallDestroyed == false
			&& parts[i] == O_FLOOR)
		{
			continue; // when ground shouldn't be destroyed
		}

		// kL_begin:
		if (tile->getMapData(O_OBJECT) != NULL										// if tile has object
			&& ((i == 1																// don't hit tileEast westwall
					&& tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_EAST)	// if eastern bigWall not destroyed
				|| (i == 2															// don't hit tileSouth northwall
					&& tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_SOUTH)))	// if southern bigWall not destroyed
		{
			continue;
		} // kL_end.


		explTest = expl;
		part = parts[i];

		if (i == 6
			&& (bigWall == BIGWALL_NESW
				|| bigWall == BIGWALL_NWSE) // diagonals
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
				for (size_t // get a yes/no volume for the object by checking its loftemps objects.
						j = 0;
						j != 12;
						++j)
				{
					if (tiles[i]->getMapData(part)->getLoftId(j) != 0)
						++vol;
				}
			}

			explTest -= tiles[i]->getMapData(part)->getArmor() * 2;

			if (i == 6
				&& (bigWall == BIGWALL_NESW // diagonals for the current tile
					|| bigWall == BIGWALL_NWSE))
			{
				diagWallDestroyed = true;
			}

			if (_battleSave->getTacticalType() == TCT_BASEDEFENSE
				&& tiles[i]->getMapData(part)->isBaseModule() == true)
			{
				_battleSave->getModuleMap()[tile->getPosition().x / 10]
										   [tile->getPosition().y / 10].second--;
			}

			// this follows transformed object parts (object can become a ground - unless your MCDs are correct)
			dieMCD = tiles[i]->getMapData(part)->getDieMCD();
			if (dieMCD != 0)
				partTemp = tiles[i]->getMapData(part)->getDataset()->getObjects()->at(static_cast<size_t>(dieMCD))->getPartType();
			else
				partTemp = part;

			if (tiles[i]->destroy( // DESTROY HERE <-|
								part,
								_battleSave->getObjectiveType()) == true)
			{
				objectiveDestroyed = true;
			}

			part = partTemp;
		}
	}

	expl = (expl + 29) / 30;

	if (tile->ignite((expl + 1) / 2) == false)
		tile->addSmoke((expl + vol + 1) / 2);

	if (tile->getSmoke() != 0) // add smoke to tiles above
	{
		Tile* const tileAbove = _battleSave->getTile(tile->getPosition() + Position(0,0,1));
		if (tileAbove != NULL
			&& tileAbove->hasNoFloor(tile) == true // TODO: use verticalBlockage() instead
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
Tile* TileEngine::checkForTerrainExplosions() const
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
 * Opens a door if any by rightclick or by walking through it.
 * @note The unit has to face in the right direction.
 * @param unit		- pointer to a BattleUnit trying the door
 * @param rtClick	- true if the player right-clicked (default false)
 * @param dir		- direction to check for a door (default -1)
 * @return, -1 there is no door or you're a tank and can't do sweet shit except blast the fuck out of it
 *			 0 normal door opened so make a squeaky sound and walk through
 *			 1 ufo door is starting to open so make a whoosh sound but don't walk through yet
 *			 3 ufo door is still opening so don't walk through it yet (have patience futuristic technology)
 *			 4 not enough TUs
 *			 5 would contravene fire reserve
 */
int TileEngine::unitOpensDoor(
		BattleUnit* const unit,
		const bool rtClick,
		int dir)
{
	//Log(LOG_INFO) << "unitOpensDoor()";
	if (dir == -1)
		dir = unit->getDirection();

	if (rtClick == true // RMB works only for cardinal directions, and not for dogs.
		&& (dir % 2 == 1
			|| (unit->getUnitRules() != NULL
				&& unit->getUnitRules()->hasHands() == false)))
	{
		return Tile::DR_NONE;
	}

	Tile
		* tile,
		* tileDoor = NULL;
	Position
		pos,
		posDoor;
	MapDataType part = O_FLOOR; // avoid vc++ linker warning.
	bool calcTu = false;
	int
		ret = Tile::DR_NONE,
		tuCost = 0,
		z;

	if (unit->getTile()->getTerrainLevel() < -12)
		z = 1; // if standing on stairs check the tile above instead
	else
		z = 0;

	const int armorSize = unit->getArmor()->getSize();
	for (int
			x = 0;
			x != armorSize
				&& ret == Tile::DR_NONE;
			++x)
	{
		for (int
				y = 0;
				y != armorSize
					&& ret == Tile::DR_NONE;
				++y)
		{
			pos = unit->getPosition() + Position(x,y,z);
			tile = _battleSave->getTile(pos);
			//Log(LOG_INFO) << ". iter unitSize " << pos;

			if (tile != NULL)
			{
				std::vector<std::pair<Position, MapDataType> > checkPair;
				switch (dir)
				{
					case 0: // north
							checkPair.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));	// origin
						if (x != 0)
							checkPair.push_back(std::make_pair(Position(0,-1, 0), O_WESTWALL));		// one tile north
					break;

					case 1: // north east
							checkPair.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));	// origin
							checkPair.push_back(std::make_pair(Position(1,-1, 0), O_WESTWALL));		// one tile north-east
					break;

					case 2: // east
							checkPair.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
					break;

					case 3: // south-east
						if (y == 0)
							checkPair.push_back(std::make_pair(Position(1, 1, 0), O_WESTWALL));		// one tile south-east
						if (x == 0)
							checkPair.push_back(std::make_pair(Position(1, 1, 0), O_NORTHWALL));	// one tile south-east
					break;

					case 4: // south
							checkPair.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));	// one tile south
					break;

					case 5: // south-west
							checkPair.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));	// origin
							checkPair.push_back(std::make_pair(Position(-1, 1, 0), O_NORTHWALL));	// one tile south-west
					break;

					case 6: // west
							checkPair.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));	// origin
						if (y != 0)
							checkPair.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL));	// one tile west
					break;

					case 7: // north-west
							checkPair.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));	// origin
							checkPair.push_back(std::make_pair(Position( 0, 0, 0), O_NORTHWALL));	// origin
						if (x != 0)
							checkPair.push_back(std::make_pair(Position(-1,-1, 0), O_WESTWALL));	// one tile north
						if (y != 0)
							checkPair.push_back(std::make_pair(Position(-1,-1, 0), O_NORTHWALL));	// one tile north
				}

				part = O_FLOOR; // just a reset for 'part'.

				for (std::vector<std::pair<Position, MapDataType> >::const_iterator
						i = checkPair.begin();
						i != checkPair.end();
						++i)
				{
					posDoor = pos + i->first;
					tileDoor = _battleSave->getTile(posDoor);
					//Log(LOG_INFO) << ". . iter checkPair " << posDoor << " part = " << i->second;
					if (tileDoor != NULL)
					{
						part = i->second;
						ret = tileDoor->openDoor(
												part,
												unit,
												_battleSave->getBattleGame()->getReservedAction());
						//Log(LOG_INFO) << ". . . openDoor = " << ret;

						if (ret != Tile::DR_NONE)
						{
							if (ret == Tile::DR_OPEN_WOOD && rtClick == true)
							{
								if (part == O_WESTWALL)
									part = O_NORTHWALL;
								else
									part = O_WESTWALL;

								calcTu = true;
							}
							else if (ret == Tile::DR_OPEN_METAL)
							{
								openAdjacentDoors(
												posDoor,
												part);

								calcTu = true;
							}
							else if (ret == Tile::DR_ERR_TU)
								calcTu = true;

							break;
						}
					}
				}
			}
		}
	}

	//Log(LOG_INFO) << "tuCost = " << tuCost;
	if (calcTu == true)
	{
		tuCost = tileDoor->getTuCostTile(
										part,
										unit->getMoveTypeUnit());
		//Log(LOG_INFO) << ". . ret = " << ret << ", part = " << part << ", tuCost = " << tuCost;

		if (_battleSave->getBattleGame()->checkReservedTu(
														unit,
														tuCost) == true)
		{
			//Log(LOG_INFO) << "check reserved tu";
			if (unit->spendTimeUnits(tuCost) == true)
			{
				//Log(LOG_INFO) << "spend tu";
				if (rtClick == true) // try this one ...... <-- let UnitWalkBState handle FoV & new unit visibility when walking (ie, not for RMB here).
				{
					//Log(LOG_INFO) << "RMB -> calcFoV";
					_battleSave->getBattleGame()->checkProxyGrenades(unit);

					calculateFOV( // calculate FoV for everyone within sight-range, incl. unit.
							unit->getPosition(),
							true);

					// look from the other side, may need to check reaction fire
					// This seems redundant but hey maybe it removes now-unseen units from a unit's visible-units vector ....
					const std::vector<BattleUnit*>* const hostileUnits = unit->getHostileUnits();
					for (size_t
							i = 0;
							i != hostileUnits->size();
							++i)
					{
						//Log(LOG_INFO) << "calcFoV hostile";
						calculateFOV(hostileUnits->at(i)); // calculate FoV for all hostile units that are visible to this unit.
					}
				}
			}
			else // not enough TU
			{
				//Log(LOG_INFO) << "unitOpensDoor() ret DR_ERR_TU";
				return Tile::DR_ERR_TU;
			}
		}
		else // reserved TU
		{
			//Log(LOG_INFO) << "unitOpensDoor() ret DR_ERR_RESERVE";
			return Tile::DR_ERR_RESERVE;
		}
	}

	//Log(LOG_INFO) << "unitOpensDoor() ret = " << ret;
	return ret;
}
/*				switch (dir)
				{
					case 0: // north
							checkPos.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));		// origin
						if (x != 0)
							checkPos.push_back(std::make_pair(Position(0,-1, 0), O_WESTWALL));		// one tile north
					break;

					case 1: // north east
							checkPos.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));		// origin
							checkPos.push_back(std::make_pair(Position(1,-1, 0), O_WESTWALL));		// one tile north-east
//						if (rtClick == true)
//						{
//							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
//							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_NORTHWALL));		// one tile east
//						}
/*						if (rtClick
							|| testAdjacentDoor(posUnit, O_NORTHWALL, 1)) // kL
						{
							checkPos.push_back(std::make_pair(Position(0, 0, 0), O_NORTHWALL));		// origin
							checkPos.push_back(std::make_pair(Position(1,-1, 0), O_WESTWALL));		// one tile north-east
							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_NORTHWALL));		// one tile east
						} *
					break;

					case 2: // east
							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
					break;

					case 3: // south-east
						if (y == 0)
							checkPos.push_back(std::make_pair(Position(1, 1, 0), O_WESTWALL));		// one tile south-east
						if (x == 0)
							checkPos.push_back(std::make_pair(Position(1, 1, 0), O_NORTHWALL));		// one tile south-east
//						if (rtClick == true)
//						{
//							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
//							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));		// one tile south
//						}
/*						if (rtClick
							|| testAdjacentDoor(posUnit, O_NORTHWALL, 3)) // kL
						{
							checkPos.push_back(std::make_pair(Position(1, 0, 0), O_WESTWALL));		// one tile east
							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));		// one tile south
							checkPos.push_back(std::make_pair(Position(1, 1, 0), O_WESTWALL));		// one tile south-east
							checkPos.push_back(std::make_pair(Position(1, 1, 0), O_NORTHWALL));		// one tile south-east
						} *
					break;

					case 4: // south
							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));		// one tile south
					break;

					case 5: // south-west
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
							checkPos.push_back(std::make_pair(Position(-1, 1, 0), O_NORTHWALL));	// one tile south-west
//						if (rtClick == true)
//						{
//							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_WESTWALL));		// one tile south
//							checkPos.push_back(std::make_pair(Position(0, 1, 0), O_NORTHWALL));		// one tile south
//						}
/*						if (rtClick
							|| testAdjacentDoor(posUnit, O_NORTHWALL, 5)) // kL
						{
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
							checkPos.push_back(std::make_pair(Position( 0, 1, 0), O_WESTWALL));		// one tile south
							checkPos.push_back(std::make_pair(Position( 0, 1, 0), O_NORTHWALL));	// one tile south
							checkPos.push_back(std::make_pair(Position(-1, 1, 0), O_NORTHWALL));	// one tile south-west
						} *
					break;

					case 6: // west
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
						if (y != 0)
							checkPos.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL));	// one tile west
					break;

					case 7: // north-west
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_NORTHWALL));	// origin
						if (x != 0)
							checkPos.push_back(std::make_pair(Position(-1,-1, 0), O_WESTWALL));		// one tile north
						if (y != 0)
							checkPos.push_back(std::make_pair(Position(-1,-1, 0), O_NORTHWALL));	// one tile north
//						if (rtClick == true)
//						{
//							checkPos.push_back(std::make_pair(Position( 0,-1, 0), O_WESTWALL));		// one tile north
//							checkPos.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL));	// one tile west
//						}
/*						if (rtClick
							|| testAdjacentDoor(posUnit, O_NORTHWALL, 7)) // kL
						{
							//Log(LOG_INFO) << ". north-west";
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_WESTWALL));		// origin
							checkPos.push_back(std::make_pair(Position( 0, 0, 0), O_NORTHWALL));	// origin
							checkPos.push_back(std::make_pair(Position( 0,-1, 0), O_WESTWALL));		// one tile north
							checkPos.push_back(std::make_pair(Position(-1, 0, 0), O_NORTHWALL));	// one tile west
						} *
				} */

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
		&& tile->getMapData(part)->isUfoDoor() == true)
	{
		return true;
	}

	return false;
} */

/**
 * Opens any doors connected to this wall at this position,
 * @note Keeps processing till it hits a non-ufo-door.
 * @param pos	- reference the starting position
 * @param part	- the wall to open (defines which direction to check)
 */
void TileEngine::openAdjacentDoors( // private.
		const Position& pos,
		MapDataType part) const
{
	Position offset;
	const bool westSide = (part == O_WESTWALL);

	for (int
			i = 1;
			;
			++i)
	{
		offset = westSide ? Position(0,i,0) : Position(i,0,0);
		Tile* const tile = _battleSave->getTile(pos + offset);
		if (tile != NULL
			&& tile->getMapData(part) != NULL
			&& tile->getMapData(part)->isUfoDoor() == true)
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
		offset = westSide ? Position(0,i,0) : Position(i,0,0);
		Tile* const tile = _battleSave->getTile(pos + offset);
		if (tile != NULL
			&& tile->getMapData(part) != NULL
			&& tile->getMapData(part)->isUfoDoor() == true)
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
int TileEngine::closeUfoDoors() const
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
			const BattleUnit* const unit = _battleSave->getTiles()[i]->getUnit();

			const Tile
				* const tile = _battleSave->getTiles()[i],
				* const tileNorth = _battleSave->getTile(tile->getPosition() + Position( 0,-1, 0)),
				* const tileWest  = _battleSave->getTile(tile->getPosition() + Position(-1, 0, 0));
			if ((tile->isUfoDoorOpen(O_NORTHWALL) == true
					&& tileNorth != NULL
					&& tileNorth->getUnit() != NULL // probly not needed.
					&& tileNorth->getUnit() == unit)
				|| (tile->isUfoDoorOpen(O_WESTWALL) == true
					&& tileWest != NULL
					&& tileWest->getUnit() != NULL // probly not needed.
					&& tileWest->getUnit() == unit))
			{
				continue;
			}
		}

		closed += _battleSave->getTiles()[i]->closeUfoDoor();
	}

	return closed;
}

/**
 * Calculates a line trajectory using bresenham algorithm in 3D.
 * @note Accuracy is NOT considered; this is a true path/trajectory.
 * @param origin		- reference the origin (voxelspace for 'doVoxelCheck'; tilespace otherwise)
 * @param target		- reference the target (voxelspace for 'doVoxelCheck'; tilespace otherwise)
 * @param storeTrj		- true will store the whole trajectory; otherwise only the last position gets stored
 * @param trj			- pointer to a vector of Positions in which the trajectory will be stored
 * @param excludeUnit	- pointer to a BattleUnit to be excluded from collision detection
 * @param doVoxelCheck	- true to check against a voxel; false to check tile blocking for FoV (true for unit visibility and line of fire, false for terrain visibility) (default true)
 * @param onlyVisible	- true to skip invisible units (default false) [used in FPS view]
 * @param excludeAllBut	- pointer to a unit that's to be considered exclusively for ray hits [optional] (default NULL)
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
VoxelType TileEngine::plotLine(
		const Position& origin,
		const Position& target,
		const bool storeTrj,
		std::vector<Position>* const trj,
		const BattleUnit* const excludeUnit,
		const bool doVoxelCheck, // false is used only for calculateFOV()
		const bool onlyVisible,
		const BattleUnit* const excludeAllBut) const
{
	VoxelType ret;
	bool
		swap_xy,
		swap_xz;
	int
		x,x0,x1, delta_x,step_x,
		y,y0,y1, delta_y,step_y,
		z,z0,z1, delta_z,step_z,

		drift_xy,drift_xz,

		cx,cy,cz,

		horiBlock,vertBlock;

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
/*		if (x < 0 || y < 0 || z < 0
			|| x > _battleSave->getMapSizeX() * 16
			|| y > _battleSave->getMapSizeY() * 16
			|| z > _battleSave->getMapSizeZ() * 24)
		{
			break;
		} */

		cx = x; cy = y; cz = z; // copy position
		if (swap_xz == true) std::swap(cx,cz); // unswap (in reverse)
		if (swap_xy == true) std::swap(cx,cy);

		if (storeTrj == true
			&& trj != NULL)
		{
			trj->push_back(Position(cx,cy,cz));
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
				if (trj != NULL) // store the position of impact
					trj->push_back(Position(cx,cy,cz));

				return ret;
			}
		}
		else // for Terrain visibility, ie. FoV / Fog of War.
		{
			Tile
				* const startTile = _battleSave->getTile(lastPoint),
				* const endTile = _battleSave->getTile(Position(cx,cy,cz));

/*			if (_battleSave->getSelectedUnit()->getId() == 389)
			{
				int dist = distance(origin, Position(cx, cy, cz));
				Log(LOG_INFO) << "unitID = " << _battleSave->getSelectedUnit()->getId() << " dist = " << dist;
			} */

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
				&& selUnit->getId() == 389
				&& startTile != endTile)
			{
//				Position posUnit = selUnit->getPosition();
//				if ((posUnit.x == cx
//						&& std::abs(posUnit.y - cy) > 4) ||
//					(posUnit.y == cy
//						&& std::abs(posUnit.x - cx) > 4))
				{
					Log(LOG_INFO) << ". start " << lastPoint << " hori = " << horiBlock;
					Log(LOG_INFO) << ". . end " << Position(cx,cy,cz) << " vert = " << vertBlock;
				}
			} */ // kL_TEST_end.

			// TODO: The returns here should be mapped to something more
			// meaningful for passing back to calculateFOV(), which is the only
			// function that uses this quirky bit.
			if (horiBlock < 0) // hit content-object
			{
				if (vertBlock < 1)
					return VOXEL_EMPTY; // -1
//					return horiBlock;

				horiBlock = 0;
			}

			horiBlock += vertBlock;
			if (horiBlock != 0) // horiBlock > 0)
				return VOXEL_WESTWALL; // 1 <- this might need to be +1 OR -1 .....
//				return horiBlock;

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
					if (trj != NULL)
						trj->push_back(Position(cx,cy,cz)); // store the position of impact

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
					if (trj != NULL) // store the position of impact
						trj->push_back(Position(cx,cy,cz));

					return ret;
				}
			}
		}
	}

	return VOXEL_EMPTY;
}

/**
 * Calculates a parabolic trajectory for thrown items.
 * @note Accuracy is NOT considered; this is a true path/trajectory.
 * @param origin		- reference the origin in voxelspace
 * @param target		- reference the target in voxelspace
 * @param storeTrj		- true will store the whole trajectory - otherwise it stores the last position only
 * @param trj			- pointer to a vector of Positions in which the trajectory will be stored
 * @param excludeUnit	- pointer to a unit to exclude - makes sure the trajectory does not hit the shooter itself
 * @param arc			- how high the parabola goes: 1.0 is almost straight throw, 3.0 is a very high throw, to throw over a fence for example
 * @param acu			- the deviation of the angles that should be taken into account. 1.0 is perfection. // now superceded by @param delta...
 * @param delta			- reference the deviation of the angles that should be taken into account, (0,0,0) is perfection (default Position(0,0,0))
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
VoxelType TileEngine::plotParabola(
		const Position& origin,
		const Position& target,
		const bool storeTrj,
		std::vector<Position>* const trj,
		const BattleUnit* const excludeUnit,
		const double arc,
		const Position& delta) const
{
	//Log(LOG_INFO) << "TileEngine::plotParabola()";
	const double ro = std::sqrt(static_cast<double>(
					 (target.x - origin.x) * (target.x - origin.x)
				   + (target.y - origin.y) * (target.y - origin.y)
				   + (target.z - origin.z) * (target.z - origin.z)));
	double
		fi = std::acos(static_cast<double>(target.z - origin.z) / ro),
		te = std::atan2(
				static_cast<double>(target.y - origin.y),
				static_cast<double>(target.x - origin.x));

	if (AreSame(ro, 0.)) // jic.
		return VOXEL_EMPTY;

	te += delta.x * M_PI / (ro * 2.);						// horizontal
	fi += (delta.z + delta.y) * M_PI * arc / (ro * 14.);	// vertical

	const double
		zA = std::sqrt(ro) * arc,
		zK = (4. * zA) / (ro * ro);

	int
		x = origin.x,
		y = origin.y,
		z = origin.z,
		i = 8;

	Position
		pos1 = Position(x,y,z),
		pos2;

	while (z > 0)
	{
		x = static_cast<int>(static_cast<double>(origin.x) + static_cast<double>(i) * std::cos(te) * std::sin(fi));
		y = static_cast<int>(static_cast<double>(origin.y) + static_cast<double>(i) * std::sin(te) * std::sin(fi));
		z = static_cast<int>(static_cast<double>(origin.z) + static_cast<double>(i) * std::cos(fi)
				- zK * (static_cast<double>(i) - ro / 2.) * (static_cast<double>(i) - ro / 2.)
				+ zA);

		if (storeTrj == true
			&& trj != NULL)
		{
			trj->push_back(Position(x,y,z));
		}

		pos2 = Position(x,y,z);
		VoxelType ret = plotLine(
								pos1,
								pos2,
								false,
								NULL,
								excludeUnit);

		if (ret != VOXEL_EMPTY)
		{
			if (pos1.z < pos2.z)
				ret = VOXEL_OUTOFBOUNDS;

			if (storeTrj == false
				&& trj != NULL)
			{
				trj->push_back(pos2);
			}

			//Log(LOG_INFO) << "TileEngine::plotParabola() EXIT w/ ret = " << ret;
			return ret;
		}

		pos1 = Position(x,y,z);
		++i;
	}

	if (storeTrj == false
		&& trj != NULL)
	{
		trj->push_back(Position(x,y,z));
	}

	//Log(LOG_INFO) << "TileEngine::plotParabola() EXIT w/ Empty";
	return VOXEL_EMPTY;
}

/**
 * Checks if a throw action is permissible.
 * @note Accuracy is NOT considered; this checks for a true path/trajectory.
 * @sa Projectile::calculateThrow()
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
		VoxelType* const voxelType) const
{
	//Log(LOG_INFO) << "\nTileEngine::validateThrow()";
	const Tile* const tile = _battleSave->getTile(action.target);
	if (ProjectileFlyBState::validThrowRange(
										&action,
										originVoxel,
										tile) == false)
	{
		//Log(LOG_INFO) << ". vT() ret FALSE, ThrowRange not valid";
		return false;
	}

	if (action.type == BA_THROW
		&& tile != NULL
		&& tile->getMapData(O_OBJECT) != NULL
		&& (tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NESW
			|| tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NWSE))
//		&& (action.weapon->getRules()->getBattleType() == BT_GRENADE
//			|| action.weapon->getRules()->getBattleType() == BT_PROXYGRENADE)
//		&& tile->getMapData(O_OBJECT)->getTuCostPart(MT_WALK) == 255)
	{
		//Log(LOG_INFO) << ". vT() ret FALSE, no diag BigWalls";
		return false; // prevent Grenades from landing on diagonal BigWalls.
	}


	const Position posTarget = targetVoxel / Position(16,16,24);
	double parabolicCoefficient; // higher parabolicCoefficient means higher arc IG. eh ......

	if (posTarget != originVoxel / Position(16,16,24))
	{
		parabolicCoefficient = 0.81;

		if (action.actor->isKneeled() == true)
			parabolicCoefficient += 0.3; // increase the arc reasons.
	}
	else
		parabolicCoefficient = 0.;
	//Log(LOG_INFO) << ". starting arc = " << parabolicCoefficient;

	// check for test up from the lowest trajectory
	bool found = false;
	while (found == false // try several different curvatures
		&& parabolicCoefficient < 6.1)
	{
		//Log(LOG_INFO) << ". . arc = " << parabolicCoefficient;
		std::vector<Position> trj;
		const VoxelType test = plotParabola(
										originVoxel,
										targetVoxel,
										false,
										&trj,
										action.actor,
										parabolicCoefficient);
		//Log(LOG_INFO) << ". . plotParabola() = " << test;

		if (test != VOXEL_OUTOFBOUNDS
			&& test != VOXEL_WESTWALL
			&& test != VOXEL_NORTHWALL
			&& (trj.at(0) / Position(16,16,24)) == posTarget)
		{
			//Log(LOG_INFO) << ". . . found TRUE";
			found = true;

			if (voxelType != NULL)
				*voxelType = test;
		}
		else
			parabolicCoefficient += 0.3;
	}

	if (parabolicCoefficient >= 6.1)
	{
		//Log(LOG_INFO) << ". vT() ret FALSE, arc > 6.1";
		return false;
	}


	// now test continues rising to find upper limit
	double parabolicCoefficient2 = parabolicCoefficient;

	found = false;
	while (found == false // try several different curvatures
		&& parabolicCoefficient2 < 6.1)
	{
		//Log(LOG_INFO) << ". . arc = " << parabolicCoefficient2;
		std::vector<Position> trj;
		const VoxelType test = plotParabola(
										originVoxel,
										targetVoxel,
										false,
										&trj,
										action.actor,
										parabolicCoefficient2);
		//Log(LOG_INFO) << ". . plotParabola() = " << test;

		if (test == VOXEL_OUTOFBOUNDS
			|| test == VOXEL_WESTWALL
			|| test == VOXEL_NORTHWALL
			|| (trj.at(0) / Position(16,16,24)) != posTarget)
		{
			//Log(LOG_INFO) << ". . . found TRUE";
			found = true;
			parabolicCoefficient2 -= 0.3; // back off from the upper lip.
		}
		else
			parabolicCoefficient2 += 0.3;
	}

	// use the average of upper & lower limits:
	// Lessens chance of bouncing a thrown item back off a wall by barely
	// skimming overtop once accuracy is applied.
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
			z != 0;
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
 * Checks for a target in voxel space.
 * @param posTarget			- reference the Position to check in voxelspace
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
VoxelType TileEngine::voxelCheck(
		const Position& posTarget,
		const BattleUnit* const excludeUnit,
		const bool excludeAllUnits,
		const bool onlyVisible,
		const BattleUnit* const excludeAllBut) const
{
	//Log(LOG_INFO) << "TileEngine::voxelCheck()"; // massive lag-to-file, Do not use.
	const Tile
		* tile = _battleSave->getTile(posTarget / Position(16,16,24)),
		* tileBelow;
	//Log(LOG_INFO) << ". tile " << tile->getPosition();
	// check if we are out of the map <- we. It's a voxel-check, not a 'we'.
	if (tile == NULL)
//		|| posTarget.x < 0
//		|| posTarget.y < 0
//		|| posTarget.z < 0)
	{
		//Log(LOG_INFO) << ". vC() ret VOXEL_OUTOFBOUNDS";
		return VOXEL_OUTOFBOUNDS;
	}

	if (tile->isVoid(false, false) == true
		&& tile->getUnit() == NULL) // TODO: tie this into the boolean-input parameters
	{
		tileBelow = _battleSave->getTile(tile->getPosition() + Position(0,0,-1));
		if (tileBelow == NULL
			|| tileBelow->getUnit() == NULL)
		{
			//Log(LOG_INFO) << ". vC() ret VOXEL_EMPTY";
			return VOXEL_EMPTY;
		}
	}

	// kL_note: should allow items to be thrown through a gravLift down to the floor below
	if (posTarget.z % 24 < 2
		&& tile->getMapData(O_FLOOR) != NULL
		&& tile->getMapData(O_FLOOR)->isGravLift() == true)
	{
		//Log(LOG_INFO) << "voxelCheck() isGravLift";
		//Log(LOG_INFO) << ". level = " << tile->getPosition().z;
		if (tile->getPosition().z == 0)
			return VOXEL_FLOOR;

		tileBelow = _battleSave->getTile(tile->getPosition() + Position(0,0,-1));
		if (tileBelow != NULL
			&& tileBelow->getMapData(O_FLOOR) != NULL
			&& tileBelow->getMapData(O_FLOOR)->isGravLift() == false)
		{
			//Log(LOG_INFO) << ". vC() ret VOXEL_FLOOR";
			return VOXEL_FLOOR;
		}
	}

	// first check TERRAIN tile/voxel data
	MapDataType partType;
	const MapData* partData;
	size_t
		layer = (static_cast<size_t>(posTarget.z) % 24) / 2,
		loftId,
		x = 15 - static_cast<size_t>(posTarget.x) % 16;		// x-axis is reversed for tileParts, standard for battleUnit.
	const size_t y = static_cast<size_t>(posTarget.y) % 16;	// y-axis is standard

	int parts = static_cast<int>(Tile::PARTS_TILE); // terrain parts [0=floor, 1/2=walls, 3=content-object]
	for (int
			part = 0;
			part != parts;
			++part)
	{
		partType = static_cast<MapDataType>(part);
		if (tile->isUfoDoorOpen(partType) == false)
		{
			partData = tile->getMapData(partType);
			if (partData != NULL)
			{
				loftId = partData->getLoftId(layer) * 16 + y;
				if (loftId < _voxelData->size() // davide, http://openxcom.org/forum/index.php?topic=2934.msg32146#msg32146 (x2 _below)
					&& _voxelData->at(loftId) & (1 << x)) // if the voxelData at loftId is "1" solid:
				{
					//Log(LOG_INFO) << ". vC() ret = " << part;
					return static_cast<VoxelType>(partType); // Note MapDataType & VoxelType correspond.
				}
			}
		}
	}

	if (excludeAllUnits == false)
	{
		const BattleUnit* targetUnit = tile->getUnit();
		if (targetUnit == NULL
			&& tile->hasNoFloor(NULL) == true)
		{
			tileBelow = _battleSave->getTile(tile->getPosition() + Position(0,0,-1));
			if (tileBelow != NULL)
				targetUnit = tileBelow->getUnit();
		}

		if (targetUnit != NULL
			&& targetUnit != excludeUnit
			&& (excludeAllBut == NULL
				|| targetUnit == excludeAllBut)
			&& (onlyVisible == false
				|| targetUnit->getUnitVisible() == true))
		{
			const Position posUnit = targetUnit->getPosition();
			const int target_z = posUnit.z * 24
							   + targetUnit->getFloatHeight()
							   - tile->getTerrainLevel(); // foot-level voxel

			if (posTarget.z > target_z
				&& posTarget.z <= target_z + targetUnit->getHeight()) // if hit is between foot- and hair-level voxel layers (z-axis)
			{
				if (targetUnit->getArmor()->getSize() > 1) // for large units...
				{
					const Position posTile = tile->getPosition();
					layer = static_cast<size_t>(posTile.x - posUnit.x) + ((posTile.y - posUnit.y) * 2);
					//Log(LOG_INFO) << ". vC, large unit, LoFT entry = " << layer;
				}
				else
					layer = 0;

//				if (layer > -1)
//				{
				x = posTarget.x % 16;
				// That should be (8,8,10) as per BattlescapeGame::handleNonTargetAction() if (_currentAction.type == BA_HIT)

				loftId = targetUnit->getLoft(layer) * 16 + y;
				//Log(LOG_INFO) << "loftId = " << loftId << " vD-size = " << (int)_voxelData->size();
				if (loftId < _voxelData->size() // davide, http://openxcom.org/forum/index.php?topic=2934.msg32146#msg32146 (x2 ^above)
					&& _voxelData->at(loftId) & (1 << x)) // if the voxelData at loftId is "1" solid:
				{
					//Log(LOG_INFO) << ". vC() ret VOXEL_UNIT";
					return VOXEL_UNIT;
				}
//				}
//				else Log(LOG_INFO) << "ERROR TileEngine::voxelCheck() LoFT entry = " << layer;
			}
		}
	}

	//Log(LOG_INFO) << ". vC() ret VOXEL_EMPTY"; // massive lag-to-file, Do not use.
	return VOXEL_EMPTY;
}

/**
 * Calculates the distance between 2 points rounded to nearest integer.
 * @param pos1 - reference the first Position
 * @param pos2 - reference the second Position
 * @param considerZ	- true to consider the z coordinate (default true)
 * @return, distance
 */
int TileEngine::distance(
		const Position& pos1,
		const Position& pos2,
		const bool considerZ) const
{
	const int
		x = pos1.x - pos2.x,
		y = pos1.y - pos2.y;

	int z;
	if (considerZ == true)
		z = pos1.z - pos2.z;
	else
		z = 0;

	return static_cast<int>(Round(
		   std::sqrt(static_cast<double>(x * x + y * y + z * z))));
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
		const bool considerZ) const
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
 * Calculates the distance between 2 points as a floating-point value.
 * @param pos1 - reference the first Position
 * @param pos2 - reference the second Position
 * @return, distance
 */
/* double TileEngine::distancePrecise(
		const Position& pos1,
		const Position& pos2) const
{
	const int
		x = pos1.x - pos2.x,
		y = pos1.y - pos2.y,
		z = pos1.z - pos2.z;

	return std::sqrt(static_cast<double>(x * x + y * y + z * z));
} */

/**
 * Performs a psionic BattleAction.
 * @param action - pointer to a BattleAction (BattlescapeGame.h)
 */
bool TileEngine::psiAttack(BattleAction* const action)
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
		if (action->type == BA_PSICOURAGE)
		{
			const int moraleGain = 10 + RNG::generate(0,20);
			action->value = moraleGain;
			victim->moraleChange(moraleGain);

			return true;
		}

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

		int
			psiStrength,
			psiSkill = 0;

		if (victim->getFaction() == FACTION_HOSTILE
			&& victim->getOriginalFaction() != FACTION_HOSTILE)
		{
			victim->hostileMcParameters(
									psiStrength,
									psiSkill);
		}
		else
		{
			psiStrength = statsVictim->psiStrength;
			psiSkill = statsVictim->psiSkill;
		}

		const double
			defense = static_cast<double>(psiStrength) + (static_cast<double>(psiSkill) / 5.),
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

		if (action->type == BA_PSICONTROL)
			attack += 15.;
		else if (action->type == BA_PSIPANIC)
			attack += 45.;
		else // Psi-Confuse
			attack += 55.;

		attack *= 100.;
		attack /= 56.;

		const int success = static_cast<int>(attack);
		action->value = success;

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
			if (action->type == BA_PSIPANIC)
			{
				//Log(LOG_INFO) << ". . . action->type == BA_PSIPANIC";
				int moraleLoss = 100;
				if (action->actor->getOriginalFaction() == FACTION_PLAYER)
					moraleLoss += statsActor->psiStrength * 2 / 3;
				else
					moraleLoss += statsActor->psiStrength / 2; // reduce aLiens' panic-effect

				moraleLoss -= statsVictim->bravery * 3 / 2;

				//Log(LOG_INFO) << ". . . moraleLoss reduction = " << moraleLoss;
				if (moraleLoss > 0)
					victim->moraleChange(-moraleLoss);

				//Log(LOG_INFO) << ". . . victim morale[1] = " << victim->getMorale();
			}
			else if (action->type == BA_PSICONFUSE)
			{
				//Log(LOG_INFO) << ". . . action->type == BA_PSICONFUSE";
				const int tuLoss = (statsActor->psiSkill + 2) / 3;
				//Log(LOG_INFO) << ". . . tuLoss = " << tuLoss;
				if (tuLoss > 0)
					victim->setTimeUnits(victim->getTimeUnits() - tuLoss);
			}
			else // BA_PSICONTROL
			{
				//Log(LOG_INFO) << ". . . action->type == BA_PSICONTROL";
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
					int
						strength = statsActor->psiStrength,
						skill = statsActor->psiSkill;
					victim->hostileMcParameters(
											strength,
											skill);

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
					{
						courage /= 2;			// xCom Morale gain for getting Mc'd back to xCom.
						victim->setExposed(-1);	// remove Exposure.
					}
				}
				victim->moraleChange(courage);
				//Log(LOG_INFO) << ". . . victim morale[2] = " << victim->getMorale();

				victim->setFaction(action->actor->getFaction());
				victim->initTu();
				victim->allowReselect();
				victim->setStatus(STATUS_STANDING);

				calculateUnitLighting();
				calculateFOV(
						victim->getPosition(),
						true);

/*				// if all units from either faction are mind controlled - auto-end the mission.
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
				} */
				//Log(LOG_INFO) << ". . . tallyUnits DONE";
			}
			//Log(LOG_INFO) << "TileEngine::psiAttack() ret TRUE";
			return true;
		}
		else // psi Fail.
		{
			std::string info;
			if (action->type == BA_PSIPANIC)
				info = "STR_PANIC_";
			else if (action->type == BA_PSICONFUSE)
				info = "STR_CONFUSE_";
			else
				info = "STR_CONTROL_";

			//Log(LOG_INFO) << "te:psiAttack() success = " << success;
			_battleSave->getBattleState()->warning(
												info,
												true,
												success);

			if (victim->getOriginalFaction() == FACTION_PLAYER
				&& Options::allowPsiStrengthImprovement == true)
			{
				int resistXp;
				if (action->actor->getFaction() == FACTION_HOSTILE)
					resistXp = 2; // xCom resisted an aLien
				else
					resistXp = 1; // xCom resisted an xCom attempt

				victim->addPsiStrengthExp(resistXp);
			}
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
Tile* TileEngine::applyGravity(Tile* const tile) const
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
 * Validates the melee range between two BattleUnits.
 * @param actor			- pointer to acting unit
 * @param dir			- direction of action (default -1)
 * @param targetUnit	- pointer to targetUnit (default NULL)
 * @return, true if within one tile
 */
bool TileEngine::validMeleeRange(
		const BattleUnit* const actor,
		int dir,
		const BattleUnit* const targetUnit) const
{
	if (dir == -1)
		dir = actor->getDirection();

	return validMeleeRange(
						actor->getPosition(),
						dir,
						actor,
						targetUnit);
}

/**
 * Validates the melee range between a Position and a BattleUnit.
 * @param pos			- reference the position of action
 * @param dir			- direction to check
 * @param actor			- pointer to acting unit
 * @param targetUnit	- pointer to targetUnit (default NULL)
 * @return, true if within one tile
 */
bool TileEngine::validMeleeRange(
		const Position& pos,
		const int dir,
		const BattleUnit* const actor,
		const BattleUnit* const targetUnit) const
{
	if (dir < 0 || dir > 7)
		return false;

	const Tile
		* tileOrigin,
		* tileTarget;
	Position
		posOrigin,
		posTarget,
		posVector,
		posOrigin_vox,
		posTarget_vox; // not used.

	Pathfinding::directionToVector(
								dir,
								&posVector);

	const int actorSize = actor->getArmor()->getSize();
	for (int
			x = 0;
			x != actorSize;
			++x)
	{
		for (int
				y = 0;
				y != actorSize;
				++y)
		{
			posOrigin = pos + Position(x,y,0);
			posTarget = posOrigin + posVector;

			tileOrigin = _battleSave->getTile(posOrigin);
			tileTarget = _battleSave->getTile(posTarget);

			if (tileOrigin != NULL && tileTarget != NULL)
			{
				if (tileTarget->getUnit() == NULL)
					tileTarget = getVerticalTile(
											posOrigin,
											posTarget);

				if (tileTarget != NULL
					&& tileTarget->getUnit() != NULL
					&& (targetUnit == NULL
						|| targetUnit == tileTarget->getUnit()))
				{
					posOrigin_vox = Position(posOrigin * Position(16,16,24))
								  + Position(
											8,8,
											actor->getHeight(true)
												- tileOrigin->getTerrainLevel()
												- 4);
					if (canTargetUnit(
								&posOrigin_vox,
								tileTarget,
								&posTarget_vox,
								actor) == true)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

/**
 * Gets an adjacent Position that can be attacked with melee.
 * @param actor - pointer to a BattleUnit
 * @return, position in direction that unit faces
 */
Position TileEngine::getMeleePosition(const BattleUnit* const actor) const
{
	const Tile* tileOrigin;
	Tile* tileTarget;
	const Position pos = actor->getPosition();
	Position
		posOrigin,
		posTarget,
		posVector,
		posOrigin_vox,
		posTarget_vox; // not used.

	const int
		armorSize = actor->getArmor()->getSize(),
		dir = actor->getDirection();

	Pathfinding::directionToVector(
								dir,
								&posVector);

	for (int
			x = 0;
			x != armorSize;
			++x)
	{
		for (int
				y = 0;
				y != armorSize;
				++y)
		{
			posOrigin = pos + Position(x,y,0);
			posTarget = posOrigin + posVector;

			tileOrigin = _battleSave->getTile(posOrigin);
			tileTarget = _battleSave->getTile(posTarget);

			if (tileOrigin != NULL && tileTarget != NULL)
			{
				if (tileTarget->getUnit() == NULL
					|| tileTarget->getUnit() == actor)
				{
					tileTarget = getVerticalTile(
											posOrigin,
											posTarget);
				}

				if (tileTarget != NULL
					&& tileTarget->getUnit() != NULL
					&& tileTarget->getUnit() != actor)
				{
					posOrigin_vox = Position(posOrigin * Position(16,16,24))
								  + Position(
											8,8,
											actor->getHeight(true)
												- tileOrigin->getTerrainLevel()
												- 4);
					if (canTargetUnit(
									&posOrigin_vox,
									tileTarget,
									&posTarget_vox,
									actor) == true)
					{
						return posTarget;	// TODO: conform this to the fact Reapers can melee vs. 2 tiles
					}						// or three tiles if their direction is diagonal.
				}
			}
		}
	}

	return Position(-1,-1,-1); // this should simply never happen because the call is made after validMeleeRange()
}

/**
 * Gets an adjacent tile with an unconscious unit if any.
 * @param actor - pointer to a BattleUnit
 * @return, pointer to a Tile
 */
Tile* TileEngine::getExecutionTile(const BattleUnit* const actor) const
{
	const Tile* tileOrigin;
	Tile* tileTarget;
	const Position pos = actor->getPosition();
	Position
		posOrigin,
		posTarget,
		posVector,
		posOrigin_vox,
		posTarget_vox; // not used.

	const int
		armorSize = actor->getArmor()->getSize(),
		dir = actor->getDirection();

	Pathfinding::directionToVector(
								dir,
								&posVector);

	for (int
			x = 0;
			x != armorSize;
			++x)
	{
		for (int
				y = 0;
				y != armorSize;
				++y)
		{
			posOrigin = pos + Position(x,y,0);
			posTarget = posOrigin + posVector;

			tileOrigin = _battleSave->getTile(posOrigin);
			tileTarget = _battleSave->getTile(posTarget);

			if (tileOrigin != NULL && tileTarget != NULL)
			{
				if (tileTarget->hasUnconsciousUnit(false) == 0)
					tileTarget = getVerticalTile(
											posOrigin,
											posTarget);

				if (tileTarget != NULL
					&& tileTarget->hasUnconsciousUnit(false) != 0)
				{
					posOrigin_vox = Position(posOrigin * Position(16,16,24))
								  + Position(
											8,8,
											actor->getHeight(true)
												- tileOrigin->getTerrainLevel()
												- 4);
					if (canTargetTile(
									&posOrigin_vox,
									tileOrigin,
									O_FLOOR,
									&posTarget_vox,
									actor) == true)
					{
						return tileTarget;
					}
				}
			}
		}
	}

	return NULL;
}

/**
 * Gets a Tile within melee range.
 * @param posOrigin - reference a position origin
 * @param posTarget - reference a position target
 * @return, pointer to a tile within melee range
 */
Tile* TileEngine::getVerticalTile( // private.
		const Position& posOrigin,
		const Position& posTarget) const
{
	Tile
		* tileOrigin = _battleSave->getTile(posOrigin),

		* tileTargetAbove = _battleSave->getTile(posTarget + Position(0,0,1)),
		* tileTargetBelow = _battleSave->getTile(posTarget + Position(0,0,-1));

	if (tileTargetAbove != NULL
		&& std::abs(tileTargetAbove->getTerrainLevel() - (tileOrigin->getTerrainLevel() + 24)) < 9)
	{
		return tileTargetAbove;
	}

	if (tileTargetBelow != NULL
		&& std::abs((tileTargetBelow->getTerrainLevel() + 24) + tileOrigin->getTerrainLevel()) < 9)
	{
		return tileTargetBelow;
	}

	return NULL;
}

/**
 * Gets the AI to look through a window.
 * @param pos - reference the current Position
 * @return, direction or -1 if no window found
 */
int TileEngine::faceWindow(const Position& pos) const
{
	int ret = -1;

	const Tile* tile = _battleSave->getTile(pos);
	if (tile != NULL)
	{
		if (tile->getMapData(O_NORTHWALL) != NULL
			&& tile->getMapData(O_NORTHWALL)->stopLOS() == false)
		{
			ret = 0;
		}
		else if (tile->getMapData(O_WESTWALL) != NULL
			&& tile->getMapData(O_WESTWALL)->stopLOS() == false)
		{
			ret = 6;
		}
	}

	tile = _battleSave->getTile(pos + Position(1,0,0));
	if (tile != NULL
		&& tile->getMapData(O_WESTWALL) != NULL
		&& tile->getMapData(O_WESTWALL)->stopLOS() == false)
	{
		ret = 2;
	}

	tile = _battleSave->getTile(pos + Position(0,1,0));
	if (tile != NULL
		&& tile->getMapData(O_NORTHWALL) != NULL
		&& tile->getMapData(O_NORTHWALL)->stopLOS() == false)
	{
		ret = 4;
	}


	if ((ret == 0
			&& pos.y == 0)
		|| (ret == 2
			&& pos.x == _battleSave->getMapSizeX() - 1)
		|| (ret == 4
			&& pos.y == _battleSave->getMapSizeY() - 1)
		|| (ret == 6
			&& pos.x == 0))
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

	const double
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
 * Marks a region of the map as "dangerous to aliens" for a turn.
 * @param pos		- reference the epicenter of the explosion in tilespace
 * @param radius	- how far to spread out
 * @param unit		- pointer to the BattleUnit that is triggering this action
 */
void TileEngine::setDangerZone(
		const Position& pos,
		const int radius,
		const BattleUnit* const unit) const
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
						// trace a line here ignoring all units to check if the
						// explosion will reach this point; granted this won't
						// properly account for explosions tearing through walls,
						// but then you can't really know that kind of
						// information before the fact so let the AI assume that
						// the wall (or tree) is enough of a shield.
						if (plotLine(
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

/**
 * Sets a tile with a diagonal bigwall as the true epicenter of an explosion.
 */
void TileEngine::setTrueTile(Tile* const tile)
{
	_trueTile = tile;
}

}
