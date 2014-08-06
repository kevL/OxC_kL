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

#define _USE_MATH_DEFINES

#include "TileEngine.h"

#include <climits>
#include <cmath>
#include <functional>
#include <set>

#include <assert.h>
#include <SDL.h>

#include "AlienBAIState.h"
#include "BattleAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "ExplosionBState.h"
#include "InfoBoxOKState.h"	// kL, for message when unconscious unit explodes.
#include "Map.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "UnitTurnBState.h"

#include "../fmath.h"

#include "../Engine/Game.h"		// kL, for message when unconscious unit explodes.
#include "../Engine/Language.h"	// kL, for message when unconscious unit explodes.
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/MapDataSet.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/Unit.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

const int TileEngine::heightFromCenter[11] = {0,-2, 2,-4, 4,-6, 6,-8, 8,-12, 12};


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
		_personalLighting(true),
		_powerE(-1),	// kL
		_powerT(-1)		// kL
{
}

/**
 * Deletes the TileEngine.
 */
TileEngine::~TileEngine()
{
}

/**
 * Calculates sun shading for the whole terrain.
 */
void TileEngine::calculateSunShading()
{
	const int layer = 0; // Ambient lighting layer.

	for (int
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->resetLight(layer);
		calculateSunShading(_battleSave->getTiles()[i]);
	}
}

/**
 * Calculates sun shading for 1 tile. Sun comes from above and is blocked by floors or objects.
 * @param tile The tile to calculate sun shading for.
 */
void TileEngine::calculateSunShading(Tile* tile)
{
	//Log(LOG_INFO) << "TileEngine::calculateSunShading()";
	const int layer = 0; // Ambient lighting layer.

	int power = 15 - _battleSave->getGlobalShade();

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
						DT_NONE)) // !=0
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
			power -= 2;
		}
	}

	tile->addLight(
				power,
				layer);
	//Log(LOG_INFO) << "TileEngine::calculateSunShading() EXIT";
}

/**
 * Recalculates lighting for the terrain: objects,items,fire.
 */
void TileEngine::calculateTerrainLighting()
{
	const int layer				= 1;	// Static lighting layer.
	const int fireLightPower	= 15;	// amount of light a fire generates

	for (int // reset all light to 0 first
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->resetLight(layer);
	}

	for (int // add lighting of terrain
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
		// only floors and objects can light up
		if (_battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)
			&& _battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getLightSource())
		{
			addLight(
					_battleSave->getTiles()[i]->getPosition(),
					_battleSave->getTiles()[i]->getMapData(MapData::O_FLOOR)->getLightSource(),
					layer);
		}

		if (_battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)
			&& _battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)->getLightSource())
		{
			addLight(
					_battleSave->getTiles()[i]->getPosition(),
					_battleSave->getTiles()[i]->getMapData(MapData::O_OBJECT)->getLightSource(),
					layer);
		}

		if (_battleSave->getTiles()[i]->getFire())
		{
			addLight(
					_battleSave->getTiles()[i]->getPosition(),
					fireLightPower,
					layer);
		}

		for (std::vector<BattleItem*>::iterator
				it = _battleSave->getTiles()[i]->getInventory()->begin();
				it != _battleSave->getTiles()[i]->getInventory()->end();
				++it)
		{
			if ((*it)->getRules()->getBattleType() == BT_FLARE)
				addLight(
						_battleSave->getTiles()[i]->getPosition(),
						(*it)->getRules()->getPower(),
						layer);
		}
	}
}

/**
 * Recalculates lighting for the units.
 */
void TileEngine::calculateUnitLighting()
{
	const int layer = 2;					// Dynamic lighting layer.
//kL	const int personalLightPower = 15;	// amount of light a unit generates
	const int personalLightPower = 9;		// kL, Try it...
	const int fireLightPower = 15;			// amount of light a fire generates

	for (int // reset all light to 0 first
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
		_battleSave->getTiles()[i]->resetLight(layer);
	}

	for (std::vector<BattleUnit*>::iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (_personalLighting // add lighting of soldiers
			&& (*i)->getFaction() == FACTION_PLAYER
//kL		&& !(*i)->isOut())
			&& (*i)->getHealth() > 0) // kL, Let unconscious soldiers glow.
		{
			addLight(
					(*i)->getPosition(),
					personalLightPower,
					layer);
		}

		if ((*i)->getFire()) // add lighting of units on fire
			addLight(
					(*i)->getPosition(),
					fireLightPower,
					layer);
	}
}

/**
 * Toggles personal lighting on / off.
 */
void TileEngine::togglePersonalLighting()
{
	_personalLighting = !_personalLighting;
	calculateUnitLighting();
}

/**
 * Adds circular light pattern starting from voxelTarget and losing power with distance travelled.
 * @param voxelTarget, Center.
 * @param power, Power.
 * @param layer, Light is separated in 3 layers: Ambient, Static and Dynamic.
 */
void TileEngine::addLight(
		const Position& voxelTarget,
		int power,
		int layer)
{
	for (int // only loop through the positive quadrant.
			x = 0;
			x <= power;
			++x)
	{
		for (int
				y = 0;
				y <= power;
				++y)
		{
			for (int
					z = 0;
					z < _battleSave->getMapSizeZ();
					z++)
			{
				int distance = static_cast<int>(Round(sqrt(static_cast<double>(x * x + y * y))));

				if (_battleSave->getTile(Position(voxelTarget.x + x, voxelTarget.y + y, z)))
					_battleSave->getTile(Position(voxelTarget.x + x, voxelTarget.y + y, z))->addLight(power - distance, layer);

				if (_battleSave->getTile(Position(voxelTarget.x - x, voxelTarget.y - y, z)))
					_battleSave->getTile(Position(voxelTarget.x - x, voxelTarget.y - y, z))->addLight(power - distance, layer);

				if (_battleSave->getTile(Position(voxelTarget.x + x, voxelTarget.y - y, z)))
					_battleSave->getTile(Position(voxelTarget.x + x, voxelTarget.y - y, z))->addLight(power - distance, layer);

				if (_battleSave->getTile(Position(voxelTarget.x - x, voxelTarget.y + y, z)))
					_battleSave->getTile(Position(voxelTarget.x - x, voxelTarget.y + y, z))->addLight(power - distance, layer);
			}
		}
	}
}

/**
 * Calculates line of sight of a BattleUnit.
 * @param unit - pointer to battleunit to check line of sight for
 * @return, true when new aliens are spotted
 */
bool TileEngine::calculateFOV(BattleUnit* unit)
{
	//Log(LOG_INFO) << "TileEngine::calculateFOV() for ID " << unit->getId();
	unit->clearVisibleUnits();
	unit->clearVisibleTiles();

	if (unit->isOut(true, true)) // check health, check stun, check status
		return false;

	bool ret = false;

	size_t preVisUnits = unit->getUnitsSpottedThisTurn().size();
	//Log(LOG_INFO) << ". . . . preVisUnits = " << (int)preVisUnits;

	int dir;
	if (Options::strafe
		&& unit->getTurretType() > -1)
	{
		dir = unit->getTurretDirection();
	}
	else
		dir = unit->getDirection();
	//Log(LOG_INFO) << ". dir = " << dir;

	bool swap = (dir == 0 || dir == 4);

	int
		sign_x[8] = { 1, 1, 1, 1,-1,-1,-1,-1},
		sign_y[8] = {-1,-1, 1, 1, 1, 1,-1,-1};

	bool diag = false;
	int
		y1 = 0,
		y2 = 0;

	if (dir %2)
	{
		diag = true;
		y2 = MAX_VIEW_DISTANCE;
	}

	std::vector<Position> _trajectory;

	Position unitPos = unit->getPosition();
	//Log(LOG_INFO) << ". posUnit " << unitPos;
	Position testPos;

	if (unit->getHeight()
				+ unit->getFloatHeight()
				- _battleSave->getTile(unitPos)->getTerrainLevel()
			>= 24 + 4)
	{
		++unitPos.z;
	}

	for (int
			x = 0; // kL_note: does the unit itself really need checking...
			x <= MAX_VIEW_DISTANCE;
			++x)
	{
		if (!diag)
		{
			//Log(LOG_INFO) << ". not Diagonal";
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
					z < _battleSave->getMapSizeZ();
					++z)
			{
				//Log(LOG_INFO) << "for (int z = 0; z < _battleSave->getMapSizeZ(); z++), z = " << z;

//				int dist = distance(position, (*i)->getPosition());
				const int distSqr = x * x + y * y;
//				const int distSqr = x * x + y * y + z * z; // kL
				//Log(LOG_INFO) << "distSqr = " << distSqr << " ; x = " << x << " ; y = " << y << " ; z = " << z; // <- HUGE write to file.
				//Log(LOG_INFO) << "x = " << x << " ; y = " << y << " ; z = " << z; // <- HUGE write to file.

				testPos.z = z;

				if (distSqr <= MAX_VIEW_DISTANCE * MAX_VIEW_DISTANCE)
				{
					//Log(LOG_INFO) << "inside distSqr";

					int deltaPos_x = (sign_x[dir] * (swap? y: x));
					int deltaPos_y = (sign_y[dir] * (swap? x: y));
					testPos.x = unitPos.x + deltaPos_x;
					testPos.y = unitPos.y + deltaPos_y;
					//Log(LOG_INFO) << "testPos.x = " << testPos.x;
					//Log(LOG_INFO) << "testPos.y = " << testPos.y;
					//Log(LOG_INFO) << "testPos.z = " << testPos.z;
					//Log(LOG_INFO) << ". testPos " << testPos;


					if (_battleSave->getTile(testPos))
					{
						//Log(LOG_INFO) << "inside getTile(testPos)";
						BattleUnit* revealUnit = _battleSave->getTile(testPos)->getUnit();

						//Log(LOG_INFO) << ". . calculateFOV(), visible() CHECK.. " << revealUnit->getId();
						if (revealUnit
							&& !revealUnit->isOut(true, true)
							&& visible(
									unit,
									_battleSave->getTile(testPos))) // reveal units & tiles <- This seems uneven.
						{
							//Log(LOG_INFO) << ". . visible() TRUE : unitID = " << unit->getId() << " ; visID = " << revealUnit->getId();
							//Log(LOG_INFO) << ". . calcFoV, distance = " << distance(unit->getPosition(), revealUnit->getPosition());

							//Log(LOG_INFO) << ". . calculateFOV(), visible() TRUE id = " << revealUnit->getId();
							if (!revealUnit->getVisible()) // spottedID = " << revealUnit->getId();
							{
								//Log(LOG_INFO) << ". . calculateFOV(), getVisible() FALSE";
								ret = true;
							}
							//Log(LOG_INFO) << ". . calculateFOV(), revealUnit -> getVisible() = " << !ret;

							if (unit->getFaction() == FACTION_PLAYER)
							{
								//Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER, set spottedTile & spottedUnit visible";
								revealUnit->getTile()->setVisible(true);
//								revealUnit->getTile()->setDiscovered(true, 2); // kL_below. sprite caching for floor+content: DO I WANT THIS.

								if (!revealUnit->getVisible())
									revealUnit->setVisible(true);
							}
							//Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER, Done";

							if ((revealUnit->getFaction() == FACTION_HOSTILE
									&& unit->getFaction() == FACTION_PLAYER)
								|| (revealUnit->getFaction() != FACTION_HOSTILE
									&& unit->getFaction() == FACTION_HOSTILE))
							{
								//Log(LOG_INFO) << ". . . opposite Factions, add Tile & revealUnit to visList";

								// adds revealUnit to _visibleUnits *and* to _unitsSpottedThisTurn:
								unit->addToVisibleUnits(revealUnit); // note: This returns a boolean; i can use that...... yeah, done & done.
								unit->addToVisibleTiles(revealUnit->getTile()); // this reveals the TILE (ie. no 'shadowed' aLiens)?

								if (unit->getFaction() == FACTION_HOSTILE
									&& revealUnit->getFaction() != FACTION_HOSTILE
									&& _battleSave->getSide() == FACTION_HOSTILE) // per Original.
								{
									//Log(LOG_INFO) << ". . calculateFOV(), spotted Unit FACTION_HOSTILE, setTurnsExposed()";
									revealUnit->setTurnsExposed(0);	// note that xCom can be seen by enemies but *not* be Exposed. hehe
																	// Only reactionFire should set them Exposed during xCom's turn.
								}
							}
							//Log(LOG_INFO) << ". . calculateFOV(), opposite Factions, Done";
						}
						//Log(LOG_INFO) << ". . calculateFOV(), revealUnit EXISTS & isVis, Done";

						if (unit->getFaction() == FACTION_PLAYER) // reveal extra tiles
						{
							//Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER";
							// this sets tiles to discovered if they are in LOS -
							// tile visibility is not calculated in voxelspace but in tilespace;
							// large units have "4 pair of eyes"
							int size = unit->getArmor()->getSize();

							for (int
									xPlayer = 0;
									xPlayer < size;
									xPlayer++)
							{
								//Log(LOG_INFO) << ". . . . calculateLine() inside [1]for Loop";
								for (int
										yPlayer = 0;
										yPlayer < size;
										yPlayer++)
								{
									//Log(LOG_INFO) << ". . . . calculateLine() inside [2]for Loop";
									_trajectory.clear();

									Position posPlayer = unitPos + Position(
																		xPlayer,
																		yPlayer,
																		0);

									//Log(LOG_INFO) << ". . calculateLine()";
									int test = calculateLine(
															posPlayer,
															testPos,
															true,
															&_trajectory,
															unit,
															false);
									//Log(LOG_INFO) << ". . . . calculateLine() test = " << test;

									size_t trajSize = _trajectory.size();

//kL								if (test > 127) // last tile is blocked thus must be cropped
									if (test > 0)	// kL: -1 - do NOT crop trajectory (ie. hit content-object)
													//		0 - expose Tile ( should never return this, unless out-of-bounds )
													//		1 - crop the trajectory ( hit regular wall )
										--trajSize;

									//Log(LOG_INFO) << ". . . trace Trajectory.";
									for (size_t
											i = 0;
											i < trajSize;
											++i)
									{
										//Log(LOG_INFO) << ". . . . calculateLine() inside TRAJECTORY Loop";
										Position posTraj = _trajectory.at(i);

										// mark every tile of line as visible (this is needed because of bresenham narrow stroke).
										Tile* visTile = _battleSave->getTile(posTraj);
//kL									visTile->setVisible(+1);
										visTile->setVisible(true); // kL
										visTile->setDiscovered(true, 2); // sprite caching for floor+content, ergo + west & north walls.

										// walls to the east or south of a visible tile, we see that too
										// kL_note: Yeh, IF there's walls or an appropriate BigWall object!
										/*	parts:
											#0 - floor
											#1 - westwall
											#2 - northwall
											#3 - object (content) */
										Tile* edgeTile = _battleSave->getTile(Position(
																					posTraj.x + 1,
																					posTraj.y,
																					posTraj.z));
										if (edgeTile) // show Tile EAST
//kL										edgeTile->setDiscovered(true, 0);
										// kL_begin:
										{
											//Log(LOG_INFO) << "calculateFOV() East edgeTile VALID";
											if (visTile->getMapData(MapData::O_OBJECT) == NULL
												|| (visTile->getMapData(MapData::O_OBJECT)
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_BLOCK
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_EAST
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_E_S))
											{
												for (int
														part = 1;
														part < 4; //edgeTile->getMapData(fuck)->getDataset()->getSize();
														part += 2)
												{
													//Log(LOG_INFO) << "part = " << part;
													if (edgeTile->getMapData(part))
													{
														//Log(LOG_INFO) << ". getMapData(part) VALID";
														if (edgeTile->getMapData(part)->getObjectType() == MapData::O_WESTWALL) // #1
														{
															//Log(LOG_INFO) << ". . MapData: WestWall VALID";
															edgeTile->setDiscovered(true, 0); // reveal westwall
														}
														else if (edgeTile->getMapData(part)->getObjectType() == MapData::O_OBJECT // #3
															&& edgeTile->getMapData(MapData::O_OBJECT)
															&& (edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
																|| edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_WEST))
														{
															//Log(LOG_INFO) << ". . MapData: Object VALID, bigWall 1 or 4";
															edgeTile->setDiscovered(true, 2); // reveal entire edgeTile
														}
													}
												}
											}
										} // kL_end.

										edgeTile = _battleSave->getTile(Position(
																			posTraj.x,
																			posTraj.y + 1,
																			posTraj.z));
										if (edgeTile) // show Tile SOUTH
//kL										edgeTile->setDiscovered(true, 1);
										// kL_begin:
										{
											//Log(LOG_INFO) << "calculateFOV() South edgeTile VALID";
											if (visTile->getMapData(MapData::O_OBJECT) == NULL
												|| (visTile->getMapData(MapData::O_OBJECT)
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_BLOCK
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_SOUTH
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_E_S))
											{
												for (int
														part = 2;
														part < 4; //edgeTile->getMapData(fuck)->getDataset()->getSize();
														++part)
												{
													//Log(LOG_INFO) << "part = " << part;
													if (edgeTile->getMapData(part))
													{
														//Log(LOG_INFO) << ". getMapData(part) VALID";
														if (edgeTile->getMapData(part)->getObjectType() == MapData::O_NORTHWALL) // #2
														{
															//Log(LOG_INFO) << ". . MapData: NorthWall VALID";
															edgeTile->setDiscovered(true, 1); // reveal northwall
														}
														else if (edgeTile->getMapData(part)->getObjectType() == MapData::O_OBJECT // #3
															&& edgeTile->getMapData(MapData::O_OBJECT)
															&& (edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
																|| edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NORTH))
														{
															//Log(LOG_INFO) << ". . MapData: Object VALID, bigWall 1 or 5";
															edgeTile->setDiscovered(true, 2); // reveal entire edgeTile
														}
													}
												}
											}
										}

										edgeTile = _battleSave->getTile(Position(
																			posTraj.x - 1,
																			posTraj.y,
																			posTraj.z));
										if (edgeTile) // show Tile WEST
										{
											//Log(LOG_INFO) << "calculateFOV() West edgeTile VALID";
											//Log(LOG_INFO) << "part = 3";
											if (visTile->getMapData(MapData::O_WESTWALL) == NULL
												|| (visTile->getMapData(MapData::O_OBJECT)
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_BLOCK
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_WEST))
											{
												if (edgeTile->getMapData(MapData::O_OBJECT) // #3
													&& (edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
														|| edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_EAST
														|| edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_E_S))
												{
													//Log(LOG_INFO) << ". . MapData: Object VALID, bigWall 1 or 6 or 8";
													edgeTile->setDiscovered(true, 2); // reveal entire edgeTile
//													break;
												}
											}
										}

										edgeTile = _battleSave->getTile(Position(
																			posTraj.x,
																			posTraj.y - 1,
																			posTraj.z));
										if (edgeTile) // show Tile NORTH
										{
											//Log(LOG_INFO) << "calculateFOV() North edgeTile VALID";
											//Log(LOG_INFO) << "part = 3";
											if (visTile->getMapData(MapData::O_NORTHWALL) == NULL
												|| (visTile->getMapData(MapData::O_OBJECT)
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_BLOCK
													&& visTile->getMapData(MapData::O_OBJECT)->getBigWall() != Pathfinding::BIGWALL_NORTH))
											{
												if (edgeTile->getMapData(MapData::O_OBJECT) // #3
													&& (edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_BLOCK
														|| edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_SOUTH
														|| edgeTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_E_S))
												{
													//Log(LOG_INFO) << ". . MapData: Object VALID, bigWall 1 or 7 or 8";
													edgeTile->setDiscovered(true, 2); // reveal entire edgeTile
//													break;
												}
											}
										} // kL_end.
										//Log(LOG_INFO) << "calculateFOV() DONE edgeTile reveal";
									}
								}
							}
						}
						//Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER Done";
					}
					//Log(LOG_INFO) << ". . calculateFOV(), getTile(testPos) Done";
				}
				//Log(LOG_INFO) << ". . calculateFOV(), distSqr Done";
			}
			//Log(LOG_INFO) << ". . calculateFOV(), getMapSizeZ() Done";
		}
		//Log(LOG_INFO) << ". . calculateFOV(), y <= y2 Done";
	}
	//Log(LOG_INFO) << ". . DONE double for() Loops";

	// kL_begin: TileEngine::calculateFOV(), stop stopping my soldiers !!
	if (unit->getFaction() == FACTION_PLAYER
		&& ret == true)
	{
		//Log(LOG_INFO) << "TileEngine::calculateFOV() Player ret TRUE";
		return true;
	}
	else if (unit->getFaction() != FACTION_PLAYER // kL_end.
	// We only react when there are at least the same amount of
	// visible units as before AND the checksum is different
	// ( kL_note: get a grip on yourself, );
	// this way we stop if there are the same amount of visible units, but a
	// different unit is seen, or we stop if there are more visible units seen
		&& !unit->getVisibleUnits()->empty()
		&& unit->getUnitsSpottedThisTurn().size() > preVisUnits)
	{
		//Log(LOG_INFO) << "TileEngine::calculateFOV() Player NOT ret TRUE";
		return true;
	}

	//Log(LOG_INFO) << "TileEngine::calculateFOV() ret FALSE";
	return false;
}

