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
#include "Map.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "UnitTurnBState.h"

#include "../fmath.h"

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

const int TileEngine::heightFromCenter[11] = { 0,-2, 2,-4, 4,-6, 6,-8, 8,-12, 12 };


/**
 * Sets up a TileEngine.
 * @param save Pointer to SavedBattleGame object.
 * @param voxelData List of voxel data.
 */
TileEngine::TileEngine(
		SavedBattleGame* save,
		std::vector<Uint16>* voxelData)
	:
		_save(save),
		_voxelData(voxelData),
		_personalLighting(true),
		_powerT(0) // kL
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
			i < _save->getMapSizeXYZ();
			++i)
	{
		_save->getTiles()[i]->resetLight(layer);
		calculateSunShading(_save->getTiles()[i]);
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

	int power = 15 - _save->getGlobalShade();

	// At night/dusk sun isn't dropping shades blocked by roofs
	if (_save->getGlobalShade() <= 4)
	{
		if (verticalBlockage(
						_save->getTile(Position(
											tile->getPosition().x,
											tile->getPosition().y,
											_save->getMapSizeZ() - 1)),
						tile,
						DT_NONE))
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
	const int layer = 1;			// Static lighting layer.
	const int fireLightPower = 15;	// amount of light a fire generates

	for (int // reset all light to 0 first
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		_save->getTiles()[i]->resetLight(layer);
	}

	for (int // add lighting of terrain
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		// only floors and objects can light up
		if (_save->getTiles()[i]->getMapData(MapData::O_FLOOR)
			&& _save->getTiles()[i]->getMapData(MapData::O_FLOOR)->getLightSource())
		{
			addLight(
					_save->getTiles()[i]->getPosition(),
					_save->getTiles()[i]->getMapData(MapData::O_FLOOR)->getLightSource(),
					layer);
		}

		if (_save->getTiles()[i]->getMapData(MapData::O_OBJECT)
			&& _save->getTiles()[i]->getMapData(MapData::O_OBJECT)->getLightSource())
		{
			addLight(
					_save->getTiles()[i]->getPosition(),
					_save->getTiles()[i]->getMapData(MapData::O_OBJECT)->getLightSource(),
					layer);
		}

		if (_save->getTiles()[i]->getFire())
			addLight(
					_save->getTiles()[i]->getPosition(),
					fireLightPower,
					layer);

		for (std::vector<BattleItem*>::iterator
				it = _save->getTiles()[i]->getInventory()->begin();
				it != _save->getTiles()[i]->getInventory()->end();
				++it)
		{
			if ((*it)->getRules()->getBattleType() == BT_FLARE)
				addLight(
						_save->getTiles()[i]->getPosition(),
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
	const int personalLightPower = 14;		// kL, Try it...
	const int fireLightPower = 15;			// amount of light a fire generates

	for (int // reset all light to 0 first
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		_save->getTiles()[i]->resetLight(layer);
	}

	for (std::vector<BattleUnit*>::iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (_personalLighting // add lighting of soldiers
			&& (*i)->getFaction() == FACTION_PLAYER
//kL			&& !(*i)->isOut())
			&& (*i)->getHealth() != 0) // kL, Let unconscious soldiers glow.
		{
			addLight(
					(*i)->getPosition(),
					personalLightPower,
					layer);
		}

		// add lighting of units on fire
		if ((*i)->getFire())
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
					z < _save->getMapSizeZ();
					z++)
			{
				int distance = static_cast<int>(Round(sqrt(static_cast<double>(x * x + y * y))));

				if (_save->getTile(Position(voxelTarget.x + x, voxelTarget.y + y, z)))
					_save->getTile(Position(voxelTarget.x + x, voxelTarget.y + y, z))->addLight(power - distance, layer);

				if (_save->getTile(Position(voxelTarget.x - x, voxelTarget.y - y, z)))
					_save->getTile(Position(voxelTarget.x - x, voxelTarget.y - y, z))->addLight(power - distance, layer);

				if (_save->getTile(Position(voxelTarget.x + x, voxelTarget.y - y, z)))
					_save->getTile(Position(voxelTarget.x + x, voxelTarget.y - y, z))->addLight(power - distance, layer);

				if (_save->getTile(Position(voxelTarget.x - x, voxelTarget.y + y, z)))
					_save->getTile(Position(voxelTarget.x - x, voxelTarget.y + y, z))->addLight(power - distance, layer);
			}
		}
	}
}

/**
 * Calculates line of sight of a BattleUnit.
 * @param unit, Unit to check line of sight of.
 * @return, True when new aliens are spotted.
 */
bool TileEngine::calculateFOV(BattleUnit* unit)
{
	//Log(LOG_INFO) << "TileEngine::calculateFOV() for ID " << unit->getId();
	unit->clearVisibleUnits(); // kL:below
	unit->clearVisibleTiles(); // kL:below

	if (unit->isOut(true, true)) // kL:below (check health, check stun, check status)
		return false;

	bool ret = false; // kL

	size_t preVisUnits = unit->getUnitsSpottedThisTurn().size();
	//Log(LOG_INFO) << ". . . . preVisUnits = " << (int)preVisUnits;

	int direction;
	if (Options::strafe
		&& unit->getTurretType() > -1)
	{
		direction = unit->getTurretDirection();
	}
	else
		direction = unit->getDirection();
	//Log(LOG_INFO) << ". direction = " << direction;

	bool swap = (direction == 0 || direction == 4);

	int
		sign_x[8] = { 1, 1, 1, 1,-1,-1,-1,-1},
//kL	int sign_y[8] = {-1, -1, -1, +1, +1, +1, -1, -1}; // is this right? (ie. 3pos & 5neg, why not 4pos & 4neg )
		sign_y[8] = {-1,-1, 1, 1, 1, 1,-1,-1}; // kL: note it does not matter.

	bool diag = false;
	int
		y1 = 0,
		y2 = 0;

	if (direction %2)
	{
		diag = true;

//		y1 = 0;
		y2 = MAX_VIEW_DISTANCE;
	}

/*kL:above	unit->clearVisibleUnits();
	unit->clearVisibleTiles();

	if (unit->isOut()) return false; */

	std::vector<Position> _trajectory;

//	Position center = unit->getPosition();
	Position unitPos = unit->getPosition();
	Position testPos;

//kL	if (unit->getHeight() + unit->getFloatHeight() + -_save->getTile(unit->getPosition())->getTerrainLevel() >= 24 + 4)
	if (unit->getHeight()
				+ unit->getFloatHeight()
				- _save->getTile(unitPos)->getTerrainLevel()
			>= 24 + 4)
	{
		++unitPos.z;
	}

	for (int
			x = 0; // kL_note: does the unit itself really need checking...
//			x = 1;
			x <= MAX_VIEW_DISTANCE;
			++x)
	{
/*		if (direction %2)
		{
			y1 = 0;
			y2 = MAX_VIEW_DISTANCE;
		}
		else
		{
			y1 = -x;
			y2 = x;
		} */
		if (!diag)
		{
			//Log(LOG_INFO) << ". not Diagonal";
/*			if (x == 0)
			{
				y1 = -MAX_VIEW_DISTANCE;
				y2 = MAX_VIEW_DISTANCE;
			}
			else
			{ */
			y1 = -x;
			y2 = x;
//			}
		}

		for (int
				y = y1;
				y <= y2;
				++y)
		{
			for (int
					z = 0;
					z < _save->getMapSizeZ();
					++z)
			{
				//Log(LOG_INFO) << "for (int z = 0; z < _save->getMapSizeZ(); z++), z = " << z;

//	int dist = distance(position, (*i)->getPosition());
				const int distSqr = x * x + y * y;
//				const int distSqr = x * x + y * y + z * z; // kL
				//Log(LOG_INFO) << "distSqr = " << distSqr << " ; x = " << x << " ; y = " << y << " ; z = " << z; // <- HUGE write to file.
				//Log(LOG_INFO) << "x = " << x << " ; y = " << y << " ; z = " << z; // <- HUGE write to file.

				testPos.z = z;

				if (distSqr <= MAX_VIEW_DISTANCE * MAX_VIEW_DISTANCE)
				{
					//Log(LOG_INFO) << "inside distSqr";

//kL					testPos.x = center.x + sign_x[direction] * (swap? y: x);
//kL					testPos.y = center.y + sign_y[direction] * (swap? x: y);
					int deltaPos_x = (sign_x[direction] * (swap? y: x));
					int deltaPos_y = (sign_y[direction] * (swap? x: y));
					testPos.x = unitPos.x + deltaPos_x;
					testPos.y = unitPos.y + deltaPos_y;
					//Log(LOG_INFO) << "testPos.x = " << testPos.x;
					//Log(LOG_INFO) << "testPos.y = " << testPos.y;
					//Log(LOG_INFO) << "testPos.z = " << testPos.z;


					if (_save->getTile(testPos))
					{
						//Log(LOG_INFO) << "inside getTile(testPos)";
						BattleUnit* visUnit = _save->getTile(testPos)->getUnit();

						//Log(LOG_INFO) << ". . calculateFOV(), visible() CHECK.. " << visUnit->getId();
						if (visUnit
							&& !visUnit->isOut(true, true)
							&& visible(
									unit,
									_save->getTile(testPos))) // reveal units & tiles <- This seems uneven.
						{
							//Log(LOG_INFO) << ". . visible() TRUE : unitID = " << unit->getId() << " ; visID = " << visUnit->getId();
							//Log(LOG_INFO) << ". . calcFoV, distance = " << distance(unit->getPosition(), visUnit->getPosition());

							//Log(LOG_INFO) << ". . calculateFOV(), visible() TRUE id = " << visUnit->getId();
							if (!visUnit->getVisible()) // kL, spottedID = " << visUnit->getId();
							{
								//Log(LOG_INFO) << ". . calculateFOV(), getVisible() FALSE";
								ret = true; // kL
							}
							//Log(LOG_INFO) << ". . calculateFOV(), visUnit -> getVisible() = " << !ret;

							if (unit->getFaction() == FACTION_PLAYER)
							{
								//Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER, set spottedTile & spottedUnit visible";
//kL								visUnit->getTile()->setVisible(+1);
								visUnit->getTile()->setVisible(true); // kL
								visUnit->getTile()->setDiscovered(true, 2); // kL_below. sprite caching for floor+content: DO I WANT THIS.
								if (!visUnit->getVisible()) // kL
									visUnit->setVisible(true);
							}
							//Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER, Done";

							if ((visUnit->getFaction() == FACTION_HOSTILE
									&& unit->getFaction() == FACTION_PLAYER)
								|| (visUnit->getFaction() != FACTION_HOSTILE
									&& unit->getFaction() == FACTION_HOSTILE))
							{
								//Log(LOG_INFO) << ". . . opposite Factions, add Tile & visUnit to visList";

								// adds visUnit to _visibleUnits *and* to _unitsSpottedThisTurn:
								unit->addToVisibleUnits(visUnit); // kL_note: This returns a boolean; i can use that...... yeah, done & done.
								unit->addToVisibleTiles(visUnit->getTile());

								if (unit->getFaction() == FACTION_HOSTILE
									&& visUnit->getFaction() != FACTION_HOSTILE
									&& _save->getSide() == FACTION_HOSTILE) // kL, per Original.
								{
									//Log(LOG_INFO) << ". . calculateFOV(), spotted Unit FACTION_HOSTILE, setTurnsExposed()";
									visUnit->setTurnsExposed(0);	// note that xCom can be seen by enemies but *not* be Exposed. hehe
																	// Only reactionFire should set them Exposed during xCom's turn.
								}
							}
							//Log(LOG_INFO) << ". . calculateFOV(), opposite Factions, Done";
						}
						//Log(LOG_INFO) << ". . calculateFOV(), visUnit EXISTS & isVis, Done";

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
									Position pPlayer = unitPos + Position(
																		xPlayer,
																		yPlayer,
																		0);

									_trajectory.clear();

									//Log(LOG_INFO) << ". . calculateLine()";
									int test = calculateLine(
														pPlayer,
														testPos,
														true,
														&_trajectory,
														unit,
														false);
									//Log(LOG_INFO) << ". . . . calculateLine() test = " << test;

									size_t trajSize = _trajectory.size();

									if (test > 127) // last tile is blocked thus must be cropped
										--trajSize;

									//Log(LOG_INFO) << ". . . trace Trajectory.";
									for (size_t
											i = 0;
											i < trajSize;
											i++)
									{
										//Log(LOG_INFO) << ". . . . calculateLine() inside TRAJECTORY Loop";
										Position pTraj = _trajectory.at(i);

										// mark every tile of line as visible (as in original)
										// this is needed because of bresenham narrow stroke.
//kL										_save->getTile(pTraj)->setVisible(+1);
										_save->getTile(pTraj)->setVisible(true); // kL
										_save->getTile(pTraj)->setDiscovered(true, 2); // sprite caching for floor+content

										// walls to the east or south of a visible tile, we see that too
										// kL_note: Yeh, IF there's walls or an appropriate BigWall object!
										Tile* t = _save->getTile(Position(
																		pTraj.x + 1,
																		pTraj.y,
																		pTraj.z));
										if (t)
//kL											t->setDiscovered(true, 0);
										// kL_begin:
										{
											//Log(LOG_INFO) << "calculateFOV() East tile VALID";
											for (int
													part = 0;
													part < 4; //t->getMapData(fuck)->getDataset()->getSize();
													++part)
											{
												//Log(LOG_INFO) << "part = " << part;
												if (t->getMapData(part))
												{
													//Log(LOG_INFO) << ". getMapData(part) VALID";
													if (t->getMapData(part)->getObjectType() == MapData::O_WESTWALL)
													{
														//Log(LOG_INFO) << ". . MapData: WestWall VALID";
														t->setDiscovered(true, 0);

//														break;
													}
													else if (t->getMapData(part)->getObjectType() == MapData::O_OBJECT
														&& t->getMapData(MapData::O_OBJECT)
														&& (t->getMapData(MapData::O_OBJECT)->getBigWall() == 1			// bigBlock
															|| t->getMapData(MapData::O_OBJECT)->getBigWall() == 4))	// westBlock
													{
														//Log(LOG_INFO) << ". . MapData: Object VALID, bigWall 1 or 4";
														t->setDiscovered(true, 2);

//														break;
													}
												}
											}
										} // kL_end.

										t = _save->getTile(Position(
																pTraj.x,
																pTraj.y + 1,
																pTraj.z));
										if (t)
//kL											t->setDiscovered(true, 1);
										// kL_begin:
										{
											//Log(LOG_INFO) << "calculateFOV() South tile VALID";
											for (int
													part = 0;
													part < 4; //t->getMapData(fuck)->getDataset()->getSize();
													++part)
											{
												//Log(LOG_INFO) << "part = " << part;
												if (t->getMapData(part))
												{
													//Log(LOG_INFO) << ". getMapData(part) VALID";
													if (t->getMapData(part)->getObjectType() == MapData::O_NORTHWALL)
													{
														//Log(LOG_INFO) << ". . MapData: NorthWall VALID";
														t->setDiscovered(true, 1);

//														break;
													}
													else if (t->getMapData(part)->getObjectType() == MapData::O_OBJECT
														&& t->getMapData(MapData::O_OBJECT)
														&& (t->getMapData(MapData::O_OBJECT)->getBigWall() == 1			// bigBlock
															|| t->getMapData(MapData::O_OBJECT)->getBigWall() == 5))	// northBlock
													{
														//Log(LOG_INFO) << ". . MapData: Object VALID, bigWall 1 or 5";
														t->setDiscovered(true, 2);

//														break;
													}
												}
											}
										} // kL_end.
										//Log(LOG_INFO) << "calculateFOV() DONE tile reveal";
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
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
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
			bu = _save->getUnits()->begin();
			bu != _save->getUnits()->end();
			++bu)
	{
		if ((*bu)->getTile() != 0)
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

		Tile* t = _save->getTile(unit->getPosition()); // origin tile

//		for (uint16_t
		for (size_t
				i = 0;
				i < _trajectory.size();
				i++)
		{
			//Log(LOG_INFO) << ". . . . . . tracing Trajectory...";
			if (t != _save->getTile(Position(
										_trajectory.at(i).x / 16,
										_trajectory.at(i).y / 16,
										_trajectory.at(i).z / 24)))
			{
				t = _save->getTile(Position(
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
			Tile* tBelow = _save->getTile(t->getPosition() + Position(0, 0, -1));
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
					- _save->getTile(unit->getPosition())->getTerrainLevel()
					- 2; // two voxels lower (nose level)
		// kL_note: Can make this equivalent to LoF origin, perhaps.....
		// hey, here's an idea: make Snaps & Auto shoot from hip, Aimed from shoulders or eyes.

	Tile* tileAbove = _save->getTile(unit->getPosition() + Position(0, 0, 1));

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
 * @param originVoxel, Voxel of trace origin (eye or gun's barrel).
 * @param tile, The tile to check for.
 * @param scanVoxel, is returned coordinate of hit.
 * @param excludeUnit, is self (not to hit self).
 * @param potentialUnit, is a hypothetical unit to draw a virtual line of fire for AI. if left blank, this function behaves normally.
 * @return, True if the unit can be targetted.
 */
bool TileEngine::canTargetUnit(
		Position* originVoxel,
		Tile* tile,
		Position* scanVoxel,
		BattleUnit* excludeUnit,
		BattleUnit* potentialUnit)
{
//kL	Position targetVoxel = Position((tile->getPosition().x * 16) + 7, (tile->getPosition().y * 16) + 8, tile->getPosition().z * 24);
	Position targetVoxel = Position(
								tile->getPosition().x * 16 + 8,
								tile->getPosition().y * 16 + 8,
								tile->getPosition().z * 24);

	std::vector<Position> _trajectory;

	bool hypothetical = (potentialUnit != 0);
	if (potentialUnit == 0)
	{
		potentialUnit = tile->getUnit();
		if (potentialUnit == 0)
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
								excludeUnit,
								true,
								false);
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
						if (   _trajectory.at(0).x / 16 == (scanVoxel->x / 16) + x + xOffset
							&& _trajectory.at(0).y / 16 == (scanVoxel->y / 16) + y + yOffset
							&& _trajectory.at(0).z >= targetMinHeight
							&& _trajectory.at(0).z <= targetMaxHeight)
						{
							return true;
						}
					}
				}
			}
			else if (test == VOXEL_EMPTY
				&& hypothetical
				&& !_trajectory.empty())
			{
				return true;
			}
		}
	}

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
		//Log(LOG_INFO) << "TileEngine::canTargetTile() EXIT, ret False (!maxZfound)";
		return false; // it's impossible to get there

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
//								true); // do voxelCheck, default=true.

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
 * @param unit, The unit to check reaction fire upon.
 * @param tuSpent, The unit's expenditure of TU if firing or throwing. kL
 * @return, True if reaction fire took place.
 */
bool TileEngine::checkReactionFire(
		BattleUnit* unit,
		int tuSpent) // kL
{
	//Log(LOG_INFO) << "TileEngine::checkReactionFire() vs targetID " << unit->getId();

	if (_save->getSide() == FACTION_NEUTRAL) // no reaction on civilian turn.
		return false;


	// trigger reaction fire only when the spotted unit is of the
	// currently playing side, and is still on the map, alive
	if (unit->getFaction() != _save->getSide()
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
				//Log(LOG_INFO) << ". . Snap by : " << reactor->getId();
				ret = true;

			reactor = getReactor( // nice shot, kid. don't get too cocky.
								spotters,
								unit,
								tuSpent); // kL
			//Log(LOG_INFO) << ". . NEXT AT BAT : " << reactor->getId();
		}

		spotters.clear();	// kL
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
			bu = _save->getUnits()->begin();
			bu != _save->getUnits()->end();
			++bu)
	{
		if ((*bu)->getFaction() != _save->getSide()
			&& !(*bu)->isOut(true, true))
		{
/*kL			AlienBAIState* aggro = dynamic_cast<AlienBAIState*>((*bu)->getCurrentAIState());
			if (((aggro != 0
						&& aggro->getWasHit()) // set in ProjectileFlyBState...
					|| (*bu)->getFaction() == FACTION_HOSTILE // note: doesn't this cover the aggro-thing, like totally
					|| (*bu)->checkViewSector(unit->getPosition())) // aLiens see all directions, btw. */
			if (((*bu)->getFaction() == FACTION_HOSTILE
					|| ((*bu)->getFaction() == FACTION_PLAYER
						&& (*bu)->checkViewSector(unit->getPosition())))
				&& visible(*bu, tile))
			{
				//Log(LOG_INFO) << ". check ID " << (*bu)->getId();

				if ((*bu)->getFaction() == FACTION_HOSTILE)
					unit->setTurnsExposed(0);

				// these two calls should already be done in calculateFOV()
//				if ((*bu)->getFaction() == FACTION_PLAYER)
//					unit->setVisible(true);
//				(*bu)->addToVisibleUnits(unit);
					// as long as calculateFOV is always done right between
					// walking, kneeling, shooting, throwing .. and checkReactionFire()
					// If so, then technically, visible() above can be replaced
					// by checking (*bu)'s _visibleUnits vector. But this is working good per.

				if (canMakeSnap(*bu, unit))
				{
					//Log(LOG_INFO) << ". . . reactor ID " << (*bu)->getId()
					//		<< " : initi = " << (int)(*bu)->getInitiative();

					spotters.push_back(*bu);
				}
			}
		}
	}

	return spotters;
}

/**
 * Checks the validity of a snap shot performed here.
 * kL: Changed to use selectFireMethod (aimed/auto/snap).
 * @param unit, The unit to check sight from
 * @param target, The unit to check sight TO
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

	if (!weapon)
	{
		//Log(LOG_INFO) << ". no weapon, return FALSE";
		return false;
	}
	// TEST
//	if (weapon->getRules()->getBattleType() == BT_MELEE)
//	{
		//Log(LOG_INFO) << ". . validMeleeRange = " << validMeleeRange(unit, target, unit->getDirection());
		//Log(LOG_INFO) << ". . unit's TU = " << unit->getTimeUnits();
		//Log(LOG_INFO) << ". . action's TU = " << unit->getActionTUs(BA_HIT, weapon);
//	}

	if (weapon->getRules()->canReactionFire() // kL add.
		&& (unit->getOriginalFaction() == FACTION_HOSTILE				// is aLien, or has researched weapon.
			|| _save->getGeoscapeSave()->isResearched(weapon->getRules()->getRequirements()))
		&& ((weapon->getRules()->getBattleType() == BT_MELEE			// has a melee weapon
				&& validMeleeRange(
								unit,
								target,
								unit->getDirection())					// is in melee range
				&& unit->getTimeUnits() >= unit->getActionTUs(			// has enough TU
															BA_HIT,
															weapon))
			|| (weapon->getRules()->getBattleType() == BT_FIREARM	// has a gun
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
 * Gets the unit with the highest reaction score from the spotter vector.
 * NOTE: the tuSpent parameter is needed because popState() doesn't
 * subtract TU until after the Initiative has been calculated.
 * @param spotters, The vector of spotting units.
 * @param unit, The unit to check scores against.
 * @param tuSpent, The unit's expenditure of TU if firing or throwing. kL
 * @return, The unit with initiative.
 */
BattleUnit* TileEngine::getReactor(
		std::vector<BattleUnit*> spotters,
		BattleUnit* unit,
		int tuSpent) // kL
{
	//Log(LOG_INFO) << "TileEngine::getReactor() vs ID " << unit->getId();

	BattleUnit* reactor = 0;
	int bestScore = -1;

	for (std::vector<BattleUnit*>::iterator
			spotter = spotters.begin();
			spotter != spotters.end();
			++spotter)
	{
		//Log(LOG_INFO) << ". . reactor ID " << (*spotter)->getId();

		if (!(*spotter)->isOut(true, true)
			&& (*spotter)->getInitiative() > bestScore)
		{
			bestScore = static_cast<int>((*spotter)->getInitiative());
			reactor = *spotter;
		}
	}


//	BattleAction action = _save->getAction();
/*	BattleItem* weapon;// = BattleState::getAction();
	if (unit->getFaction() == FACTION_PLAYER
		&& unit->getOriginalFaction() == FACTION_PLAYER)
	{
		weapon = unit->getItem(unit->getActiveHand());
	}
	else
		weapon = unit->getMainHandWeapon(); // kL_note: no longer returns grenades. good
//	if (!weapon) return false;  // it *has* to be there by now!
		// note these calc's should be refactored; this calc happens what 3 times now!!!
		// Ought get the BattleAction* and just toss it around among these RF determinations.

	int tuShoot = unit->getActionTUs(BA_AUTOSHOT, weapon) */


	//Log(LOG_INFO) << ". ID " << unit->getId() << " initi = " << static_cast<int>(unit->getInitiative(tuSpent));

	// reactor has to *best* unit.Initi to get initiative
	// Analysis: It appears that unit's tu for firing/throwing
	// are not subtracted before getInitiative() is called.
	if (bestScore > static_cast<int>(unit->getInitiative(tuSpent)))
	{
		if (reactor->getOriginalFaction() == FACTION_PLAYER)
			reactor->addReactionExp();
	}
	else
	{
		//Log(LOG_INFO) << ". . initi returns to ID " << unit->getId();
		reactor = unit;
	}

	//Log(LOG_INFO) << ". bestScore (reactor) = " << bestScore;
	return reactor;
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

//kL	action.type = BA_SNAPSHOT;									// reaction fire is ALWAYS snap shot.
																// kL_note: not true in Orig. aliens did auto at times

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
		if (aggro == 0) // should not happen, but just in case...
		{
			aggro = new AlienBAIState(
									_save,
									unit,
									0);
			unit->setAIState(aggro);
		}

//kL		if (action.weapon->getAmmoItem()->getRules()->getExplosionRadius()
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
		action.cameraPosition = _save->getBattleState()->getMap()->getCamera()->getMapOffset();	// kL, was above under "BattleAction action;"

		_save->getBattleGame()->statePushBack(new UnitTurnBState(
															_save->getBattleGame(),
															action));
		_save->getBattleGame()->statePushBack(new ProjectileFlyBState(
																_save->getBattleGame(),
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

	int distance = _save->getTileEngine()->distance(
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
	action.type = BA_NONE; // should never happen.

	int
		tuUnit = action.actor->getTimeUnits(),
		distance = _save->getTileEngine()->distance(
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
 * Handles bullet/weapon hits.
 *
 * A bullet/weapon hits a voxel.
 * kL_note: called from ExplosionBState
 * @param pTarget_voxel, Center of the hit in voxelspace.
 * @param power, Power of the hit/explosion.
 * @param type, The damage type of the hit.
 * @param attacker, The unit that caused the hit.
 * @param melee, True if no projectile, trajectory, etc. is needed. kL
 * @return, The Unit that got hit.
 */
BattleUnit* TileEngine::hit(
		const Position& pTarget_voxel,
		int power,
		ItemDamageType type,
		BattleUnit* attacker,
		bool melee) // kL add.
{
	Log(LOG_INFO) << "TileEngine::hit() by ID " << attacker->getId()
				<< " @ " << attacker->getPosition()
				<< " : power = " << power
				<< " : type = " << (int)type;

	if (type != DT_NONE) // TEST for Psi-attack.
	{
		//Log(LOG_INFO) << "DT_ type = " << static_cast<int>(type);
		Tile* tile = _save->getTile(Position(
										pTarget_voxel.x / 16,
										pTarget_voxel.y / 16,
										pTarget_voxel.z / 24));
		//Log(LOG_INFO) << ". targetTile " << tile->getPosition() << " targetVoxel " << pTarget_voxel;

		if (!tile)
		{
			//Log(LOG_INFO) << ". Position& pTarget_voxel : NOT Valid, return NULL";
			return 0;
		}

		BattleUnit* buTarget = 0;
		if (tile->getUnit())
		{
			buTarget = tile->getUnit();
			//Log(LOG_INFO) << ". . buTarget Valid ID = " << buTarget->getId();
		}

		// This is returning part < 4 when using a stunRod against a unit outside the north (or east) UFO wall. ERROR!!!
		// later: Now it's returning VOXEL_EMPTY (-1) when a silicoid attacks a poor civvie!!!!! And on acid-spits!!!
//kL		int const part = voxelCheck(pTarget_voxel, attacker);
		// kL_note: for hit==TRUE, just put part=VOXEL_UNIT here. Like,
		// and take out the 'hit' parameter from voxelCheck() unless
		// I want to flesh-out melee & psi attacks more than they are.
		int part = VOXEL_UNIT; // kL, no longer const
		if (!melee) // kL
			part = voxelCheck(
							pTarget_voxel,
							attacker,
							false,
							false,
							0,
							melee); // kL
		//Log(LOG_INFO) << ". voxelCheck() part = " << part;

		if (VOXEL_EMPTY < part && part < VOXEL_UNIT	// 4 terrain parts ( 0..3 )
			&& type != DT_STUN)						// kL, workaround for Stunrod. ( might include DT_SMOKE & DT_IN )
		{
			//Log(LOG_INFO) << ". . terrain hit";
//kL			const int randPower = RNG::generate(power / 4, power * 3 / 4); // RNG::boxMuller(power, power/6)
			power = RNG::generate( // 25% to 75%
								power / 4,
								power * 3 / 4);
			//Log(LOG_INFO) << ". . RNG::generate(power) = " << power;

			if (part == VOXEL_OBJECT
				&& _save->getMissionType() == "STR_BASE_DEFENSE")
			{
				if (power >= tile->getMapData(MapData::O_OBJECT)->getArmor()
					&& tile->getMapData(VOXEL_OBJECT)->isBaseModule())
				{
					_save->getModuleMap()
									[(pTarget_voxel.x / 16) / 10]
									[(pTarget_voxel.y / 16) / 10].second--;
				}
			}

			// kL_note: This may be necessary only on aLienBase missions...
			if (tile->damage(
							part,
							power))
			{
				_save->setObjectiveDestroyed(true);
			}

			// kL_note: This would be where to adjust damage based on effectiveness of weapon vs Terrain!
		}
		else if (part == VOXEL_UNIT)	// battleunit part
//			|| type == DT_STUN)			// kL, workaround for Stunrod. (not needed, huh?)
		{
			//Log(LOG_INFO) << ". . battleunit hit";

			// power 0 - 200% -> 1 - 200%
//kL			const int randPower = RNG::generate(1, power * 2); // RNG::boxMuller(power, power / 3)
			int verticaloffset = 0;

			if (!buTarget)
			{
				//Log(LOG_INFO) << ". . . buTarget NOT Valid, check tileBelow";

				// it's possible we have a unit below the actual tile, when he
				// stands on a stairs and sticks his head up into the above tile.
				// kL_note: yeah, just like in LoS calculations!!!! cf. visible() etc etc .. idiots.
				Tile* tileBelow = _save->getTile(Position(
														pTarget_voxel.x / 16,
														pTarget_voxel.y / 16,
														pTarget_voxel.z / 24 - 1));
				if (tileBelow
					&& tileBelow->getUnit())
				{
					buTarget = tileBelow->getUnit();
					verticaloffset = 24;
				}
			}

			if (buTarget)
			{
				//Log(LOG_INFO) << ". . . buTarget Valid ID = " << buTarget->getId();

				// kL_note: This section needs adjusting ...!
				const int wounds = buTarget->getFatalWounds();
				// "adjustedDamage = buTarget->damage(relative, rndPower, type);" -> GOES HERE
				// if it's going to bleed to death and it's not a player, give credit for the kill.
				// kL_note: not just if not a player; Give Credit!!!
				// NOTE: Move this code-chunk below(s).
//kL				if (unit
//kL					&& buTarget->getFaction() != FACTION_PLAYER
//					&& buTarget->getOriginalFaction() != FACTION_PLAYER // kL
//kL					&& wounds < buTarget->getFatalWounds())
//				{
//kL					buTarget->killedBy(unit->getFaction());
//				} // kL_note: Not so sure that's been setup right (cf. other kill-credit code as well as DebriefingState)

/*kL				const int size = buTarget->getArmor()->getSize() * 8;
				const Position targetPos = (buTarget->getPosition() * Position(16, 16, 24)) // convert tilespace to voxelspace
						+ Position(
								size,
								size,
								buTarget->getFloatHeight() - tile->getTerrainLevel());
				const Position relPos = pTarget_voxel - targetPos - Position(0, 0, verticaloffset); */
				int const size = buTarget->getArmor()->getSize() * 8;
				Position const targetPos = (buTarget->getPosition() * Position(16, 16, 24)) // convert tilespace to voxelspace
										+ Position(
												size,
												size,
												buTarget->getFloatHeight() - tile->getTerrainLevel());
				Position const relPos = pTarget_voxel
									- targetPos
									- Position(
											0,
											0,
											verticaloffset);

				// kL_begin: TileEngine::hit(), Silacoids can set targets on fire!!
//kL				if (type == DT_IN)
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

				bool ignoreArmor = (type == DT_STUN);	// kL. stun ignores armor... does now! UHM....
														// note it still gets Vuln.modifier, but not armorReduction.
				int adjustedDamage = buTarget->damage(
													relPos,
													power,
													type,
													ignoreArmor);
				//Log(LOG_INFO) << ". . . adjustedDamage = " << adjustedDamage;

				if (adjustedDamage > 0)
//					&& !buTarget->isOut()
				{
					// kL_begin:
					if (attacker
						&& wounds < buTarget->getFatalWounds())
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
								modifier = _save->getMoraleModifier();
							else if (buTarget->getOriginalFaction() == FACTION_HOSTILE)
								modifier = _save->getMoraleModifier(0, false);

							const int morale_loss = 10 * adjustedDamage * bravery / modifier;
							//Log(LOG_INFO) << ". . . . morale_loss = " << morale_loss;

							buTarget->moraleChange(-morale_loss);
						}
					}
				}

				//Log(LOG_INFO) << ". . check for Cyberdisc expl.";
				//Log(LOG_INFO) << ". . health = " << buTarget->getHealth();
				//Log(LOG_INFO) << ". . stunLevel = " << buTarget->getStunlevel();
				if (buTarget->getSpecialAbility() == SPECAB_EXPLODEONDEATH // cyberdiscs
					&& (buTarget->getHealth() == 0
						|| buTarget->getHealth() <= buTarget->getStunlevel()))
//					&& !buTarget->isOut(false, true))	// kL. don't explode if stunned. Maybe... wrong!!!
														// Cannot be STATUS_DEAD OR STATUS_UNCONSCIOUS!
				{
					//Log(LOG_INFO) << ". . . Cyberdisc down!!";
					if (type != DT_STUN		// don't explode if stunned. Maybe... see above.
						&& type != DT_HE)	// don't explode if taken down w/ explosives -> wait a sec, this is hit() not explode() ...
					{
						//Log(LOG_INFO) << ". . . . new ExplosionBState(), !DT_STUN & !DT_HE";
						// kL_note: wait a second. hit() creates an ExplosionBState,
						// but ExplosionBState::explode() creates a hit() ! -> terrain..

						Position unitPos = Position(
//kL												buTarget->getPosition().x * 16,
//kL												buTarget->getPosition().y * 16,
												buTarget->getPosition().x * 16 + 8, // kL
												buTarget->getPosition().y * 16 + 8, // kL
												buTarget->getPosition().z * 24);

						_save->getBattleGame()->statePushNext(new ExplosionBState(
																			_save->getBattleGame(),
																			unitPos,
																			0,
																			buTarget));
					}
				}

				if (buTarget->getOriginalFaction() == FACTION_HOSTILE	// target is aLien Mc'd or not.
					&& attacker											// shooter exists
					&& attacker->getOriginalFaction() == FACTION_PLAYER	// shooter is Xcom
					&& attacker->getFaction() == FACTION_PLAYER			// shooter is not Mc'd Xcom
//					&& type != DT_NONE
					&& _save->getBattleGame()->getCurrentAction()->type != BA_HIT
					&& _save->getBattleGame()->getCurrentAction()->type != BA_STUN)
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
		calculateFOV(pTarget_voxel / Position(16, 16, 24));	// walls & objects could have been ruined


		//if (buTarget) Log(LOG_INFO) << "TileEngine::hit() EXIT, return buTarget";
		//else Log(LOG_INFO) << "TileEngine::hit() EXIT, return 0";

		return buTarget;
	}
	//else Log(LOG_INFO) << ". DT_ = " << static_cast<int>(type);

	//Log(LOG_INFO) << "TileEngine::hit() EXIT, return NULL";
	return 0; // end_TEST
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
 * @param voxelTarget, Center of the explosion in voxelspace.
 * @param power, Power of the explosion.
 * @param type, The damage type of the explosion.
 * @param maxRadius, The maximum radius of the explosion.
 * @param unit, The unit that caused the explosion.
 */
void TileEngine::explode(
			const Position& voxelTarget,
			int power,
			ItemDamageType type,
			int maxRadius,
			BattleUnit* unit)
{
	Log(LOG_INFO) << "TileEngine::explode() power = " << power
				<< ", type = " << (int)type
				<< ", maxRadius = " << maxRadius;
	if (power == 0) // kL, quick out.
		return;

	double
		centerX = static_cast<double>(voxelTarget.x / 16) + 0.5,
		centerY = static_cast<double>(voxelTarget.y / 16) + 0.5,
		centerZ = static_cast<double>(voxelTarget.z / 24) + 0.5;

	std::set<Tile*> tilesAffected;
	std::pair<std::set<Tile*>::iterator, bool> tilePair;

//		powerEff	= 0,

	int
		vertdec		= 1000, // default flat explosion
		explHeight	= std::max(
							0,
							std::min(
									3,
									Options::battleExplosionHeight));
	switch (explHeight)
	{
		case 1:
			vertdec = 30;
		break;
		case 2:
			vertdec = 20;
		break;
		case 3:
			vertdec = 10;
		break;

		default:
			vertdec = 1000;
		break;
	}

	if (type == DT_IN)
	{
		power /= 2;
		//Log(LOG_INFO) << ". DT_IN power = " << power;
	}

	Tile
		* origin	= 0,
		* destTile	= 0;

	int
		tileX,
		tileY,
		tileZ,
		testPower;

	double
		r,
		r_Max = static_cast<double>(maxRadius),

		vx,
		vy,
		vz,

		sin_te,
		cos_te,
		sin_fi,
		cos_fi;


	_powerT = 0;

//	for (int fi = 0; fi == 0; ++fi) // kL_note: Looks like a TEST ray. ( 0 == horizontal )
	for (int
			fi = -90;
			fi <= 90;
			fi += 5)
	{
//		for (int te = 180; te == 180; ++te) // kL_note: Looks like a TEST ray. ( 0 == south, 180 == north )
		for (int // raytracing every 3 degrees makes sure we cover all tiles in a circle.
				te = 0;
				te <= 360;
				te += 3)
		{
			sin_te = sin(static_cast<double>(te) * M_PI / 180.0);
			cos_te = cos(static_cast<double>(te) * M_PI / 180.0);
			sin_fi = sin(static_cast<double>(fi) * M_PI / 180.0);
			cos_fi = cos(static_cast<double>(fi) * M_PI / 180.0);

			origin = _save->getTile(Position(
										static_cast<int>(centerX),
										static_cast<int>(centerY),
										static_cast<int>(centerZ)));

//kL		_powerT = power + 1;
			_powerT = power; // kL: re-initialize _powerT, for each ray.
			r = 0.0;

			while (_powerT > 0
				&& r - 1.0 < r_Max) // kL_note: Allows explosions of 0 radius(!), single tile only hypothetically.
									// the idea is to show an explosion animation but affect only that one tile.
			{
				vx = centerX + r * sin_te * cos_fi;
				vy = centerY + r * cos_te * cos_fi;
				vz = centerZ + r * sin_fi;

				tileZ = static_cast<int>(floor(vz));
				tileX = static_cast<int>(floor(vx));
				tileY = static_cast<int>(floor(vy));

				destTile = _save->getTile(Position(
												tileX,
												tileY,
												tileZ));

				if (!destTile) // out of map!
					break;

				//Log(LOG_INFO) << ". _powerT = "	<< _powerT;
				//		<< ", dir = "		<< dir;
				//		<< ", origin "		<< origin->getPosition()
				//		<< " dest "			<< destTile->getPosition();

				//Log(LOG_INFO) << ". r = " << r;
				r += 1.0;

				testPower = _powerT;
				if (type == DT_IN)
				{
					int dir;
					Pathfinding::vectorToDirection(
											destTile->getPosition() - origin->getPosition(), // kL
											dir);
					if (dir != -1
						&& dir %2)
					{
						testPower -= 5; // diagonal movement costs an extra 50% for fire.
					}
				}

				testPower -= (10 // explosive damage decreases by 10 per tile
						+ horizontalBlockage( // not *2 -> try *2
										origin,
										destTile,
										type) * 2
						+ verticalBlockage( // not *2 -> try *2
										origin,
										destTile,
										type) * 2);

				if (testPower < 1)
					break;


				//Log(LOG_INFO) << ". _powerT > 0";
				if (type == DT_HE) // explosions do 50% damage to terrain and 50% to 150% damage to units
				{
					destTile->setExplosive(_powerT);
				}

				tilePair = tilesAffected.insert(destTile); // check if we had this tile already
				if (tilePair.second) // true if a new tile was inserted.
				{
					Log(LOG_INFO) << ". . new tile TRUE"
								<< ". _powerT = " << _powerT
								<< ". r = " << r - 1
								<< ". origin " << origin->getPosition()
								<< " dest " << destTile->getPosition();

					if (origin->getPosition().z != tileZ) // 3d explosion factor
						_powerT -= vertdec;

					BattleUnit* targetUnit = destTile->getUnit();
					if (targetUnit
						&& targetUnit->getTaken()) // -> THIS NEEDS TO BE REMOVED LATER (or earlier) !!!
					{
						targetUnit = NULL;
					}

					int wounds = 0;
					if (unit
						&& targetUnit)
					{
						wounds = targetUnit->getFatalWounds();
					}

					switch (type)
					{
						case DT_STUN: // power 0 - 200%
						{
							int powerVsUnit = RNG::generate(
														1,
														_powerT * 2);

							if (targetUnit)
							{
								if (distance(
											destTile->getPosition(),
											Position(
												static_cast<int>(centerX),
												static_cast<int>(centerY),
												static_cast<int>(centerZ)))
										< 2)
								{
									//Log(LOG_INFO) << ". . . powerVsUnit = " << powerVsUnit << " DT_STUN, GZ";
									targetUnit->damage(
													Position(0, 0, 0),
													powerVsUnit,
													DT_STUN);
								}
								else
								{
									//Log(LOG_INFO) << ". . . powerVsUnit = " << powerVsUnit << " DT_STUN, not GZ";
									targetUnit->damage(
													Position(
														static_cast<int>(centerX),
														static_cast<int>(centerY),
														static_cast<int>(centerZ)) - destTile->getPosition(),
													powerVsUnit,
													DT_STUN);
								}
							}

							for (std::vector<BattleItem*>::iterator
									item = destTile->getInventory()->begin();
									item != destTile->getInventory()->end();
									++item)
							{
								if ((*item)->getUnit())
								{
									//Log(LOG_INFO) << ". . . . powerVsUnit (corpse) = " << powerVsUnit << " DT_STUN";
									(*item)->getUnit()->damage(
															Position(0, 0, 0),
															powerVsUnit,
															DT_STUN);
								}
							}
						}
						break;
						case DT_HE: // power 50 - 150%, 65% of that if kneeled. 85% @ GZ
						{
							//Log(LOG_INFO) << ". . type == DT_HE";
							if (targetUnit)
							{
								int powerVsUnit = static_cast<int>(RNG::generate( // 50% to 150%
														static_cast<double>(_powerT) * 0.5,
														static_cast<double>(_powerT) * 1.5));

								if (distance(
											destTile->getPosition(),
											Position(
												static_cast<int>(centerX),
												static_cast<int>(centerY),
												static_cast<int>(centerZ)))
										< 2)
								{
									// ground zero effect is in effect
									//Log(LOG_INFO) << ". . . powerVsUnit = " << powerVsUnit << " DT_HE, GZ";
									if (targetUnit->isKneeled())
									{
										powerVsUnit = powerVsUnit * 17 / 20; // 85% damage
										//Log(LOG_INFO) << ". . . powerVsUnit(kneeled) = " << powerVsUnit << " DT_HE, GZ";
									}

									targetUnit->damage(
													Position(0, 0, 0),
													powerVsUnit,
													DT_HE);
								}
								else // directional damage relative to explosion position.
								{
									// units above the explosion will be hit in the legs, units lateral to or below will be hit in the torso

									//Log(LOG_INFO) << ". . . powerVsUnit = " << powerVsUnit << " DT_HE, not GZ";
									if (targetUnit->isKneeled())
									{
										powerVsUnit = powerVsUnit * 13 / 20; // 65% damage
										//Log(LOG_INFO) << ". . . powerVsUnit(kneeled) = " << powerVsUnit << " DT_HE, not GZ";
									}

									targetUnit->damage(
													Position(
														static_cast<int>(centerX),
														static_cast<int>(centerY),
														static_cast<int>(centerZ) + 5) - destTile->getPosition(),
													powerVsUnit,
													DT_HE);
								}
							}

							// kL_note: REVERT,
							bool done = false;
							while (!done)
							{
								done = destTile->getInventory()->empty();

								for (std::vector<BattleItem*>::iterator
										it = destTile->getInventory()->begin();
										it != destTile->getInventory()->end();
										)
								{
									if (_powerT > (*it)->getRules()->getArmor())
									{
										if ((*it)->getUnit()
											&& (*it)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
										{
											(*it)->getUnit()->instaKill();
										}

										_save->removeItem((*it));

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
										// kL_note: Could do instant smoke inhalation damage here (sorta like Fire or Stun).
							if (destTile->getSmoke() < 10
								&& destTile->getTerrainLevel() > -24)
							{
								destTile->setFire(0);
								destTile->setSmoke(RNG::generate(7, 15));
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
								if (destTile->getFire() == 0)
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
//kL										int fire = RNG::generate(4, 11);
										int firePower = RNG::generate( // kL: 25% - 75%
																	_powerT / 4,
																	_powerT * 3 / 4);

										targetUnit->damage(
														Position(
															0,
															0,
															12 - destTile->getTerrainLevel()),
														firePower,
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


					if (unit
						&& targetUnit)
					{
						// if it's going to bleed to death and it's not a player, give credit for the kill.
						// kL_note: See Above^
						if (wounds < targetUnit->getFatalWounds())
						{
							targetUnit->killedBy(unit->getFaction());
						}

						if (unit->getOriginalFaction() == FACTION_PLAYER			// kL, shooter is Xcom
							&& unit->getFaction() == FACTION_PLAYER					// kL, shooter is not Mc'd Xcom
							&& targetUnit->getOriginalFaction() == FACTION_HOSTILE	// kL, target is aLien Mc'd or not; no Xp for shooting civies...
							&& type != DT_SMOKE)									// sorry, no Xp for smoke!
//							&& targetUnit->getFaction() != unit->getFaction())
						{
							unit->addFiringExp();
						}
					}

					if (targetUnit)
						targetUnit->setTaken(true);
				}// add a new tile.

				_powerT = testPower;
				origin = destTile;
			}// power & radius left. (length ray)

		}// 360 degrees

	}// +- 90 degrees

	_powerT = 0;

	// now detonate the tiles affected with HE
	if (type == DT_HE)
	{
		//Log(LOG_INFO) << ". explode Tiles";
		for (std::set<Tile*>::iterator
				i = tilesAffected.begin();
				i != tilesAffected.end();
				++i)
		{
			if (detonate(*i))
				_save->setObjectiveDestroyed(true);

			applyGravity(*i);

			Tile* j = _save->getTile((*i)->getPosition() + Position(0, 0, 1));
			if (j)
				applyGravity(j);
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
 * Handles explosions.
 *
 * HE, smoke and fire explodes in a circular pattern on 1 level only. HE however damages floor tiles of the above level. Not the units on it.
 * HE destroys an object if its armor is lower than the explosive power, then it's HE blockage is applied for further propagation.
 * See http://www.ufopaedia.org/index.php?title=Explosions for more info.
 * @param center Center of the explosion in voxelspace.
 * @param power Power of the explosion.
 * @param type The damage type of the explosion.
 * @param maxRadius The maximum radius othe explosion.
 * @param unit The unit that caused the explosion.
 */
/* stock code:
void TileEngine::explode(const Position &center, int power, ItemDamageType type, int maxRadius, BattleUnit *unit)
{
	double centerZ = center.z / 24 + 0.5;
	double centerX = center.x / 16 + 0.5;
	double centerY = center.y / 16 + 0.5;
	int power_, penetration;
	std::set<Tile*> tilesAffected;
	std::pair<std::set<Tile*>::iterator,bool> ret;

	if (type == DT_IN)
	{
		power /= 2;
	}

	int exHeight = std::max(0, std::min(3, Options::battleExplosionHeight));
	int vertdec = 1000; //default flat explosion

	switch (exHeight)
	{
	case 1:
		vertdec = 30;
		break;
	case 2:
		vertdec = 10;
		break;
	case 3:
		vertdec = 5;
	}


	for (int fi = -90; fi <= 90; fi += 5)
//	for (int fi = 0; fi <= 0; fi += 10)
	{
		// raytrace every 3 degrees makes sure we cover all tiles in a circle.
		for (int te = 0; te <= 360; te += 3)
		{
			double cos_te = cos(te * M_PI / 180.0);
			double sin_te = sin(te * M_PI / 180.0);
			double sin_fi = sin(fi * M_PI / 180.0);
			double cos_fi = cos(fi * M_PI / 180.0);

			Tile *origin = _save->getTile(Position(centerX, centerY, centerZ));
			double l = 0;
			double vx, vy, vz;
			int tileX, tileY, tileZ;
			power_ = power + 1;
			penetration = power_;
			while (power_ > 0 && l <= maxRadius)
			{
				vx = centerX + l * sin_te * cos_fi;
				vy = centerY + l * cos_te * cos_fi;
				vz = centerZ + l * sin_fi;

				tileZ = int(floor(vz));
				tileX = int(floor(vx));
				tileY = int(floor(vy));

				Tile *dest = _save->getTile(Position(tileX, tileY, tileZ));
				if (!dest) break; // out of map!


				// blockage by terrain is deducted from the explosion power
				if (std::abs(l) > 0) // no need to block epicentrum
				{
					power_ -= 10; // explosive damage decreases by 10 per tile
					if (origin->getPosition().z != tileZ) power_ -= vertdec; //3d explosion factor
					if (type == DT_IN)
					{
						int dir;
						Pathfinding::vectorToDirection(origin->getPosition() - dest->getPosition(), dir);
						if (dir != -1 && dir %2) power_ -= 5; // diagonal movement costs an extra 50% for fire.
					}
					penetration = power_ - (horizontalBlockage(origin, dest, type) + verticalBlockage(origin, dest, type)) * 2;
				}

				if (penetration > 0)
				{
					if (type == DT_HE)
					{
						// explosives do 1/2 damage to terrain and 1/2 up to 3/2 random damage to units (the halving is handled elsewhere)
						dest->setExplosive(power_);
					}

					ret = tilesAffected.insert(dest); // check if we had this tile already
					if (ret.second)
					{
						int dmgRng = (type == DT_HE || Options::TFTDDamage) ? 50 : 100;
						int min = power_ * (100 - dmgRng) / 100;
						int max = power_ * (100 + dmgRng) / 100;
						BattleUnit *bu = dest->getUnit();
						int wounds = 0;
						if (bu && unit)
						{
							wounds = bu->getFatalWounds();
						}
						switch (type)
						{
						case DT_STUN:
							// power 0 - 200%
							if (bu)
							{
								if (distance(dest->getPosition(), Position(centerX, centerY, centerZ)) < 2)
								{
									bu->damage(Position(0, 0, 0), RNG::generate(min, max), type);
								}
								else
								{
									bu->damage(Position(centerX, centerY, centerZ) - dest->getPosition(), RNG::generate(min, max), type);
								}
							}
							for (std::vector<BattleItem*>::iterator it = dest->getInventory()->begin(); it != dest->getInventory()->end(); ++it)
							{
								if ((*it)->getUnit())
								{
									(*it)->getUnit()->damage(Position(0, 0, 0), RNG::generate(min, max), type);
								}
							}
							break;
						case DT_HE:
							{
								// power 50 - 150%
								if (bu)
								{
									if (distance(dest->getPosition(), Position(centerX, centerY, centerZ)) < 2)
									{
										// ground zero effect is in effect
										bu->damage(Position(0, 0, 0), (int)(RNG::generate(min, max)), type);
									}
									else
									{
										// directional damage relative to explosion position.
										// units above the explosion will be hit in the legs, units lateral to or below will be hit in the torso
										bu->damage(Position(centerX, centerY, centerZ + 5) - dest->getPosition(), (int)(RNG::generate(min, max)), type);
									}
								}
								bool done = false;
								while (!done)
								{
									done = dest->getInventory()->empty();
									for (std::vector<BattleItem*>::iterator it = dest->getInventory()->begin(); it != dest->getInventory()->end(); )
									{
										if (power_ > (*it)->getRules()->getArmor())
										{
											if ((*it)->getUnit() && (*it)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
												(*it)->getUnit()->instaKill();
											_save->removeItem((*it));
											break;
										}
										else
										{
											++it;
											done = it == dest->getInventory()->end();
										}
									}
								}
							}
							break;

						case DT_SMOKE:
							// smoke from explosions always stay 6 to 14 turns - power of a smoke grenade is 60
							if (dest->getSmoke() < 10 && dest->getTerrainLevel() > -24)
							{
								dest->setFire(0);
								dest->setSmoke(RNG::generate(7, 15));
							}
							break;

						case DT_IN:
							if (!dest->isVoid())
							{
								if (dest->getFire() == 0)
								{
									dest->setFire(dest->getFuel() + 1);
									dest->setSmoke(std::max(1, std::min(15 - (dest->getFlammability() / 10), 12)));
								}
								if (bu)
								{
									float resistance = bu->getArmor()->getDamageModifier(DT_IN);
									if (resistance > 0.0)
									{
										bu->damage(Position(0, 0, 12-dest->getTerrainLevel()), RNG::generate(5, 10), DT_IN, true);
										int burnTime = RNG::generate(0, int(5 * resistance));
										if (bu->getFire() < burnTime)
										{
											bu->setFire(burnTime); // catch fire and burn
										}
									}
								}
							}
							break;
						default:
							break;
						}

						if (unit && bu && bu->getFaction() != unit->getFaction())
						{
							unit->addFiringExp();
							// if it's going to bleed to death and it's not a player, give credit for the kill.
							if (wounds < bu->getFatalWounds() && bu->getFaction() != FACTION_PLAYER)
							{
								bu->killedBy(unit->getFaction());
							}
						}

					}
				}
				power_ = penetration;
				origin = dest;
				l++;
			}
		}
	}
	// now detonate the tiles affected with HE

	if (type == DT_HE)
	{
		for (std::set<Tile*>::iterator i = tilesAffected.begin(); i != tilesAffected.end(); ++i)
		{
			if (detonate(*i))
				_save->setObjectiveDestroyed(true);
			applyGravity(*i);
			Tile *j = _save->getTile((*i)->getPosition() + Position(0,0,1));
			if (j)
				applyGravity(j);
		}
	}

	calculateSunShading(); // roofs could have been destroyed
	calculateTerrainLighting(); // fires could have been started
	calculateFOV(center / Position(16,16,24));
} */

/**
 * Applies the explosive power to the tile parts.
 * This is where the actual destruction takes place.
 * Must affect 7 objects (6 box sides and the object inside).
 * @param tile, Tile affected.
 * @return, True if the objective was destroyed.
 */
bool TileEngine::detonate(Tile* tile)
{
	Log(LOG_INFO) << "TileEngine::detonate()";
	int explosive = tile->getExplosive();
	tile->setExplosive(0, true);

	bool objective = false;

	static const int parts[7] = {0, 1, 2, 0, 1, 2, 3};
	Position pos = tile->getPosition();

	Tile* tiles[7];
	tiles[0] = _save->getTile(Position( // ceiling
									pos.x,
									pos.y,
									pos.z + 1));
	tiles[1] = _save->getTile(Position( // east wall
									pos.x + 1,
									pos.y,
									pos.z));
	tiles[2] = _save->getTile(Position( // south wall
									pos.x,
									pos.y + 1,
									pos.z));
	tiles[3]	= tiles[4]
				= tiles[5]
				= tiles[6]
				= tile;

	if (explosive)
	{
		// explosions create smoke which only stays 1 or 2 turns; kL_note: or 3
		// smoke added to an already smoking tile will increase smoke to max.15
		tile->setSmoke(
					std::max(
							1,
							std::min(
									tile->getSmoke() + RNG::generate(0, 3),
									15)));

		int fuel = tile->getFuel() + 1;
		int flam = tile->getFlammability();
		int remainingPower = explosive;

		for (int
				i = 0;
				i < 7;
				++i)
		{
			if (tiles[i]
				&& tiles[i]->getMapData(parts[i]))
			{
				remainingPower = explosive;
//kL				while (remainingPower > -1
				while (remainingPower > 0 // kL
					&& tiles[i]->getMapData(parts[i]))
				{
					remainingPower -= 2 * tiles[i]->getMapData(parts[i])->getArmor();
//kL					if (remainingPower > -1)
					if (remainingPower > 0) // kL
					{
						int volume = 0;
						for (int // get the volume of the object by checking its loftemps objects.
								j = 0;
								j < 12;
								j++)
						{
							if (tiles[i]->getMapData(parts[i])->getLoftID(j) != 0)
								++volume;
						}

						if (i > 3)
						{
							tiles[i]->setFire(0);

							int
								smoke = RNG::generate(
													0,
													(volume / 2) + 2);
								smoke += (volume / 2) + 1;

							if (smoke > tiles[i]->getSmoke())
								tiles[i]->setSmoke(std::max(
														0,
														std::min(
																smoke,
																15)));
						}

						if (_save->getMissionType() == "STR_BASE_DEFENSE"
							&& i == 6
							&& tile->getMapData(MapData::O_OBJECT)
							&& tile->getMapData(VOXEL_OBJECT)->isBaseModule())
						{
							_save->getModuleMap()[tile->getPosition().x / 10][tile->getPosition().y / 10].second--;
						}

						if (tiles[i]->destroy(parts[i]))
							objective = true;

						if (tiles[i]->getMapData(parts[i]))
						{
							flam = tiles[i]->getFlammability();
							fuel = tiles[i]->getFuel() + 1;
						}
					}
				}

				if (i > 3
					&& remainingPower > flam * 2
					&& (tile->getMapData(MapData::O_FLOOR)
						|| tile->getMapData(MapData::O_OBJECT)))
				{
					tile->setFire(fuel);
					tile->setSmoke(std::max(
										1,
										std::min(
												15 - (flam / 10),
												12)));
				}
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::detonate() EXIT, objective = " << objective;
	return objective;
}

/**
 * Checks for chained explosions.
 *
 * Chained explosions are explosions which occur after an explosive map object is destroyed.
 * May be due a direct hit, other explosion or fire.
 * @return tile on which a explosion occurred
 */
Tile* TileEngine::checkForTerrainExplosions()
{
	for (int
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		if (_save->getTiles()[i]->getExplosive())
			return _save->getTiles()[i];
	}

	return 0;
}

/**
 * Calculates the amount of power that is blocked going from one tile to another on a different level.
 * Can cross more than one level. Only floor & object tiles are taken into account.
 * @param startTile, The tile where the power starts
 * @param endTile, The adjacent tile where the power ends
 * @param type, The type of power/damage
 * @return, Amount of blockage of this power
 */
int TileEngine::verticalBlockage(
		Tile* startTile,
		Tile* endTile,
		ItemDamageType type)
{
	//Log(LOG_INFO) << "TileEngine::verticalBlockage()";
	if (startTile == 0 // safety check
		|| endTile == 0)
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

	if (dirZ < 0) // down
	{
		for (
				;
				z > endTile->getPosition().z;
				z--)
		{
			block += blockage(
							_save->getTile(Position(x, y, z)),
							MapData::O_FLOOR,
							type)
					+ blockage(
							_save->getTile(Position(x, y, z)),
							MapData::O_OBJECT,
							type,
							Pathfinding::DIR_DOWN);
		}

		if (x != endTile->getPosition().x
			|| y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			z = startTile->getPosition().z;

			block += horizontalBlockage(
									startTile,
									_save->getTile(Position(x, y, z)),
									type);

			for (
					;
					z > endTile->getPosition().z;
					z--)
			{
				block += blockage(
								_save->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								type)
						+ blockage(
								_save->getTile(Position(x, y, z)),
								MapData::O_OBJECT,
								type); // note: no Dir vs typeOBJECT
			}
		}
	}
	else if (dirZ > 0) // up
	{
		for (
				z += 1;
				z <= endTile->getPosition().z;
				z++)
		{
			block += blockage(
							_save->getTile(Position(x, y, z)),
							MapData::O_FLOOR,
							type)
					+ blockage(
							_save->getTile(Position(x, y, z)),
							MapData::O_OBJECT,
							type,
							Pathfinding::DIR_UP);
		}

		if (x != endTile->getPosition().x
			|| y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			z = startTile->getPosition().z;

			block += horizontalBlockage(
									startTile,
									_save->getTile(Position(x, y, z)),
									type);

			for (
					z += 1;
					z <= endTile->getPosition().z;
					z++)
			{
				block += blockage(
								_save->getTile(Position(x, y, z)),
								MapData::O_FLOOR,
								type)
						+ blockage(
								_save->getTile(Position(x, y, z)),
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
 * Can cross more than one level. Only floor tiles are taken into account.
 * @param startTile The tile where the power starts.
 * @param endTile The adjacent tile where the power ends.
 * @param type The type of power/damage.
 * @return Amount of blockage of this power.
 */
/* stock code:
int TileEngine::verticalBlockage(Tile *startTile, Tile *endTile, ItemDamageType type)
{
	int block = 0;

	// safety check
	if (startTile == 0 || endTile == 0) return 0;
	int direction = endTile->getPosition().z - startTile->getPosition().z;

	if (direction == 0 ) return 0;

	int x = startTile->getPosition().x;
	int y = startTile->getPosition().y;

	if (direction < 0) // down
	{
		for (int z = startTile->getPosition().z; z > endTile->getPosition().z; z--)
		{
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type, Pathfinding::DIR_DOWN);
		}
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), type);
			for (; z > endTile->getPosition().z; z--)
			{
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type);
			}
		}
	}
	else if (direction > 0) // up
	{
		for (int z = startTile->getPosition().z + 1; z <= endTile->getPosition().z; z++)
		{
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type, Pathfinding::DIR_UP);
		}
		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			int z = startTile->getPosition().z;
			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), type);
			for (z = startTile->getPosition().z + 1; z <= endTile->getPosition().z; z++)
			{
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type);
			}
		}
	}

	return block;
} */

/**
 * Calculates the amount of power that is blocked going from one tile to another on the same level.
 * @param startTile, The tile where the power starts.
 * @param endTile, The adjacent tile where the power ends.
 * @param type, The type of power/damage.
 * @return, Amount of blockage (-1 for Big Wall tile, 0 on noBlock or ERROR).
 */
int TileEngine::horizontalBlockage(
		Tile* startTile,
		Tile* endTile,
		ItemDamageType type)
{
	//Log(LOG_INFO) << "TileEngine::horizontalBlockage()";
	if (startTile == 0 // safety checks
		|| endTile == 0
		|| startTile->getPosition().z != endTile->getPosition().z)
	{
		return 0;
	}


	int dir;
	Pathfinding::vectorToDirection(
							endTile->getPosition() - startTile->getPosition(),
							dir);

	if (dir == -1) // tilePos == tilePos.
		return 0;


	static const Position oneTileNorth	= Position( 0,-1, 0);
	static const Position oneTileEast	= Position( 1, 0, 0);
	static const Position oneTileSouth	= Position( 0, 1, 0);
	static const Position oneTileWest	= Position(-1, 0, 0);

	int block = 0;

	switch (dir)
	{
		case 0:	// north
			block = blockage(
							startTile,
							MapData::O_NORTHWALL,
							type,
							-1, // kL
							dir); // kL
		break;
		case 1: // north east
			if (type == DT_NONE) // this is two-way diagonal visibility check, used in original game
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
								_save->getTile(startTile->getPosition() + oneTileNorth),
								MapData::O_OBJECT,
								type,
								3);

				if (block == 0) // this way is opened
					break;

				block = blockage( // right+up
								_save->getTile(startTile->getPosition() + oneTileEast),
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileEast),
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileEast),
								MapData::O_OBJECT,
								type,
								7);
			}
			else // dt_NE
			{
				block = (blockage(
								startTile,
								MapData::O_NORTHWALL,
								type)
							+ blockage(
								endTile,
								MapData::O_WESTWALL,
								type))
							/ 2
						+ (blockage(
								_save->getTile(startTile->getPosition() + oneTileEast),
								MapData::O_WESTWALL,
								type)
							+ blockage(
								_save->getTile(startTile->getPosition() + oneTileEast),
								MapData::O_NORTHWALL,
								type))
							/ 2;

				if (!endTile->getMapData(MapData::O_OBJECT))
				{
					block += (blockage(
									_save->getTile(startTile->getPosition() + oneTileEast),
									MapData::O_OBJECT,
									type,
									dir)
								+ blockage(
									_save->getTile(startTile->getPosition() + oneTileNorth),
									MapData::O_OBJECT,
									type,
									4)
								+ blockage(
									_save->getTile(startTile->getPosition() + oneTileNorth),
									MapData::O_OBJECT,
									type,
									2))
								/ 2;
				}

//				if (type == DT_HE)
//					block += (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4)
//							+ blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 6)) / 2;
//				else
//					block = (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4)
//							+ blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 6)) < 510? 0: 255;
			}
		break;
		case 2: // east
			block = blockage(
							endTile,
							MapData::O_WESTWALL,
							type,
							-1, // kL
							dir); // kL
		break;
		case 3: // south east
			if (type == DT_NONE)
			{
				block = blockage( // down+right
								_save->getTile(startTile->getPosition() + oneTileSouth),
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								endTile,
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileSouth),
								MapData::O_OBJECT,
								type,
								1);

				if (block == 0) // this way is opened
					break;

				block = blockage( // right+down
								_save->getTile(startTile->getPosition() + oneTileEast),
								MapData::O_WESTWALL,
								type)
						+ blockage(
								endTile,
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileEast),
								MapData::O_OBJECT,
								type,
								5);
			}
			else // dt_SE
			{
				block = (blockage(
								endTile,
								MapData::O_WESTWALL,
								type)
							+ blockage(
								endTile,
								MapData::O_NORTHWALL,
								type))
							/ 2
						+ (blockage(
								_save->getTile(startTile->getPosition() + oneTileEast),
								MapData::O_WESTWALL,
								type)
							+ blockage(
								_save->getTile(startTile->getPosition() + oneTileSouth),
								MapData::O_NORTHWALL,
								type))
							/ 2;

				if (!endTile->getMapData(MapData::O_OBJECT))
				{
					block += (blockage(
									_save->getTile(startTile->getPosition() + oneTileSouth),
									MapData::O_OBJECT,
									type,
									2)
								+ blockage(
									_save->getTile(startTile->getPosition() + oneTileEast),
									MapData::O_OBJECT,
									type,
									4))
								/ 2;
				}

//				if (type == DT_HE)
//					block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 0) +
//							blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 6)) / 2;
//				else
//					block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 0) +
//							blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 6)) < 510? 0: 255;
			}
		break;
		case 4: // south
			block = blockage(
							endTile,
							MapData::O_NORTHWALL,
							type,
							-1, // kL
							dir); // kL
		break;
		case 5: // south west
			if (type == DT_NONE)
			{
				block = blockage( // down+left
								_save->getTile(startTile->getPosition() + oneTileSouth),
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileSouth),
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileSouth),
								MapData::O_OBJECT,
								type,
								7);

				if (block == 0) // this way is opened
					break;

				block = blockage( // left+down
								startTile,
								MapData::O_WESTWALL,
								type)
						+ blockage(
								endTile,
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileWest),
								MapData::O_OBJECT,
								type,
								3);
			}
			else // dt_SW
			{
				block = (blockage(
								endTile,
								MapData::O_NORTHWALL,
								type)
							+ blockage(
								startTile,
								MapData::O_WESTWALL,
								type))
							/ 2
						+ (blockage(
								_save->getTile(startTile->getPosition() + oneTileSouth),
								MapData::O_WESTWALL,
								type)
							+ blockage(
								_save->getTile(startTile->getPosition() + oneTileSouth),
								MapData::O_NORTHWALL,
								type))
							/ 2;

				if (!endTile->getMapData(MapData::O_OBJECT))
				{
					block += (blockage(
									_save->getTile(startTile->getPosition() + oneTileSouth),
									MapData::O_OBJECT,
									type,
									dir)
								+ blockage(
									_save->getTile(startTile->getPosition() + oneTileWest),
									MapData::O_OBJECT,
									type,
									2)
								+ blockage(
									_save->getTile(startTile->getPosition() + oneTileWest),
									MapData::O_OBJECT,
									type,
									4))
								/ 2;
				}

//				if (type == DT_HE)
//					block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 0) +
//							blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2)) / 2;
//				else
//					block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 0) +
//							blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2)) < 510? 0: 255;
			}
		break;
		case 6: // west
			block = blockage(
							startTile,
							MapData::O_WESTWALL,
							type,
							-1, // kL
							dir); // kL
		break;
		case 7: // north west
			if (type == DT_NONE)
			{
				block = blockage( // up+left
								startTile,
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileNorth),
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileNorth),
								MapData::O_OBJECT,
								type,
								5);

				if (block == 0) // this way is opened
					break;

				block = blockage( // left+up
								startTile,
								MapData::O_WESTWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileWest),
								MapData::O_NORTHWALL,
								type)
						+ blockage(
								_save->getTile(startTile->getPosition() + oneTileWest),
								MapData::O_OBJECT,
								type,
								1);
			}
			else // dt_NW
			{
				block = (blockage(
								startTile,
								MapData::O_WESTWALL,
								type)
							+ blockage(
								startTile,
								MapData::O_NORTHWALL,
								type))
							/ 2
						+ (blockage(
								_save->getTile(startTile->getPosition() + oneTileNorth),
								MapData::O_WESTWALL,
								type)
							+ blockage(
								_save->getTile(startTile->getPosition() + oneTileWest),
								MapData::O_NORTHWALL,
								type))
							/ 2;

				if (!endTile->getMapData(MapData::O_OBJECT))
				{
					block += (blockage(
									_save->getTile(startTile->getPosition() + oneTileNorth),
									MapData::O_OBJECT,
									type,
									4)
								+ blockage(
									_save->getTile(startTile->getPosition() + oneTileWest),
									MapData::O_OBJECT,
									type,
									2))
								/ 2;
				}

//				if (type == DT_HE)
//					block += (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4) +
//							blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2))/2;
//				else
//					block += (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4) +
//							blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2)) < 510? 0: 255;
			}
		break;

		default:
		break;
	}

    block += blockage(
					startTile,
					MapData::O_OBJECT,
					type,
					dir,
					-1,
					true);

	if (type != DT_NONE)
	{
		dir += 4;
		if (dir > 7)
			dir -= 8;

		block += blockage(
						endTile,
						MapData::O_OBJECT,
						type,
						dir);
	}
	else // type == DT_NONE
	{
        if (block <= 127)
        {
            dir += 4;
            if (dir > 7)
                dir -= 8;

            if (blockage(
						endTile,
						MapData::O_OBJECT,
						type,
						dir)
					> 127)
			{
				//Log(LOG_INFO) << "TileEngine::horizontalBlockage() EXIT, ret = -1";
				return -1; // hit bigwall, reveal bigwall tile
			}
        }
	}

	//Log(LOG_INFO) << "TileEngine::horizontalBlockage() EXIT, ret = " << block;
	return block;
}

/**
 * Calculates the amount of power that is blocked going from one tile to another on the same level.
 * @param startTile The tile where the power starts.
 * @param endTile The adjacent tile where the power ends.
 * @param type The type of power/damage.
 * @return Amount of blockage.
 */
/* stock code:
int TileEngine::horizontalBlockage(Tile *startTile, Tile *endTile, ItemDamageType type)
{
	static const Position oneTileNorth = Position(0, -1, 0);
	static const Position oneTileEast = Position(1, 0, 0);
	static const Position oneTileSouth = Position(0, 1, 0);
	static const Position oneTileWest = Position(-1, 0, 0);

	// safety check
	if (startTile == 0 || endTile == 0) return 0;
	if (startTile->getPosition().z != endTile->getPosition().z) return 0;

	int direction;
	Pathfinding::vectorToDirection(endTile->getPosition() - startTile->getPosition(), direction);
	if (direction == -1) return 0;
	int block = 0;

	switch(direction)
	{
	case 0:	// north
		block = blockage(startTile, MapData::O_NORTHWALL, type);
		break;
	case 1: // north east
		if (type == DT_NONE) //this is two-way diagonal visiblity check, used in original game
		{
			block = blockage(startTile, MapData::O_NORTHWALL, type) + blockage(endTile, MapData::O_WESTWALL, type); //up+right
			block += blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 3);
			if (block == 0) break; //this way is opened
			block = blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_NORTHWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_WESTWALL, type); //right+up
			block += blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 7);
		}
		else
		{
			block = (blockage(startTile,MapData::O_NORTHWALL, type) + blockage(endTile,MapData::O_WESTWALL, type))/2
				+ (blockage(_save->getTile(startTile->getPosition() + oneTileEast),MapData::O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileEast),MapData::O_NORTHWALL, type))/2;

			if (!endTile->getMapData(MapData::O_OBJECT))
			{
				block += (blockage(_save->getTile(startTile->getPosition() + oneTileEast),MapData::O_OBJECT, type, direction)
					+ blockage(_save->getTile(startTile->getPosition() + oneTileNorth),MapData::O_OBJECT, type, 4)
					+ blockage(_save->getTile(startTile->getPosition() + oneTileNorth),MapData::O_OBJECT, type, 2))/2;
			}
		}
		break;
	case 2: // east
		block = blockage(endTile,MapData::O_WESTWALL, type);
		break;
	case 3: // south east
		if (type == DT_NONE)
		{
			block = blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_NORTHWALL, type)
				+ blockage(endTile, MapData::O_WESTWALL, type); //down+right
			block += blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 1);
			if (block == 0) break; //this way is opened
			block = blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_WESTWALL, type)
				+ blockage(endTile, MapData::O_NORTHWALL, type); //right+down
			block += blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 5);
		}
		else
		{
			block = (blockage(endTile,MapData::O_WESTWALL, type) + blockage(endTile,MapData::O_NORTHWALL, type))/2
				+ (blockage(_save->getTile(startTile->getPosition() + oneTileEast),MapData::O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth),MapData::O_NORTHWALL, type))/2;

			if (!endTile->getMapData(MapData::O_OBJECT))
			{
				block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth),MapData::O_OBJECT, type, 2)
					+ blockage(_save->getTile(startTile->getPosition() + oneTileEast),MapData::O_OBJECT, type, 4))/2;
			}
		}
		break;
	case 4: // south
		block = blockage(endTile,MapData::O_NORTHWALL, type);
		break;
	case 5: // south west
		if (type == DT_NONE)
		{
			block = blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_NORTHWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_WESTWALL, type); //down+left
			block += blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 7);
			if (block == 0) break; //this way is opened
			block = blockage(startTile, MapData::O_WESTWALL, type) + blockage(endTile, MapData::O_NORTHWALL, type); //left+down
			block += blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 3);
		}
		else
		{
			block = (blockage(endTile,MapData::O_NORTHWALL, type) + blockage(startTile,MapData::O_WESTWALL, type))/2
				+ (blockage(_save->getTile(startTile->getPosition() + oneTileSouth),MapData::O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth),MapData::O_NORTHWALL, type))/2;
			if (!endTile->getMapData(MapData::O_OBJECT))
			{
				block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth),MapData::O_OBJECT, type, direction)
					+ blockage(_save->getTile(startTile->getPosition() + oneTileWest),MapData::O_OBJECT, type, 2)
					+ blockage(_save->getTile(startTile->getPosition() + oneTileWest),MapData::O_OBJECT, type, 4))/2;
			}
		}
		break;
	case 6: // west
		block = blockage(startTile,MapData::O_WESTWALL, type);
		break;
	case 7: // north west

		if (type == DT_NONE)
		{
			block = blockage(startTile, MapData::O_NORTHWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_WESTWALL, type); //up+left
			block += blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 5);
			if (block == 0) break; //this way is opened
			block = blockage(startTile, MapData::O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_NORTHWALL, type); //left+up
			block += blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 1);
		}
		else
		{
			block = (blockage(startTile,MapData::O_WESTWALL, type) + blockage(startTile,MapData::O_NORTHWALL, type))/2
				+ (blockage(_save->getTile(startTile->getPosition() + oneTileNorth),MapData::O_WESTWALL, type)
				+ blockage(_save->getTile(startTile->getPosition() + oneTileWest),MapData::O_NORTHWALL, type))/2;

			if (!endTile->getMapData(MapData::O_OBJECT))
			{
				block += (blockage(_save->getTile(startTile->getPosition() + oneTileNorth),MapData::O_OBJECT, type, 4)
					+ blockage(_save->getTile(startTile->getPosition() + oneTileWest),MapData::O_OBJECT, type, 2))/2;
			}
		}
		break;
	}

    block += blockage(startTile,MapData::O_OBJECT, type, direction, true);
//    block += blockage(startTile,MapData::O_OBJECT, type, direction, -1, true); // kL
	if (type != DT_NONE)
	{
		direction += 4;
		if (direction > 7)
			direction -= 8;
		block += blockage(endTile,MapData::O_OBJECT, type, direction);
	}
	else
	{
        if ( block <= 127 )
        {
            direction += 4;
            if (direction > 7)
                direction -= 8;
            if (blockage(endTile,MapData::O_OBJECT, type, direction) > 127){
                return -1; //hit bigwall, reveal bigwall tile
            }
        }
	}

	return block;
} */

/**
 * Calculates the amount of damage-power or FoV that certain types of
 * wall/bigwall or floor or object parts of a tile blocks.
 * @param startTile, The tile where the power starts
 * @param part, The part of the tile the power needs to go through
 * @param type, The type of power/damage
 * @param dir, Direction the power travels (default -1)
 * @param dirTest, Direction for not blocking FoV when gazing down a wall (default -1)
 * @return, Amount of power/damage that gets blocked.
 */
int TileEngine::blockage(
		Tile* tile,
		const int part,
		ItemDamageType type,
		int dir,
		int dirTest, // kL_add.
		bool checkingFromOrigin)
{
	//Log(LOG_INFO) << "TileEngine::blockage() dir " << dir;

	// open ufo doors are actually still closed behind the scenes. So a special
	// trick is needed to see if they are open. If they are open, block=0
	if (tile == 0 // probably outside the map here
		|| tile->isUfoDoorOpen(part))
	{
		//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( ufoDoorOpen )";
		return 0;
	}

	if (tile->getMapData(part))
	{
		//Log(LOG_INFO) << ". getMapData(part) stopLOS() = " << tile->getMapData(part)->stopLOS();
		if (dir == -1) // west/north wall or floor.
		{
			if (type == DT_NONE
				|| type == DT_SMOKE
				|| type == DT_STUN
				|| type == DT_IN)
			{
//				int dirTest;
//				Pathfinding::vectorToDirection(
//										endTile->getPosition() - startTile->getPosition(),
//										dirTest);

				if (tile->getMapData(part)->stopLOS()
					|| tile->getMapData(part)->getObjectType() == MapData::O_FLOOR)
				{
					return 255; // hardblock.
				}
				else
					return 0;

/* Old
				if (!tile->getMapData(part)->stopLOS()
					&& tile->getMapData(part)->getObjectType() != MapData::O_FLOOR)
//					&& (tile->getMapData(part)->getObjectType() == MapData::O_WESTWALL
//						|| tile->getMapData(part)->getObjectType() == MapData::O_NORTHWALL))
				{
					return 0;
				}
//				else if (tile->getMapData(part)->getObjectType() == MapData::O_FLOOR
//					|| tile->getMapData(part)->stopLOS())
//				{
//					return 255; // hardblock.
//				}
				else if (tile->getMapData(part)->getObjectType() == MapData::O_FLOOR
					|| tile->getMapData(part)->stopLOS())
//						&& ((tile->getMapData(part)->getObjectType() == MapData::O_WESTWALL
//								&& (dirTest == 0
//									|| dirTest == 4))
//							|| (tile->getMapData(part)->getObjectType() == MapData::O_NORTHWALL
//								&& (dirTest == 2
//									|| dirTest == 6)))))
				{
					return 255; // hardblock.
				}
Old_end */

			}
			else if (_powerT < tile->getMapData(part)->getArmor())
				return 255;
		}
		else if (dir != -1)
		{
			int bigWall = tile->getMapData(MapData::O_OBJECT)->getBigWall(); // 0..8 or, per MCD.
			//Log(LOG_INFO) << ". bigWall = " << bigWall;

			if (checkingFromOrigin
				&& (   bigWall == Pathfinding::BIGWALLNESW
					|| bigWall == Pathfinding::BIGWALLNWSE))
			{
				return 0;
			}

			switch (dir)
			{
				case 0: // north
					if (   bigWall == Pathfinding::BIGWALLWEST
						|| bigWall == Pathfinding::BIGWALLEAST
						|| bigWall == Pathfinding::BIGWALLSOUTH
						|| bigWall == Pathfinding::BIGWALLEASTANDSOUTH)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 0 north )";
						return 0; // part By-passed.
					}
				break;

				case 1: // north east
					if (   bigWall == Pathfinding::BIGWALLWEST
						|| bigWall == Pathfinding::BIGWALLSOUTH)
//						|| bigWall == Pathfinding::BIGWALLNESW) // kL
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 1 northeast )";
						return 0;
					}
				break;

				case 2: // east
					if (   bigWall == Pathfinding::BIGWALLNORTH
						|| bigWall == Pathfinding::BIGWALLSOUTH
						|| bigWall == Pathfinding::BIGWALLWEST)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 2 east )";
						return 0;
					}
				break;

				case 3: // south east
					if (   bigWall == Pathfinding::BIGWALLNORTH
						|| bigWall == Pathfinding::BIGWALLWEST)
//						|| bigWall == Pathfinding::BIGWALLNWSE) // kL
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 3 southeast )";
						return 0;
					}
				break;

				case 4: // south
					if (   bigWall == Pathfinding::BIGWALLWEST
						|| bigWall == Pathfinding::BIGWALLEAST
						|| bigWall == Pathfinding::BIGWALLNORTH)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 4 south )";
						return 0;
					}
				break;

				case 5: // south west
					if (   bigWall == Pathfinding::BIGWALLNORTH
						|| bigWall == Pathfinding::BIGWALLEAST)
//						|| bigWall == Pathfinding::BIGWALLNESW) // kL
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 5 southwest )";
						return 0;
					}
				break;

				case 6: // west
					if (   bigWall == Pathfinding::BIGWALLNORTH
						|| bigWall == Pathfinding::BIGWALLSOUTH
						|| bigWall == Pathfinding::BIGWALLEAST
						|| bigWall == Pathfinding::BIGWALLEASTANDSOUTH)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 6 west )";
						return 0;
					}
				break;

				case 7: // north west
					if (   bigWall == Pathfinding::BIGWALLSOUTH
						|| bigWall == Pathfinding::BIGWALLEAST
						|| bigWall == Pathfinding::BIGWALLEASTANDSOUTH)
//						|| bigWall == Pathfinding::BIGWALLNWSE) // kL
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 7 northwest )";
						return 0;
					}
				break;

				case 8: // up
				case 9: // down
					if (bigWall != Pathfinding::BLOCK)
					{
						//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0 ( dir 8,9 up,down )";
						return 0;
					}
				break;

				default:
//						return 0;
				break;
			}

			if (tile->getMapData(part)->stopLOS() // if (bigWall && ...
				&& (   type == DT_NONE
					|| type == DT_SMOKE
					|| type == DT_STUN
					|| type == DT_IN
					|| _powerT < tile->getMapData(part)->getArmor()))
			{
				return 255;
			}
		}

		if (type != DT_NONE) // FoV is blocked above, or gets a pass here ( ie. vs Content )
		{
			int ret = tile->getMapData(part)->getBlock(type);
			//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret = " << ret;

			return ret;
		}
	}

	//Log(LOG_INFO) << "TileEngine::blockage() EXIT, ret 0";
	return 0;
}

/**
 * Calculates the amount this certain wall or floor-part of the tile blocks.
 * @param startTile The tile where the power starts.
 * @param part The part of the tile the power needs to go through.
 * @param type The type of power/damage.
 * @param direction Direction the power travels.
 * @return Amount of blockage.
 */
/* stock code:
int TileEngine::blockage(Tile *tile, const int part, ItemDamageType type, int direction, bool checkingFromOrigin)
{
	int blockage = 0;

	if (tile == 0) return 0; // probably outside the map here
	if (tile->getMapData(part))
	{
		bool check = true;
		int wall = -1;
		if (direction != -1)
		{
			wall = tile->getMapData(MapData::O_OBJECT)->getBigWall();

			if (checkingFromOrigin &&
				(wall == Pathfinding::BIGWALLNESW ||
				wall == Pathfinding::BIGWALLNWSE))
			{
				check = false;
			}
			switch (direction)
			{
			case 0: // north
				if (wall == Pathfinding::BIGWALLWEST ||
					wall == Pathfinding::BIGWALLEAST ||
					wall == Pathfinding::BIGWALLSOUTH ||
					wall == Pathfinding::BIGWALLEASTANDSOUTH)
				{
					check = false;
				}
				break;
			case 1: // north east
				if (wall == Pathfinding::BIGWALLWEST ||
					wall == Pathfinding::BIGWALLSOUTH)
				{
					check = false;
				}
				break;
			case 2: // east
				if (wall == Pathfinding::BIGWALLNORTH ||
					wall == Pathfinding::BIGWALLSOUTH ||
					wall == Pathfinding::BIGWALLWEST)
				{
					check = false;
				}
				break;
			case 3: // south east
				if (wall == Pathfinding::BIGWALLNORTH ||
					wall == Pathfinding::BIGWALLWEST)
				{
					check = false;
				}
				break;
			case 4: // south
				if (wall == Pathfinding::BIGWALLWEST ||
					wall == Pathfinding::BIGWALLEAST ||
					wall == Pathfinding::BIGWALLNORTH)
				{
					check = false;
				}
				break;
			case 5: // south west
				if (wall == Pathfinding::BIGWALLNORTH ||
					wall == Pathfinding::BIGWALLEAST)
				{
					check = false;
				}
				break;
			case 6: // west
				if (wall == Pathfinding::BIGWALLNORTH ||
					wall == Pathfinding::BIGWALLSOUTH ||
					wall == Pathfinding::BIGWALLEAST ||
					wall == Pathfinding::BIGWALLEASTANDSOUTH)
				{
					check = false;
				}
				break;
			case 7: // north west
				if (wall == Pathfinding::BIGWALLSOUTH ||
					wall == Pathfinding::BIGWALLEAST ||
					wall == Pathfinding::BIGWALLEASTANDSOUTH)
				{
					check = false;
				}
				break;
			case 8: // up
			case 9: // down
				if (wall != 0 && wall != Pathfinding::BLOCK)
				{
					check = false;
				}
				break;
			default:
				break;
			}
		}

		if (check)
		{
			// -1 means we have a regular wall, and anything over 0 means we have a bigwall.
			if (type == DT_SMOKE && wall != 0 && !tile->isUfoDoorOpen(part))
			{
				return 256;
			}
			blockage += tile->getMapData(part)->getBlock(type);
		}
	}

	// open ufo doors are actually still closed behind the scenes
	// so a special trick is needed to see if they are open, if they are, they obviously don't block anything
	if (tile->isUfoDoorOpen(part))
		blockage = 0;

	return blockage;
} */

/**
 * Opens a door (if any) by rightclick, or by walking through it. The unit has to face in the right direction.
 * @param unit Unit.
 * @param rClick Whether the player right clicked.
 * @param dir Direction.
 * @return, -1 there is no door, you can walk through; or you're a tank and can't do sweet shit with a door except blast the fuck out of it.
 *			0 normal door opened, make a squeaky sound and you can walk through;
 *			1 ufo door is starting to open, make a whoosh sound, don't walk through;
 *			3 ufo door is still opening, don't walk through it yet. (have patience, futuristic technology...)
 *			4 not enough TUs
 *			5 would contravene fire reserve
 */
int TileEngine::unitOpensDoor(
		BattleUnit* unit,
		bool rClick,
		int dir)
{
	//Log(LOG_INFO) << "unitOpensDoor()";
	int
		TUCost = 0,
		door = -1;

	int size = unit->getArmor()->getSize();
	if (size > 1
		&& rClick)
	{
		return door;
	}

	if (dir == -1)
		dir = unit->getDirection();

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
			Tile* tile = _save->getTile(
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
					if (rClick)
					{
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_NORTHWALL));	// one tile east
					}
/*					if (rClick
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
					if (rClick)
					{
						checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
						checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
					}
/*					if (rClick
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
					if (rClick)
					{
						checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_WESTWALL));	// one tile south
						checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
					}
/*					if (rClick
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
					if (rClick)
					{
						checkPositions.push_back(std::make_pair(Position( 0,-1, 0), MapData::O_WESTWALL));	// one tile north
						checkPositions.push_back(std::make_pair(Position(-1, 0, 0), MapData::O_NORTHWALL));	// one tile west
					}
/*					if (rClick
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

			int wall = 0;
			for (std::vector<std::pair<Position, int> >::const_iterator
					i = checkPositions.begin();
					i != checkPositions.end()
						&& door == -1;
					++i)
			{
				tile = _save->getTile(
									posUnit
										+ Position(x, y, z)
										+ i->first);
				if (tile)
				{
					door = tile->openDoor(
										i->second,
										unit,
										_save->getBattleGame()->getReservedAction());
					if (door != -1)
					{
						wall = i->second;

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
				&& rClick)
			{
				if (wall == MapData::O_WESTWALL)
					wall = MapData::O_NORTHWALL;
				else
					wall = MapData::O_WESTWALL;

				TUCost = tile->getTUCost(
									wall,
									unit->getArmor()->getMovementType());
			}
			else if (door == 1
				|| door == 4)
			{
				TUCost = tile->getTUCost(
									wall,
									unit->getArmor()->getMovementType());
			}
		}
	}

	if (TUCost != 0)
	{
		if (_save->getBattleGame()->checkReservedTU(unit, TUCost))
		{
			if (unit->spendTimeUnits(TUCost))
			{
				calculateFOV(unit->getPosition());

				// look from the other side (may be need check reaction fire?)
				// kL_note: and what about mutual surprise rule?
				std::vector<BattleUnit*>* visUnits = unit->getVisibleUnits();
				for (size_t
						i = 0;
						i < visUnits->size();
						++i)
				{
					calculateFOV(visUnits->at(i));
				}
			}
			else
				return 4;
		}
		else
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
 * kL. Checks for a door connected to this wall at this position
 * so that units can open double doors diagonally.
 * @param pos The starting position
 * @param wall The wall to test
 * @param dir The direction to check out
 */
bool TileEngine::testAdjacentDoor( // kL
		Position pos,
		int wall,
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

	Tile* tile = _save->getTile(pos + offset);
	if (tile
		&& tile->getMapData(wall)
		&& tile->getMapData(wall)->isUFODoor())
	{
		return true;
	}

	return false;
}

/**
 * Opens any doors connected to this wall at this position,
 * Keeps processing till it hits a non-ufo-door.
 * @param pos The starting position
 * @param wall The wall to open, defines which direction to check.
 */
void TileEngine::openAdjacentDoors(
		Position pos,
		int wall)
{
	Position offset;
	bool westSide = (wall == 1);

	for (int
			i = 1;
			;
			++i)
	{
		offset = westSide? Position(0, i, 0): Position(i, 0, 0);
		Tile* tile = _save->getTile(pos + offset);
		if (tile
			&& tile->getMapData(wall)
			&& tile->getMapData(wall)->isUFODoor())
		{
			tile->openDoor(wall);
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
		Tile* tile = _save->getTile(pos + offset);
		if (tile
			&& tile->getMapData(wall)
			&& tile->getMapData(wall)->isUFODoor())
		{
			tile->openDoor(wall);
		}
		else
			break;
	}
}

/**
 * Closes ufo doors.
 * @return, Whether doors are closed.
 */
int TileEngine::closeUfoDoors()
{
	int doorsclosed = 0;

	for (int // prepare a list of tiles on fire/smoke & close any ufo doors
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		if (_save->getTiles()[i]->getUnit()
			&& _save->getTiles()[i]->getUnit()->getArmor()->getSize() > 1)
		{
			BattleUnit* bu = _save->getTiles()[i]->getUnit();

			Tile* tile = _save->getTiles()[i];
			Tile* oneTileNorth = _save->getTile(tile->getPosition() + Position(0,-1, 0));
			Tile* oneTileWest = _save->getTile(tile->getPosition() + Position(-1, 0, 0));
			if ((tile->isUfoDoorOpen(MapData::O_NORTHWALL)
					&& oneTileNorth
					&& oneTileNorth->getUnit()
					&& oneTileNorth->getUnit() == bu)
				|| (tile->isUfoDoorOpen(MapData::O_WESTWALL)
					&& oneTileWest
					&& oneTileWest->getUnit()
					&& oneTileWest->getUnit() == bu))
			{
				continue;
			}
		}

		doorsclosed += _save->getTiles()[i]->closeUfoDoor();
	}

	return doorsclosed;
}

/**
 * Calculates a line trajectory, using bresenham algorithm in 3D.
 * @param origin, Origin (voxel??).
 * @param target, Target (also voxel??).
 * @param storeTrajectory, True will store the whole trajectory - otherwise it just stores the last position.
 * @param trajectory, A vector of positions in which the trajectory is stored.
 * @param excludeUnit, Excludes this unit in the collision detection.
 * @param doVoxelCheck, Check against voxel or tile blocking? (first one for unit visibility and line of fire, second one for terrain visibility).
 * @param onlyVisible, Skip invisible units? used in FPS view.
 * @param excludeAllBut, [Optional] The only unit to be considered for ray hits.
 * @return, The objectnumber(0-3) or unit(4) or out-of-map(5) or -1(hit nothing).
 */
int TileEngine::calculateLine(
				const Position& origin,
				const Position& target,
				bool storeTrajectory,
				std::vector<Position>* trajectory,
				BattleUnit* excludeUnit,
				bool doVoxelCheck,
				bool onlyVisible,
				BattleUnit* excludeAllBut)
{
	//Log(LOG_INFO) << "TileEngine::calculateLine()";
	int
		x,
		x0,
		x1,
		delta_x,
		step_x,

		y,
		y0,
		y1,
		delta_y,
		step_y,

		z,
		z0,
		z1,
		delta_z,
		step_z,

		swap_xy,
		swap_xz,

		drift_xy,
		drift_xz,

		cx,
		cy,
		cz,

		result,
		result2;

	Position lastPoint (origin);

	x0 = origin.x; // start and end points
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

		if (doVoxelCheck) // passes through this voxel, for Unit visibility
		{
			result = voxelCheck(
							Position(cx, cy, cz),
							excludeUnit,
							false,
							onlyVisible,
							excludeAllBut);
			if (result != VOXEL_EMPTY)
			{
				if (trajectory) // store the position of impact
					trajectory->push_back(Position(cx, cy, cz));

				//Log(LOG_INFO) << ". [1]ret = " << result;
				return result;
			}
		}
		else // for Terrain visibility
		{
			result = horizontalBlockage(
									_save->getTile(lastPoint),
									_save->getTile(Position(cx, cy, cz)),
									DT_NONE);
			result2 = verticalBlockage(
									_save->getTile(lastPoint),
									_save->getTile(Position(cx, cy, cz)),
									DT_NONE);
			if (result == -1)
			{
				if (result2 > 127)
					result = 0;
				else
					//Log(LOG_INFO) << "TileEngine::calculateLine(), odd return1, could be Big Wall = " << result;
					//Log(LOG_INFO) << ". [2]ret = " << result;
					return result; // We hit a big wall. kL_note: BigWall? or just ... wall or other visBlock object
			}

			result += result2;
			if (result > 127)
				//Log(LOG_INFO) << "TileEngine::calculateLine(), odd return2, could be Big Wall = " << result;
				//Log(LOG_INFO) << ". [3]ret = " << result;
				return result;

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

					//Log(LOG_INFO) << ". [4]ret = " << result;
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

					//Log(LOG_INFO) << ". [5]ret = " << result;
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
 * @param origin, Orign in voxelspace.
 * @param target, Target in voxelspace.
 * @param storeTrajectory, True will store the whole trajectory - otherwise it just stores the last position.
 * @param trajectory, A vector of positions in which the trajectory is stored.
 * @param excludeUnit, Makes sure the trajectory does not hit the shooter itself.
 * @param arc, How high the parabola goes: 1.0 is almost straight throw, 3.0 is a very high throw, to throw over a fence for example.

 * @param acu, Is the deviation of the angles it should take into account. 1.0 is perfection. // this is superceded by @param delta...
Wb.131129 * @param delta Is the deviation of the angles it should take into account, 0,0,0 is perfection.

 * @return, The objectnumber(0-3) or unit(4) or out of map (5) or -1(hit nothing).
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
								0,
								excludeUnit);
//		int test = voxelCheck(
//							Position(x, y, z),
//							excludeUnit);
		if (test != VOXEL_EMPTY)
		{
			if (lastPosition.z < nextPosition.z)
				test = VOXEL_OUTOFBOUNDS;

			if (!storeTrajectory // store only the position of impact
				&& trajectory != 0)
			{
				trajectory->push_back(nextPosition);
			}

			return test;
		}

		lastPosition = Position(x, y, z);
		++i;
	}

	if (!storeTrajectory // store only the position of impact
		&& trajectory != 0)
	{
		trajectory->push_back(Position(x, y, z));
	}

	return VOXEL_EMPTY;
}

/**
 * Validates a throw action.
 * @param action The action to validate.
 * @param originVoxel The origin point of the action.
 * @param targetVoxel The target point of the action.
 * @param curve The curvature of the throw.
 * @param voxelType The type of voxel at which this parabola terminates.
 * @return, Validity of action.
 */
bool TileEngine::validateThrow(
						BattleAction& action,
						Position originVoxel,
						Position targetVoxel,
						double* curve,
						int* voxelType)
{
	//Log(LOG_INFO) << "TileEngine::validateThrow()";
//kL	double arc = 0.5;
	double arc = 1.2; // kL

	if (action.type == BA_THROW)
	{
		double kneel = 0.0;
		if (action.actor->isKneeled())
			kneel = 0.1;

		arc = std::max(
					0.48,
					1.73 / sqrt(
								sqrt(
									static_cast<double>(action.actor->getStats()->strength)
										/ static_cast<double>(action.weapon->getRules()->getWeight())))
							+ kneel);
	}
	//Log(LOG_INFO) << ". starting arc = " << arc;

	Tile* targetTile = _save->getTile(action.target);
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
		std::vector<Position> trajectory;
		int test = VOXEL_OUTOFBOUNDS;
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

	Tile* t = _save->getTile(tmpCoord);
	while (t
		&& t->isVoid()
		&& !t->getUnit())
	{
		zstart = tmpCoord.z * 24;
		--tmpCoord.z;

		t = _save->getTile(tmpCoord);
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
 * Checks if we hit a pTarget_voxel.
 * @param pTarget_voxel, The voxel to check.
 * @param excludeUnit, Don't do checks on this unit.
 * @param excludeAllUnits, Don't do checks on any unit.
 * @param onlyVisible, Whether to consider only visible units.
 * @param excludeAllBut, If set, the only unit to be considered for ray hits.
 * @param hit, True if no projectile, trajectory, etc. is needed. kL
 * @return, The objectnumber(0-3) or unit(4) or out of map (5) or -1 (hit nothing).
 */
int TileEngine::voxelCheck(
		const Position& pTarget_voxel,
		BattleUnit* excludeUnit,
		bool excludeAllUnits,
		bool onlyVisible,
		BattleUnit* excludeAllBut,
		bool hit) // kL add.
{
	//Log(LOG_INFO) << "TileEngine::voxelCheck()"; // massive lag-to-file, Do not use.

	if (hit)
		return VOXEL_UNIT; // kL; i think Wb may have this covered now.


	Tile* tTarget = _save->getTile(pTarget_voxel / Position(16, 16, 24)); // converts to tilespace -> Tile
	//Log(LOG_INFO) << ". tTarget " << tTarget->getPosition();
	// check if we are out of the map
	if (tTarget == 0
		|| pTarget_voxel.x < 0
		|| pTarget_voxel.y < 0
		|| pTarget_voxel.z < 0)
	{
		//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret 5";
		return VOXEL_OUTOFBOUNDS;
	}

	Tile* tTarget_below = _save->getTile(tTarget->getPosition() + Position(0, 0, -1));
	if (tTarget->isVoid()
		&& tTarget->getUnit() == 0
		&& (!tTarget_below
			|| tTarget_below->getUnit() == 0))
	{
		//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret(1) -1";
		return VOXEL_EMPTY;
	}

	if ((pTarget_voxel.z %24 == 0
			|| pTarget_voxel.z %24 == 1)
		&& tTarget->getMapData(MapData::O_FLOOR)
		&& tTarget->getMapData(MapData::O_FLOOR)->isGravLift())
	{
		if (tTarget->getPosition().z == 0
			|| (tTarget_below
				&& tTarget_below->getMapData(MapData::O_FLOOR)
				&& !tTarget_below->getMapData(MapData::O_FLOOR)->isGravLift()))
		{
			//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret 0";
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
		if (tTarget->isUfoDoorOpen(i))
			continue;

		MapData* dataTarget = tTarget->getMapData(i);
		if (dataTarget != 0)
		{
			int x = 15 - pTarget_voxel.x %16;	// x-direction is reversed
			int y = pTarget_voxel.y %16;		// y-direction is standard

			int LoftIdx = ((dataTarget->getLoftID((pTarget_voxel.z %24) / 2) * 16) + y); // wtf
			if (_voxelData->at(LoftIdx) & (1 << x))
			{
				//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret i = " << i;
				return i;
			}
		}
	}

	if (!excludeAllUnits)
	{
		BattleUnit* buTarget = tTarget->getUnit();
		// sometimes there is unit on the tile below, but sticks up into this tile with its head.
		if (buTarget == 0
			&& tTarget->hasNoFloor(0))
		{
			tTarget = _save->getTile(Position( // tileBelow
										pTarget_voxel.x / 16,
										pTarget_voxel.y / 16,
										pTarget_voxel.z / 24 - 1));
			if (tTarget)
				buTarget = tTarget->getUnit();
		}

		if (buTarget != 0
			&& buTarget != excludeUnit
			&& (!excludeAllBut || buTarget == excludeAllBut)
			&& (!onlyVisible || buTarget->getVisible()))
		{
			Position pTarget_bu = buTarget->getPosition();
			int tz = (pTarget_bu.z * 24) + buTarget->getFloatHeight() - tTarget->getTerrainLevel(); // floor-level voxel

			if (pTarget_voxel.z > tz
				&& pTarget_voxel.z <= tz + buTarget->getHeight()) // if hit is between foot- and hair-level voxel layers (z-axis)
			{
				int entry = 0;

				int x = pTarget_voxel.x %16; // where on the x-axis
				int y = pTarget_voxel.y %16; // where on the y-axis
					// That should be (8,8,10) as per BattlescapeGame::handleNonTargetAction(), if (_currentAction.type == BA_HIT)

				if (buTarget->getArmor()->getSize() > 1) // for large units...
				{
					Position pTarget_tile = tTarget->getPosition();
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
 * @param action, Pointer to a BattleAction.
 * @return bool, True if it succeeded.
 */
bool TileEngine::psiAttack(BattleAction* action)
{
	//Log(LOG_INFO) << "TileEngine::psiAttack()";
	//Log(LOG_INFO) << ". attackerID " << action->actor->getId();

//	bool ret = false;

	Tile* t = _save->getTile(action->target);
	if (t
		&& t->getUnit())
	{
		//Log(LOG_INFO) << ". . tile EXISTS, so does Unit";
		BattleUnit* victim = t->getUnit();
		//Log(LOG_INFO) << ". . defenderID " << victim->getId();
		//Log(LOG_INFO) << ". . target(pos) " << action->target;


		double attackStr = static_cast<double>(action->actor->getStats()->psiStrength)
							* static_cast<double>(action->actor->getStats()->psiSkill)
							/ 50.0;
		//Log(LOG_INFO) << ". . . attackStr = " << (int)attackStr;

		double defenseStr = static_cast<double>(victim->getStats()->psiStrength)
							+ (static_cast<double>(victim->getStats()->psiSkill)
								/ 5.0);
		//Log(LOG_INFO) << ". . . defenseStr = " << (int)defenseStr;

		double d = static_cast<double>(distance(
											action->actor->getPosition(),
											action->target));
		//Log(LOG_INFO) << ". . . d = " << d;

//kL		attackStr -= d;
		attackStr -= d * 2; // kL

		attackStr -= defenseStr;
		if (action->type == BA_MINDCONTROL)
//kL			attackStr += 25.0;
			attackStr += 15.0; // kL
		else
			attackStr += 45.0;

		attackStr *= 100.0;
		attackStr /= 56.0;

		if (action->actor->getOriginalFaction() == FACTION_PLAYER)
			action->actor->addPsiExp();

		int chance = static_cast<int>(attackStr);

//TEMP!!!		if (_save->getSide() == FACTION_PLAYER)
//		{
//			std::string& info = tr("STR_UFO_").arg(ufo->getId())
/*			std::stringstream info;
			if (action->type == BA_PANIC)
				info << "panic ";
			else
				info << "control ";
			info << chance;
			// note: this is a bit borky 'cause it's looking for a string in YAML ...
			_save->getBattleState()->warning(info.str()); */// kL, info.

		std::string info = "";
		if (action->type == BA_PANIC)
			info = "STR_PANIC";
		else
			info = "STR_CONTROL";
		_save->getBattleState()->warning(
										info,
										true,
										chance);
//		}

		//Log(LOG_INFO) << ". . . attackStr Success @ " << chance;
		if (RNG::percent(chance))
		{
			//Log(LOG_INFO) << ". . Success";
			if (action->actor->getOriginalFaction() == FACTION_PLAYER)
				action->actor->addPsiExp(2);

			if (action->type == BA_PANIC)
			{
				//Log(LOG_INFO) << ". . . action->type == BA_PANIC";

				int moraleLoss = (110 - victim->getStats()->bravery);
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
				if (_save->getSide() == FACTION_PLAYER
					&& Options::battleAutoEnd
					&& Options::allowPsionicCapture)
				{
					//Log(LOG_INFO) << ". . . . inside tallyUnits codeblock";

					int liveAliens = 0;
					int liveSoldiers = 0;

					_save->getBattleGame()->tallyUnits(
													liveAliens,
													liveSoldiers,
													false);

					if (liveAliens == 0
						|| liveSoldiers == 0)
					{
						_save->setSelectedUnit(0);
						_save->getBattleGame()->cancelCurrentAction(true);

						_save->getBattleGame()->requestEndTurn();
					}
				}
				//Log(LOG_INFO) << ". . . tallyUnits codeblock DONE";
			}

			//Log(LOG_INFO) << "TileEngine::psiAttack() ret TRUE";
			return true;
		}
	}
//	else // kL_begin:
//	{
		//Log(LOG_INFO) << ". victim not found";
//		return false;
//	}


	// kL_note: double check this... because of my extra check, if victim=Valid up at the top.
	// Can prob. take it out from just above ..... barring the return=TRUE clause there

	// if all units from either faction are mind controlled - auto-end the mission.
/*	if (_save->getSide() == FACTION_PLAYER
		&& Options::getBool("battleAutoEnd")
		&& Options::getBool("allowPsionicCapture"))
	{
		//Log(LOG_INFO) << ". . inside tallyUnits codeblock 2";

		int liveAliens = 0;
		int liveSoldiers = 0;

		_save->getBattleState()->getBattleGame()->tallyUnits(liveAliens, liveSoldiers, false);

		if (liveAliens == 0 || liveSoldiers == 0)
		{
			_save->setSelectedUnit(0);
			_save->getBattleState()->getBattleGame()->requestEndTurn();
		}
		//Log(LOG_INFO) << ". . tallyUnits codeblock 2 DONE";
	} // kL_end.
*/
	//Log(LOG_INFO) << "TileEngine::psiAttack() ret FALSE";
	return false;
}

/**
 * Applies gravity to a tile. Causes items and units to drop.
 * @param t, Tile.
 * @return, Tile where the items end up eventually.
 */
Tile* TileEngine::applyGravity(Tile* t)
{
	if (!t)							// skip this if there is no tile.
		return 0;

	if (t->getInventory()->empty()	// skip this if there are no items;
		&& !t->getUnit())			// skip this if there is no unit in the tile. huh
	{
		return t;
	}

	Position p = t->getPosition();
	Tile* rt = t;
	Tile* rtb;

	BattleUnit* occupant = t->getUnit();
	if (occupant)
//		&& (occupant->getArmor()->getMovementType() != MT_FLY
//			|| occupant->isOut()))
	{
		Position unitPos = occupant->getPosition();
		while (unitPos.z >= 0)
		{
			bool canFall = true;

			for (int
					y = 0;
					y < occupant->getArmor()->getSize()
						&& canFall;
					++y)
			{
				for (int
						x = 0;
						x < occupant->getArmor()->getSize()
							&& canFall;
						++x)
				{
					rt = _save->getTile(Position(
											unitPos.x + x,
											unitPos.y + y,
											unitPos.z));
					rtb = _save->getTile(Position( // below
											unitPos.x + x,
											unitPos.y + y,
											unitPos.z - 1));
					if (!rt->hasNoFloor(rtb))
						canFall = false;
				}
			}

			if (!canFall)
				break;

			unitPos.z--;
		}

		if (unitPos != occupant->getPosition())
		{
//kL			if (occupant->getHealth() != 0
//kL				&& occupant->getStunlevel() < occupant->getHealth())
			if (!occupant->isOut(true, true)) // kL
			{
				if (occupant->getArmor()->getMovementType() == MT_FLY)
				{
					// move to the position you're already in. this will unset the kneeling flag, set the floating flag, etc.
					occupant->startWalking(
									occupant->getDirection(),
									occupant->getPosition(),
									_save->getTile(occupant->getPosition() + Position(0, 0, -1)),
									true);
					// and set our status to standing (rather than walking or flying) to avoid weirdness.
					occupant->setStatus(STATUS_STANDING);
				}
				else
				{
					occupant->startWalking(
									Pathfinding::DIR_DOWN,
									occupant->getPosition() + Position(0, 0, -1),
									_save->getTile(occupant->getPosition() + Position(0, 0, -1)),
									true);
					//Log(LOG_INFO) << "TileEngine::applyGravity(), addFallingUnit() ID " << occupant->getId();
					_save->addFallingUnit(occupant);
				}
			}
			else if (occupant->isOut(true, true))
			{
				Position origin = occupant->getPosition();

				for (int
						y = occupant->getArmor()->getSize() - 1;
						y >= 0;
						--y)
				{
					for (int
							x = occupant->getArmor()->getSize() - 1;
							x >= 0;
							--x)
					{
						_save->getTile(origin + Position(x, y, 0))->setUnit(0);
					}
				}

				occupant->setPosition(unitPos);
			}
		}
	}

	rt = t;

	bool canFall = true;
	while (p.z >= 0
		&& canFall)
	{
		rt = _save->getTile(p);
		rtb = _save->getTile(Position( // below
									p.x,
									p.y,
									p.z - 1));
		if (!rt->hasNoFloor(rtb))
			canFall = false;

		p.z--;
	}

	for (std::vector<BattleItem*>::iterator
			it = t->getInventory()->begin();
			it != t->getInventory()->end();
			++it)
	{
		if ((*it)->getUnit()
			&& t->getPosition() == (*it)->getUnit()->getPosition())
		{
			(*it)->getUnit()->setPosition(rt->getPosition());
		}

		if (t != rt)
			rt->addItem(
					*it,
					(*it)->getSlot());
	}

	if (t != rt) // clear tile
		t->getInventory()->clear();

	return rt;
}

/**
 * Validates the melee range between two units.
 * @param attacker, The attacking unit
 * @param target, The unit we want to attack
 * @param dir, Direction to check
 * @return, True when the range is valid
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
 * @param pos, Position to check from
 * @param direction, Direction to check
 * @param size, For large units, we have to do extra checks
 * @param target, The unit we want to attack, 0 for any unit
 * @param dest, destination
 * @return, True when the range is valid
 */
bool TileEngine::validMeleeRange(
		Position pos,
		int direction,
		BattleUnit* attacker,
		BattleUnit* target,
		Position* dest)
{
	//Log(LOG_INFO) << "TileEngine::validMeleeRange()";
	if (direction < 0 || 7 < direction)
		return false;


	Position p;
	Pathfinding::directionToVector(
								direction,
								&p);
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
			tileOrigin = _save->getTile(Position(pos + Position(x, y, 0)));
			tileTarget = _save->getTile(Position(pos + Position(x, y, 0) + p));

			if (tileOrigin
				&& tileTarget)
			{
				tileTarget_above = _save->getTile(Position(pos + Position(x, y, 1) + p));
				tileTarget_below = _save->getTile(Position(pos + Position(x, y,-1) + p));

				if (!tileTarget->getUnit()) // kL
				{
//kL					if (tileOrigin->getTerrainLevel() <= -16
					if (tileOrigin->getTerrainLevel() < -7 // kL: standing on a rise only 1/3 up z-axis reaches adjacent tileAbove.
						&& tileTarget_above)
//kL						&& !tileTarget_above->hasNoFloor(tileTarget)) // kL_note: floaters...
					{
						tileTarget = tileTarget_above;
					}
					else if (tileTarget_below
						&& tileTarget->hasNoFloor(tileTarget_below)
//kL						&& tileTarget_below->getTerrainLevel() <= -16)
						&& tileTarget_below->getTerrainLevel() < -7) // kL: can reach target standing on a rise only 1/3 up z-axis on adjacent tileBelow.
					{
						tileTarget = tileTarget_below;
					}
				}

				if (tileTarget->getUnit())
				{
					//Log(LOG_INFO) << ". . targetted tileUnit is valid ID = " << tileTarget->getUnit()->getId();
					if (target == 0
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
 * @return Direction or -1 when no window found.
 */
int TileEngine::faceWindow(const Position& position)
{
	static const Position oneTileEast = Position(1, 0, 0);
	static const Position oneTileSouth = Position(0, 1, 0);

	Tile* tile = _save->getTile(position);
	if (tile
		&& tile->getMapData(MapData::O_NORTHWALL)
		&& tile->getMapData(MapData::O_NORTHWALL)->getBlock(DT_NONE) == 0)
	{
		return 0;
	}

	tile = _save->getTile(position + oneTileEast);
	if (tile
		&& tile->getMapData(MapData::O_WESTWALL)
		&& tile->getMapData(MapData::O_WESTWALL)->getBlock(DT_NONE) == 0)
	{
		return 2;
	}

	tile = _save->getTile(position + oneTileSouth);
	if (tile
		&& tile->getMapData(MapData::O_NORTHWALL)
		&& tile->getMapData(MapData::O_NORTHWALL)->getBlock(DT_NONE) == 0)
	{
		return 4;
	}

	tile = _save->getTile(position);
	if (tile
		&& tile->getMapData(MapData::O_WESTWALL)
		&& tile->getMapData(MapData::O_WESTWALL)->getBlock(DT_NONE) == 0)
	{
		return 6;
	}

	return -1;
}

/**
 * Returns the direction from origin to target.
 * kL_note: This function is almost identical to BattleUnit::directionTo().
 * @return, direction.
 */
int TileEngine::getDirectionTo(
		const Position& origin,
		const Position& target) const
{
	if (origin == target) return 0; // kL. safety


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
 * Get the origin-voxel of a shot or missile.
 * @param action, Reference to the BattleAction
 * @param tile, Pointer to the start tile
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
			Tile* tileAbove = _save->getTile(origin + Position(0, 0, 1));
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

/*
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
	Tile* tile = _save->getTile(pos);
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
					tile = _save->getTile(pos + Position(x, y, 0));
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
										unit) == VOXEL_EMPTY)
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