/**
 * Calculates line of sight of all units within range of the Position
 * (used when terrain has changed, which can reveal new parts of terrain or units).
 * @param position, Position of the changed terrain.
 */
// kL_begin: TileEngine::calculateFOV, stop stopping my soldiers !!
void TileEngine::calculateFOV(const Position& position)
{
	//Log(LOG_INFO) << "TileEngine::calculateFOV(Pos&)";
	for (std::vector<BattleUnit*>::iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		//Log(LOG_INFO) << ". iterate ID = " << (*i)->getId();
		int dist = distance(
						position,
						(*i)->getPosition());
		//Log(LOG_INFO) << ". distance to Pos& = " << dist;
		if (dist <= MAX_VIEW_DISTANCE)
		{
			//Log(LOG_INFO) << ". . Pos& in Range, cont.";
			calculateFOV(*i);
		}
		//else Log(LOG_INFO) << ". . Pos& out of Range, cont.";
	}
	//Log(LOG_INFO) << "TileEngine::calculateFOV(Pos&) EXIT";
}

/**
 * Recalculates FOV of all units in-game.
 */
void TileEngine::recalculateFOV()
{
	//Log(LOG_INFO) << "TileEngine::recalculateFOV(), calculateFOV() calls";
	for (std::vector<BattleUnit*>::iterator
			bu = _battleSave->getUnits()->begin();
			bu != _battleSave->getUnits()->end();
			++bu)
	{
		if ((*bu)->getTile() != NULL)
			calculateFOV(*bu);
	}
}

/**
 * Checks for an opposing unit on a tile.
 * @param unit, The watcher.
 * @param tile, The tile to check for
 * @return, True if visible.
 */
bool TileEngine::visible(
		BattleUnit* unit,
		Tile* tile)
{
	//Log(LOG_INFO) << "TileEngine::visible() spotterID = " << unit->getId();

	// if there is no tile or no unit, we can't see it
	if (!tile
		|| !tile->getUnit())
	{
		return false;
	}

	BattleUnit* targetUnit = tile->getUnit();
	//Log(LOG_INFO) << ". target ID = " << targetUnit->getId();

	if (targetUnit->isOut(true, true))
		//Log(LOG_INFO) << ". . target is Dead, ret FALSE";
		return false;

	if (unit->getFaction() == targetUnit->getFaction())
		//Log(LOG_INFO) << ". . target is Friend, ret TRUE";
		return true;

	float dist = static_cast<float>(distance(
											unit->getPosition(),
											targetUnit->getPosition()));
	if (static_cast<int>(dist) > MAX_VIEW_DISTANCE)
		//Log(LOG_INFO) << ". . too far to see Tile, ret FALSE";
		return false;

	// aliens can see in the dark, xcom can see at a distance of 9 or less, further if there's enough light.
	//Log(LOG_INFO) << ". tileShade = " << tile->getShade();
	if (unit->getFaction() == FACTION_PLAYER
//		&& distance(
//					unit->getPosition(),
//					tile->getPosition())
		&& dist > 9.f
		&& tile->getShade() > MAX_SHADE_TO_SEE_UNITS)
	{
		//Log(LOG_INFO) << ". . too dark to see Tile, ret FALSE";
		return false;
	}
	//else Log(LOG_INFO) << ". . see Tile : Unit in light = " << tile->getUnit()->getId();


	// for large units origin voxel is in the middle ( not anymore )
	// kL_note: this leads to problems with large units trying to shoot around corners, b.t.w.
	// because it might See with a clear LoS, but the LoF is taken from a different, offset voxel.
	// further, i think Lines of Sight and Fire determinations are getting mixed up somewhere!!!
	Position originVoxel = getSightOriginVoxel(unit);
	Position scanVoxel;
	std::vector<Position> _trajectory;

	// kL_note: Is an intermediary object *not* obstructing viewing
	// or targetting, when it should be?? Like, around corners?
	bool unitIsSeen = canTargetUnit(
							&originVoxel,
							tile,
							&scanVoxel,
							unit);
	if (unitIsSeen)
	{
		//Log(LOG_INFO) << ". . . canTargetUnit(), unitIsSeen";

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

		// floatify this Smoke ( & Fire ) thing.
		float effDist = static_cast<float>(_trajectory.size());
		float factor = dist * 16.f / effDist; // how many 'real distance' units there are in each 'effective distance' unit.

		//Log(LOG_INFO) << ". . . effDist = " << effDist / 16.f;
		//Log(LOG_INFO) << ". . . dist = " << dist;
		//Log(LOG_INFO) << ". . . factor = " << factor;

		Tile* t = _battleSave->getTile(unit->getPosition()); // origin tile

//		for (uint16_t
		for (size_t
				i = 0;
				i < _trajectory.size();
				i++)
		{
			//Log(LOG_INFO) << ". . . . . . tracing Trajectory...";
			if (t != _battleSave->getTile(Position(
										_trajectory.at(i).x / 16,
										_trajectory.at(i).y / 16,
										_trajectory.at(i).z / 24)))
			{
				t = _battleSave->getTile(Position(
										_trajectory.at(i).x / 16,
										_trajectory.at(i).y / 16,
										_trajectory.at(i).z / 24));
			}

			// the 'origin tile' now steps along through voxel/tile-space, picking up extra
			// weight (subtracting distance for both distance and obscuration) as it goes
			effDist += static_cast<float>(t->getSmoke()) * factor / 3.f;
			//Log(LOG_INFO) << ". . . . . . . . -smoke : " << effDist;
			effDist += static_cast<float>(t->getFire()) * factor / 2.f;
			//Log(LOG_INFO) << ". . . . . . . . -fire : " << effDist;

			if (effDist > static_cast<float>(MAX_VOXEL_VIEW_DISTANCE))
			{
				//Log(LOG_INFO) << ". . . . Distance is too far. ret FALSE - effDist = " << (int)effDist / 16;
				unitIsSeen = false;

				break;
			}
			//else Log(LOG_INFO) << ". . . . unit is Seen, effDist = " << (int)effDist / 16;
		}
		//Log(LOG_INFO) << ". . effective sight range = " << effDist / 16.f;

		// Check if unitSeen is really targetUnit.
		//Log(LOG_INFO) << ". . . . 1 unitIsSeen = " << unitIsSeen;
		if (unitIsSeen)
		{
			// have to check if targetUnit is poking its head up from tileBelow
			Tile* tBelow = _battleSave->getTile(t->getPosition() + Position(0, 0, -1));
			if (!
				(t->getUnit() == targetUnit
					|| (tBelow // could add a check for && t->hasNoFloor() around here.
						&& tBelow->getUnit()
						&& tBelow->getUnit() == targetUnit)))
			{
				unitIsSeen = false;
				//if (kL_Debug) Log(LOG_INFO) << ". . . . 2 unitIsSeen = " << unitIsSeen;
			}
		}
	}

	//Log(LOG_INFO) << ". spotted ID " << targetUnit->getId() << " Ret unitIsSeen = " << unitIsSeen;
	return unitIsSeen;
}

/**
 * Gets the origin voxel of a unit's LoS.
 * @param unit, The watcher.
 * @return, Approximately an eyeball voxel.
 */
Position TileEngine::getSightOriginVoxel(BattleUnit* unit)
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

	Tile* tileAbove = _battleSave->getTile(unit->getPosition() + Position(0, 0, 1));

	// kL_note: let's stop this. Tanks appear to make their FoV etc. Checks from all four quadrants anyway.
/*	if (unit->getArmor()->getSize() > 1)
	{
		originVoxel.x += 8;
		originVoxel.y += 8;
		originVoxel.z += 1; // topmost voxel
	} */

	if (originVoxel.z >= (unit->getPosition().z + 1) * 24
		&& (!tileAbove
			|| !tileAbove->hasNoFloor(0)))
	{
		while (originVoxel.z >= (unit->getPosition().z + 1) * 24)
		{
			// careful with that ceiling, Eugene.
			originVoxel.z--;
		}
	}

	return originVoxel;
}

/**
 // kL_note: THIS IS NOT USED.
 * Checks for how exposed unit is for another unit.
 * @param originVoxel, Voxel of trace origin (eye or gun's barrel).
 * @param tile, The tile to check for.
 * @param excludeUnit, Is self (not to hit self).
 * @param excludeAllBut, [Optional] is unit which is the only one to be considered for ray hits.
 * @return, Degree of exposure (as percent).
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
 * @param originVoxel	- pointer to voxel of trace origin (eye or gun's barrel)
 * @param tile			- pointer to a tile to check for
 * @param scanVoxel		- pointer to voxel that is returned coordinate of hit
 * @param excludeUnit	- pointer to unitSelf (to not hit self)
 * @param potentialUnit	- pointer to a hypothetical unit to draw a virtual line of fire for AI; if left blank, this function behaves normally
 * @return, true if the unit can be targeted
 */
bool TileEngine::canTargetUnit(
		Position* originVoxel,
		Tile* tile,
		Position* scanVoxel,
		BattleUnit* excludeUnit,
		BattleUnit* potentialUnit)
{
	//Log(LOG_INFO) << "TileEngine::canTargetUnit()";
//kL	Position targetVoxel = Position((tile->getPosition().x * 16) + 7, (tile->getPosition().y * 16) + 8, tile->getPosition().z * 24);
	Position targetVoxel = Position(
								tile->getPosition().x * 16 + 8,
								tile->getPosition().y * 16 + 8,
								tile->getPosition().z * 24);

	std::vector<Position> _trajectory;

	bool hypothetical = (potentialUnit != NULL);
	if (potentialUnit == NULL)
	{
		potentialUnit = tile->getUnit();
		if (potentialUnit == NULL)
			return false; // no unit in this tile, even if it's elevated and appearing in it.
	}

	if (potentialUnit == excludeUnit)
		return false; // skip self

	int targetMinHeight = targetVoxel.z
						- tile->getTerrainLevel()
						+ potentialUnit->getFloatHeight(),
		targetMaxHeight = targetMinHeight,
		targetCenterHeight,
	// if there is an other unit on target tile, we assume we want to check against this unit's height
		heightRange,
		unitRadius = potentialUnit->getLoftemps(), // width == loft in default loftemps set
		xOffset = potentialUnit->getPosition().x - tile->getPosition().x,
		yOffset = potentialUnit->getPosition().y - tile->getPosition().y,
		targetSize = potentialUnit->getArmor()->getSize() - 1;
	if (targetSize > 0)
		unitRadius = 3;

	// vector manipulation to make scan work in view-space
	Position relPos = targetVoxel - *originVoxel;

	float normal = static_cast<float>(unitRadius)
					/ sqrt(static_cast<float>((relPos.x * relPos.x) + (relPos.y * relPos.y)));
	int
		relX = static_cast<int>(floor(static_cast<float>( relPos.y) * normal + 0.5f)),
		relY = static_cast<int>(floor(static_cast<float>(-relPos.x) * normal + 0.5f)),

		sliceTargets[10] =
	{
		 0,		 0,
		 relX,	 relY,
		-relX,	-relY,
		 relY,	-relX,
		-relY,	 relX
	};

	if (!potentialUnit->isOut())
		heightRange = potentialUnit->getHeight();
	else
		heightRange = 12;

	targetMaxHeight += heightRange;
	targetCenterHeight = (targetMaxHeight + targetMinHeight) / 2;
	heightRange /= 2;
	if (heightRange > 10)
		heightRange = 10;
	if (heightRange < 0)
		heightRange = 0;

	// scan ray from top to bottom plus different parts of target cylinder
	for (int
			i = 0;
			i <= heightRange;
			++i)
	{
		scanVoxel->z = targetCenterHeight + heightFromCenter[i];

		for (int
				j = 0;
				j < 5;
				++j)
		{
			if (i < heightRange - 1
				&& j > 2)
			{
				break; // skip unnecessary checks
			}

			scanVoxel->x = targetVoxel.x + sliceTargets[j * 2];
			scanVoxel->y = targetVoxel.y + sliceTargets[j * 2 + 1];

			_trajectory.clear();

			int test = calculateLine(
								*originVoxel,
								*scanVoxel,
								false,
								&_trajectory,
								excludeUnit);
			if (test == VOXEL_UNIT)
			{
				for (int
						x = 0;
						x <= targetSize;
						++x)
				{
					for (int
							y = 0;
							y <= targetSize;
							++y)
					{
						// voxel of hit must be inside of scanned box
						if (_trajectory.at(0).x / 16 == (scanVoxel->x / 16) + x + xOffset
							&& _trajectory.at(0).y / 16 == (scanVoxel->y / 16) + y + yOffset
							&& _trajectory.at(0).z >= targetMinHeight
							&& _trajectory.at(0).z <= targetMaxHeight)
						{
							//Log(LOG_INFO) << "TileEngine::canTargetUnit() EXIT[1] true";
							return true;
						}
					}
				}
			}
			else if (test == VOXEL_EMPTY
				&& hypothetical
				&& !_trajectory.empty())
			{
				//Log(LOG_INFO) << "TileEngine::canTargetUnit() EXIT[2] true";
				return true;
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::canTargetUnit() EXIT false";
	return false;
}

/**
 * Checks for a tile part available for targeting and what particular voxel.
 * @param originVoxel, Voxel of trace origin (gun's barrel).
 * @param tile, The tile to check for.
 * @param part, Tile part to check for.
 * @param scanVoxel, Is returned coordinate of hit.
 * @param excludeUnit, Is self (not to hit self).
 * @return, True if the tile can be targetted.
 */
bool TileEngine::canTargetTile(
		Position* originVoxel,
		Tile* tile,
		int part,
		Position* scanVoxel,
		BattleUnit* excludeUnit)
{
	//Log(LOG_INFO) << "TileEngine::canTargetTile()";
	static int sliceObjectSpiral[82] =
	{
		8,8,  8,6, 10,6, 10,8, 10,10, 8,10,  6,10,  6,8,  6,6,											// first circle
		8,4, 10,4, 12,4, 12,6, 12,8, 12,10, 12,12, 10,12, 8,12, 6,12, 4,12, 4,10, 4,8, 4,6, 4,4, 6,4,	// second circle
		8,1, 12,1, 15,1, 15,4, 15,8, 15,12, 15,15, 12,15, 8,15, 4,15, 1,15, 1,12, 1,8, 1,4, 1,1, 4,1	// third circle
	};

	static int northWallSpiral[14] =
	{
		7,0, 9,0, 6,0, 11,0, 4,0, 13,0, 2,0
	};

	static int westWallSpiral[14] =
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
		spiralCount,

		minZ = 0,
		maxZ = 0;
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
//		minZfound = true;
//		minZ = 0;
//		maxZfound = true;
//		maxZ = 0;
		minZfound = maxZfound = true;
		minZ = maxZ = 0;
	}
	else
		//Log(LOG_INFO) << "TileEngine::canTargetTile() EXIT, ret False (part is not a tileObject)";
		return false;

	if (!minZfound) // find out height range
	{
		for (int
				j = 1;
				j < 12;
				++j)
		{
			if (minZfound)
				break;

			for (int
					i = 0;
					i < spiralCount;
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
									0,
									true)
								== part) // bingo
				{
					if (!minZfound)
					{
						minZ = j * 2;
						minZfound = true;

						break;
					}
				}
			}
		}
	}

	if (!minZfound)
		//Log(LOG_INFO) << "TileEngine::canTargetTile() EXIT, ret False (!minZfound)";
		return false; // empty object!!!

	if (!maxZfound)
	{
		for (int
				j = 10;
				j >= 0;
				--j)
		{
			if (maxZfound) break;

			for (int
					i = 0;
					i < spiralCount;
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
									0,
									true)
								== part) // bingo
				{
					if (!maxZfound)
					{
						maxZ = j * 2;
						maxZfound = true;

						break;
					}
				}
			}
		}
	}

	if (!maxZfound)
	{
		//Log(LOG_INFO) << "TileEngine::canTargetTile() EXIT, ret False (!maxZfound)";
		return false; // it's impossible to get there
	}

	if (minZ > maxZ)
		minZ = maxZ;

	int
		rangeZ = maxZ - minZ,
		centerZ = (maxZ + minZ) / 2;

	for (int
			j = 0;
			j <= rangeZ;
			++j)
	{
		scanVoxel->z = targetVoxel.z + centerZ + heightFromCenter[j];

		for (int
				i = 0;
				i < spiralCount;
				++i)
		{
			scanVoxel->x = targetVoxel.x + spiralArray[i * 2];
			scanVoxel->y = targetVoxel.y + spiralArray[i * 2 + 1];

			_trajectory.clear();

			int test = calculateLine(
								*originVoxel,
								*scanVoxel,
								false,
								&_trajectory,
								excludeUnit);
			if (test == part) // bingo
			{
				if (_trajectory.at(0).x / 16 == scanVoxel->x / 16
					&& _trajectory.at(0).y / 16 == scanVoxel->y / 16
					&& _trajectory.at(0).z / 24 == scanVoxel->z / 24)
				{
					//Log(LOG_INFO) << "TileEngine::canTargetTile() EXIT, ret True (test == part)";
					return true;
				}
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::canTargetTile() EXIT, ret False";
	return false;
}

/**
 * Checks if a sniper from the opposing faction sees this unit. The unit with the
 * highest reaction score will be compared with the current unit's reaction score.
 * If it's higher, a shot is fired when enough time units, a weapon and ammo are available.
 * NOTE: the tuSpent parameter is needed because popState() doesn't
 * subtract TU until after the Initiative has been calculated.
 * @param unit		- pointer to a unit to check reaction fire against
 * @param tuSpent	- the unit's triggering expenditure of TU if firing or throwing. kL
 * @return, true if reaction fire took place
 */
bool TileEngine::checkReactionFire(
		BattleUnit* unit,
		int tuSpent) // kL
{
	//Log(LOG_INFO) << "TileEngine::checkReactionFire() vs targetID " << unit->getId();

	if (_battleSave->getSide() == FACTION_NEUTRAL) // no reaction on civilian turn.
		return false;


	// trigger reaction fire only when the spotted unit is of the
	// currently playing side, and is still on the map, alive
	if (unit->getFaction() != _battleSave->getSide()
		|| unit->getTile() == 0
		|| unit->isOut(true, true))	// kL (note getTile() may return false for corpses anyway)
	{
		//Log(LOG_INFO) << ". ret FALSE pre";
		return false;
	}

	bool ret = false;

	// not mind controlled, or is player's side:
	// kL. If spotted unit is not mind controlled,
	// or is mind controlled but not an alien;
	// ie, never reaction fire on a mind-controlled xCom soldier;
	// but *do* reaction fire on a mind-controlled aLien (or civilian.. ruled out above).
	if (unit->getFaction() == unit->getOriginalFaction()
//kL		|| unit->getFaction() != FACTION_HOSTILE)
		|| unit->getFaction() == FACTION_PLAYER) // kL
	{
		//Log(LOG_INFO) << ". Target = VALID";
		std::vector<BattleUnit*> spotters = getSpottingUnits(unit);
		//Log(LOG_INFO) << ". # spotters = " << spotters.size();

		BattleUnit* reactor = getReactor( // get the first man up to bat.
										spotters,
										unit,
										tuSpent); // kL
		// start iterating through the possible reactors until
		// the current unit is the one with the highest score.
		while (reactor != unit)
		{
			// !!!!!SHOOT!!!!!!!
			if (!tryReactionSnap( // <- statePushBack(new ProjectileFlyBState()
								reactor,
								unit))
			{
				//Log(LOG_INFO) << ". . no Snap by : " << reactor->getId();
				// can't make a reaction snapshot for whatever reason, boot this guy from the vector.
				for (std::vector<BattleUnit*>::iterator
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
				//Log(LOG_INFO) << ". . Snap by : " << reactor->getId();
				ret = true;
			}

			reactor = getReactor( // nice shot, kid. don't get too cocky.
								spotters,
								unit,
								tuSpent); // kL
			//Log(LOG_INFO) << ". . NEXT AT BAT : " << reactor->getId();
		}

		spotters.clear(); // kL
	}

	return ret;
}

/**
 * Creates a vector of units that can spot this unit.
 * @param unit, The unit to check for spotters of.
 * @return, A vector of units that can see this unit.
 */
std::vector<BattleUnit*> TileEngine::getSpottingUnits(BattleUnit* unit)
{
	//Log(LOG_INFO) << "TileEngine::getSpottingUnits() vs. ID " << unit->getId();
//no longer accurate			<< " : initi = " << (int)(unit)->getInitiative();

	Tile* tile = unit->getTile();

	std::vector<BattleUnit*> spotters;
	for (std::vector<BattleUnit*>::const_iterator
			spotter = _battleSave->getUnits()->begin();
			spotter != _battleSave->getUnits()->end();
			++spotter)
	{
		if ((*spotter)->getFaction() != _battleSave->getSide()
			&& !(*spotter)->isOut(true, true))
		{
/*kL			AlienBAIState* aggro = dynamic_cast<AlienBAIState*>((*spotter)->getCurrentAIState());
			if (((aggro != 0
						&& aggro->getWasHit()) // set in ProjectileFlyBState...
					|| (*spotter)->getFaction() == FACTION_HOSTILE // note: doesn't this cover the aggro-thing, like totally
					|| (*spotter)->checkViewSector(unit->getPosition())) // aLiens see all directions, btw. */
			if (((*spotter)->getFaction() == FACTION_HOSTILE					// Mc'd xCom units will RF on loyal xCom units
					|| ((*spotter)->getOriginalFaction() == FACTION_PLAYER		// but Mc'd aLiens won't RF on other aLiens ...
						&& (*spotter)->checkViewSector(unit->getPosition())))
				&& visible(*spotter, tile))
			{
				//Log(LOG_INFO) << ". check ID " << (*spotter)->getId();
				if ((*spotter)->getFaction() == FACTION_HOSTILE)
					unit->setTurnsExposed(0);

				// these two calls should already be done in calculateFOV()
//				if ((*spotter)->getFaction() == FACTION_PLAYER)
//					unit->setVisible(true);
//				(*spotter)->addToVisibleUnits(unit);
					// as long as calculateFOV is always done right between
					// walking, kneeling, shooting, throwing .. and checkReactionFire()
					// If so, then technically, visible() above can be replaced
					// by checking (*spotter)'s _visibleUnits vector. But this is working good per.

				if (canMakeSnap(*spotter, unit))
				{
					//Log(LOG_INFO) << ". . . reactor ID " << (*spotter)->getId()
					//		<< " : initi = " << (int)(*spotter)->getInitiative();

					spotters.push_back(*spotter);
				}
			}
		}
	}

	return spotters;
}

/**
 * Checks the validity of a reaction method.
 * kL: Changed to use selectFireMethod (aimed/auto/snap).
 * @param unit		- pointer to the spotting unit
 * @param target	- pointer to the spotted unit
 * @return, True if a shot can happen
 */
bool TileEngine::canMakeSnap(
		BattleUnit* unit,
		BattleUnit* target)
{
	//Log(LOG_INFO) << "TileEngine::canMakeSnap() reactID " << unit->getId() << " vs targetID " << target->getId();
	BattleItem* weapon; // = unit->getMainHandWeapon(true);
	if (unit->getFaction() == FACTION_PLAYER
		&& unit->getOriginalFaction() == FACTION_PLAYER)
	{
		weapon = unit->getItem(unit->getActiveHand());
	}
	else
		weapon = unit->getMainHandWeapon(); // kL_note: no longer returns grenades. good

	if (weapon == NULL)
	{
		//Log(LOG_INFO) << ". no weapon, return FALSE";
		return false;
	}


	bool canMelee = false;
	bool isMelee = weapon->getRules()->getBattleType() == BT_MELEE;

	if (isMelee)
	{
		//Log(LOG_INFO) << ". . validMeleeRange = " << validMeleeRange(unit, target, unit->getDirection());
		//Log(LOG_INFO) << ". . unit's TU = " << unit->getTimeUnits();
		//Log(LOG_INFO) << ". . action's TU = " << unit->getActionTUs(BA_HIT, weapon);
		for (int // check all 8 directions for Melee reaction
				i = 0;
				i < 8;
				++i)
		{
			canMelee = validMeleeRange(
									unit,
									target,
									i);

			if (canMelee)
				break;
		}
	}

	if (weapon->getRules()->canReactionFire() // kL add.
		&& (unit->getOriginalFaction() == FACTION_HOSTILE				// is aLien, or has researched weapon.
			|| _battleSave->getGeoscapeSave()->isResearched(weapon->getRules()->getRequirements()))
//		&& ((weapon->getRules()->getBattleType() == BT_MELEE			// has a melee weapon
		&& ((isMelee
				&& canMelee
//				&& validMeleeRange(
//								unit,
//								target,
//								unit->getDirection())					// is in melee range
				&& unit->getTimeUnits() >= unit->getActionTUs(			// has enough TU
															BA_HIT,
															weapon))
			|| (weapon->getRules()->getBattleType() == BT_FIREARM	// has a gun

					// ARE THESE REALLY CHECKED SOMEWHERE:

//kL				&& weapon->getRules()->getTUSnap()							// can make snapshot
//kL				&& weapon->getAmmoItem()									// gun is loaded, checked in "getMainHandWeapon()" -> what about getActiveHand() ?
//kL				&& unit->getTimeUnits() >= unit->getActionTUs(				// has enough TU
//kL															BA_SNAPSHOT,
//kL															weapon)
				&& testFireMethod(									// kL, has enough TU for a firing method.
								unit,
								target,
								weapon))))
	{
		//Log(LOG_INFO) << ". ret TRUE";
		return true;
	}

	//Log(LOG_INFO) << ". ret FALSE";
	return false;
}

/**
 * Gets the unit with the highest reaction score from the spotters vector.
 * NOTE: the tuSpent parameter is needed because popState() doesn't
 * subtract TU until after the Initiative has been calculated.
 * @param spotters	- vector of the pointers to spotting battleunits
 * @param defender	- pointer to the defending battleunit to check reaction scores against
 * @param tuSpent	- defending battleunit's expenditure of TU that had caused reaction checks - kL
 * @return, pointer to battleunit with the initiative (next up!)
 */
BattleUnit* TileEngine::getReactor(
		std::vector<BattleUnit*> spotters,
		BattleUnit* defender,
		int tuSpent) // kL
{
	//Log(LOG_INFO) << "TileEngine::getReactor() vs ID " << defender->getId();
	BattleUnit* nextReactor = NULL;
	int highestInit = -1;

	for (std::vector<BattleUnit*>::iterator
			spotter = spotters.begin();
			spotter != spotters.end();
			++spotter)
	{
		//Log(LOG_INFO) << ". . nextReactor ID " << (*spotter)->getId();
		if ((*spotter)->isOut(true, true))
			continue;

		if ((*spotter)->getInitiative() > highestInit)
		{
			highestInit = static_cast<int>((*spotter)->getInitiative());
			nextReactor = *spotter;
		}
	}


//	BattleAction action = _battleSave->getAction();
/*	BattleItem* weapon;// = BattleState::getAction();
	if (defender->getFaction() == FACTION_PLAYER
		&& defender->getOriginalFaction() == FACTION_PLAYER)
	{
		weapon = defender->getItem(defender->getActiveHand());
	}
	else
		weapon = defender->getMainHandWeapon(); // kL_note: no longer returns grenades. good
//	if (!weapon) return false;  // it *has* to be there by now!
		// note these calc's should be refactored; this calc happens what 3 times now!!!
		// Ought get the BattleAction* and just toss it around among these RF determinations.

	int tuShoot = defender->getActionTUs(BA_AUTOSHOT, weapon) */


	//Log(LOG_INFO) << ". ID " << defender->getId() << " initi = " << static_cast<int>(defender->getInitiative(tuSpent));

	// nextReactor has to *best* defender.Initi to get initiative
	// Analysis: It appears that defender's tu for firing/throwing
	// are not subtracted before getInitiative() is called.
	if (nextReactor
		&& highestInit > static_cast<int>(defender->getInitiative(tuSpent)))
	{
		if (nextReactor->getOriginalFaction() == FACTION_PLAYER)
			nextReactor->addReactionExp();
	}
	else
	{
		//Log(LOG_INFO) << ". . initi returns to ID " << defender->getId();
		nextReactor = defender;
	}

	//Log(LOG_INFO) << ". highestInit (nextReactor) = " << highestInit;
	return nextReactor;
}

/**
 * Attempts to perform a reaction snap shot.
 * @param unit, The unit to check sight from.
 * @param target, The unit to check sight TO.
 * @return, True if the action should (theoretically) succeed.
 */
bool TileEngine::tryReactionSnap(
		BattleUnit* unit,
		BattleUnit* target)
{
	//Log(LOG_INFO) << "TileEngine::tryReactionSnap() reactID " << unit->getId() << " vs targetID " << target->getId();
	BattleAction action;

	// note that other checks for/of weapon were done in "canMakeSnap()"
	// redone here to fill the BattleAction object...
	if (unit->getFaction() == FACTION_PLAYER)
		action.weapon = unit->getItem(unit->getActiveHand());
	else
		action.weapon = unit->getMainHandWeapon(); // kL_note: no longer returns grenades. good

	if (!action.weapon)
	{
		//Log(LOG_INFO) << ". no Weapon, ret FALSE";
		return false;
	}

//kL	action.type = BA_SNAPSHOT;	// reaction fire is ALWAYS snap shot.
									// kL_note: not true in Orig. aliens did auto at times
									// kL_note: changed to ALL shot-modes!

	action.actor = unit; // kL, was above under "BattleAction action;"
	action.target = target->getPosition();

	if (action.weapon->getRules()->getBattleType() == BT_MELEE)	// unless we're a melee unit.
		action.type = BA_HIT;									// kL_note: in which case you might not react at all. ( yet )
	else
		action.type = selectFireMethod(action); // kL, Let's try this. Might want to exclude soldiers, apply only to aLiens...

	action.TU = unit->getActionTUs(
								action.type,
								action.weapon);

	// kL_note: Does this handle melee hits, as reaction shots? really.
//	if (action.weapon->getAmmoItem()						// lasers & melee are their own ammo-items.
															// Unless their battleGame #ID happens to be 0
//		&& action.weapon->getAmmoItem()->getAmmoQuantity()	// returns 255 for laser; 0 for melee
//		&& unit->getTimeUnits() >= action.TU)
	// That's all been done!!!
//	{
	action.targeting = true;

	if (unit->getFaction() == FACTION_HOSTILE) // aLien units will go into an "aggro" state when they react.
	{
		AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(unit->getCurrentAIState());
		if (aggro == NULL) // should not happen, but just in case...
		{
			aggro = new AlienBAIState(
									_battleSave,
									unit,
									NULL);
			unit->setAIState(aggro);
		}

//kL	if (action.weapon->getAmmoItem()->getRules()->getExplosionRadius()
		if (action.weapon->getAmmoItem()->getRules()->getExplosionRadius() > -1 // kL
			&& aggro->explosiveEfficacy(
									action.target,
									unit,
									action.weapon->getAmmoItem()->getRules()->getExplosionRadius(),
									-1)
								== false)
		{
			action.targeting = false;
		}
	}

	if (action.targeting
		&& action.type != BA_NONE // kL
		&& unit->spendTimeUnits(action.TU))
	{
		//Log(LOG_INFO) << ". Reaction Fire by ID " << unit->getId();
		action.TU = 0;
		action.cameraPosition = _battleSave->getBattleState()->getMap()->getCamera()->getMapOffset(); // kL, was above under "BattleAction action;"

		_battleSave->getBattleGame()->statePushBack(new UnitTurnBState(
															_battleSave->getBattleGame(),
															action));
		_battleSave->getBattleGame()->statePushBack(new ProjectileFlyBState(
																_battleSave->getBattleGame(),
																action));

//		if (unit->getFaction() == FACTION_PLAYER)
//			unit->setTurnsExposed(0); // kL: That's for giving our position away!!

		return true;
	}
//	}

	return false;
}

/**
 * kL. Tests for a fire method based on range & time units.
 * lifted from: AlienBAIState::selectFireMethod()
 * @param unit, Pointer to a BattleUnit
 * @param target, Pointer to a targetUnit
 * @param weapon, Pointer to the unit's weapon
 * @return, True if a firing method is successfully chosen
 */
bool TileEngine::testFireMethod(
		BattleUnit* unit,
		BattleUnit* target,
		BattleItem* weapon) const
{
	//Log(LOG_INFO) << "TileEngine::testFireMethod()";

	int tuUnit = unit->getTimeUnits();
	//Log(LOG_INFO) << ". tuUnit = " << tuUnit;
	//Log(LOG_INFO) << ". tuAuto = " << unit->getActionTUs(BA_AUTOSHOT, weapon);
	//Log(LOG_INFO) << ". tuSnap = " << unit->getActionTUs(BA_SNAPSHOT, weapon);
	//Log(LOG_INFO) << ". tuAimed = " << unit->getActionTUs(BA_AIMEDSHOT, weapon);

	int distance = _battleSave->getTileEngine()->distance(
												unit->getPosition(),
												target->getPosition());
	//Log(LOG_INFO) << ". distance = " << distance;
//	if (distance < 7)
	if (distance <= weapon->getRules()->getAutoRange())
	{
		if (weapon->getRules()->getTUAuto()							// weapon can do this action-type
			&& tuUnit >= unit->getActionTUs(BA_AUTOSHOT, weapon))	// accounts for flatRate or not.
		{
			return true;
		}
		else if (weapon->getRules()->getTUSnap()
			&& tuUnit >= unit->getActionTUs(BA_SNAPSHOT, weapon))
		{
			return true;
		}
		else if (weapon->getRules()->getTUAimed()
			&& tuUnit >= unit->getActionTUs(BA_AIMEDSHOT, weapon))
		{
			return true;
		}
	}
//	else if (distance < 13)
	else if (distance <= weapon->getRules()->getSnapRange())
	{
		if (weapon->getRules()->getTUSnap()
			&& tuUnit >= unit->getActionTUs(BA_SNAPSHOT, weapon))
		{
			return true;
		}
		else if (weapon->getRules()->getTUAimed()
			&& tuUnit >= unit->getActionTUs(BA_AIMEDSHOT, weapon))
		{
			return true;
		}
		else if (weapon->getRules()->getTUAuto()
			&& tuUnit >= unit->getActionTUs(BA_AUTOSHOT, weapon))
		{
			return true;
		}
	}
//	else // distance > 12
	else if (distance <= weapon->getRules()->getAimRange())
	{
		if (weapon->getRules()->getTUAimed()
			&& tuUnit >= unit->getActionTUs(BA_AIMEDSHOT, weapon))
		{
			return true;
		}
		else if (weapon->getRules()->getTUSnap()
			&& tuUnit >= unit->getActionTUs(BA_SNAPSHOT, weapon))
		{
			return true;
		}
		else if (weapon->getRules()->getTUAuto()
			&& tuUnit >= unit->getActionTUs(BA_AUTOSHOT, weapon))
		{
			return true;
		}
	}

	//Log(LOG_INFO) << "TileEngine::testFireMethod() EXIT false";
	return false;
}

/**
 * kL. Selects a fire method based on range & time units.
 * lifted from: AlienBAIState::selectFireMethod()
 * @param action, A BattleAction
 * @return, The calculated BattleAction type
 */
BattleActionType TileEngine::selectFireMethod(BattleAction action) // could/should use a pointer for this(?)
{
	//Log(LOG_INFO) << "TileEngine::selectFireMethod()";

	action.type = BA_NONE; // should never happen.

	int
		tuUnit = action.actor->getTimeUnits(),
		distance = _battleSave->getTileEngine()->distance(
												action.actor->getPosition(),
												action.target);
//	if (distance < 7)
	if (distance <= action.weapon->getRules()->getAutoRange())
	{
		if (action.weapon->getRules()->getTUAuto()									// weapon can do this action-type
			&& tuUnit >= action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))	// accounts for flatRate or not.
		{
			action.type = BA_AUTOSHOT;
		}
		else if (action.weapon->getRules()->getTUSnap()
			&& tuUnit >= action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
		{
			action.type = BA_SNAPSHOT;
		}
		else if (action.weapon->getRules()->getTUAimed()
			&& tuUnit >= action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
		{
			action.type = BA_AIMEDSHOT;
		}
	}
//	else if (distance < 13)
	else if (distance <= action.weapon->getRules()->getSnapRange())
	{
		if (action.weapon->getRules()->getTUSnap()
			&& tuUnit >= action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
		{
			action.type = BA_SNAPSHOT;
		}
		else if (action.weapon->getRules()->getTUAimed()
			&& tuUnit >= action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
		{
			action.type = BA_AIMEDSHOT;
		}
		else if (action.weapon->getRules()->getTUAuto()
			&& tuUnit >= action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))
		{
			action.type = BA_AUTOSHOT;
		}
	}
//	else // distance > 12
	else if (distance <= action.weapon->getRules()->getAimRange())
	{
		if (action.weapon->getRules()->getTUAimed()
			&& tuUnit >= action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
		{
			action.type = BA_AIMEDSHOT;
		}
		else if (action.weapon->getRules()->getTUSnap()
			&& tuUnit >= action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
		{
			action.type = BA_SNAPSHOT;
		}
		else if (action.weapon->getRules()->getTUAuto()
			&& tuUnit >= action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))
		{
			action.type = BA_AUTOSHOT;
		}
	}

	return action.type;
}

/**
 * Handles bullet/weapon hits. A bullet/weapon hits a voxel.
 * kL_note: called from ExplosionBState
 * @param pTarget_voxel	- reference to the center of hit in voxelspace
 * @param power			- power of the hit/explosion
 * @param type			- damage type of the hit (RuleItem.h)
 * @param attacker		- pointer to unit that caused the hit
 * @param melee			- true if no projectile, trajectory, etc. is needed. kL
 * @return, the BattleUnit that got hit
 */
BattleUnit* TileEngine::hit(
		const Position& pTarget_voxel,
		int power,
		ItemDamageType type,
		BattleUnit* attacker,
		bool melee) // kL add.
{
	//Log(LOG_INFO) << "TileEngine::hit() by ID " << attacker->getId()
	//			<< " @ " << attacker->getPosition()
	//			<< " : power = " << power
	//			<< " : type = " << (int)type;

	if (type != DT_NONE) // bypass Psi-attacks.
	{
		//Log(LOG_INFO) << "DT_ type = " << static_cast<int>(type);
		Position pTarget_tile = Position(
										pTarget_voxel.x / 16,
										pTarget_voxel.y / 16,
										pTarget_voxel.z / 24);
		Tile* tile = _battleSave->getTile(pTarget_tile);
/*		Tile* tile = _battleSave->getTile(Position(
										pTarget_voxel.x / 16,
										pTarget_voxel.y / 16,
										pTarget_voxel.z / 24)); */
		//Log(LOG_INFO) << ". targetTile " << tile->getPosition() << " targetVoxel " << pTarget_voxel;

		if (!tile)
		{
			//Log(LOG_INFO) << ". Position& pTarget_voxel : NOT Valid, return NULL";
			return NULL;
		}

		BattleUnit* buTarget = NULL;
		if (tile->getUnit())
		{
			buTarget = tile->getUnit();
			//Log(LOG_INFO) << ". . buTarget Valid ID = " << buTarget->getId();
		}

		// This is returning part < 4 when using a stunRod against a unit outside the north (or east) UFO wall. ERROR!!!
		// later: Now it's returning VOXEL_EMPTY (-1) when a silicoid attacks a poor civvie!!!!! And on acid-spits!!!
//kL	int const part = voxelCheck(pTarget_voxel, attacker);
		// kL_note: for melee==TRUE, just put part=VOXEL_UNIT here. Like,
		// and take out the 'melee' parameter from voxelCheck() unless
		// I want to flesh-out melee & psi attacks more than they are.
		int part = VOXEL_UNIT; // kL, no longer const
		if (!melee) // kL
			part = voxelCheck(
							pTarget_voxel,
							attacker,
							false,
							false,
							NULL,
							melee); // kL
		//Log(LOG_INFO) << ". voxelCheck() part = " << part;

		if (VOXEL_EMPTY < part && part < VOXEL_UNIT	// 4 terrain parts ( 0..3 )
			&& type != DT_STUN						// kL, workaround for Stunrod. ( might include DT_SMOKE & DT_IN )
			&& type != DT_SMOKE)
		{
			//Log(LOG_INFO) << ". . terrain hit";
//kL		const int randPower = RNG::generate(power / 4, power * 3 / 4); // RNG::boxMuller(power, power/6)
			power = RNG::generate( // 25% to 75%
								power / 4,
								power * 3 / 4);
			//Log(LOG_INFO) << ". . RNG::generate(power) = " << power;

			if (part == VOXEL_OBJECT
				&& _battleSave->getMissionType() == "STR_BASE_DEFENSE")
			{
				if (power >= tile->getMapData(MapData::O_OBJECT)->getArmor()
					&& tile->getMapData(VOXEL_OBJECT)->isBaseModule())
				{
					_battleSave->getModuleMap()
										[(pTarget_voxel.x / 16) / 10]
										[(pTarget_voxel.y / 16) / 10].second--;
				}
			}

			// kL_note: This may be necessary only on aLienBase missions...
			if (tile->damage(
							part,
							power))
			{
				_battleSave->setObjectiveDestroyed(true);
			}

			// kL_note: This would be where to adjust damage based on effectiveness of weapon vs Terrain!
		}
		else if (part == VOXEL_UNIT)	// battleunit part
//			|| type == DT_STUN)			// kL, workaround for Stunrod. (not needed, huh?)
		{
			//Log(LOG_INFO) << ". . battleunit hit";

			// power 0 - 200% -> 1 - 200%
//kL		const int randPower = RNG::generate(1, power * 2); // RNG::boxMuller(power, power / 3)

			int vertOffset = 0;

			if (!buTarget)
			{
				//Log(LOG_INFO) << ". . . buTarget NOT Valid, check tileBelow";

				// it's possible we have a unit below the actual tile, when he
				// stands on a stairs and sticks his head up into the above tile.
				// kL_note: yeah, just like in LoS calculations!!!! cf. visible() etc etc .. idiots.
				Tile* tileBelow = _battleSave->getTile(pTarget_tile + Position(0, 0,-1));
/*				Tile* tileBelow = _battleSave->getTile(Position(
															pTarget_voxel.x / 16,
															pTarget_voxel.y / 16,
															pTarget_voxel.z / 24 - 1)); */
				if (tileBelow
					&& tileBelow->getUnit())
				{
					buTarget = tileBelow->getUnit();
					vertOffset = 24;
				}
			}

			if (buTarget)
			{
				//Log(LOG_INFO) << ". . . buTarget Valid ID = " << buTarget->getId();

				// kL_note: This section needs adjusting ...!
//kL			const int wounds = buTarget->getFatalWounds();

				// "adjustedDamage = buTarget->damage(relative, rndPower, type);" -> GOES HERE

				// if targetUnit is going to bleed to death and is not a player, give credit for the kill.
				// kL_note: not just if not a player; Give Credit!!!
				// NOTE: Move this code-chunk below(s).
//kL				if (unit
//kL					&& buTarget->getFaction() != FACTION_PLAYER
//					&& buTarget->getOriginalFaction() != FACTION_PLAYER // kL
//kL					&& wounds < buTarget->getFatalWounds())
//				{
//kL					buTarget->killedBy(unit->getFaction());
//				} // kL_note: Not so sure that's been setup right (cf. other kill-credit code as well as DebriefingState)

/*kL
				const int size = buTarget->getArmor()->getSize() * 8;
				const Position targetPos = (buTarget->getPosition() * Position(16, 16, 24)) // convert tilespace to voxelspace
						+ Position(
								size,
								size,
								buTarget->getFloatHeight() - tile->getTerrainLevel());
				const Position relPos = pTarget_voxel - targetPos - Position(0, 0, vertOffset); */
				const int size = buTarget->getArmor()->getSize() * 8;
				const Position
					targetPos = (buTarget->getPosition() * Position(16, 16, 24)) // convert tilespace to voxelspace
								+ Position(
										size,
										size,
										buTarget->getFloatHeight() - tile->getTerrainLevel()),
					relPos = pTarget_voxel
							- targetPos
							- Position(
									0,
									0,
									vertOffset);

				// kL_begin: TileEngine::hit(), Silacoids can set targets on fire!!
//kL			if (type == DT_IN)
				if (attacker->getSpecialAbility() == SPECAB_BURNFLOOR)
				{
					float modifier = buTarget->getArmor()->getDamageModifier(DT_IN);
					if (modifier > 0.f)
					{
						// generate(4, 11)
						int firePower = RNG::generate( // kL: 25% - 75% / 2
												power / 8,
												power * 3 / 8);
						//Log(LOG_INFO) << ". . . . DT_IN : firePower = " << firePower;

						// generate(5, 10)
						int check = buTarget->damage(
												Position(0, 0, 0),
												firePower,
												DT_IN,
												true);
						//Log(LOG_INFO) << ". . . . DT_IN : " << buTarget->getId() << " takes " << check;

						int burnTime = RNG::generate(
												0,
												static_cast<int>(5.f * modifier));
						if (buTarget->getFire() < burnTime)
							buTarget->setFire(burnTime); // catch fire and burn
					}
				} // kL_end.

				double range = 100.0;
				if (type == DT_HE
					|| Options::TFTDDamage)
				{
					range = 50.0;
				}

				int
					min = static_cast<int>(static_cast<double>(power) * (100.0 - range) / 100.0) + 1,
					max = static_cast<int>(static_cast<double>(power) * (100.0 + range) / 100.0);

				power = RNG::generate(min, max);

/*				if (type == DT_HE
					|| Options::TFTDDamage)
				{
					power = RNG::generate(
										power / 2,
										power * 3 / 2);
				}
				else
					power = RNG::generate(
										1,
										power * 2); */
				//Log(LOG_INFO) << ". . . RNG::generate(power) = " << power;


				const int wounds = buTarget->getFatalWounds();
				bool ignoreArmor = type == DT_STUN			// kL. stun ignores armor... does now! UHM....
									|| type == DT_SMOKE;	// note it still gets Vuln.modifier, but not armorReduction.
				int adjDamage = buTarget->damage(
												relPos,
												power,
												type,
												ignoreArmor);
				//Log(LOG_INFO) << ". . . adjDamage = " << adjDamage;

				if (adjDamage > 0)
//					&& !buTarget->isOut()
				{
					// kL_begin:
					if (attacker
						&& (wounds < buTarget->getFatalWounds()
							|| buTarget->getHealth() == 0)) // kL .. just do this here and bDone with it. Regularly done in BattlescapeGame::checkForCasualties()
					{
						buTarget->killedBy(attacker->getFaction());
					} // kL_end.
					// kL_note: Not so sure that's been setup right (cf. other kill-credit code as well as DebriefingState)
					// I mean, shouldn't that be checking that the thing actually DIES?
					// And, prob don't have to state if killed by aLiens: probly assumed in DebriefingState.

					if (buTarget->getHealth() > 0)
					{
						const int bravery = (110 - buTarget->getStats()->bravery) / 10;
						if (bravery > 0)
						{
							int modifier = 100;
							if (buTarget->getOriginalFaction() == FACTION_PLAYER)
								modifier = _battleSave->getMoraleModifier();
							else if (buTarget->getOriginalFaction() == FACTION_HOSTILE)
								modifier = _battleSave->getMoraleModifier(0, false);

							const int morale_loss = 10 * adjDamage * bravery / modifier;
							//Log(LOG_INFO) << ". . . . morale_loss = " << morale_loss;

							buTarget->moraleChange(-morale_loss);
						}
					}

					//Log(LOG_INFO) << ". . check for Cyberdisc expl.";
					//Log(LOG_INFO) << ". . health = " << buTarget->getHealth();
					//Log(LOG_INFO) << ". . stunLevel = " << buTarget->getStunlevel();
					if (buTarget->getSpecialAbility() == SPECAB_EXPLODEONDEATH // cyberdiscs
						&& (buTarget->getHealth() == 0
							|| buTarget->getStunlevel() >= buTarget->getHealth()))
	//					&& !buTarget->isOut(false, true))	// kL. don't explode if stunned. Maybe... wrong!!!
															// Cannot be STATUS_DEAD OR STATUS_UNCONSCIOUS!
					{
						//Log(LOG_INFO) << ". . . Cyberdisc down!!";
						if (type != DT_STUN		// don't explode if stunned. Maybe... see above.
							&& type != DT_SMOKE
							&& type != DT_HE)	// don't explode if taken down w/ explosives -> wait a sec, this is hit() not explode() ...
						{
							//Log(LOG_INFO) << ". . . . new ExplosionBState(), !DT_STUN & !DT_HE";
							// kL_note: wait a second. hit() creates an ExplosionBState,
							// but ExplosionBState::explode() creates a hit() ! -> terrain..

							Position unitPos = Position(
	//kL											buTarget->getPosition().x * 16,
	//kL											buTarget->getPosition().y * 16,
	//kL											buTarget->getPosition().z * 24);
	//												buTarget->getPosition().x * 16 + 8, // kL
	//												buTarget->getPosition().y * 16 + 8, // kL
													buTarget->getPosition().x * 16 + 16,	// kL, cyberdisc a big unit.
													buTarget->getPosition().y * 16 + 16,	// kL
													buTarget->getPosition().z * 24 + 12);	// kL

							_battleSave->getBattleGame()->statePushNext(new ExplosionBState(
																						_battleSave->getBattleGame(),
																						unitPos,
																						NULL,
																						buTarget));
						}
					}
				}

				if (buTarget->getOriginalFaction() == FACTION_HOSTILE	// target is aLien Mc'd or not.
					&& attacker											// shooter exists
					&& attacker->getOriginalFaction() == FACTION_PLAYER	// shooter is Xcom
					&& attacker->getFaction() == FACTION_PLAYER			// shooter is not Mc'd Xcom
//					&& type != DT_NONE
					&& _battleSave->getBattleGame()->getCurrentAction()->type != BA_HIT
					&& _battleSave->getBattleGame()->getCurrentAction()->type != BA_STUN)
				{
					//Log(LOG_INFO) << ". . addFiringExp() - huh, even for Melee?"; // okay they're working on it ...
					// kL_note: with my workaround for Stunrods above, this needs to check
					// whether a StunLauncher fires or a melee attack has been done, and
					// Exp altered accordingly:
					attacker->addFiringExp();
				}
			}
		}

		//Log(LOG_INFO) << ". applyGravity()";
		applyGravity(tile);
		//Log(LOG_INFO) << ". calculateSunShading()";
		calculateSunShading();								// roofs could have been destroyed
		//Log(LOG_INFO) << ". calculateTerrainLighting()";
		calculateTerrainLighting();							// fires could have been started
		//Log(LOG_INFO) << ". calculateFOV()";
//		calculateFOV(pTarget_voxel / Position(16, 16, 24));	// walls & objects could have been ruined
		calculateFOV(pTarget_tile);


		//if (buTarget) Log(LOG_INFO) << "TileEngine::hit() EXIT, return buTarget";
		//else Log(LOG_INFO) << "TileEngine::hit() EXIT, return 0";

		return buTarget;
	}
	//else Log(LOG_INFO) << ". DT_ = " << static_cast<int>(type);

	//Log(LOG_INFO) << "TileEngine::hit() EXIT, return NULL";
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
 * @param type			- damage type of explosion (enum ItemDamageType)
 * @param maxRadius		- maximum radius of explosion
 * @param unit			- pointer to a unit that caused explosion
 */
void TileEngine::explode(
			const Position& voxelTarget,
			int power,
			ItemDamageType type,
			int maxRadius,
			BattleUnit* unit)
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

	//Log(LOG_INFO) << "TileEngine::explode() power = " << power << ", type = " << (int)type << ", maxRadius = " << maxRadius;
	if (type == DT_IN)
	{
		power /= 2;
		//Log(LOG_INFO) << ". DT_IN power = " << power;
	}

	if (power < 1) // kL, quick out.
		return;

	BattleUnit* targetUnit = NULL;

	std::set<Tile*> tilesAffected;
	std::pair<std::set<Tile*>::iterator, bool> tilePair;

	int z_Dec = 1000; // default flat explosion

	switch (Options::battleExplosionHeight)
	{
		case 3: z_Dec = 10; break;
		case 2: z_Dec = 20; break;
		case 1: z_Dec = 30; break;

		case 0: // default flat explosion
		default:
			z_Dec = 1000;
		break;
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

//	int testIter = 0; // TEST.
	//Log(LOG_INFO) << ". r_Max = " << r_Max;

//	for (int fi = 0; fi == 0; ++fi) // kL_note: Looks like a TEST ray. ( 0 == horizontal )
//	for (int fi = 90; fi == 90; ++fi) // vertical: UP
	for (int
			fi = -90;
			fi <= 90;
			fi += 5)
	{
//		for (int te = 90; te == 90; ++te) // kL_note: Looks like a TEST ray. ( 0 == south, 180 == north, goes CounterClock-wise )
//		for (int te = 90; te < 360; te += 180) // E & W
//		for (int te = 45; te < 360; te += 180) // SE & NW
//		for (int te = 225; te < 420; te += 180) // NW & SE
		for (int // raytracing every 3 degrees makes sure we cover all tiles in a circle.
				te = 0;
				te <= 360;
				te += 3)
		{
			sin_te = sin(static_cast<double>(te) * M_PI / 180.0);
			cos_te = cos(static_cast<double>(te) * M_PI / 180.0);
			sin_fi = sin(static_cast<double>(fi) * M_PI / 180.0);
			cos_fi = cos(static_cast<double>(fi) * M_PI / 180.0);

			origin = _battleSave->getTile(Position(
												centerX,
												centerY,
												centerZ));

			_powerE = _powerT = power;	// initialize _powerE & _powerT for each ray.
			r = 0.0;					// initialize radial length, also.

			while (_powerE > 0
				&& r - 1.0 < r_Max) // kL_note: Allows explosions of 0 radius(!), single tile only hypothetically.
									// the idea is to show an explosion animation but affect only that one tile.
			{
				//Log(LOG_INFO) << ". . . . .";
				//++testIter;
				//Log(LOG_INFO) << ". i = " << testIter;
				//Log(LOG_INFO) << ". r = " << r; // << ", r_Max = " << r_Max;
				//Log(LOG_INFO) << ". _powerE = " << _powerE;

				vect_x = static_cast<double>(centerX) + r * sin_te * cos_fi;
				vect_y = static_cast<double>(centerY) + r * cos_te * cos_fi;
				vect_z = static_cast<double>(centerZ) + r * sin_fi;

				tileX = static_cast<int>(floor(vect_x));
				tileY = static_cast<int>(floor(vect_y));
				tileZ = static_cast<int>(floor(vect_z));

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
					&& origin != destTile)	// don't double blockage from the same tiles (when diagonal this happens).
				{
					if (type == DT_IN)
					{
						int dir;
						Pathfinding::vectorToDirection(
													destTile->getPosition() - origin->getPosition(), // kL
													dir);
						if (dir != -1
							&& dir %2)
						{
							_powerE -= 5; // diagonal movement costs an extra 50% for fire.
						}
					}

					_powerE -= 10;
					if (_powerE < 1)
					{
						//Log(LOG_INFO) << ". _powerE < 1 BREAK[hori] " << Position(tileX, tileY, tileZ);
						break;
					}

					if (origin->getPosition().z != tileZ) // up/down explosion decrease
					{
						_powerE -= z_Dec;
						if (_powerE < 1)
						{
							//Log(LOG_INFO) << ". _powerE < 1 BREAK[vert] " << Position(tileX, tileY, tileZ);
							break;
						}
					}

					_powerT = _powerE;

					int horiBlock = horizontalBlockage(
													origin,
													destTile,
													type);
					//Log(LOG_INFO) << ". horiBlock = " << horiBlock;
					int vertBlock = verticalBlockage(
												origin,
												destTile,
												type);
					//Log(LOG_INFO) << ". vertBlock = " << vertBlock;

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
								//Log(LOG_INFO) << ". horiBlock BREAK " << Position(tileX, tileY, tileZ);
								break;
							}
						}

						if (vertBlock > 0) // only !visLike will return > 0 for these breaks here.
						{
							_powerT -= vertBlock; // terrain takes 200% power to destruct. <- But this isn't for destruction.
							if (_powerT < 1)
							{
								//Log(LOG_INFO) << ". vertBlock BREAK " << Position(tileX, tileY, tileZ);
								break;
							}
						}
					}
				}

				// set this to the power-value *before* BLOCK reduces it, and *after* distance is accounted for!
				if (type == DT_HE) // explosions do 50% damage to terrain and 50% to 150% damage to units
					destTile->setExplosive(_powerE);

				_powerE = _powerT;


				// ** DAMAGE begins w/ _powerE ***

				//Log(LOG_INFO) << ". pre insert Tile " << Position(tileX, tileY, tileZ);
				tilePair = tilesAffected.insert(destTile); // check if we had this tile already
				//Log(LOG_INFO) << ". post insert Tile";
				if (tilePair.second) // true if a new tile was inserted.
				{
					//Log(LOG_INFO) << ". . > tile TRUE : origin " << origin->getPosition() << " dest " << destTile->getPosition(); //<< ". _powerE = " << _powerE << ". r = " << r;
					//Log(LOG_INFO) << ". . > _powerE = " << _powerE;
					targetUnit = destTile->getUnit();
					if (targetUnit
						&& targetUnit->getTakenExpl()) // hit large units only once ... stop experience exploitation near the end of this loop, also. Lulz
					{
						//Log(LOG_INFO) << ". . targetUnit ID " << targetUnit->getId() << ", set Unit = NULL";
						targetUnit = NULL;
					}

					int wounds = 0;
					if (unit
						&& targetUnit)
					{
						wounds = targetUnit->getFatalWounds();
					}

					int powerVsUnit = 0;

					switch (type)
					{
						case DT_STUN: // power 0 - 200%
						{
							if (targetUnit)
							{
								powerVsUnit = RNG::generate(
														1,
														_powerE * 2);
								//Log(LOG_INFO) << ". . . powerVsUnit = " << powerVsUnit << " DT_STUN";
								targetUnit->damage(
												Position(0, 0, 0),
												powerVsUnit,
												DT_STUN,
												true);
							}

							for (std::vector<BattleItem*>::iterator
									item = destTile->getInventory()->begin();
									item != destTile->getInventory()->end();
									++item)
							{
								if ((*item)->getUnit())
								{
									powerVsUnit = RNG::generate(
															1,
															_powerE * 2);
									//Log(LOG_INFO) << ". . . . powerVsUnit (corpse) = " << powerVsUnit << " DT_STUN";
									(*item)->getUnit()->damage(
															Position(0, 0, 0),
															powerVsUnit,
															DT_STUN,
															true);
								}
							}
						}
						break;
						case DT_HE: // power 50 - 150%, 65% of that if kneeled. 85% @ GZ
						{
							//Log(LOG_INFO) << ". . type == DT_HE";
							if (targetUnit)
							{
								powerVsUnit = static_cast<int>(RNG::generate( // 50% to 150%
													static_cast<double>(_powerE) * 0.5,
													static_cast<double>(_powerE) * 1.5));
								//Log(LOG_INFO) << ". . DT_HE power = " << powerVsUnit << ", vs ID " << targetUnit->getId();

								if (distance(
											destTile->getPosition(),
											Position(
												centerX,
												centerY,
												centerZ))
										< 2)
								{
									//Log(LOG_INFO) << ". . . powerVsUnit = " << powerVsUnit << " DT_HE, GZ";
									if (targetUnit->isKneeled())
									{
										powerVsUnit = powerVsUnit * 17 / 20; // 85% damage
										//Log(LOG_INFO) << ". . . powerVsUnit(kneeled) = " << powerVsUnit << " DT_HE, GZ";
									}

									targetUnit->damage( // Ground zero effect is in effect
													Position(0, 0, 0),
													powerVsUnit,
													DT_HE);
									//Log(LOG_INFO) << ". . . realDamage = " << damage << " DT_HE, GZ";
								}
								else
								{
									// units above the explosion will be hit in the legs, units lateral to or below will be hit in the torso

									//Log(LOG_INFO) << ". . . powerVsUnit = " << powerVsUnit << " DT_HE, not GZ";
									if (targetUnit->isKneeled())
									{
										powerVsUnit = powerVsUnit * 13 / 20; // 65% damage
										//Log(LOG_INFO) << ". . . powerVsUnit(kneeled) = " << powerVsUnit << " DT_HE, not GZ";
									}

									targetUnit->damage( // Directional damage relative to explosion position.
													Position(
															centerX * 16 - destTile->getPosition().x * 16,
															centerY * 16 - destTile->getPosition().y * 16,
															centerZ * 24 - destTile->getPosition().z * 24),
													powerVsUnit,
													DT_HE);
									//Log(LOG_INFO) << ". . . realDamage = " << damage << " DT_HE, not GZ";
								}
							}

							bool done = false;
							while (!done)
							{
								done = destTile->getInventory()->empty();

								for (std::vector<BattleItem*>::iterator
										it = destTile->getInventory()->begin();
										it != destTile->getInventory()->end();
										)
								{
									powerVsUnit = static_cast<int>(RNG::generate( // 50% to 150%
														static_cast<double>(_powerE) * 0.5,
														static_cast<double>(_powerE) * 1.5));

									if (_powerE > (*it)->getRules()->getArmor())
									{
										if ((*it)->getUnit()
											&& (*it)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
										{
											(*it)->getUnit()->instaKill();
											// send Death notice.
											if (Options::battleNotifyDeath)
											{
												Game* game = _battleSave->getBattleState()->getGame();
												game->pushState(new InfoboxOKState(game->getLanguage()->getString(
																											"STR_HAS_BEEN_KILLED", // "has exploded ..."
																											(*it)->getUnit()->getGender())
																										.arg((*it)->getUnit()->getName(game->getLanguage()))));
											}
										}

										_battleSave->removeItem((*it));

										break;
									}
									else
									{
										++it;
										done = (it == destTile->getInventory()->end());
									}
								}
							}
						}
						break;
						case DT_SMOKE:	// smoke from explosions always stay 6 to 14 turns - power of a smoke grenade is 60
							if (destTile->getSmoke() < 10
								&& destTile->getTerrainLevel() > -24)
							{
								destTile->setFire(0);
								destTile->setSmoke(RNG::generate(7, 15));
							}

							if (targetUnit) // kL: add this in for smoke inhalation
							{
								float modifier = targetUnit->getArmor()->getDamageModifier(DT_SMOKE);
								if (modifier > 0.f)
								{
									powerVsUnit = RNG::generate( // 10% to 20%
															_powerE / 10,
															_powerE / 5);

									targetUnit->damage(
													Position(0, 0, 0), // 12 - destTile->getTerrainLevel()
													powerVsUnit,
													DT_SMOKE,
													true);
									//Log(LOG_INFO) << ". . DT_IN : " << targetUnit->getId() << " takes " << firePower << " firePower";
								}
							}
						break;
						case DT_IN:
							if (!destTile->isVoid())
							{
								// kL_note: So, this just sets a tile on fire/smoking regardless of its content.
								// cf. Tile::ignite() -> well, not regardless, but automatically. That is,
								// ignite() checks for Flammability first: if (getFlammability() == 255) don't do it.
								// So this is, like, napalm from an incendiary round, while ignite() is for parts
								// of the tile itself self-igniting.
								if (destTile->getFire() == 0
									&& (destTile->getMapData(MapData::O_FLOOR)
										|| destTile->getMapData(MapData::O_OBJECT)))
								{
									destTile->setFire(destTile->getFuel() + 1);
									destTile->setSmoke(std::max(
															1,
															std::min(
																	15 - (destTile->getFlammability() / 10),
																	12)));
								}

								// kL_note: fire damage is also caused by BattlescapeGame::endTurn()
								// -- but previously by BattleUnit::prepareNewTurn()!!!!
								if (targetUnit)
								{
									float modifier = targetUnit->getArmor()->getDamageModifier(DT_IN);
									if (modifier > 0.f)
									{
//kL									int fire = RNG::generate(4, 11);
										powerVsUnit = RNG::generate( // kL: 25% - 75%
																_powerE / 4,
																_powerE * 3 / 4);

										targetUnit->damage(
														Position(0, 0, 0), // 12 - destTile->getTerrainLevel()
														powerVsUnit,
														DT_IN,
														true);
										//Log(LOG_INFO) << ". . DT_IN : " << targetUnit->getId() << " takes " << firePower << " firePower";

										int burnTime = RNG::generate(
																0,
																static_cast<int>(5.f * modifier));
										if (targetUnit->getFire() < burnTime) // catch fire and burn
											targetUnit->setFire(burnTime);
									}
								}
							}
						break;

						default:
						break;
					}


					if (targetUnit)
					{
						//Log(LOG_INFO) << ". . targetUnit ID " << targetUnit->getId() << ", setTaken TRUE";
						targetUnit->setTakenExpl(true);

						// if it's going to bleed to death and it's not a player, give credit for the kill.
						// kL_note: See Above^
						if (unit)
						{
							if (wounds < targetUnit->getFatalWounds()
								|| targetUnit->getHealth() == 0) // kL .. just do this here and bDone with it. Regularly done in BattlescapeGame::checkForCasualties()
							{
								targetUnit->killedBy(unit->getFaction());
							}

							if (unit->getOriginalFaction() == FACTION_PLAYER			// kL, shooter is Xcom
								&& unit->getFaction() == FACTION_PLAYER					// kL, shooter is not Mc'd Xcom
								&& targetUnit->getOriginalFaction() == FACTION_HOSTILE	// kL, target is aLien Mc'd or not; no Xp for shooting civies...
								&& type != DT_SMOKE)									// sorry, no Xp for smoke!
//								&& targetUnit->getFaction() != unit->getFaction())
							{
								unit->addFiringExp();
							}
						}
					}
				}

//				_powerE = _powerT;
				origin = destTile;
				r += 1.0;
			}
		}
	}


	_powerE = _powerT = -1;

	for (std::vector<BattleUnit*>::iterator
			bu = _battleSave->getUnits()->begin();
			bu != _battleSave->getUnits()->end();
			++bu)
	{
		if ((*bu)->getTakenExpl())
		{
			//Log(LOG_INFO) << ". . unitTaken ID " << (*bu)->getId() << ", reset Taken";
			(*bu)->setTakenExpl(false);
		}
	}


	if (type == DT_HE) // detonate tiles affected with HE
	{
		//Log(LOG_INFO) << ". explode Tiles";
		for (std::set<Tile*>::iterator
				explTile = tilesAffected.begin();
				explTile != tilesAffected.end();
				++explTile)
		{
			if (detonate(*explTile))
				_battleSave->setObjectiveDestroyed(true);

			applyGravity(*explTile);

			Tile* tileAbove = _battleSave->getTile((*explTile)->getPosition() + Position(0, 0, 1));
			if (tileAbove)
				applyGravity(tileAbove);
		}
		//Log(LOG_INFO) << ". explode Tiles DONE";
	}

	calculateSunShading();		// roofs could have been destroyed
	calculateTerrainLighting();	// fires could have been started

//kL	calculateFOV(voxelTarget / Position(16, 16, 24));
	recalculateFOV(); // kL
	//Log(LOG_INFO) << "TileEngine::explode() EXIT";
}

/**
 * Calculates the amount of power that is blocked as it passes
 * from one tile to another on the same z-level.
 * @param startTile	- pointer to tile where the power starts
 * @param endTile	- pointer to adjacent tile where the power ends
 * @param type		- type of power (DT_* RuleItem.h)
 * @return, -99 special case for Content & bigWalls to block vision and still get revealed, and for invalid tiles also
 *			-1 hardblock power / vision (can be less than -1)
 *			 0 no block
 *			 1+ variable power blocked
 */
int TileEngine::horizontalBlockage(
		Tile* startTile,
		Tile* endTile,
		ItemDamageType type)
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
	bool visLike = type == DT_NONE
				|| type == DT_IN
				|| type == DT_STUN
				|| type == DT_SMOKE;

	if (startTile == NULL // safety checks
		|| endTile == NULL
		|| startTile->getPosition().z != endTile->getPosition().z)
	{
		return 0;
	}

	// debug:
	//bool debug = type == DT_HE;

	int dir;
	Pathfinding::vectorToDirection( // Set&Get direction by reference
							endTile->getPosition() - startTile->getPosition(),
							dir);
	if (dir == -1) // startTile == endTile
		return 0;


	static const Position tileNorth	= Position( 0,-1, 0);
	static const Position tileEast	= Position( 1, 0, 0);
	static const Position tileSouth	= Position( 0, 1, 0);
	static const Position tileWest	= Position(-1, 0, 0);

	int block = 0;

	switch (dir)
	{
		case 0:	// north
			block = blockage(
							startTile,
							MapData::O_NORTHWALL,
							type);

			if (!visLike) // visLike does this after the switch()
				block += blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								0,
								true)
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								4);
		break;
		case 1: // north east
//			if (type == DT_NONE) // this is two-way diagonal visibility check
			if (visLike)
			{
				block = blockage( // up+right
								startTile,
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								endTile,
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								type,
								3); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // right+up
//				block += blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								type,
								7); // checks Content/bigWalls
			}
			else // dt_NE
			{
				block = blockage(
								startTile,
								MapData::O_NORTHWALL,
								type) / 2
						+ blockage(
								endTile,
								MapData::O_WESTWALL,
								type) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_NORTHWALL,
								type) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_WESTWALL,
								type) / 2

						+ blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								0,
								true) / 2
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								2,
								true) / 2

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								type,
								2) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								type,
								4) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								type,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								type,
								6) / 2 // checks Content/bigWalls

						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								4) / 2 // checks Content/bigWalls
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								6) / 2; // checks Content/bigWalls
			}
		break;
		case 2: // east
			block = blockage(
							endTile,
							MapData::O_WESTWALL,
							type);

			if (!visLike) // visLike does this after the switch()
				block += blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								6)
						+  blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								2,
								true);
		break;
		case 3: // south east
//			if (type == DT_NONE) // this is two-way diagonal visibility check
			if (visLike)
			{
				block = blockage( // down+right
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								endTile,
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								type,
								1); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // right+down
//				block += blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_WESTWALL,
								type)
						+ blockage(
								endTile,
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								type,
								5); // checks Content/bigWalls
			}
			else // dt_SE
			{
				block = blockage(
								endTile,
								MapData::O_WESTWALL,
								type) / 2
						+ blockage(
								endTile,
								MapData::O_NORTHWALL,
								type) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_WESTWALL,
								type) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_NORTHWALL,
								type) / 2

						+ blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								2,
								true) / 2 // checks Content/bigWalls
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								4,
								true) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								type,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								type,
								2) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								type,
								4) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileEast),
								MapData::O_OBJECT,
								type,
								6) / 2 // checks Content/bigWalls

						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								6) / 2; // checks Content/bigWalls
			}
		break;
		case 4: // south
			block = blockage(
							endTile,
							MapData::O_NORTHWALL,
							type);

			if (!visLike) // visLike does this after the switch()
				block += blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								0)
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								4,
								true);
		break;
		case 5: // south west
//			if (type == DT_NONE) // this is two-way diagonal visibility check
			if (visLike)
			{
				block = blockage( // down+left
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								type,
								7); // checks Content/bigWalls

				if (block == 0) break; // this way is opened

				block = blockage( // left+down
//				block += blockage(
								startTile,
								MapData::O_WESTWALL,
								type)
						+ blockage(
								endTile,
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								type,
								3);
			}
			else // dt_SW
			{
				block = blockage(
								endTile,
								MapData::O_NORTHWALL,
								type) / 2
						+ blockage(
								startTile,
								MapData::O_WESTWALL,
								type) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_WESTWALL,
								type) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_NORTHWALL,
								type) / 2

						+ blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								4,
								true) / 2 // checks Content/bigWalls
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								6,
								true) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								type,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileSouth),
								MapData::O_OBJECT,
								type,
								6) / 2 // checks Content/bigWalls

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								type,
								2) / 2 // checks Content/bigWalls
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								type,
								4) / 2 // checks Content/bigWalls

						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								0) / 2 // checks Content/bigWalls
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								2) / 2; // checks Content/bigWalls
			}
		break;
		case 6: // west
			block = blockage(
							startTile,
							MapData::O_WESTWALL,
							type);

			if (!visLike) // visLike does this after the switch()
			{
				block += blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								6,
								true)
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								2);
			}
		break;
		case 7: // north west
//			if (type == DT_NONE) // this is two-way diagonal visibility check
			if (visLike)
			{
				block = blockage( // up+left
								startTile,
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								type,
								5);

				if (block == 0) break; // this way is opened

				block = blockage( // left+up
//				block += blockage(
								startTile,
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								type,
								1);
			}
			else // dt_NW
			{
				block = blockage(
								startTile,
								MapData::O_WESTWALL,
								type) / 2
						+ blockage(
								startTile,
								MapData::O_NORTHWALL,
								type) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_WESTWALL,
								type) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_NORTHWALL,
								type) / 2

						+ blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								0,
								true) / 2
						+ blockage(
								startTile,
								MapData::O_OBJECT,
								type,
								6,
								true) / 2

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								type,
								4) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileNorth),
								MapData::O_OBJECT,
								type,
								6) / 2

						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								type,
								0) / 2
						+ blockage(
								_battleSave->getTile(startTile->getPosition() + tileWest),
								MapData::O_OBJECT,
								type,
								2) / 2

						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								2) / 2
						+ blockage(
								endTile,
								MapData::O_OBJECT,
								type,
								4) / 2;
			}
		break;

		default:
		break;
	}

	if (visLike)
	{
		block += blockage(
						startTile,
						MapData::O_OBJECT,
						type,
						dir, // checks Content/bigWalls
						true,
						true);
		if (block == 0		// if, no vision block yet ...
			&& blockage(	// so check for content @endTile & reveal it by not cutting trajectory.
					endTile,
					MapData::O_OBJECT,
					type,
					(dir + 4) %8) // opposite direction
				!= 0) // should always be, < 1; ie. this conditions checks [if -1]
		{
			if (type == DT_NONE)
				return -1;
			else
				return 500; // this is a hardblock and should be greater than the most powerful explosions.
		}
	}

	//Log(LOG_INFO) << "TileEngine::horizontalBlockage() EXIT, ret = " << block;
	return block;
}

/**
 * Calculates the amount of power that is blocked going from one tile to another on a different z-level.
 * Can cross more than one level (used for lighting). Only floor & object tiles are taken into account ... not really!
 * @param startTile	- pointer to tile where the power starts
 * @param endTile	- pointer to adjacent tile where the power ends
 * @param type		- type of power (RuleItem.h)
 * @return, (int)block	-99 special case for Content-objects to block vision, and for invalid tiles
 *						-1 hardblock power / vision (can be less than -1)
 *						 0 no block
 *						 1+ variable power blocked
 */
int TileEngine::verticalBlockage(
		Tile* startTile,
		Tile* endTile,
		ItemDamageType type)
{
	//Log(LOG_INFO) << "TileEngine::verticalBlockage()";
/*	bool visLike = type == DT_NONE
				|| type == DT_IN
				|| type == DT_STUN
				|| type == DT_SMOKE; */

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
					z++)
			{
				block += blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								type)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_OBJECT,
								type,
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
									type);

			for (
					z += 1;
					z <= endTile->getPosition().z;
					++z)
			{
				block += blockage( // these check the endTile
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								type)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_OBJECT,
								type); // note: no Dir vs typeOBJECT
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
					z--)
			{
				block += blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								type)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_OBJECT,
								type,
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
									type);

			for (
					;
					z > endTile->getPosition().z;
					--z)
			{
				block += blockage( // these check the endTile
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								type)
						+ blockage(
								_battleSave->getTile(Position(x, y, z)),
								MapData::O_OBJECT,
								type); // note: no Dir vs typeOBJECT
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
 * @param type The type of power/damage.
 * @return Amount of blockage of this power.
 */
/* int TileEngine::verticalBlockage(Tile *startTile, Tile *endTile, ItemDamageType type, bool skipObject)
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
		block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type, Pathfinding::DIR_DOWN);
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), type, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type);
		}
	}
	else if (direction > 0) // up
	{
		block += blockage(_save->getTile(Position(x, y, z+1)), MapData::O_FLOOR, type);
		if (!skipObject)
			block += blockage(_save->getTile(Position(x, y, z+1)), MapData::O_OBJECT, type, Pathfinding::DIR_UP);
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z+1;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), type, skipObject);
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
			if (!skipObject)
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type);
		}
	}

	return block;
} */

/**
 * Calculates the amount of power or LoS/FoV/LoF that various types of
 * walls/bigwalls or floors or object parts of a tile blocks.
 * @param startTile			- pointer to tile where the power starts
 * @param part				- the part of the tile that the power tries to go through
 * @param type				- the type of power (RuleItem.h) DT_NONE if line-of-vision
 * @param dir				- direction the power travels	-1	walls & floors (default)
 *															 0+	big-walls & content
 * @param checkingOrigin	- true if the origin tile is being examined for bigWalls;
 *								used only when dir is specified (default: false)
 * @param trueDir			- for checking if dir is *really* from the direction of sight (true)
 *								or, in the case of some bigWall determinations, perpendicular to it (false);
 *								used only when dir is specified (default: false)
 * @return, (int)block	-99 special case for invalid tiles
 *						-1 hardblock power / vision
 *						 0 no block
 *						 1+ variable power blocked
 */
int TileEngine::blockage(
		Tile* tile,
		const int part,
		ItemDamageType type,
		int dir,
		bool checkingOrigin,
		bool trueDir) // kL_add
{
	//Log(LOG_INFO) << "TileEngine::blockage() dir " << dir;
	bool visLike = type == DT_NONE
				|| type == DT_SMOKE
				|| type == DT_STUN
				|| type == DT_IN;

	if (tile == NULL					// probably outside the map here
		|| tile->isUfoDoorOpen(part))	// open ufo doors are actually still closed behind the scenes
	{
		//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret ( no tile OR ufo-door open )";
		return 0;
	}

	if (tile->getMapData(part))
	{
		//Log(LOG_INFO) << ". getMapData(part) stopLOS() = " << tile->getMapData(part)->stopLOS();
		if (dir == -1) // regular north/west wall (not BigWall), or it's a floor, or a Content-object vs upward-diagonal.
		{
			if (visLike)
			{
				if ((tile->getMapData(part)->stopLOS()
							|| (type == DT_SMOKE
								&& tile->getMapData(part)->getBlock(DT_SMOKE) == 1)) // some tiles do not stopLOS yet should block smoke (eg. Skyranger cockpit)
						&& (tile->getMapData(part)->getObjectType() == MapData::O_OBJECT // this one is for verticalBlockage() only.
							|| tile->getMapData(part)->getObjectType() == MapData::O_NORTHWALL
							|| tile->getMapData(part)->getObjectType() == MapData::O_WESTWALL)
					|| tile->getMapData(part)->getObjectType() == MapData::O_FLOOR)	// all floors that block LoS should have their stopLOS flag set true.
																					// because they aren't. But stock OxC code sets them stopLOS=true anyway ...
																					// HA, maybe not ......
				{
//					if (type == DT_NONE)
//						return 1;// hardblock.
//					else
					//Log(LOG_INFO) << ". . . . RET 501 part = " << part << " " << tile->getPosition();
					return 500;
				}
			}
			else if (tile->getMapData(part)->stopLOS()
				&& _powerE > -1
				&& _powerE < tile->getMapData(part)->getArmor() * 2) // terrain absorbs 200% damage from DT_HE!
			{
				//Log(LOG_INFO) << ". . . . RET 502 part = " << part << " " << tile->getPosition();
				return 500; // this is a hardblock for HE; hence it has to be higher than the highest HE power in the Rulesets.
			}
		}
		else // dir > -1 -> OBJECT part. ( BigWalls & content ) *always* an OBJECT-part gets passed in through here, and *with* a direction.
		{
			int bigWall = tile->getMapData(MapData::O_OBJECT)->getBigWall(); // 0..8 or, per MCD.
			//Log(LOG_INFO) << ". bigWall = " << bigWall;

			if (checkingOrigin)	// kL (the ContentOBJECT already got hit as the previous endTile...
								// but can still block LoS when looking down ...)
			{
/*				if (bigWall == 0 // if (only Content-part == true)
//					|| (dir =	// this would need to check which side the *missile* is coming from first
								// but grenades that land on this diagonal bigWall would be exempt regardless!!!
								// so, just can it (for now): HardBlock!
//						&& bigWall == Pathfinding::BIGWALL_NESW)
//					|| bigWall == Pathfinding::BIGWALL_NWSE)
					|| (dir == 9
						&& !tile->getMapData(MapData::O_OBJECT)->stopLOS())) */
				if (!visLike
						&& bigWall == 0 // if (only Content-part == true)
					|| (dir == 9
						&& !tile->getMapData(MapData::O_OBJECT)->stopLOS()
						&& !(type == DT_SMOKE
							&& tile->getMapData(MapData::O_OBJECT)->getBlock(DT_SMOKE) == 1)))
				{
					return 0;
				}
				else if (!visLike
					&& (bigWall == Pathfinding::BIGWALL_NESW
						|| bigWall == Pathfinding::BIGWALL_NWSE)
					&& tile->getMapData(MapData::O_OBJECT)->stopLOS()
					&& _powerE > -1
					&& _powerE < tile->getMapData(MapData::O_OBJECT)->getArmor() * 2)
				{
					//Log(LOG_INFO) << ". . . . RET 503 part = " << part << " " << tile->getPosition();
					return 500;	// unfortunately this currently blocks expl-propagation originating
								// vs diagonal walls traveling out from those walls (see note above)
				}
			}

			if (visLike // hardblock for visLike
				&& (tile->getMapData(MapData::O_OBJECT)->stopLOS()
					|| (type == DT_SMOKE
						&& tile->getMapData(MapData::O_OBJECT)->getBlock(DT_SMOKE) == 1))
				&& bigWall == 0)
			{
//				if (type == DT_NONE)
//					return -1;
//				else
				//Log(LOG_INFO) << ". . . . RET 504 part = " << part << " " << tile->getPosition();
				return 500;
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
							&& !trueDir))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 1 northeast )";
						return 0;
					}
				break;

				case 2: // east
					if (bigWall == Pathfinding::BIGWALL_NORTH
						|| bigWall == Pathfinding::BIGWALL_SOUTH
						|| bigWall == Pathfinding::BIGWALL_WEST)
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
							&& !trueDir))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 3 southeast )";
						return 0;
					}
				break;

				case 4: // south
					if (bigWall == Pathfinding::BIGWALL_WEST
						|| bigWall == Pathfinding::BIGWALL_EAST
						|| bigWall == Pathfinding::BIGWALL_NORTH)
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
							&& !trueDir))
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
							&& !trueDir))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 7 northwest )";
						return 0;
					}
				break;

				case 8: // up
				case 9: // down
					if ((bigWall != Pathfinding::BIGWALL_NONE			// lets content-objects Block explosions
							&& bigWall != Pathfinding::BIGWALL_BLOCK)	// includes stopLoS (floors handled above under non-directional condition)
						|| (visLike
							&& !tile->getMapData(MapData::O_OBJECT)->stopLOS()
							&& !(type == DT_SMOKE
								&& tile->getMapData(MapData::O_OBJECT)->getBlock(DT_SMOKE) == 1)))
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 8,9 up,down )";
						return 0;
					}
				break;


				default:
					return 0; // .....
				break;
			}

			// might be Content-part or remaining-bigWalls block here
			if (tile->getMapData(MapData::O_OBJECT)->stopLOS() // use stopLOS to hinder explosions from propagating through bigWalls freely.
				|| (type == DT_SMOKE
					&& tile->getMapData(MapData::O_OBJECT)->getBlock(DT_SMOKE) == 1))
			{
				if (visLike
					|| (_powerE > -1
						&& _powerE < tile->getMapData(MapData::O_OBJECT)->getArmor() * 2)) // terrain absorbs 200% damage from DT_HE!
				{
					//Log(LOG_INFO) << ". . . . RET 505 part = " << part << " " << tile->getPosition();
					return 500; // this is a hardblock for HE; hence it has to be higher than the highest HE power in the Rulesets.
				}
			}
		}

		if (!visLike) // only non-visLike can get partly blocked; other damage-types are either completely blocked or get a pass here
		{
//			int ret = tile->getMapData(part)->getBlock(type);
			//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret = " << ret;
			return tile->getMapData(part)->getBlock(type);
		}
	}

	//Log(LOG_INFO) << "TileEngine::blockage() EXIT, (no valid part) ret 0";
	return 0; // no Valid [part].
}

/**
 * Applies the explosive power to the tile parts. This is where the actual destruction takes place.
 * Must affect 9 objects (6 box sides and the object inside plus 2 outer walls).
 * @param tile Tile affected.
 * @return True if the objective was destroyed.
 */
bool TileEngine::detonate(Tile* tile)
{
	int explosive = tile->getExplosive();
	if (explosive == 0) return false; // no damage applied for this tile

	tile->setExplosive(0,true);
	bool objective = false;
	Tile* tiles[9];
	static const int parts[9]={0,1,2,0,1,2,3,3,3}; //6th is the object of current
	Position pos = tile->getPosition();

	tiles[0] = _battleSave->getTile(Position(pos.x, pos.y, pos.z+1)); //ceiling
	tiles[1] = _battleSave->getTile(Position(pos.x+1, pos.y, pos.z)); //east wall
	tiles[2] = _battleSave->getTile(Position(pos.x, pos.y+1, pos.z)); //south wall
	tiles[3] = tiles[4] = tiles[5] = tiles[6] = tile;
	tiles[7] = _battleSave->getTile(Position(pos.x, pos.y-1, pos.z)); //north bigwall
	tiles[8] = _battleSave->getTile(Position(pos.x-1, pos.y, pos.z)); //west bigwall

	// explosions create smoke which only stays 1 or 2 turns
//	tile->setSmoke(std::max(1, std::min(tile->getSmoke() + RNG::generate(0,2), 15)));

	int remainingPower, fireProof, fuel;
	bool destroyed, bigwalldestroyed = true;
	for (int i = 8; i >=0; --i)
	{
		if (!tiles[i] || !tiles[i]->getMapData(parts[i]))
			continue; //skip out of map and emptiness
		int bigwall = tiles[i]->getMapData(parts[i])->getBigWall();
		if (i > 6 && !( (bigwall==1) || (bigwall==8) || (i==8 && bigwall==6) || (i==7 && bigwall==7)))
			continue;
		if (!bigwalldestroyed && parts[i]==0) //when ground shouldn't be destroyed
			continue;
		remainingPower = explosive;
		destroyed = false;
		int volume = 0;
		int currentpart = parts[i], currentpart2, diemcd;
		fireProof = tiles[i]->getFlammability(currentpart);
		fuel = tiles[i]->getFuel(currentpart) + 1;
		// get the volume of the object by checking it's loftemps objects.
		for (int j = 0; j < 12; j++)
		{
			if (tiles[i]->getMapData(currentpart)->getLoftID(j) != 0)
				++volume;
		}
		if ( i == 6 &&
			(bigwall == 2 || bigwall == 3) && //diagonals
			(2 * tiles[i]->getMapData(currentpart)->getArmor()) > remainingPower) //not enough to destroy
		{
			bigwalldestroyed = false;
		}
		// iterate through tile armor and destroy if can
		while (	tiles[i]->getMapData(currentpart) &&
				(2 * tiles[i]->getMapData(currentpart)->getArmor()) <= remainingPower &&
				tiles[i]->getMapData(currentpart)->getArmor() != 255)
		{
			if ( i == 6 && (bigwall == 2 || bigwall == 3)) //diagonals for the current tile
			{
				bigwalldestroyed = true;
			}
			remainingPower -= 2 * tiles[i]->getMapData(currentpart)->getArmor();
			destroyed = true;
			if (_battleSave->getMissionType() == "STR_BASE_DEFENSE" &&
				tiles[i]->getMapData(currentpart)->isBaseModule())
			{
				_battleSave->getModuleMap()[tile->getPosition().x/10][tile->getPosition().y/10].second--;
			}
			//this trick is to follow transformed object parts (object can become a ground)
			diemcd = tiles[i]->getMapData(currentpart)->getDieMCD();
			if (diemcd!=0)
				currentpart2 = tiles[i]->getMapData(currentpart)->getDataset()->getObjects()->at(diemcd)->getObjectType();
			else
				currentpart2 = currentpart;
			if (tiles[i]->destroy(currentpart))
				objective = true;
			currentpart =  currentpart2;
			if (tiles[i]->getMapData(currentpart)) // take new values
			{
				fireProof = tiles[i]->getFlammability(currentpart);
				fuel = tiles[i]->getFuel(currentpart) + 1;
			}
		}
		// set tile on fire
		if ((2 * fireProof) < remainingPower)
		{
			if (tiles[i]->getMapData(MapData::O_FLOOR) || tiles[i]->getMapData(MapData::O_OBJECT))
			{
				tiles[i]->setFire(fuel);
				tiles[i]->setSmoke(std::max(1, std::min(15 - (fireProof / 10), 12)));
			}
		}
		// add some smoke if tile was destroyed and not set on fire
		if (destroyed && !tiles[i]->getFire())
		{
			int smoke = RNG::generate(1, (volume / 2) + 3) + (volume / 2);
			if (smoke > tiles[i]->getSmoke())
			{
				tiles[i]->setSmoke(std::max(0, std::min(smoke, 15)));
			}
		}
	}
	return objective;
}

/**
 * Checks for chained explosions.
 *
 * Chained explosions are explosions which occur after an explosive map object is destroyed.
 * May be due a direct hit, other explosion or fire.
 * @return, tile on which an explosion occurred
 */
Tile* TileEngine::checkForTerrainExplosions()
{
	for (int
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
		if (_battleSave->getTiles()[i]->getExplosive())
			return _battleSave->getTiles()[i];
	}

	return NULL;
}

/**
 * Opens a door (if any) by rightclick, or by walking through it. The unit has to face in the right direction.
 * @param unit			- pointer to a battleunit
 * @param rightClick	- true if the player right-clicked
 * @param dir			- direction to check for a door
 * @return, -1 there is no door, you can walk through; or you're a tank and can't do sweet shit with a door except blast the fuck out of it.
 *			 0 normal door opened, make a squeaky sound and you can walk through;
 *			 1 ufo door is starting to open, make a whoosh sound, don't walk through;
 *			 3 ufo door is still opening, don't walk through it yet. (have patience, futuristic technology...)
 *			 4 not enough TUs
 *			 5 would contravene fire reserve
 */
int TileEngine::unitOpensDoor(
		BattleUnit* unit,
		bool rightClick,
		int dir)
{
	//Log(LOG_INFO) << "unitOpensDoor()";
	int
		TUCost = 0,
		door = -1;

	int size = unit->getArmor()->getSize();
	if (size > 1
		&& rightClick)
	{
		return door;
	}

	if (dir == -1)
		dir = unit->getDirection();

	Tile* tile; // cfailde:doorcost

	int z = unit->getTile()->getTerrainLevel() < -12? 1: 0; // if we're standing on stairs, check the tile above instead.
	for (int
			x = 0;
			x < size
				&& door == -1;
			x++)
	{
		for (int
				y = 0;
				y < size
					&& door == -1;
				y++)
		{
			std::vector<std::pair<Position, int> > checkPositions;
			tile = _battleSave->getTile(
								unit->getPosition()
								+ Position(x, y, z));

			if (!tile)
				continue;

			Position posUnit = unit->getPosition(); // kL

			switch (dir)
			{
				case 0: // north
						checkPositions.push_back(std::make_pair(Position(0, 0, 0), MapData::O_NORTHWALL));	// origin
					if (x != 0)
						checkPositions.push_back(std::make_pair(Position(0,-1, 0), MapData::O_WESTWALL));	// one tile north
				break;
				case 1: // north east
						checkPositions.push_back(std::make_pair(Position(0, 0, 0), MapData::O_NORTHWALL));	// origin
						checkPositions.push_back(std::make_pair(Position(1,-1, 0), MapData::O_WESTWALL));	// one tile north-east
					if (rightClick)
					{
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_NORTHWALL));	// one tile east
					}
/*					if (rightClick
						|| testAdjacentDoor(posUnit, MapData::O_NORTHWALL, 1)) // kL
					{
						checkPositions.push_back(std::make_pair(Position(0, 0, 0), MapData::O_NORTHWALL));	// origin
						checkPositions.push_back(std::make_pair(Position(1,-1, 0), MapData::O_WESTWALL));	// one tile north-east
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_NORTHWALL));	// one tile east
					} */
				break;
				case 2: // east
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
				break;
				case 3: // south-east
					if (!y)
						checkPositions.push_back(std::make_pair(Position(1, 1, 0), MapData::O_WESTWALL));	// one tile south-east
					if (!x)
						checkPositions.push_back(std::make_pair(Position(1, 1, 0), MapData::O_NORTHWALL));	// one tile south-east
					if (rightClick)
					{
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
						checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
					}
/*					if (rightClick
						|| testAdjacentDoor(posUnit, MapData::O_NORTHWALL, 3)) // kL
					{
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
						checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
						checkPositions.push_back(std::make_pair(Position(1, 1, 0), MapData::O_WESTWALL));	// one tile south-east
						checkPositions.push_back(std::make_pair(Position(1, 1, 0), MapData::O_NORTHWALL));	// one tile south-east
					} */
				break;
				case 4: // south
						checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
				break;
				case 5: // south-west
						checkPositions.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
						checkPositions.push_back(std::make_pair(Position(-1, 1, 0), MapData::O_NORTHWALL));	// one tile south-west
					if (rightClick)
					{
						checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_WESTWALL));	// one tile south
						checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
					}
/*					if (rightClick
						|| testAdjacentDoor(posUnit, MapData::O_NORTHWALL, 5)) // kL
					{
						checkPositions.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
						checkPositions.push_back(std::make_pair(Position( 0, 1, 0), MapData::O_WESTWALL));	// one tile south
						checkPositions.push_back(std::make_pair(Position( 0, 1, 0), MapData::O_NORTHWALL));	// one tile south
						checkPositions.push_back(std::make_pair(Position(-1, 1, 0), MapData::O_NORTHWALL));	// one tile south-west
					} */
				break;
				case 6: // west
						checkPositions.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
					if (y != 0)
						checkPositions.push_back(std::make_pair(Position(-1, 0, 0), MapData::O_NORTHWALL));	// one tile west
				break;
				case 7: // north-west
						checkPositions.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
						checkPositions.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_NORTHWALL));	// origin
					if (x)
						checkPositions.push_back(std::make_pair(Position(-1,-1, 0), MapData::O_WESTWALL));	// one tile north
					if (y)
						checkPositions.push_back(std::make_pair(Position(-1,-1, 0), MapData::O_NORTHWALL));	// one tile north
					if (rightClick)
					{
						checkPositions.push_back(std::make_pair(Position( 0,-1, 0), MapData::O_WESTWALL));	// one tile north
						checkPositions.push_back(std::make_pair(Position(-1, 0, 0), MapData::O_NORTHWALL));	// one tile west
					}
/*					if (rightClick
						|| testAdjacentDoor(posUnit, MapData::O_NORTHWALL, 7)) // kL
					{
						//Log(LOG_INFO) << ". north-west";
						checkPositions.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_WESTWALL));	// origin
						checkPositions.push_back(std::make_pair(Position( 0, 0, 0), MapData::O_NORTHWALL));	// origin
						checkPositions.push_back(std::make_pair(Position( 0,-1, 0), MapData::O_WESTWALL));	// one tile north
						checkPositions.push_back(std::make_pair(Position(-1, 0, 0), MapData::O_NORTHWALL));	// one tile west
					} */
				break;

				default:
				break;
			}


			int part = 0;

			for (std::vector<std::pair<Position, int> >::const_iterator
					i = checkPositions.begin();
					i != checkPositions.end()
						&& door == -1;
					++i)
			{
				tile = _battleSave->getTile(
									posUnit
										+ Position(x, y, z)
										+ i->first);
				if (tile)
				{
					door = tile->openDoor(
										i->second,
										unit,
										_battleSave->getBattleGame()->getReservedAction());
					if (door != -1)
					{
						part = i->second;

						if (door == 1)
						{
							openAdjacentDoors(
											posUnit
												+ Position(x, y, z)
												+ i->first,
											i->second);
						}
					}
				}
			}

			if (door == 0
				&& rightClick)
			{
				if (part == MapData::O_WESTWALL)
					part = MapData::O_NORTHWALL;
				else
					part = MapData::O_WESTWALL;

				TUCost = tile->getTUCost(
										part,
										unit->getArmor()->getMovementType());
				//Log(LOG_INFO) << ". normal door, RMB, part = " << part << ", TUcost = " << TUCost;
			}
			else if (door == 1
				|| door == 4)
			{
				TUCost = tile->getTUCost(
										part,
										unit->getArmor()->getMovementType());
				//Log(LOG_INFO) << ". UFO door, part = " << part << ", TUcost = " << TUCost;
			}
		}
	}

	if (TUCost != 0)
	{
		if (_battleSave->getBattleGame()->checkReservedTU(unit, TUCost))
		{
			if (unit->spendTimeUnits(TUCost))
			{
				tile->animate(); // cfailde:doorcost : ensures frame advances for ufo doors to update TU cost

				if (rightClick) // kL: try this one ...... <--- let UnitWalkBState handle FoV & new unit visibility, when walking (ie, not RMB).
				{
					_battleSave->getBattleGame()->checkForProximityGrenades(unit); // kL

					calculateFOV(unit->getPosition()); // calculate FoV for everyone within sight-range, incl. unit.

					// look from the other side (may need check reaction fire)
					// kL_note: This seems redundant, but hey maybe it removes now-unseen units from a unit's visible-units vector ....
					std::vector<BattleUnit*>* visUnits = unit->getVisibleUnits();
					for (size_t
							i = 0;
							i < visUnits->size();
							++i)
					{
						calculateFOV(visUnits->at(i)); // calculate FoV for all units that are visible to this unit.
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
 * kL. Checks for a door connected to a wall at this position,
 * so that units can open double doors diagonally.
 * @param pos	- the starting position
 * @param part	- the wall to test
 * @param dir	- the direction to check out
 */
bool TileEngine::testAdjacentDoor( // kL
		Position pos,
		int part,
		int dir)
{
	Position offset;
	switch (dir)
	{
		// only Northwall-doors are handled at present
		case 1: // northwall in tile to east
			offset = Position(1, 0, 0);
		break;
		case 3: // northwall in tile to south-east
			offset = Position(1, 1, 0);
		break;
		case 5: // northwall in tile to south-west
			offset = Position(-1, 1, 0);
		break;
		case 7: // northwall in tile to west
			offset = Position(-1, 0, 0);
		break;

		default:
		break;
	}

	Tile* tile = _battleSave->getTile(pos + offset);
	if (tile
		&& tile->getMapData(part)
		&& tile->getMapData(part)->isUFODoor())
	{
		return true;
	}

	return false;
}

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
	bool westSide = (part == 1);

	for (int
			i = 1;
			;
			++i)
	{
		offset = westSide? Position(0, i, 0): Position(i, 0, 0);
		Tile* tile = _battleSave->getTile(pos + offset);
		if (tile
			&& tile->getMapData(part)
			&& tile->getMapData(part)->isUFODoor())
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
		Tile* tile = _battleSave->getTile(pos + offset);
		if (tile
			&& tile->getMapData(part)
			&& tile->getMapData(part)->isUFODoor())
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

	for (int // prepare a list of tiles on fire/smoke & close any ufo doors
			i = 0;
			i < _battleSave->getMapSizeXYZ();
			++i)
	{
		if (_battleSave->getTiles()[i]->getUnit()
			&& _battleSave->getTiles()[i]->getUnit()->getArmor()->getSize() > 1)
		{
			BattleUnit* bu = _battleSave->getTiles()[i]->getUnit();

			Tile* tile = _battleSave->getTiles()[i];
			Tile* tileNorth = _battleSave->getTile(tile->getPosition() + Position(0,-1, 0));
			Tile* tileWest = _battleSave->getTile(tile->getPosition() + Position(-1, 0, 0));
			if ((tile->isUfoDoorOpen(MapData::O_NORTHWALL)
					&& tileNorth
					&& tileNorth->getUnit()
					&& tileNorth->getUnit() == bu)
				|| (tile->isUfoDoorOpen(MapData::O_WESTWALL)
					&& tileWest
					&& tileWest->getUnit()
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
 * @param origin			- reference to the origin (voxelspace for 'doVoxelCheck'; tilespace otherwise)
 * @param target			- reference to the target (voxelspace for 'doVoxelCheck'; tilespace otherwise)
 * @param storeTrajectory	- true will store the whole trajectory - otherwise it just stores the last position
 * @param trajectory		- pointer to a vector of positions in which the trajectory will be stored
 * @param excludeUnit		- pointer to a unit to be excluded from collision detection
 * @param doVoxelCheck		- check against voxel or tile blocking? (first one for unit visibility and line of fire, second one for terrain visibility)
 * @param onlyVisible		- skip invisible units? used in FPS view
 * @param excludeAllBut		- pointer to a unit that's to be considered exclusively for ray hits [optional]
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
				bool storeTrajectory,
				std::vector<Position>* trajectory,
				BattleUnit* excludeUnit,
				bool doVoxelCheck, // false is used only for calculateFOV()
				bool onlyVisible,
				BattleUnit* excludeAllBut)
{
	//Log(LOG_INFO) << "TileEngine::calculateLine()";
	int
		x, x0, x1,
		delta_x, step_x,

		y, y0, y1,
		delta_y, step_y,

		z, z0, z1,
		delta_z, step_z,

		swap_xy, swap_xz,

		drift_xy, drift_xz,

		cx, cy, cz,

		result, horiBlock, vertBlock;

	Position lastPoint (origin);

	x0 = origin.x; // start & end points
	x1 = target.x;

	y0 = origin.y;
	y1 = target.y;

	z0 = origin.z;
	z1 = target.z;

	swap_xy = abs(y1 - y0) > abs(x1 - x0); // 'steep' xy Line, make longest delta x plane
	if (swap_xy)
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
	}

	swap_xz = abs(z1 - z0) > abs(x1 - x0); // do same for xz
	if (swap_xz)
	{
		std::swap(x0, z0);
		std::swap(x1, z1);
	}

	delta_x = abs(x1 - x0); // delta is Length in each plane
	delta_y = abs(y1 - y0);
	delta_z = abs(z1 - z0);

	drift_xy = delta_x / 2; // drift controls when to step in 'shallow' planes;
	drift_xz = delta_x / 2; // starting value keeps Line centered

	step_x = 1; // direction of Line
	if (x0 > x1)
		step_x = -1;

	step_y = 1;
	if (y0 > y1)
		step_y = -1;

	step_z = 1;
	if (z0 > z1)
		step_z = -1;

	y = y0; // starting point
	z = z0;

	for ( // step through longest delta (which we have swapped to x)
			x = x0;
			x != x1 + step_x;
			x += step_x)
	{
		cx = x; // copy position
		cy = y;
		cz = z;
		if (swap_xz) // unswap (in reverse)
			std::swap(cx, cz);
		if (swap_xy)
			std::swap(cx, cy);

		if (storeTrajectory
			&& trajectory)
		{
			trajectory->push_back(Position(cx, cy, cz));
		}

		if (doVoxelCheck) // passes through this voxel, for Unit visibility & LoS/LoF
		{
			result = voxelCheck(
							Position(cx, cy, cz),
							excludeUnit,
							false,
							onlyVisible,
							excludeAllBut);
			if (result != VOXEL_EMPTY) // hit.
			{
				if (trajectory) // store the position of impact
					trajectory->push_back(Position(cx, cy, cz));

				//Log(LOG_INFO) << "TileEngine::calculateLine() [1]ret = " << result;
				return result;
			}
		}
		else // for Terrain visibility, ie. FoV / Fog of War.
		{
			Tile* startTile = _battleSave->getTile(lastPoint);
			Tile* endTile = _battleSave->getTile(Position(cx, cy, cz));

			horiBlock = horizontalBlockage(
									startTile,
									endTile,
									DT_NONE);
			vertBlock = verticalBlockage(
									startTile,
									endTile,
									DT_NONE);
			// kL_TEST:
//			BattleUnit* selUnit = _battleSave->getSelectedUnit();
/*			if (selUnit
				&& selUnit->getId() == 375
				&& startTile != endTile)
//				&& selUnit->getFaction() == FACTION_PLAYER
//				&& _battleSave->getDebugMode())
//				&& _battleSave->getTurn() > 1)
			{
				Position posUnit = selUnit->getPosition();
				if ((posUnit.x == cx
						&& abs(posUnit.y - cy) > 4)
					|| (posUnit.y == cy
						&& abs(posUnit.x - cx) > 4))
				{
					Log(LOG_INFO) << "start " << lastPoint << " hori = " << result;
					Log(LOG_INFO) << ". end " << Position(cx, cy, cz) << " vert = " << result2;
				}
			} */ // kL_TEST_end.

			if (horiBlock < 0) // hit content-object
			{
				if (vertBlock > 0)
					horiBlock = 0;
				else
					return horiBlock;
			}

			horiBlock += vertBlock;
			if (horiBlock > 0)
				return horiBlock;

			lastPoint = Position(cx, cy, cz);
		}

		drift_xy = drift_xy - delta_y; // update progress in other planes
		drift_xz = drift_xz - delta_z;

		if (drift_xy < 0) // step in y plane
		{
			y = y + step_y;
			drift_xy = drift_xy + delta_x;

			if (doVoxelCheck) // check for xy diagonal intermediate voxel step, for Unit visibility
			{
				cx = x;
				cz = z;
				cy = y;
				if (swap_xz)
					std::swap(cx, cz);
				if (swap_xy)
					std::swap(cx, cy);

				result = voxelCheck(
								Position(cx, cy, cz),
								excludeUnit,
								false,
								onlyVisible,
								excludeAllBut);
				if (result != VOXEL_EMPTY)
				{
					if (trajectory != 0)
						trajectory->push_back(Position(cx, cy, cz)); // store the position of impact

					//Log(LOG_INFO) << "TileEngine::calculateLine() [2]ret = " << result;
					return result;
				}
			}
		}

		if (drift_xz < 0) // same in z
		{
			z = z + step_z;
			drift_xz = drift_xz + delta_x;

			if (doVoxelCheck) // check for xz diagonal intermediate voxel step
			{
				cx = x;
				cz = z;
				cy = y;
				if (swap_xz)
					std::swap(cx, cz);
				if (swap_xy)
					std::swap(cx, cy);

				result = voxelCheck(
								Position(cx, cy, cz),
								excludeUnit,
								false,
								onlyVisible,
								excludeAllBut);
				if (result != VOXEL_EMPTY)
				{
					if (trajectory != 0) // store the position of impact
						trajectory->push_back(Position(cx, cy, cz));

					//Log(LOG_INFO) << "TileEngine::calculateLine() [3]ret = " << result;
					return result;
				}
			}
		}
	}

	//Log(LOG_INFO) << ". EXIT ret -1";
	return VOXEL_EMPTY;
}

/**
 * Calculates a parabola trajectory, used for throwing items.
 * @param origin			- reference to the origin in voxelspace
 * @param target			- reference to the target in voxelspace
 * @param storeTrajectory	- true will store the whole trajectory - otherwise it stores the last position only
 * @param trajectory		- poniter to a vector of positions in which the trajectory will be stored
 * @param excludeUnit		- pointer to a unit to exclude - makes sure the trajectory does not hit the shooter itself
 * @param arc				- how high the parabola goes: 1.0 is almost straight throw, 3.0 is a very high throw, to throw over a fence for example
 * @param acu				- the deviation of the angles that should be taken into account. 1.0 is perfection. // now superceded by @param delta...
 * @param delta				- the deviation of the angles that should be taken into account, (0,0,0) is perfection
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
				bool storeTrajectory,
				std::vector<Position>* trajectory,
				BattleUnit* excludeUnit,
				double arc,
				const Position delta)
{
	//Log(LOG_INFO) << "TileEngine::calculateParabola()";
	double
		ro = sqrt(static_cast<double>(
				  (target.x - origin.x) * (target.x - origin.x)
				+ (target.y - origin.y) * (target.y - origin.y)
				+ (target.z - origin.z) * (target.z - origin.z))),
		fi = acos(static_cast<double>(target.z - origin.z) / ro),
		te = atan2(
				static_cast<double>(target.y - origin.y),
				static_cast<double>(target.x - origin.x));

	if (AreSame(ro, 0.0)) // just in case.
		return VOXEL_EMPTY;

//	te *= acu;
//	fi *= acu;
	te += (delta.x / ro) / 2.0 * M_PI;						// horizontal magic value
	fi += ((delta.z + delta.y) / ro) / 14.0 * M_PI * arc;	// another magic value (vertical), to make it in line with fire spread

	double
		zA = sqrt(ro) * arc,
		zK = 4.0 * zA / ro / ro;

	int
		x = origin.x,
		y = origin.y,
		z = origin.z,
		i = 8;

	Position lastPosition = Position(x, y, z);

	while (z > 0)
	{
		x = static_cast<int>(static_cast<double>(origin.x) + static_cast<double>(i) * cos(te) * sin(fi));
		y = static_cast<int>(static_cast<double>(origin.y) + static_cast<double>(i) * sin(te) * sin(fi));
		z = static_cast<int>(static_cast<double>(origin.z) + static_cast<double>(i) * cos(fi)
				- zK * (static_cast<double>(i) - ro / 2.0) * (static_cast<double>(i) - ro / 2.0) + zA);

		if (storeTrajectory
			&& trajectory)
		{
			trajectory->push_back(Position(x, y, z));
		}

		Position nextPosition = Position(x, y, z);
		int test = calculateLine(
								lastPosition,
								nextPosition,
								false,
								NULL,
								excludeUnit);
//		int test = voxelCheck(
//							Position(x, y, z),
//							excludeUnit);
		if (test != VOXEL_EMPTY)
		{
			if (lastPosition.z < nextPosition.z)
				test = VOXEL_OUTOFBOUNDS;

			if (!storeTrajectory // store only the position of impact
				&& trajectory != NULL)
			{
				trajectory->push_back(nextPosition);
			}

			return test;
		}

		lastPosition = Position(x, y, z);
		++i;
	}

	if (!storeTrajectory // store only the position of impact
		&& trajectory != NULL)
	{
		trajectory->push_back(Position(x, y, z));
	}

	return VOXEL_EMPTY;
}

/**
 * Validates a throw action.
 * @param action		- reference to the action to validate
 * @param originVoxel	- the origin point of the action
 * @param targetVoxel	- the target point of the action
 * @param curve			- pointer to a curvature of the throw
 * @param voxelType		- pointer to a type of voxel at which this parabola terminates
 * @return, true if action is valid
 */
bool TileEngine::validateThrow(
						BattleAction& action,
						Position originVoxel,
						Position targetVoxel,
						double* curve,
						int* voxelType)
{
	//Log(LOG_INFO) << "TileEngine::validateThrow(), cf Projectile::calculateThrow()";
//kL	double arc = 0.5;
	double arc = 1.12; // kL, for acid spit only ....... throw done next.

	if (action.type == BA_THROW)
	{
		double kneel = 0.0;
		if (action.actor->isKneeled())
			kneel = 0.15;

		arc = std::max( // kL_note: this is for throwing (instead of spitting...)
					0.48,
					1.73 / sqrt(
								sqrt(
									static_cast<double>(action.actor->getStats()->strength)
										/ static_cast<double>(action.weapon->getRules()->getWeight())))
							+ kneel);
	}
	//Log(LOG_INFO) << ". starting arc = " << arc;

	Tile* targetTile = _battleSave->getTile(action.target);
	if (/*kL (action.type == BA_THROW
			&& targetTile
			&& targetTile->getMapData(MapData::O_OBJECT)
			&& targetTile->getMapData(MapData::O_OBJECT)->getTUCost(MT_WALK) == 255) || */
		ProjectileFlyBState::validThrowRange(
											&action,
											originVoxel,
											targetTile)
										== false)
	{
		//Log(LOG_INFO) << ". . ret FALSE, validThrowRange not valid OR targetTile is nonwalkable.";
		return false; // object blocking - can't throw here
	}

	// try several different curvatures to reach our goal.
	bool found = false;
	while (!found
		&& arc < 5.0)
	{
		int test = VOXEL_OUTOFBOUNDS;
		std::vector<Position> trajectory;
		test = calculateParabola(
							originVoxel,
							targetVoxel,
							false,
							&trajectory,
							action.actor,
							arc,
							Position(0, 0, 0));
		if (test != VOXEL_OUTOFBOUNDS
			&& (trajectory.at(0) / Position(16, 16, 24)) == (targetVoxel / Position(16, 16, 24)))
		{
			//Log(LOG_INFO) << ". . . Loop arc = " << arc;

			if (voxelType)
				*voxelType = test;

			found = true;
		}
		else
			arc += 0.5;
	}

	if (arc >= 5.0)
		//Log(LOG_INFO) << ". . ret FALSE, arc > 5";
		return false;

	if (curve)
		*curve = arc;

	//Log(LOG_INFO) << ". ret TRUE";
	return true;
}

/**
 * Calculates z "grounded" value for a particular voxel (used for projectile shadow).
 * @param voxel The voxel to trace down.
 * @return z coord of "ground".
 */
int TileEngine::castedShade(const Position& voxel)
{
	int zstart = voxel.z;
	Position tmpCoord = voxel / Position(16, 16, 24);

	Tile* t = _battleSave->getTile(tmpCoord);
	while (t
		&& t->isVoid()
		&& !t->getUnit())
	{
		zstart = tmpCoord.z * 24;
		--tmpCoord.z;

		t = _battleSave->getTile(tmpCoord);
	}

	Position tmpVoxel = voxel;

	int z;
	for (
			z = zstart;
			z > 0;
			z--)
	{
		tmpVoxel.z = z;

		if (voxelCheck(tmpVoxel, 0) != VOXEL_EMPTY)
			break;
	}

	return z;
}

/**
 * Traces voxel visibility.
 * @param voxel, Voxel coordinates.
 * @return, True if visible.
 */
bool TileEngine::isVoxelVisible(const Position& voxel)
{
	int zstart = voxel.z + 3; // slight Z adjust
	if (zstart / 24 != voxel.z / 24)
		return true; // visible!

	Position tmpVoxel = voxel;

	int zend = (zstart/24) * 24 + 24;
	for (int // only OBJECT can cause additional occlusion (because of any shape)
			z = zstart;
			z < zend;
			z++)
	{
		tmpVoxel.z = z;
		if (voxelCheck(tmpVoxel, 0) == VOXEL_OBJECT)
			return false;

		++tmpVoxel.x;
		if (voxelCheck(tmpVoxel, 0) == VOXEL_OBJECT)
			return false;

		++tmpVoxel.y;
		if (voxelCheck(tmpVoxel, 0) == VOXEL_OBJECT)
			return false;
	}

	return true;
}

/**
 * Checks if we hit a targetPos in voxel space.
 * @param targetPos			- reference to the voxel to check
 * @param excludeUnit		- pointer to unit NOT to do checks for
 * @param excludeAllUnits	- true to NOT do checks on any unit
 * @param onlyVisible		- true to consider only visible units
 * @param excludeAllBut		- pointer to an only unit to be considered
 * @param hit				- true if no projectile, trajectory, or any of this is needed - kL
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
		BattleUnit* excludeUnit,
		bool excludeAllUnits,
		bool onlyVisible,
		BattleUnit* excludeAllBut,
		bool hit) // kL add.
{
	//Log(LOG_INFO) << "TileEngine::voxelCheck()"; // massive lag-to-file, Do not use.
	if (hit)
		return VOXEL_UNIT; // kL; i think Wb may have this covered now.


	Tile* targetTile = _battleSave->getTile(targetPos / Position(16, 16, 24)); // converts to tilespace -> Tile
	//Log(LOG_INFO) << ". targetTile " << targetTile->getPosition();
	// check if we are out of the map
	if (targetTile == NULL
		|| targetPos.x < 0
		|| targetPos.y < 0
		|| targetPos.z < 0)
	{
		//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret 5";
		return VOXEL_OUTOFBOUNDS;
	}

	Tile* belowTile = _battleSave->getTile(targetTile->getPosition() + Position(0, 0,-1));
	if (targetTile->isVoid()
		&& targetTile->getUnit() == NULL
		&& (!belowTile
			|| belowTile->getUnit() == NULL))
	{
		//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret(1) -1";
		return VOXEL_EMPTY;
	}

	// kL_note: should allow items to be thrown through a gravLift down to the floor below
	if ((targetPos.z %24 == 0
			|| targetPos.z %24 == 1)
		&& targetTile->getMapData(MapData::O_FLOOR)
		&& targetTile->getMapData(MapData::O_FLOOR)->isGravLift())
	{
		//Log(LOG_INFO) << "voxelCheck() isGravLift";
		//Log(LOG_INFO) << ". level = " << targetTile->getPosition().z;

		if (targetTile->getPosition().z == 0
			|| (belowTile
				&& belowTile->getMapData(MapData::O_FLOOR)
				&& !belowTile->getMapData(MapData::O_FLOOR)->isGravLift()))
		{
			//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret 0";
			//Log(LOG_INFO) << ". . EXIT, ret Voxel_Floor";
			return VOXEL_FLOOR;
		}
	}

	// first we check TERRAIN tile/voxel data,
	// not to allow 2x2 units stick through walls <- English pls??
	for (int // terrain parts (floor, 2x walls, & content-object)
			i = 0;
			i < 4;
			++i)
	{
		if (targetTile->isUfoDoorOpen(i))
			continue;

		MapData* dataTarget = targetTile->getMapData(i);
		if (dataTarget)
		{
			int x = 15 - targetPos.x %16;	// x-direction is reversed
			int y = targetPos.y %16;		// y-direction is standard

			int LoftIdx = ((dataTarget->getLoftID((targetPos.z %24) / 2) * 16) + y); // wtf
			if (_voxelData->at(LoftIdx) & (1 << x))
			{
				//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret i = " << i;
				return i;
			}
		}
	}

	if (!excludeAllUnits)
	{
		BattleUnit* buTarget = targetTile->getUnit();
		// sometimes there is unit on the tile below, but sticks up into this tile with its head.
		if (buTarget == NULL
			&& targetTile->hasNoFloor(0))
		{
			targetTile = _battleSave->getTile(Position( // tileBelow
										targetPos.x / 16,
										targetPos.y / 16,
										targetPos.z / 24 - 1));
			if (targetTile)
				buTarget = targetTile->getUnit();
		}

		if (buTarget
			&& buTarget != excludeUnit
			&& (!excludeAllBut || buTarget == excludeAllBut)
			&& (!onlyVisible || buTarget->getVisible()))
		{
			Position pTarget_bu = buTarget->getPosition();
			int tz = (pTarget_bu.z * 24) + buTarget->getFloatHeight() - targetTile->getTerrainLevel(); // floor-level voxel

			if (targetPos.z > tz
				&& targetPos.z <= tz + buTarget->getHeight()) // if hit is between foot- and hair-level voxel layers (z-axis)
			{
				int entry = 0;

				int x = targetPos.x %16; // where on the x-axis
				int y = targetPos.y %16; // where on the y-axis
					// That should be (8,8,10) as per BattlescapeGame::handleNonTargetAction(), if (_currentAction.type == BA_HIT)

				if (buTarget->getArmor()->getSize() > 1) // for large units...
				{
					Position pTarget_tile = targetTile->getPosition();
					entry = ((pTarget_tile.x - pTarget_bu.x) + ((pTarget_tile.y - pTarget_bu.y) * 2));
				}

				int LoftIdx = ((buTarget->getLoftemps(entry) * 16) + y);
				//Log(LOG_INFO) << "LoftIdx = " << LoftIdx;
				if (_voxelData->at(LoftIdx) & (1 << x))
					// if the voxelData at LoftIdx is "1" solid:
					//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret 4";
					return VOXEL_UNIT;
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret(2) -1"; // massive lag-to-file, Do not use.
	return VOXEL_EMPTY;
}

/**
 * Calculates the distance between 2 points. Rounded down to first INT.
 * @param pos1, Position of first square.
 * @param pos2, Position of second square.
 * @return, Distance.
 */
int TileEngine::distance(
		const Position& pos1,
		const Position& pos2) const
{
	int x = pos1.x - pos2.x;
	int y = pos1.y - pos2.y;
	int z = pos1.z - pos2.z; // kL

	return static_cast<int>(
						Round(
							sqrt(static_cast<double>(x * x + y * y + z * z)))); // kL: 3-d
}

/**
 * Calculates the distance squared between 2 points.
 * No sqrt(), not floating point math, and sometimes it's all you need.
 * @param pos1, Position of first square.
 * @param pos2, Position of second square.
 * @param considerZ, Whether to consider the z coordinate.
 * @return, Distance.
 */
int TileEngine::distanceSq(
		const Position& pos1,
		const Position& pos2,
		bool considerZ) const
{
	int x = pos1.x - pos2.x;
	int y = pos1.y - pos2.y;
	int z = considerZ? (pos1.z - pos2.z): 0;

	return x * x + y * y + z * z;
}

/**
 * Attempts a panic or mind-control action.
 * @param action - pointer to a BattleAction (BattlescapeGame.h)
 * @return, true if attack succeeds
 */
bool TileEngine::psiAttack(BattleAction* action)
{
	//Log(LOG_INFO) << "TileEngine::psiAttack()";
	//Log(LOG_INFO) << ". attackerID " << action->actor->getId();
	Tile* tile = _battleSave->getTile(action->target);
	if (tile
		&& tile->getUnit())
	{
		//Log(LOG_INFO) << ". . tile EXISTS, so does Unit";
		BattleUnit* victim = tile->getUnit();
		//Log(LOG_INFO) << ". . defenderID " << victim->getId();
		//Log(LOG_INFO) << ". . target(pos) " << action->target;

		double attack = static_cast<double>(action->actor->getStats()->psiStrength)
							* static_cast<double>(action->actor->getStats()->psiSkill)
							/ 50.0;
		//Log(LOG_INFO) << ". . . attack = " << (int)attack;

		double defense = static_cast<double>(victim->getStats()->psiStrength)
							+ (static_cast<double>(victim->getStats()->psiSkill)
								/ 5.0);
		//Log(LOG_INFO) << ". . . defense = " << (int)defense;

		double dist = static_cast<double>(distance(
											action->actor->getPosition(),
											action->target));
		//Log(LOG_INFO) << ". . . dist = " << dist;

//kL	attack -= dist;
		attack -= dist * 2.0; // kL

		attack -= defense;
		if (action->type == BA_MINDCONTROL)
//kL		attack += 25.0;
			attack += 15.0; // kL
		else
			attack += 45.0;

		attack *= 100.0;
		attack /= 56.0;

		if (action->actor->getOriginalFaction() == FACTION_PLAYER)
			action->actor->addPsiSkillExp();

		if (victim->getOriginalFaction() == FACTION_PLAYER
			&& Options::allowPsiStrengthImprovement)
		{
			victim->addPsiStrengthExp();
		}

		int success = static_cast<int>(attack);

		std::string info = "";
		if (action->type == BA_PANIC)
			info = "STR_PANIC";
		else
			info = "STR_CONTROL";

		_battleSave->getBattleState()->warning(
											info,
											true,
											success);

		//Log(LOG_INFO) << ". . . attack Success @ " << success;
		if (RNG::percent(success))
		{
			//Log(LOG_INFO) << ". . Success";
			if (action->actor->getOriginalFaction() == FACTION_PLAYER)
				action->actor->addPsiSkillExp(2);

			if (action->type == BA_PANIC)
			{
				//Log(LOG_INFO) << ". . . action->type == BA_PANIC";

				int moraleLoss = 110
								- victim->getStats()->bravery * 2
								+ action->actor->getStats()->psiStrength / 2;
				if (moraleLoss > 0)
					victim->moraleChange(-moraleLoss);
			}
			else //if (action->type == BA_MINDCONTROL)
			{
				//Log(LOG_INFO) << ". . . action->type == BA_MINDCONTROL";
				victim->convertToFaction(action->actor->getFaction());
				victim->setTimeUnits(victim->getStats()->tu);
				victim->setEnergy(victim->getStats()->stamina); // kL
				victim->allowReselect();
				victim->setStatus(STATUS_STANDING);

				calculateUnitLighting();
				calculateFOV(victim->getPosition());

				// if all units from either faction are mind controlled - auto-end the mission.
				if (_battleSave->getSide() == FACTION_PLAYER
					&& Options::battleAutoEnd
					&& Options::allowPsionicCapture)
				{
					//Log(LOG_INFO) << ". . . . inside tallyUnits";

					int liveAliens = 0;
					int liveSoldiers = 0;

					_battleSave->getBattleGame()->tallyUnits(
														liveAliens,
														liveSoldiers);

					if (liveAliens == 0
						|| liveSoldiers == 0)
					{
						_battleSave->setSelectedUnit(0);
						_battleSave->getBattleGame()->cancelCurrentAction(true);

						_battleSave->getBattleGame()->requestEndTurn();
					}
				}
				//Log(LOG_INFO) << ". . . tallyUnits DONE";
			}
			//Log(LOG_INFO) << "TileEngine::psiAttack() ret TRUE";
			return true;
		}
		else if (victim->getOriginalFaction() == FACTION_PLAYER
			&& Options::allowPsiStrengthImprovement)
		{
			victim->addPsiStrengthExp(2);
		}
	}
	//Log(LOG_INFO) << "TileEngine::psiAttack() ret FALSE";
	return false;
}

/**
 * Applies gravity to a tile. Causes items and units to drop.
 * @param tile - pointer to a tile from which stuff is going to drop
 * @return, pointer to the tile where stuff eventually ends up
 */
Tile* TileEngine::applyGravity(Tile* tile)
{
	if (tile == NULL)
		return NULL;

	Position pos = tile->getPosition();
	if (pos.z == 0)
		return tile;

	BattleUnit* unit = tile->getUnit();
	bool hasNoItems = tile->getInventory()->empty();

	if (unit == NULL
		&& hasNoItems == true)
	{
		return tile;
	}

	Tile
		* dt = tile,
		* dtb = NULL;
	Position posBelow = pos;

	if (unit)
	{
		while (posBelow.z > 0)
		{
			bool canFall = true;

			for (int
					y = 0;
					y < unit->getArmor()->getSize()
						&& canFall;
					++y)
			{
				for (int
						x = 0;
						x < unit->getArmor()->getSize()
							&& canFall;
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
					if (!dt->hasNoFloor(dtb))	// note: polar water has no floor, so units that die on them ... uh, sink.
						canFall = false;		// ... before I changed the loop condition to > 0, that is
				}
			}

			if (canFall == false)
				break;

			posBelow.z--;
		}

		if (posBelow != pos)
		{
			if (unit->isOut())
			{
				for (int
						y = unit->getArmor()->getSize() - 1;
						y > -1;
						--y)
				{
					for (int
							x = unit->getArmor()->getSize() - 1;
							x > -1;
							--x)
					{
						_battleSave->getTile(pos + Position(x, y, 0))->setUnit(NULL);
					}
				}

				unit->setPosition(posBelow);
			}
			else // if (!unit->isOut(true, true))
			{
				if (unit->getArmor()->getMovementType() == MT_FLY)
				{
					// move to the position you're already in. this will unset the kneeling flag, set the floating flag, etc.
					unit->startWalking(
									unit->getDirection(),
									unit->getPosition(),
									_battleSave->getTile(unit->getPosition() + Position(0, 0,-1)),
									true);
					// and set our status to standing (rather than walking or flying) to avoid weirdness.
					unit->setStatus(STATUS_STANDING);
				}
				else
				{
					unit->startWalking(
									Pathfinding::DIR_DOWN,
									unit->getPosition() + Position(0, 0,-1),
									_battleSave->getTile(unit->getPosition() + Position(0, 0,-1)),
									true);
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

		posBelow.z--;
	}

	if (posBelow != pos)
	{
		dt = _battleSave->getTile(posBelow);

		if (hasNoItems == false)
		{
			for (std::vector<BattleItem*>::iterator
					i = tile->getInventory()->begin();
					i != tile->getInventory()->end();
					++i)
			{
				if ((*i)->getUnit()) // corpse
//					&& tile->getPosition() == (*i)->getUnit()->getPosition())
//				{
					(*i)->getUnit()->setPosition(dt->getPosition());
//				}

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
		BattleUnit* attacker,
		BattleUnit* target,
		int dir)
{
	return validMeleeRange(
						attacker->getPosition(),
						dir,
						attacker,
						target,
						0);
}

/**
 * Validates the melee range between a tile and a unit.
 * @param pos		- position to check from
 * @param dir		- direction to check
 * @param attacker	- pointer to an attacking unit
 * @param target	- pointer to the unit to attack (NULL = any unit)
 * @param dest		- pointer to destination position
 * @return, true if range is valid
 */
bool TileEngine::validMeleeRange(
		Position pos,
		int dir,
		BattleUnit* attacker,
		BattleUnit* target,
		Position* dest)
{
	//Log(LOG_INFO) << "TileEngine::validMeleeRange()";
	if (dir < 0 || 7 < dir)
	{
		//Log(LOG_INFO) << ". dir inValid, EXIT false";
		return false;
	}
	//Log(LOG_INFO) << ". dir = " << dir;


	Position posTarget;
	Pathfinding::directionToVector(
								dir,
								&posTarget);
	Tile
		* tileOrigin,
		* tileTarget,
		* tileTarget_above,
		* tileTarget_below;

	int size = attacker->getArmor()->getSize() - 1;
	for (int
			x = 0;
			x <= size;
			++x)
	{
		for (int
				y = 0;
				y <= size;
				++y)
		{
			//Log(LOG_INFO) << ". iterate over Size";

			tileOrigin = _battleSave->getTile(Position(pos + Position(x, y, 0)));
			tileTarget = _battleSave->getTile(Position(pos + Position(x, y, 0) + posTarget));

			if (tileOrigin
				&& tileTarget)
			{
				//Log(LOG_INFO) << ". tile Origin & Target VALID";

				tileTarget_above = _battleSave->getTile(Position(pos + Position(x, y, 1) + posTarget));
				tileTarget_below = _battleSave->getTile(Position(pos + Position(x, y,-1) + posTarget));

				if (!tileTarget->getUnit()) // kL
				{
					//Log(LOG_INFO) << ". . no targetUnit";

//kL				if (tileOrigin->getTerrainLevel() <= -16
					if (tileTarget_above // kL_note: standing on a rise only 1/3 up z-axis reaches adjacent tileAbove.
						&& tileOrigin->getTerrainLevel() < -7) // kL
//kL					&& !tileTarget_above->hasNoFloor(tileTarget)) // kL_note: floaters...
					{
						//Log(LOG_INFO) << ". . . targetUnit on tileAbove";

						tileTarget = tileTarget_above;
					}
					else if (tileTarget_below // kL_note: can reach target standing on a rise only 1/3 up z-axis on adjacent tileBelow.
						&& tileTarget->hasNoFloor(tileTarget_below)
//kL					&& tileTarget_below->getTerrainLevel() <= -16)
						&& tileTarget_below->getTerrainLevel() < -7) // kL
					{
						//Log(LOG_INFO) << ". . . targetUnit on tileBelow";

						tileTarget = tileTarget_below;
					}
				}

				if (tileTarget->getUnit())
				{
					//Log(LOG_INFO) << ". . targeted tileUnit is valid ID = " << tileTarget->getUnit()->getId();
					if (target == NULL
						|| target == tileTarget->getUnit())
					{
						//Log(LOG_INFO) << ". . . target and tileUnit are same";
						Position voxelOrigin = Position(tileOrigin->getPosition() * Position(16, 16, 24))
												+ Position(
														8,
														8,
														attacker->getHeight()
															+ attacker->getFloatHeight()
															- tileOrigin->getTerrainLevel()
															- 4);

						Position voxelTarget;
						if (canTargetUnit(
										&voxelOrigin,
										tileTarget,
										&voxelTarget,
										attacker))
						{
							if (dest)
								*dest = tileTarget->getPosition();

							//Log(LOG_INFO) << "TileEngine::validMeleeRange() EXIT true";
							return true;
						}
					}
				}
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::validMeleeRange() EXIT false";
	return false;
}

/**
 * Gets the AI to look through a window.
 * @param position	- Reference to the current position
 * @return, Direction or -1 when no window found.
 */
int TileEngine::faceWindow(const Position& position)
{
	static const Position tileEast = Position(1, 0, 0);
	static const Position tileSouth = Position(0, 1, 0);

	Tile* tile = _battleSave->getTile(position);
	if (tile
		&& tile->getMapData(MapData::O_NORTHWALL)
		&& !tile->getMapData(MapData::O_NORTHWALL)->stopLOS())
	{
		return 0;
	}

	tile = _battleSave->getTile(position + tileEast);
	if (tile
		&& tile->getMapData(MapData::O_WESTWALL)
		&& !tile->getMapData(MapData::O_WESTWALL)->stopLOS())
	{
		return 2;
	}

	tile = _battleSave->getTile(position + tileSouth);
	if (tile
		&& tile->getMapData(MapData::O_NORTHWALL)
		&& !tile->getMapData(MapData::O_NORTHWALL)->stopLOS())
	{
		return 4;
	}

	tile = _battleSave->getTile(position);
	if (tile
		&& tile->getMapData(MapData::O_WESTWALL)
		&& !tile->getMapData(MapData::O_WESTWALL)->stopLOS())
	{
		return 6;
	}

	return -1;
}

/**
 * Returns the direction from origin to target.
 * kL_note: This function is almost identical to BattleUnit::directionTo().
 * @param origin - Reference to the origin point of the action.
 * @param target - Reference to the target point of the action.
 * @return, direction.
 */
int TileEngine::getDirectionTo(
		const Position& origin,
		const Position& target) const
{
	if (origin == target) // kL. safety
		return 0;


	double offset_x = target.x - origin.x;
	double offset_y = target.y - origin.y;

	// kL_note: atan2() usually takes the y-value first;
	// and that's why things may seem so fucked up.
	double theta = atan2( // radians: + = y > 0; - = y < 0;
						-offset_y,
						offset_x);

	// divide the pie in 4 thetas, each at 1/8th before each quarter
	double m_pi_8 = M_PI / 8.0;				// a circle divided into 16 sections (rads) -> 22.5 deg
	double d = 0.1;							// kL, a bias toward cardinal directions. (0.1..0.12)
	double pie[4] =
	{
		M_PI - m_pi_8 - d,					// 2.7488935718910690836548129603696	-> 157.5 deg
		(M_PI * 3.0 / 4.0) - m_pi_8 + d,	// 1.9634954084936207740391521145497	-> 112.5 deg
		M_PI_2 - m_pi_8 - d,				// 1.1780972450961724644234912687298	-> 67.5 deg
		m_pi_8 + d							// 0.39269908169872415480783042290994	-> 22.5 deg
	};

	int dir = 2;
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

	return dir;
}

/**
 * Gets the origin-voxel of a shot or missile.
 * @param action	- Reference to the BattleAction
 * @param tile		- Pointer to a start tile
 * @return, Position of the origin in voxel-space
 */
Position TileEngine::getOriginVoxel(
		BattleAction& action,
		Tile* tile)
{
//kL	const int dirXshift[24] = {9, 15, 15, 13,  8,  1, 1, 3, 7, 13, 15, 15,  9,  3, 1, 1, 8, 14, 15, 14,  8,  2, 1, 2};
//kL	const int dirYshift[24] = {1,  3,  9, 15, 15, 13, 7, 1, 1,  1,  7, 13, 15, 15, 9, 3, 1,  2,  8, 14, 15, 14, 8, 2};

	if (!tile)
		tile = action.actor->getTile();

	Position origin = tile->getPosition();
	Position originVoxel = Position(
								origin.x * 16,
								origin.y * 16,
								origin.z * 24);

	// take into account soldier height and terrain level if the projectile is launched from a soldier
	if (action.actor->getPosition() == origin
		|| action.type != BA_LAUNCH)
	{
		// calculate vertical offset of the starting point of the projectile
		originVoxel.z += action.actor->getHeight()
					+ action.actor->getFloatHeight()
					- tile->getTerrainLevel();
//kL					- 4; // for good luck. (kL_note: looks like 2 voxels lower than LoS origin or something like it.)
			// Ps. don't need luck - need precision.

//		if (action.type == BA_THROW)	// kL
//			originVoxel.z -= 4;			// kL
/*kL		if (action.type == BA_THROW)
			originVoxel.z -= 3;
		else
			originVoxel.z -= 4; */

		if (originVoxel.z >= (origin.z + 1) * 24)
		{
			Tile* tileAbove = _battleSave->getTile(origin + Position(0, 0, 1));
			if (tileAbove
				&& tileAbove->hasNoFloor(0))
			{
				origin.z++;
			}
			else
			{
				while (originVoxel.z >= (origin.z + 1) * 24)
					originVoxel.z--;

				originVoxel.z -= 4; // keep originVoxel 4 voxels below any ceiling.
			}
		}

		// kL_note: This is the old code that does not use the dirX/Yshift stuff...
		//
		// Originally used the dirXShift and dirYShift as detailed above;
		// this however results in MUCH more predictable results.
		// center Origin in the originTile (or the center of all four tiles for large units):
		int offset = action.actor->getArmor()->getSize() * 8;
		originVoxel.x += offset;
		originVoxel.y += offset;
			// screw Warboy's obscurantist glamor-elitist campaign!!!! Have fun with that!!
			// MUCH more predictable results. <- I didn't write that; just iterating it.

/*kL		int offset = 0;
		if (action.actor->getArmor()->getSize() > 1)
			offset = 16;
		else if (action.weapon == action.weapon->getOwner()->getItem("STR_LEFT_HAND")
			&& !action.weapon->getRules()->isTwoHanded())
		{
			offset = 8;
		}

		int direction = getDirectionTo(
									origin,
									action.target);
		originVoxel.x += dirXshift[direction + offset] * action.actor->getArmor()->getSize();
		originVoxel.y += dirYshift[direction + offset] * action.actor->getArmor()->getSize(); */
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
 * mark a region of the map as "dangerous" for a turn.
 * @param pos is the epicenter of the explosion.
 * @param radius how far to spread out.
 * @param unit the unit that is triggering this action.
 */
void TileEngine::setDangerZone(
		Position pos,
		int radius,
		BattleUnit* unit)
{
	Tile* tile = _battleSave->getTile(pos);
	if (!tile)
		return;

	// set the epicenter as dangerous
	tile->setDangerous();
	Position originVoxel = (pos * Position(16, 16, 24))
								+ Position(
										8,
										8,
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
			// we can skip the epicenter
			if (x != 0 || y != 0)
			{
				// make sure it's within the radius
				if ((x * x) + (y * y) <= radius * radius)
				{
					tile = _battleSave->getTile(pos + Position(x, y, 0));
					if (tile)
					{
						targetVoxel = ((pos + Position(x, y, 0)) * Position(16, 16, 24))
										+ Position(8, 8, 12 - tile->getTerrainLevel());

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
										unit)
									== VOXEL_EMPTY)
						{
							if (trajectory.size()
								&& (trajectory.back() / Position(16, 16, 24)) == pos + Position(x, y, 0))
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
