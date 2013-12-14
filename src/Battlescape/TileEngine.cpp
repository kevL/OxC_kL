/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
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

#include "../aresame.h"

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
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

const int TileEngine::heightFromCenter[11] = {0, -2, +2, -4, +4, -6, +6, -8, +8, -12, +12};

/**
 * Sets up a TileEngine.
 * @param save Pointer to SavedBattleGame object.
 * @param voxelData List of voxel data.
 */
TileEngine::TileEngine(SavedBattleGame* save, std::vector<Uint16>* voxelData)
	:
		_save(save),
		_voxelData(voxelData),
		_personalLighting(true)
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

	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
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
	const int layer = 0; // Ambient lighting layer.

	int power = 15 - _save->getGlobalShade();

	// At night/dusk sun isn't dropping shades blocked by roofs
	if (_save->getGlobalShade() <= 4)
	{
		if (verticalBlockage(_save->getTile(Position(tile->getPosition().x, tile->getPosition().y, _save->getMapSizeZ() - 1)), tile, DT_NONE))
		{
			power -= 2;
		}
	}

	tile->addLight(power, layer);
}

/**
  * Recalculates lighting for the terrain: objects,items,fire.
  */
void TileEngine::calculateTerrainLighting()
{
	const int layer = 1; // Static lighting layer.
	const int fireLightPower = 15; // amount of light a fire generates

	// reset all light to 0 first
	for (int
			i = 0;
			i < _save->getMapSizeXYZ();
			++i)
	{
		_save->getTiles()[i]->resetLight(layer);
	}

	// add lighting of terrain
	for (int
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

		// fires
		if (_save->getTiles()[i]->getFire())
		{
			addLight(
					_save->getTiles()[i]->getPosition(),
					fireLightPower,
					layer);
		}

		for (std::vector<BattleItem*>::iterator
				it = _save->getTiles()[i]->getInventory()->begin();
				it != _save->getTiles()[i]->getInventory()->end();
				++it)
		{
			if ((*it)->getRules()->getBattleType() == BT_FLARE)
			{
				addLight(
						_save->getTiles()[i]->getPosition(),
						(*it)->getRules()->getPower(),
						layer);
			}
		}
	}
}

/**
  * Recalculates lighting for the units.
  */
void TileEngine::calculateUnitLighting()
{
	const int layer = 2;				// Dynamic lighting layer.
	const int personalLightPower = 15;	// amount of light a unit generates
	const int fireLightPower = 15;		// amount of light a fire generates

	// reset all light to 0 first
	for (int
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
		// add lighting of soldiers
		if (_personalLighting
			&& (*i)->getFaction() == FACTION_PLAYER
			&& !(*i)->isOut(true, true))
		{
			addLight(
					(*i)->getPosition(),
					personalLightPower,
					layer);
		}

		// add lighting of units on fire
		if ((*i)->getFire())
		{
			addLight(
					(*i)->getPosition(),
					fireLightPower,
					layer);
		}
	}
}

/**
 * Adds circular light pattern starting from voxelTarget and losing power with distance travelled.
 * @param voxelTarget, Center.
 * @param power, Power.
 * @param layer, Light is separated in 3 layers: Ambient, Static and Dynamic.
 */
void TileEngine::addLight(const Position& voxelTarget, int power, int layer)
{
	// only loop through the positive quadrant.
	for (int x = 0; x <= power; ++x)
	{
		for (int y = 0; y <= power; ++y)
		{
			for (int z = 0; z < _save->getMapSizeZ(); z++)
			{
//kL				int distance = int(floor(sqrt(float(x * x + y * y)) + 0.5));
				int distance = (int)floor(sqrt((float)(x * x + y * y)));		// kL

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
 * Calculates line of sight of a soldier.
 * @param unit, Unit to check line of sight of.
 * @return, True when new aliens are spotted.
 */
bool TileEngine::calculateFOV(BattleUnit* unit)
//bool TileEngine::calculateFOV(BattleUnit* unit, bool bPos)	// kL
{
	//Log(LOG_INFO) << "TileEngine::calculateFOV() for ID " << unit->getId();
//	bool kL_Debug = false;
//	if (unit->getId() == 1000001) kL_Debug = true;


	unit->clearVisibleUnits();			// kL:below
	unit->clearVisibleTiles();			// kL:below

	if (unit->isOut(true, true))		// kL:below (check health, check stun, check status)
		return false;

	bool ret = false;					// kL

	size_t preVisibleUnits = unit->getUnitsSpottedThisTurn().size();
	//if (kL_Debug) Log(LOG_INFO) << ". . . . preVisibleUnits = " << (int)preVisibleUnits;


	int direction;
	if (_save->getStrafeSetting()
		&& unit->getTurretType() > -1)
	{
		direction = unit->getTurretDirection();
	}
	else
	{
		direction = unit->getDirection();
	}
	//Log(LOG_INFO) << ". direction = " << direction;

	bool swap = (direction == 0 || direction == 4);

	int sign_x[8] = {+1, +1, +1, +1, -1, -1, -1, -1};
//kL	int sign_y[8] = {-1, -1, -1, +1, +1, +1, -1, -1};	// is this right? (ie. 3pos & 5neg, why not 4pos & 4neg )
	int sign_y[8] = {-1, -1, +1, +1, +1, +1, -1, -1};		// kL: note it does not matter.

	int y1, y2;
	bool diag = false;
	if (direction %2)
	{
		diag = true;

		y1 = 0;
		y2 = MAX_VIEW_DISTANCE;
	}

/*kL:above	unit->clearVisibleUnits();
	unit->clearVisibleTiles();

	if (unit->isOut()) return false; */

	std::vector<Position> _trajectory;

//	Position center = unit->getPosition();
	Position unitPos = unit->getPosition();
	Position test;

//kL	if (unit->getHeight() + unit->getFloatHeight() + -_save->getTile(unit->getPosition())->getTerrainLevel() >= 24 + 4)
	if (unit->getHeight() + unit->getFloatHeight() - _save->getTile(unitPos)->getTerrainLevel() >= 24 + 4)	// kL
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
				const int distanceSqr = x * x + y * y;
//				const int distanceSqr = x * x + y * y + z * z;	// kL
				//Log(LOG_INFO) << "distanceSqr = " << distanceSqr << " ; x = " << x << " ; y = " << y << " ; z = " << z; // <- HUGE write to file.
				//Log(LOG_INFO) << "x = " << x << " ; y = " << y << " ; z = " << z; // <- HUGE write to file.

				test.z = z;

				if (distanceSqr <= MAX_VIEW_DISTANCE * MAX_VIEW_DISTANCE)
				{
					//Log(LOG_INFO) << "inside distanceSqr";

//kL					test.x = center.x + sign_x[direction] * (swap? y: x);
//kL					test.y = center.y + sign_y[direction] * (swap? x: y);
					int deltaPos_x = (sign_x[direction] * (swap? y: x));
					int deltaPos_y = (sign_y[direction] * (swap? x: y));
					test.x = unitPos.x + deltaPos_x;
					test.y = unitPos.y + deltaPos_y;
					//Log(LOG_INFO) << "test.x = " << test.x;
					//Log(LOG_INFO) << "test.y = " << test.y;
					//Log(LOG_INFO) << "test.z = " << test.z;


					if (_save->getTile(test))
					{
						//Log(LOG_INFO) << "inside getTile(test)";
						BattleUnit* visUnit = _save->getTile(test)->getUnit();

						//if (kL_Debug) Log(LOG_INFO) << ". . calculateFOV(), visible() CHECK.. " << visUnit->getId();
						if (visUnit
							&& !visUnit->isOut(true, true)
							&& visible(unit, _save->getTile(test))) // reveal units & tiles <- This seems uneven.
						{
							//Log(LOG_INFO) << ". . visible() TRUE : unitID = " << unit->getId() << " ; visID = " << visUnit->getId();
							//Log(LOG_INFO) << ". . calcFoV, distance = " << distance(unit->getPosition(), visUnit->getPosition());

							//if (kL_Debug) Log(LOG_INFO) << ". . calculateFOV(), visible() TRUE " << visUnit->getId();
							if (!visUnit->getVisible())		// kL,  spottedID = " << visUnit->getId();
							{
								//Log(LOG_INFO) << ". . calculateFOV(), getVisible() FALSE";
								ret = true;						// kL
							}
							//Log(LOG_INFO) << ". . calculateFOV(), visUnit -> getVisible() = " << !ret;

							if (unit->getFaction() == FACTION_PLAYER)
							{
								//if (kL_Debug) Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER, set spottedTile & spottedUnit visible";
								visUnit->getTile()->setVisible(+1);
								visUnit->getTile()->setDiscovered(true, 2); // kL_below. sprite caching for floor+content: DO I WANT THIS.
								visUnit->setVisible(true);
							}
							//if (kL_Debug) Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER, Done";

							if ((visUnit->getFaction() == FACTION_HOSTILE && unit->getFaction() != FACTION_HOSTILE)		// kL_note: or not Neutral?
								|| (visUnit->getFaction() != FACTION_HOSTILE && unit->getFaction() == FACTION_HOSTILE))	// kL_note: what about Mc'd spooters?
							{
								//Log(LOG_INFO) << ". . . opposite Factions, add Tile & visUnit to visList";

								unit->addToVisibleUnits(visUnit);
								unit->addToVisibleTiles(visUnit->getTile());

								if (unit->getFaction() == FACTION_HOSTILE)
//kL									&& visUnit->getFaction() != FACTION_HOSTILE)
								{
									//if (kL_Debug) Log(LOG_INFO) << ". . calculateFOV(), spotted Unit FACTION_HOSTILE, setTurnsExposed()";
									visUnit->setTurnsExposed(0);
								}
							}
							//if (kL_Debug) Log(LOG_INFO) << ". . calculateFOV(), opposite Factions, Done";
						}
						//if (kL_Debug) Log(LOG_INFO) << ". . calculateFOV(), visUnit EXISTS & isVis, Done";

						if (unit->getFaction() == FACTION_PLAYER) // reveal extra tiles
						{
							//if (kL_Debug) Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER";
							// this sets tiles to discovered if they are in LOS -
							// tile visibility is not calculated in voxelspace but in tilespace;
							// large units have "4 pair of eyes"
							int size = unit->getArmor()->getSize();

							for (int
									xPlayer = 0;
									xPlayer < size;
									xPlayer++)
							{
								//Log(LOG_INFO) << ". . . . calculateLine() inside for(1) Loop";
								for (int
										yPlayer = 0;
										yPlayer < size;
										yPlayer++)
								{
									//Log(LOG_INFO) << ". . . . calculateLine() inside for(2) Loop";
									Position pPlayer = unitPos + Position(xPlayer, yPlayer, 0);

// int calculateLine(const Position& origin, const Position& target, bool storeTrajectory, std::vector<Position>* trajectory,
//		BattleUnit* excludeUnit, bool doVoxelCheck = true, bool onlyVisible = false, BattleUnit* excludeAllBut = 0);

									_trajectory.clear();

									//Log(LOG_INFO) << ". . calculateLine()";
									int tst = calculateLine(pPlayer, test, true, &_trajectory, unit, false);
									//Log(LOG_INFO) << ". . . . calculateLine() tst = " << tst;

									size_t trajectorySize = _trajectory.size();

									if (tst > 127) // last tile is blocked thus must be cropped
										--trajectorySize;

									for (size_t
											i = 0;
											i < trajectorySize;
											i++)
									{
										//Log(LOG_INFO) << ". . . . calculateLine() inside for(3) Loop";
										Position pTrajectory = _trajectory.at(i);

										// mark every tile of line as visible (as in original)
										// this is needed because of bresenham narrow stroke. 
										_save->getTile(pTrajectory)->setVisible(+1);
										_save->getTile(pTrajectory)->setDiscovered(true, 2); // sprite caching for floor+content

										// walls to the east or south of a visible tile, we see that too
										Tile* t = _save->getTile(Position(pTrajectory.x + 1, pTrajectory.y, pTrajectory.z));
										if (t)
											t->setDiscovered(true, 0);

										t = _save->getTile(Position(pTrajectory.x, pTrajectory.y + 1, pTrajectory.z));
										if (t)
											t->setDiscovered(true, 1);
									}
								}
							}
						}
						//Log(LOG_INFO) << ". . calculateFOV(), FACTION_PLAYER Done";
					}
					//Log(LOG_INFO) << ". . calculateFOV(), getTile(test) Done";
				}
				//Log(LOG_INFO) << ". . calculateFOV(), distanceSqr Done";
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
		&& unit->getUnitsSpottedThisTurn().size() > preVisibleUnits)
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
/*kL void TileEngine::calculateFOV(const Position& position)
{
	for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
	{
		if (distance(position, (*i)->getPosition()) < MAX_VIEW_DISTANCE)
		{
			calculateFOV(*i);
		}
	}
} */
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

		int dist = distance(position, (*i)->getPosition());
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

	for (std::vector<BattleUnit*>::iterator bu = _save->getUnits()->begin(); bu != _save->getUnits()->end(); ++bu)
	{
		if ((*bu)->getTile() != 0)
		{
			calculateFOV(*bu);
		}
	}
}

/**
 * Gets the origin voxel of a unit's LoS.
 * @param currentUnit, The watcher.
 * @return, Approximately an eyeball voxel.
 */
Position TileEngine::getSightOriginVoxel(BattleUnit* currentUnit)
{
	// determine the origin (and target) voxels for calculations
//	Position originVoxel;
//kL	originVoxel = Position((currentUnit->getPosition().x * 16) + 7, (currentUnit->getPosition().y * 16) + 8, currentUnit->getPosition().z * 24);
	Position originVoxel = Position(
			(currentUnit->getPosition().x * 16) + 8,
			(currentUnit->getPosition().y * 16) + 8,
			currentUnit->getPosition().z * 24);			// kL

//kL	originVoxel.z += -_save->getTile(currentUnit->getPosition())->getTerrainLevel();
//	originVoxel.z -= _save->getTile(currentUnit->getPosition())->getTerrainLevel();		// kL
	originVoxel.z += currentUnit->getHeight() + currentUnit->getFloatHeight()
			- _save->getTile(currentUnit->getPosition())->getTerrainLevel() - 2; // two voxels lower (nose level)
		// kL_note: Can make this equivalent to LoF origin, perhaps.....
		// hey, here's an idea: make Snaps & Auto shoot from hip, Aimed from shoulders or eyes.

	Tile* tileAbove = _save->getTile(currentUnit->getPosition() + Position(0, 0, 1));

	// kL_note: let's stop this. Tanks appear to make their FoV etc. Checks from all four quadrants anyway.
/*	if (currentUnit->getArmor()->getSize() > 1)
	{
		originVoxel.x += 8;
		originVoxel.y += 8;
		originVoxel.z += 1; // topmost voxel
	} */

	if (originVoxel.z >= (currentUnit->getPosition().z + 1) * 24
		&& (!tileAbove || !tileAbove->hasNoFloor(0)))
	{
		while (originVoxel.z >= (currentUnit->getPosition().z + 1) * 24)
		{
			// careful with that ceiling, Eugene.
			originVoxel.z--;
		}
	}

	return originVoxel;
}

/**
 * Checks for an opposing unit on this tile.
 * @param currentUnit, The watcher.
 * @param tile, The tile to check for
 * @return, True if visible.
 */
bool TileEngine::visible(BattleUnit* currentUnit, Tile* tile)
{
	//Log(LOG_INFO) << "TileEngine::visible()";
	//Log(LOG_INFO) << ". spotter / reactor ID = " << currentUnit->getId();

	// if there is no tile or no unit, we can't see it
	if (!tile
		|| !tile->getUnit())
	{
		return false;
	}

	BattleUnit* targetUnit = tile->getUnit();

	//Log(LOG_INFO) << ". target ID = " << targetUnit->getId();

	if (targetUnit->isOut(true, true))
	{
		//Log(LOG_INFO) << ". . target is Dead, ret FALSE";
		return false;
	}

	if (currentUnit->getFaction() == targetUnit->getFaction())
	{
		//Log(LOG_INFO) << ". . target is Friend, ret TRUE";
		return true;
	}

	float realDist = static_cast<float>(distance(currentUnit->getPosition(), targetUnit->getPosition()));
	if (realDist > MAX_VIEW_DISTANCE)
	{
		//Log(LOG_INFO) << ". . too far to see Tile, ret FALSE";
		return false;
	}

	// aliens can see in the dark, xcom can see at a distance of 9 or less, further if there's enough light.
	if (currentUnit->getFaction() == FACTION_PLAYER
		&& distance(currentUnit->getPosition(), tile->getPosition()) > 9
		&& tile->getShade() > MAX_DARKNESS_TO_SEE_UNITS)
	{
		//Log(LOG_INFO) << ". . too dark to see Tile, ret FALSE";
		return false;
	}


	// for large units origin voxel is in the middle ( not anymore )
	// kL_note: this leads to problems with large units trying to shoot around corners, b.t.w.
	// because it might See with a clear LoS, but the LoF is taken from a different, offset voxel.
	// further, i think Lines of Sight and Fire determinations are getting mixed up somewhere!!!
	Position originVoxel = getSightOriginVoxel(currentUnit);
	Position scanVoxel;
	std::vector<Position> _trajectory;

	bool unitIsSeen = false;

	// kL_note: Is an intermediary object *not* obstructing viewing
	// or targetting, when it should be?? Like, around corners?
	unitIsSeen = canTargetUnit(
			&originVoxel,
			tile,
			&scanVoxel,
			currentUnit);

	if (unitIsSeen)
	{
		//Log(LOG_INFO) << ". . . canTargetUnit()";

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
				currentUnit);

		// visible() - Check if targetUnit is really targetUnit.
/*		Log(LOG_INFO) << ". . . . identifying targetUnit @ end of trajectory...";
		Tile* endTile = _save->getTile(Position(_trajectory.back().x / 16, _trajectory.back().y / 16, _trajectory.back().z / 24));
		Log(LOG_INFO) << ". . . . got endTile, now Id unit...";
		BattleUnit* targetUnit2;
		if (!endTile->hasNoFloor(endTile))
		{
			targetUnit2 = endTile->getUnit();
			Log(LOG_INFO) << ". . . . targetUnit2 identified, ID = " << targetUnit2->getId();
		}

		if (targetUnit->getId() != targetUnit2->getId())
		{
			Log(LOG_INFO) << ". . . . targetUnit != targetUnit2 -> ret FALSE";
//			unitIsSeen = false;
			return false;
		} */

		// floatify this Smoke ( & Fire ) thing.
		float effDist = static_cast<float>(_trajectory.size());
		float factor = realDist * 16.f / effDist; // how many 'real distance' units there are in each 'effective distance' unit.

		//Log(LOG_INFO) << ". . . effDist = " << effDist / 16.f;
		//Log(LOG_INFO) << ". . . realDist = " << realDist;
		//Log(LOG_INFO) << ". . . factor = " << factor;

		Tile* t = _save->getTile(currentUnit->getPosition()); // origin tile

		for (uint16_t
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
/*			if (t != _save->getTile(Position(
					_trajectory.at(i).x * static_cast<int>(factor) / 16,
					_trajectory.at(i).y * static_cast<int>(factor) / 16,
					_trajectory.at(i).z * static_cast<int>(factor) / 24)))
			{
				t = _save->getTile(Position(
						_trajectory.at(i).x * static_cast<int>(factor) / 16,
						_trajectory.at(i).y * static_cast<int>(factor) / 16,
						_trajectory.at(i).z * static_cast<int>(factor) / 24));
			} */

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

		//Log(LOG_INFO) << ". . . . 1 unitIsSeen = " << unitIsSeen;
		if (unitIsSeen)
		{
			// have to check if targetUnit is poking its head up from tileBelow
			Tile* tBelow = _save->getTile(t->getPosition() + Position(0, 0, -1));
			if (!(t->getUnit() == targetUnit
				|| (tBelow
					&& tBelow->getUnit()
					&& tBelow->getUnit() == targetUnit)))
			{
				unitIsSeen = false;
				//if (kL_Debug) Log(LOG_INFO) << ". . . . 2 unitIsSeen = " << unitIsSeen;
			}
		}
	}

	//Log(LOG_INFO) << ". unitIsSeen = " << unitIsSeen;
	return unitIsSeen;
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

	bool hypothetical = potentialUnit != 0;
	if (potentialUnit == 0)
	{
		potentialUnit = tile->getUnit();
		if (potentialUnit == 0)
			return false; // no unit in this tile, even if it elevated and appearing in it.
	}

	if (potentialUnit == excludeUnit)
		return false; // skip self

	int targetMinHeight = targetVoxel.z - tile->getTerrainLevel();
	targetMinHeight += potentialUnit->getFloatHeight();

	int targetMaxHeight = targetMinHeight;
	int targetCenterHeight;
	// if there is an other unit on target tile, we assume we want to check against this unit's height
	int heightRange;

	int unitRadius = potentialUnit->getLoftemps(); // width == loft in default loftemps set
	int targetSize = potentialUnit->getArmor()->getSize() - 1;
	if (targetSize > 0)
	{
		unitRadius = 3;
	}

	// vector manipulation to make scan work in view-space
	Position relPos = targetVoxel - *originVoxel;

	float normal = static_cast<float>(unitRadius) / sqrt(static_cast<float>(relPos.x * relPos.x + relPos.y * relPos.y));
	int relX = static_cast<int>(floor(static_cast<float>(relPos.y) * normal + 0.5f));
	int relY = static_cast<int>(floor(static_cast<float>(-relPos.x) * normal + 0.5f));

	int sliceTargets[10] =
	{
		0,		0,
		relX,	relY,
		-relX,	-relY,
		relY,	-relX,
		-relY,	relX
	};

	if (!potentialUnit->isOut(true, true))
	{
		heightRange = potentialUnit->getHeight();
	}
	else
	{
		heightRange = 12;
	}

	targetMaxHeight += heightRange;
	targetCenterHeight = (targetMaxHeight + targetMinHeight) / 2;
	heightRange /= 2;
	if (heightRange > 10) heightRange = 10;
	if (heightRange < 0) heightRange = 0;

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
			if (i < heightRange - 1 && j > 2)
				break; // skip unnecessary checks

			scanVoxel->x = targetVoxel.x + sliceTargets[j * 2];
			scanVoxel->y = targetVoxel.y + sliceTargets[j * 2 + 1];

			_trajectory.clear();

			int test = calculateLine(*originVoxel, *scanVoxel, false, &_trajectory, excludeUnit, true, false);
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
						if (_trajectory.at(0).x / 16 == (scanVoxel->x / 16) + x
							&& _trajectory.at(0).y / 16 == (scanVoxel->y / 16) + y
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

	int* spiralArray;
	int spiralCount;

	int minZ, maxZ;
	bool minZfound = false, maxZfound = false;

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
		minZfound = true;
		minZ = 0;
		maxZfound = true;
		maxZ = 0;
	}
	else
	{
		return false;
	}

	if (!minZfound) // find out height range
	{
		for (int
				j = 1;
				j < 12;
				++j)
		{
			if (minZfound) break;

			for (int
					i = 0;
					i < spiralCount;
					++i)
			{
				int tX = spiralArray[i * 2];
				int tY = spiralArray[i * 2 + 1];

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

	if (!minZfound) return false; // empty object!!!

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
				int tX = spiralArray[i * 2];
				int tY = spiralArray[i * 2 + 1];

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

	if (!maxZfound) return false; // it's impossible to get there

	if (minZ > maxZ)
		minZ = maxZ;
	int rangeZ = maxZ - minZ;
	int centerZ = (maxZ + minZ) / 2;

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
//					true); // do voxelCheck, default=true.

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
 * Creates a vector of units that can spot this unit.
 * @param unit, The unit to check for spotters of.
 * @return, A vector of units that can see this unit.
 */
std::vector<BattleUnit*> TileEngine::getSpottingUnits(BattleUnit* unit)
{
	Log(LOG_INFO) << "TileEngine::getSpottingUnits() targetID " << (unit)->getId() << " : initi = " << (int)(unit)->getInitiative();

	Tile* tile = unit->getTile();

	std::vector<BattleUnit*> spotters;
	for (std::vector<BattleUnit*>::const_iterator
			bu = _save->getUnits()->begin();
			bu != _save->getUnits()->end();
			++bu)
	{
//		int buIniti = static_cast<int>((*bu)->getInitiative()); // purely Debug info, here.

		if (*bu != unit																		// don't put spottee unit itself in with the spotters.
			&& !(*bu)->isOut(true, true)													// not dead/unconscious
//			&& (*bu)->getHealth() != 0														// not dying, checked by "isOut(true)"
//			&& (*bu)->getStunlevel() < (*bu)->getHealth()									// not about to pass out
			&& ((*bu)->getFaction() != _save->getSide()										// not a friend, unless...
				|| (unit->getOriginalFaction() == FACTION_HOSTILE							// aLiens will shott themselves
					&& unit->getFaction() == FACTION_PLAYER									// when mind-controlled,
					&& _save->getSide() == FACTION_HOSTILE)))								// but not xCom
//			&& distance(unit->getPosition(), (*bu)->getPosition()) <= MAX_VIEW_DISTANCE)	// closer than 20 tiles, checked by "visible()"
		{
//			Position originVoxel = _save->getTileEngine()->getSightOriginVoxel(*bu);
//			originVoxel.z -= 2;
//			Position targetVoxel;

			AlienBAIState* aggro = dynamic_cast<AlienBAIState*>((*bu)->getCurrentAIState());
			if (((*bu)->checkViewSector(unit->getPosition())			// spotter is looking in the right direction
					|| (aggro != 0 && aggro->getWasHit()))				// spotter has been aggro'd
//				&& canTargetUnit(&originVoxel, tile, &targetVoxel, *bu)	// can actually target the unit, checked by "visible()",
																		// although origin is placed 2 voxels lower here.
				&& visible(*bu, tile))									// can actually see the unit through smoke/fire & within viewRange
			{
				if ((*bu)->getFaction() == FACTION_PLAYER)
				{
					unit->setVisible(true);
				}

				(*bu)->addToVisibleUnits(unit);

//				if (_save->getSide() != FACTION_NEUTRAL // no reaction on civilian turn. done in "checkReactionFire()"
				if (canMakeSnap(*bu, unit))
				{
					Log(LOG_INFO) << ". . . reactID " << (*bu)->getId() << " : initi = " << (int)(*bu)->getInitiative();
					//		<< " : ADD";

					spotters.push_back(*bu);
				}
//				else
				{
					//Log(LOG_INFO) << ". . reactID " << (*bu)->getId() << " : initi = "  << buIniti
					//		<< " : can't makeSnap.";
				}
			}
//			else
			{
				//Log(LOG_INFO) << ". . reactID " << (*bu)->getId() << " : initi = "  << buIniti
				//		<< " : not facing AND not aggro, OR target obscured/OoR";
			}
		}
//		else
		{
			//Log(LOG_INFO) << ". . reactID " << (*bu)->getId() << " : initi = "  << buIniti
			//		<< " : isOut(true, true) OR side's faction";
		}
	}

	return spotters;
}

/**
 * Checks the validity of a snap shot performed here.
 * @param unit, The unit to check sight from.
 * @param target, The unit to check sight TO.
 * @return, True if the target is valid.
 */
bool TileEngine::canMakeSnap(BattleUnit* unit, BattleUnit* target)
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

	if (weapon																		// has a weapon
		&& (weapon->getRules()->getBattleType() == BT_MELEE							// has a melee weapon
			&& validMeleeRange(unit, target, unit->getDirection()					// is in melee range
			&& unit->getTimeUnits() >= unit->getActionTUs(BA_HIT, weapon))			// has enough TU
		|| (weapon->getRules()->getBattleType() == BT_FIREARM						// has a gun
			&& weapon->getRules()->getTUSnap()										// can make snapshot
//			&& weapon->getAmmoItem()												// gun is loaded, checked in "getMainHandWeapon()"
			&& unit->getTimeUnits() >= unit->getActionTUs(BA_SNAPSHOT, weapon))))	// has enough TU
	{
		//Log(LOG_INFO) << ". ret TRUE";
		return true;
	}

	//Log(LOG_INFO) << ". ret FALSE";
	return false;
}

/**
 * Checks if a sniper from the opposing faction sees this unit. The unit with the
 * highest reaction score will be compared with the current unit's reaction score.
 * If it's higher, a shot is fired when enough time units, a weapon and ammo are available.
 * @param unit, The unit to check reaction fire upon.
 * @return, True if reaction fire took place.
 */
bool TileEngine::checkReactionFire(BattleUnit* unit)
{
	Log(LOG_INFO) << "TileEngine::checkReactionFire() vs targetID " << unit->getId();

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

	bool result = false;

	// not mind controlled, or controlled by the player
	// kL. If spotted unit is not mind controlled,
	// or is mind controlled but not an alien;
	// ie, never reaction fire on a mind-controlled xCom soldier;
	// but *do* reaction fire on a mind-controlled aLien (or civilian.. ruled out above).
	if (unit->getFaction() == unit->getOriginalFaction()
//kL		|| unit->getFaction() != FACTION_HOSTILE)
		|| unit->getFaction() == FACTION_PLAYER)		// kL
	{
		//Log(LOG_INFO) << ". Target = VALID";
		std::vector<BattleUnit*> spotters = getSpottingUnits(unit);

		//Log(LOG_INFO) << ". # spotters = " << spotters.size();

		// get the first man up to bat.
		BattleUnit* reactor = getReactor(spotters, unit);
		// start iterating through the possible reactors until the current unit is the one with the highest score.
		while (reactor != unit)
//			&& !unit->isOut(true, true))	// <- this doesn't want to take effect until after cycling & shots is over
											// iow, this probably just cycles and sets up the ProjectileFlyBState()'s,
											// leaving the kill, if any, for later.......
		{
			// !!!!!SHOOT!!!!!
			if (!tryReactionSnap(reactor, unit))	// <- statePushBack(new ProjectileFlyBState()
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

				// avoid setting result to true, but carry on, just cause one unit can't
				// react doesn't mean the rest of the units in the vector (if any) can't
				reactor = getReactor(spotters, unit);

				continue;
			}
			else
			{
				//Log(LOG_INFO) << ". . Snap by : " << reactor->getId();
				result = true;
			}

			//Log(LOG_INFO) << ". . Snap by : " << reactor->getId();

			// ... not working!
//			if (unit->isOut(true, true))	// <- try this down there also.
//				break;
			
			// nice shot, kid. don't get too cocky.
			reactor = getReactor(spotters, unit);
			//Log(LOG_INFO) << ". . NEXT AT BAT : " << reactor->getId();
		}

		//Log(LOG_INFO) << ". clear Spotters.vect";
		spotters.clear();	// kL
	}

	//Log(LOG_INFO) << ". . Reactor == unit, EXIT = " << result;
	return result;
}

/**
 * Gets the unit with the highest reaction score from the spotter vector.
 * @param spotters, The vector of spotting units.
 * @param unit, The unit to check scores against.
 * @return, The unit with initiative.
 */
BattleUnit* TileEngine::getReactor(std::vector<BattleUnit*> spotters, BattleUnit* unit)
{
	Log(LOG_INFO) << "TileEngine::getReactor() vs targetID " << unit->getId();

	int bestScore = -1;
	BattleUnit* reactor = 0;

	for (std::vector<BattleUnit*>::iterator
			spotter = spotters.begin();
			spotter != spotters.end();
			++spotter)
	{
		Log(LOG_INFO) << ". . reactID " << (*spotter)->getId();

		if (!(*spotter)->isOut(true, true)
//			&& canMakeSnap((*spotter), unit)				// done in "getSpottingUnits()"
			&& (*spotter)->getInitiative() > bestScore)
//			&& (*spotter) != bu)	// kL, stop unit from reacting twice (unless target uses more TU, hopefully)
		{
			bestScore = static_cast<int>((*spotter)->getInitiative());
			reactor = *spotter;
		}
	}

	// reactor has to *best* unit.Initi to get initiative
	if (bestScore > static_cast<int>(unit->getInitiative()))
	{
		if (reactor->getOriginalFaction() == FACTION_PLAYER)
		{
			reactor->addReactionExp();
		}
	}
	else
	{
		Log(LOG_INFO) << ". . initi returns to ID " << unit->getId();

		reactor = unit;
	}

	Log(LOG_INFO) << ". bestScore = " << bestScore;
	return reactor;
}

/**
 * Attempts to perform a reaction snap shot.
 * @param unit, The unit to check sight from.
 * @param target, The unit to check sight TO.
 * @return, True if the action should (theoretically) succeed.
 */
bool TileEngine::tryReactionSnap(BattleUnit* unit, BattleUnit* target)
{
	//Log(LOG_INFO) << "TileEngine::tryReactionSnap() reactID " << unit->getId() << " vs targetID " << target->getId();
	BattleAction action;

	// note that other checks for/of weapon were done in "canMakeSnap()"
	// redone here to fill the BattleAction object...
	if (unit->getFaction() == FACTION_PLAYER)
	{
		action.weapon = unit->getItem(unit->getActiveHand());
	}
	else
		action.weapon = unit->getMainHandWeapon(); // kL_note: no longer returns grenades. good

	if (!action.weapon)
	{
		//Log(LOG_INFO) << ". no Weapon, ret FALSE";
		return false;
	}

	action.type = BA_SNAPSHOT;									// reaction fire is ALWAYS snap shot.
																// kL_note: not true in Orig. aliens did auto at times
	if (action.weapon->getRules()->getBattleType() == BT_MELEE)	// unless we're a melee unit.
																// kL_note: in which case you won't react at all. ( yet )
	{
		action.type = BA_HIT;
	}

	action.TU = unit->getActionTUs(action.type, action.weapon);

	// kL_note: Does this handle melee hits, as reaction shots? really.
//	if (action.weapon->getAmmoItem()						// lasers & melee are their own ammo-items.
															// Unless their battleGame #ID happens to be 0
//		&& action.weapon->getAmmoItem()->getAmmoQuantity()	// returns 255 for laser; 0 for melee
//		&& unit->getTimeUnits() >= action.TU)
	// That's all been done!!!
	{
		action.targeting = true;
		action.target = target->getPosition();

		if (unit->getFaction() == FACTION_HOSTILE) // aLien units will go into an "aggro" state when they react.
		{
			AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(unit->getCurrentAIState());
			if (aggro == 0) // should not happen, but just in case...
			{
				aggro = new AlienBAIState(_save, unit, 0);
				unit->setAIState(aggro);
			}

			if (action.weapon->getAmmoItem()->getRules()->getExplosionRadius()
				&& aggro->explosiveEfficacy(action.target, unit, action.weapon->getAmmoItem()->getRules()->getExplosionRadius(), -1) == false)
			{
				action.targeting = false;
			}
		}

		if (action.targeting
			&& unit->spendTimeUnits(action.TU))
		{
			Log(LOG_INFO) << ". Reaction Fire by reactID " << unit->getId();

			action.TU = 0;

			action.cameraPosition = _save->getBattleState()->getMap()->getCamera()->getMapOffset();	// kL, was above under "BattleAction action;"
			action.actor = unit;																	// kL, was above under "BattleAction action;"

			_save->getBattleGame()->statePushBack(new UnitTurnBState(_save->getBattleGame(), action));
			_save->getBattleGame()->statePushBack(new ProjectileFlyBState(_save->getBattleGame(), action));

			return true;
		}
	}

	return false;
}

/**
 * Handles bullet/weapon hits.
 *
 * A bullet/weapon hits a voxel.
 * @param pTarget_voxel, Center of the hit in voxelspace.
 * @param power, Power of the hit/explosion.
 * @param type, The damage type of the hit.
 * @param attacker, The unit that caused the hit.
 * @return, The Unit that got hit.
 */
BattleUnit* TileEngine::hit(
		const Position& pTarget_voxel,
		int power,
		ItemDamageType type,
		BattleUnit* attacker,
		bool hit)	// kL add.
{
	Log(LOG_INFO) << "TileEngine::hit() by ID " << attacker->getId() << " @ " << attacker->getPosition()
			<< " : power = " << power
			<< " : type = " << static_cast<int>(type);

	if (type != DT_NONE)	// TEST for Psi-attack.
	{
		//Log(LOG_INFO) << "DT_ type = " << static_cast<int>(type);


		Tile* tile = _save->getTile(Position(
										pTarget_voxel.x / 16,
										pTarget_voxel.y / 16,
										pTarget_voxel.z / 24));
		Log(LOG_INFO) << ". targetTile " << tile->getPosition() << " targetVoxel " << pTarget_voxel;

		if (!tile)
		{
			//Log(LOG_INFO) << ". Position& pTarget_voxel : NOT Valid, return NULL";
			return 0;
		}

//kL		BattleUnit* buTarget = tile->getUnit();
		BattleUnit* buTarget = 0;			// kL_begin:
		if (tile->getUnit())
		{
			buTarget = tile->getUnit();
			Log(LOG_INFO) << ". . buTarget Valid ID = " << buTarget->getId();
		}
//		else
//			buTarget = 0; // kL_end.

//kL		int adjustedDamage = 0;

		// This is returning part < 4 when using a stunRod against a unit outside the north (or east) UFO wall. ERROR!!!
		// later: Now it's returning VOXEL_EMPTY (-1) when a silicoid attacks a poor civvie!!!!! And on acid-spits!!!
//kL		int const part = voxelCheck(pTarget_voxel, attacker);
		int const part = voxelCheck(pTarget_voxel, attacker, false, false, 0, hit);		// kL
		Log(LOG_INFO) << ". voxelCheck() ret part = " << part;

		if (VOXEL_EMPTY < part && part < VOXEL_UNIT	// 4 terrain parts ( #0 - #3 )
			&& type != DT_STUN)						// kL, workaround for Stunrod.
		{
			Log(LOG_INFO) << ". . terrain hit";
			// power 25% to 75%
//kL			const int randPower = RNG::generate(power / 4, power * 3 / 4); // RNG::boxMuller(power, power/6)
			power = RNG::generate(power / 4, power * 3 / 4);
			Log(LOG_INFO) << ". . RNG::generate(power) = " << power;
			if (tile->damage(part, power))
			{
				_save->setObjectiveDestroyed(true);
			}
			// kL_note: This would be where to adjust damage based on effectiveness of weapon vs Terrain!
		}
		else if (part == VOXEL_UNIT)	// battleunit part
//			|| type == DT_STUN)			// kL, workaround for Stunrod. (not needed, huh?)
		{
			Log(LOG_INFO) << ". . battleunit hit";

			// power 0 - 200% -> 1 - 200%
//kL			const int randPower = RNG::generate(1, power * 2); // RNG::boxMuller(power, power/3)
			int verticaloffset = 0;

			if (!buTarget)
			{
				Log(LOG_INFO) << ". . . buTarget NOT Valid, check tileBelow";

				// it's possible we have a unit below the actual tile, when he stands on a stairs and sticks his head up into the above tile.
				// kL_note: yeah, just like in LoS calculations!!!! cf. visible() etc etc .. idiots.
				Tile* tileBelow = _save->getTile(Position(pTarget_voxel.x / 16, pTarget_voxel.y / 16, (pTarget_voxel.z / 24) - 1));
				if (tileBelow
					&& tileBelow->getUnit())
				{
					buTarget = tileBelow->getUnit();
					verticaloffset = 24;
				}
			}

			if (buTarget)
			{
				Log(LOG_INFO) << ". . . buTarget Valid ID = " << buTarget->getId();

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
				Position const relPos = pTarget_voxel - targetPos - Position(0, 0, verticaloffset);

//kL				const int randPower	= RNG::generate(1, power * 2);	// kL_above
				power = RNG::generate(1, power * 2);
				Log(LOG_INFO) << ". . . RNG::generate(power) = " << power;
				bool ignoreArmor = (type == DT_STUN);			// kL. stun ignores armor... does now! UHM....
				int adjustedDamage = buTarget->damage(relPos, power, type, ignoreArmor);
				Log(LOG_INFO) << ". . . adjustedDamage = " << adjustedDamage;

				if (adjustedDamage > 0
					&& !buTarget->isOut(true)) // -> do morale hit only if health still > 0.
				{
					const int bravery = (110 - buTarget->getStats()->bravery) / 10;
					if (bravery > 0)
					{
						int modifier = 100;
						if (buTarget->getOriginalFaction() == FACTION_PLAYER)
						{
							modifier = _save->getMoraleModifier();
						}
						else if (buTarget->getOriginalFaction() == FACTION_HOSTILE)
						{
							modifier = _save->getMoraleModifier(0, false);
						}

						const int morale_loss = 10 * adjustedDamage * bravery / modifier;
						Log(LOG_INFO) << ". . . . morale_loss = " << morale_loss;

						buTarget->moraleChange(-morale_loss);
					}
				}

				if (buTarget->getSpecialAbility() == SPECAB_EXPLODEONDEATH
					&& !buTarget->isOut() // <- don't explode if stunned. Maybe... kL
					&& buTarget->getHealth() == 0)	// kL
//kL					(buTarget->getHealth() == 0 || buTarget->getStunlevel() >= buTarget->getHealth()))
				{
					if (type != DT_STUN
						&& type != DT_HE)
					{
						Log(LOG_INFO) << ". . . . new ExplosionBState(), !DT_STUN & !DT_HE";
						// kL_note: wait a second. hit() creates an ExplosionBState, but ExplosionBState::explode() creates a hit() ! -> terrain..

						Position unitPos = Position(
								buTarget->getPosition().x * 16,
								buTarget->getPosition().y * 16,
								buTarget->getPosition().z * 24);

						_save->getBattleGame()->statePushNext(
								new ExplosionBState(
										_save->getBattleGame(),
										unitPos,
										0,
										buTarget,
										0));
					}
				}

				if (buTarget->getOriginalFaction() == FACTION_HOSTILE
					&& attacker->getOriginalFaction() == FACTION_PLAYER
					&& type != DT_NONE)
				{
					Log(LOG_INFO) << ". . addFiringExp() - huh, even for Melee?";
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
		calculateSunShading();							// roofs could have been destroyed
		//Log(LOG_INFO) << ". calculateTerrainLighting()";
		calculateTerrainLighting();						// fires could have been started
		//Log(LOG_INFO) << ". calculateFOV()";
		calculateFOV(pTarget_voxel / Position(16, 16, 24));	// walls & objects could have been ruined


		if (buTarget) Log(LOG_INFO) << "TileEngine::hit() EXIT, return buTarget";
		else Log(LOG_INFO) << "TileEngine::hit() EXIT, return 0";

		return buTarget;
	}
	//else Log(LOG_INFO) << ". DT_ = " << static_cast<int>(type);

	Log(LOG_INFO) << "TileEngine::hit() EXIT, return NULL";
	return 0; // end_TEST
}

/**
 * Handles explosions.
 *
 * HE, smoke and fire explodes in a circular pattern on 1 level only.
 * HE however damages floor tiles of the above level. Not the units on it.
 * HE destroys an object if its armor is lower than the explosive power,
 * then its HE blockage is applied for further propagation.
 * See http://www.ufopaedia.org/index.php?title=Explosions for more info.
 * @param voxelTarget, Center of the explosion in voxelspace.
 * @param power, Power of the explosion.
 * @param type, The damage type of the explosion.
 * @param maxRadius, The maximum radius othe explosion.
 * @param unit, The unit that caused the explosion.
 */
void TileEngine::explode(
			const Position& voxelTarget,
			int power,
			ItemDamageType type,
			int maxRadius,
			BattleUnit* unit)
{
	Log(LOG_INFO) << "TileEngine::explode() power = " << power << " ; type = " << (int)type << " ; maxRadius = " << maxRadius;

	double centerZ = static_cast<double>((voxelTarget.z / 24) + 0.5);		// kL
	double centerX = static_cast<double>((voxelTarget.x / 16) + 0.5);		// kL
	double centerY = static_cast<double>((voxelTarget.y / 16) + 0.5);		// kL

	int power_;

	std::set<Tile*> tilesAffected;
	std::pair<std::set<Tile*>::iterator, bool> ret;

	if (type == DT_IN)
	{
		power /= 2;
		Log(LOG_INFO) << ". DT_IN power = " << power;
	}

	int vertdec = 1000; // default flat explosion

	int exHeight = Options::getInt("battleExplosionHeight");
	if (exHeight < 0) exHeight = 0;
	if (exHeight > 3) exHeight = 3;
	switch (exHeight)
	{
		case 1:
			vertdec = 30;
			break;
		case 2:
//kL			vertdec = 10;
			vertdec = 20;		// kL
			break;
		case 3:
//kL			vertdec = 5;
			vertdec = 10;		// kL
	}

	for (int fi = -90; fi <= 90; fi += 5)
//	for (int fi = 0; fi <= 0; fi += 10)
	{
		// raytrace every 3 degrees makes sure we cover all tiles in a circle.
		for (int te = 0; te <= 360; te += 3)
		{
			double cos_te = cos(static_cast<double>(te) * M_PI / 180.0);
			double sin_te = sin(static_cast<double>(te) * M_PI / 180.0);
			double sin_fi = sin(static_cast<double>(fi) * M_PI / 180.0);
			double cos_fi = cos(static_cast<double>(fi) * M_PI / 180.0);

			Tile* origin = _save->getTile(Position((int)centerX, (int)centerY, (int)centerZ));
			double l = 0.0;
			double vx, vy, vz;
			int tileX, tileY, tileZ;
			power_ = power + 1;

			while (power_ > 0
				&& l <= static_cast<double>(maxRadius))
			{
				vx = centerX + l * sin_te * cos_fi;
				vy = centerY + l * cos_te * cos_fi;
				vz = centerZ + l * sin_fi;

				tileZ = static_cast<int>(floor(vz));
				tileX = static_cast<int>(floor(vx));
				tileY = static_cast<int>(floor(vy));

				Tile* dest = _save->getTile(Position(tileX, tileY, tileZ));

				if (!dest) break; // out of map!


				// blockage by terrain is deducted from the explosion power
				if (std::abs(l) > 0) // no need to block epicentrum
				{
					power_ -= (horizontalBlockage(origin, dest, type) + verticalBlockage(origin, dest, type)) * 2;
					power_ -= 10; // explosive damage decreases by 10 per tile
					if (origin->getPosition().z != tileZ) // 3d explosion factor
						power_ -= vertdec;

					if (type == DT_IN)
					{
						int dir;
						Pathfinding::vectorToDirection(origin->getPosition() - dest->getPosition(), dir);

						if (dir != -1 && dir %2)
						{
							power_ -= 5; // diagonal movement costs an extra 50% for fire.
						}
					}
				}

				if (power_ > 0)
				{
					//Log(LOG_INFO) << ". power_ > 0";

					if (type == DT_HE)
					{
						// explosives do 1/2 damage to terrain and 1/2 up to 3/2
						// random damage to units (the halving is handled elsewhere)
						dest->setExplosive(power_);
					}

					ret = tilesAffected.insert(dest); // check if we had this tile already
					if (ret.second)
					{
						if (type == DT_STUN) // power 0 - 200%
						{
							if (dest->getUnit())
							{
//kL								dest->getUnit()->damage(Position(0, 0, 0), RNG::generate(0, power_ * 2), type);
								int powerUnit = RNG::generate(1, power_ * 2);
								Log(LOG_INFO) << ". . . powerUnit = " << powerUnit << " DT_STUN";
								dest->getUnit()->damage(Position(0, 0, 0), powerUnit, type);	// kL
							}

							for (std::vector<BattleItem*>::iterator
									it = dest->getInventory()->begin();
									it != dest->getInventory()->end();
									++it)
							{
								if ((*it)->getUnit())
								{
//kL									(*it)->getUnit()->damage(Position(0, 0, 0), RNG::generate(0, power_ * 2), type);
									int powerCorpse = RNG::generate(1, power_ * 2);
									Log(LOG_INFO) << ". . . . powerCorpse = " << powerCorpse << " DT_STUN";
									(*it)->getUnit()->damage(Position(0, 0, 0), powerCorpse, type);		// kL
								}
							}
						}

						if (type == DT_HE) // power 50 - 150%, 60% of that if kneeled.
						{
							//Log(LOG_INFO) << ". . type == DT_HE";

							BattleUnit* targetUnit = dest->getUnit();

							if (targetUnit)
							{
								int powerVsUnit = static_cast<int>(
										RNG::generate(static_cast<float>(power_) * 0.5f, static_cast<float>(power_) * 1.5f));
								Log(LOG_INFO) << ". . . powerVsUnit = " << powerVsUnit << " DT_HE";
	
								if (targetUnit->isKneeled())
								{
									powerVsUnit = powerVsUnit * 3 / 5;
									Log(LOG_INFO) << ". . . powerVsUnit(kneeled) = " << powerVsUnit << " DT_HE";
								}

								targetUnit->damage(
										Position(0, 0, 0),
										powerVsUnit,
										type);
							}

							bool done = false;
							while (!done) // kL_note: what the fuck kind of a loop is this ??!!1?
							{
								done = dest->getInventory()->empty();
								for (std::vector<BattleItem*>::iterator
										it = dest->getInventory()->begin();
										it != dest->getInventory()->end();
										)
								{
									if (power_ > (*it)->getRules()->getArmor())
									{
										if ((*it)->getUnit()
											&& (*it)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
										{
											Log(LOG_INFO) << ". . . . Frank blow'd up.";
											(*it)->getUnit()->instaKill();
										}

										Log(LOG_INFO) << ". . . item destroyed";
										_save->removeItem((*it));

										break;
									}
									else
									{
										++it;
										done = (it == dest->getInventory()->end());
									}
								}
							}
						}
						//Log(LOG_INFO) << ". type == DT_HE DONE";

						// kL_note: Could do instant smoke inhalation damage here (sorta like Fire or Stun).
						if (type == DT_SMOKE) // smoke from explosions always stay 6 to 14 turns - power of a smoke grenade is 60
						{
							if (dest->getSmoke() < 10)
							{
								dest->setFire(0);
								dest->setSmoke(RNG::generate(7, 15));
							}
						}

						if (type == DT_IN
							&& !dest->isVoid())
						{
							if (dest->getFire() == 0)
							{
								dest->setFire(dest->getFuel() + 1);
								dest->setSmoke(std::max(1, std::min(15 - (dest->getFlammability() / 10), 12)));
							}

							if (dest->getUnit())
							// kL_note: fire damage is also caused by BattlescapeGame::endTurn() -- but previously by BattleUnit::prepareNewTurn()!!!!
							{
								float resistance = dest->getUnit()->getArmor()->getDamageModifier(DT_IN);
								if (resistance > 0.f)
								{
//kL									int fire = RNG::generate(4, 11);					// <- why is this not based on power_
									int fire = RNG::generate(power_ / 4, power_ * 3 / 4);	// kL: 25 - 50%

//kL									dest->getUnit()->damage(Position(0, 0, 12-dest->getTerrainLevel()), RNG::generate(5, 10), DT_IN, true);
									dest->getUnit()->damage(
														Position(
																0,
																0,
																12 - dest->getTerrainLevel()),
															fire,
															DT_IN,
															true);		// kL
									Log(LOG_INFO) << ". . DT_IN : " << dest->getUnit()->getId() << " takes " << fire << " fire";

//kL									int burnTime = RNG::generate(0, int(5 * resistance));
									int burnTime = RNG::generate(0, static_cast<int>(5.f * resistance));	// kL
									if (dest->getUnit()->getFire() < burnTime)
									{
										dest->getUnit()->setFire(burnTime); // catch fire and burn
									}
								}
							}
						}

						if (unit
							&& dest->getUnit()
							&& unit->getOriginalFaction() == FACTION_PLAYER			// kL
							&& unit->getFaction() == FACTION_PLAYER					// kL
							&& dest->getUnit()->getFaction() != FACTION_PLAYER)		// kL
//							&& dest->getUnit()->getFaction() != unit->getFaction())
						{
							unit->addFiringExp();
						}
					}
				}

				origin = dest;

				l++;
			}
		}
	}

	// now detonate the tiles affected with HE
	if (type == DT_HE)
	{
		Log(LOG_INFO) << ". explode Tiles";

		for (std::set<Tile*>::iterator i = tilesAffected.begin(); i != tilesAffected.end(); ++i)
		{
			if (detonate(*i))
				_save->setObjectiveDestroyed(true);

			applyGravity(*i);

			Tile *j = _save->getTile((*i)->getPosition() + Position(0, 0, 1));
			if (j)
				applyGravity(j);
		}
		Log(LOG_INFO) << ". explode Tiles DONE";
	}

	calculateSunShading();		// roofs could have been destroyed
	calculateTerrainLighting();	// fires could have been started

	calculateFOV(voxelTarget / Position(16, 16, 24));
	Log(LOG_INFO) << "TileEngine::explode() EXIT";
}

/**
 * Applies the explosive power to the tile parts. This is where the actual destruction takes place.
 * Must affect 7 objects (6 box sides and the object inside).
 * @param tile, Tile affected.
 * @return, True if the objective was destroyed.
 */
bool TileEngine::detonate(Tile* tile)
{
	//Log(LOG_INFO) << "TileEngine::detonate()";

	int explosive = tile->getExplosive();
	tile->setExplosive(0, true);

	bool objective = false;

	Tile* tiles[7];
	static const int parts[7] = {0, 1, 2, 0, 1, 2, 3};

	Position pos = tile->getPosition();
	tiles[0] = _save->getTile(Position(pos.x, pos.y, pos.z + 1)); // ceiling
	tiles[1] = _save->getTile(Position(pos.x + 1, pos.y, pos.z)); // east wall
	tiles[2] = _save->getTile(Position(pos.x, pos.y + 1, pos.z)); // south wall
	tiles[3] = tiles[4] = tiles[5] = tiles[6] = tile;

	if (explosive)
	{
		int remainingPower = explosive;
		int flam = tile->getFlammability();
		int fuel = tile->getFuel() + 1;

		// explosions create smoke which only stays 1 or 2 turns; kL_note: or 3
		// smoke added to an already smoking tile will increase smoke to max.15
		tile->setSmoke(std::max(1, std::min(tile->getSmoke() + RNG::generate(0, 3), 15)));

		for (int i = 0; i < 7; ++i)
		{
			if (tiles[i]
				&& tiles[i]->getMapData(parts[i]))
			{
				remainingPower = explosive;
				while (remainingPower >= 0
					&& tiles[i]->getMapData(parts[i]))
				{
					remainingPower -= 2 * tiles[i]->getMapData(parts[i])->getArmor();
					if (remainingPower >= 0)
					{
						int volume = 0;
						// get the volume of the object by checking its loftemps objects.
						for (int j = 0; j < 12; j++)
						{
							if (tiles[i]->getMapData(parts[i])->getLoftID(j) != 0)
								++volume;
						}

						if (i > 3)
						{
							tiles[i]->setFire(0);

							int smoke = RNG::generate(0, (volume / 2) + 2);
							smoke += (volume / 2) + 1;
							if (smoke > tiles[i]->getSmoke())
							{
								tiles[i]->setSmoke(std::max(0, std::min(smoke, 15)));
							}
						}

						if (tiles[i]->destroy(parts[i]))
						{
							objective = true;
						}

						if (tiles[i]->getMapData(parts[i]))
						{
							flam = tiles[i]->getFlammability();
							fuel = tiles[i]->getFuel() + 1;
						}
					}
				}

				if (2 * flam < remainingPower)
				{
					tile->setFire(fuel);
					tile->setSmoke(std::max(1, std::min(15 - (flam / 10), 12)));
				}
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
 * @return tile on which a explosion occurred
 */
Tile* TileEngine::checkForTerrainExplosions()
{
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		if (_save->getTiles()[i]->getExplosive())
		{
			return _save->getTiles()[i];
		}
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
	int block = 0;

	// safety check
	if (startTile == 0 || endTile == 0) return 0;

	int direction = endTile->getPosition().z - startTile->getPosition().z;
	if (direction == 0) return 0;

	int x = startTile->getPosition().x;
	int y = startTile->getPosition().y;
	int z = startTile->getPosition().z;		// kL

	if (direction < 0) // down
	{
		for (
				;
				z > endTile->getPosition().z;
				z--)
		{
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type, Pathfinding::DIR_DOWN);
		}

		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			z = startTile->getPosition().z;

			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), type);

			for (
					;
					z > endTile->getPosition().z;
					z--)
			{
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type);
			}
		}
	}
	else if (direction > 0) // up
	{
		for (
				z += 1;
				z <= endTile->getPosition().z;
				z++)
		{
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
			block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type, Pathfinding::DIR_UP);
		}

		if (x != endTile->getPosition().x || y != endTile->getPosition().y)
		{
			x = endTile->getPosition().x;
			y = endTile->getPosition().y;
			z = startTile->getPosition().z;

			block += horizontalBlockage(startTile, _save->getTile(Position(x, y, z)), type);

			for (
					z += 1;
					z <= endTile->getPosition().z;
					z++)
			{
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_FLOOR, type);
				block += blockage(_save->getTile(Position(x, y, z)), MapData::O_OBJECT, type);
			}
		}
	}

	return block;
}

/**
 * Calculates the amount of power that is blocked going from one tile to another on the same level.
 * @param startTile, The tile where the power starts.
 * @param endTile, The adjacent tile where the power ends.
 * @param type, The type of power/damage.
 * @return, Amount of blockage (-1 for Big Wall tile, 0 on noBlock or ERROR).
 */
int TileEngine::horizontalBlockage(Tile* startTile, Tile* endTile, ItemDamageType type)
{
	static const Position oneTileNorth	= Position(0, -1, 0);
	static const Position oneTileEast	= Position(1, 0, 0);
	static const Position oneTileSouth	= Position(0, 1, 0);
	static const Position oneTileWest	= Position(-1, 0, 0);

	// safety check
	if (startTile == 0 || endTile == 0)
		return 0;

	if (startTile->getPosition().z != endTile->getPosition().z)
		return 0;

	int direction; // kL_note: should this be (int&)
	Pathfinding::vectorToDirection(endTile->getPosition() - startTile->getPosition(), direction);

	if (direction == -1)
		return 0;

	int block = 0;
	switch (direction)
	{
		case 0:	// north
			block = blockage(startTile, MapData::O_NORTHWALL, type);
		break;
		case 1: // north east
			if (type == DT_NONE) // this is two-way diagonal visiblity check, used in original game
			{
				block = blockage(startTile, MapData::O_NORTHWALL, type) + blockage(endTile, MapData::O_WESTWALL, type); // up+right
				block += blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 3);

				if (block == 0) break; // this way is opened

				block = blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_NORTHWALL, type)
						+ blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_WESTWALL, type); // right+up
				block += blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 7);
			}
			else
			{
				block = (blockage(startTile,MapData::O_NORTHWALL, type) + blockage(endTile,MapData::O_WESTWALL, type)) / 2
						+ (blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_WESTWALL, type)
							+ blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_NORTHWALL, type)) / 2
				+ blockage(_save->getTile(startTile->getPosition() + oneTileEast),MapData::O_OBJECT, type, direction)
						+ (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4)
							+ blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 2)) / 2;

/*				if (type == DT_HE)
				{
					block += (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4)
							+ blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 6)) / 2;
				}
				else
				{
					block = (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4)
							+ blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 6)) < 510? 0: 255;
				} */
			}
		break;
		case 2: // east
			block = blockage(endTile,MapData::O_WESTWALL, type);
		break;
		case 3: // south east
			if (type == DT_NONE)
			{
				block = blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_NORTHWALL, type)
						+ blockage(endTile, MapData::O_WESTWALL, type); // down+right
				block += blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 1);

				if (block == 0) break; // this way is opened

				block = blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_WESTWALL, type)
						+ blockage(endTile, MapData::O_NORTHWALL, type); // right+down
				block += blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 5);
			}
			else
			{
				block = (blockage(endTile,MapData::O_WESTWALL, type) + blockage(endTile, MapData::O_NORTHWALL, type)) / 2
						+ (blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_WESTWALL, type)
							+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_NORTHWALL, type)) / 2
						+ (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 2)
							+ blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 4)) / 2;

/*				if (type == DT_HE)
					block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 0) +
							blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 6)) / 2;
				else
					block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 0) +
							blockage(_save->getTile(startTile->getPosition() + oneTileEast), MapData::O_OBJECT, type, 6)) < 510? 0: 255; */
			}
		break;
		case 4: // south
			block = blockage(endTile, MapData::O_NORTHWALL, type);
		break;
		case 5: // south west
			if (type == DT_NONE)
			{
				block = blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_NORTHWALL, type)
						+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_WESTWALL, type); // down+left
				block += blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 7);

				if (block == 0) break; // this way is opened

				block = blockage(startTile, MapData::O_WESTWALL, type) + blockage(endTile, MapData::O_NORTHWALL, type); // left+down
				block += blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 3);
			}
			else
			{
				block = (blockage(endTile,MapData::O_NORTHWALL, type) + blockage(startTile, MapData::O_WESTWALL, type)) / 2
						+ (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_WESTWALL, type)
							+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_NORTHWALL, type)) / 2
				+ blockage(_save->getTile(startTile->getPosition() + oneTileSouth),MapData::O_OBJECT, type, direction)
						+ (blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2)
							+ blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 4)) / 2;

/*				if (type == DT_HE)
					block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 0) +
							blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2)) / 2;
				else
					block += (blockage(_save->getTile(startTile->getPosition() + oneTileSouth), MapData::O_OBJECT, type, 0) +
							blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2)) < 510? 0: 255; */
			}
		break;
		case 6: // west
			block = blockage(startTile,MapData::O_WESTWALL, type);
		break;
		case 7: // north west
			if (type == DT_NONE)
			{
				block = blockage(startTile, MapData::O_NORTHWALL, type)
						+ blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_WESTWALL, type); // up+left
				block += blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 5);

				if (block == 0) break; // this way is opened

				block = blockage(startTile, MapData::O_WESTWALL, type)
						+ blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_NORTHWALL, type); // left+up
				block += blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 1);
			}
			else
			{
				block = (blockage(startTile,MapData::O_WESTWALL, type) + blockage(startTile, MapData::O_NORTHWALL, type))/2
					+ (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_WESTWALL, type)
						+ blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_NORTHWALL, type))/2
					+ (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4)
						+ blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2))/2;

/*				if (type == DT_HE)
					block += (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4) +
							blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2))/2;
				else
					block += (blockage(_save->getTile(startTile->getPosition() + oneTileNorth), MapData::O_OBJECT, type, 4) +
							blockage(_save->getTile(startTile->getPosition() + oneTileWest), MapData::O_OBJECT, type, 2)) < 510? 0: 255; */
			}
		break;

		default:
		break;
	}

    block += blockage(startTile, MapData::O_OBJECT, type, direction);

	if (type != DT_NONE)
	{
		direction += 4;
		if (direction > 7)
			direction -= 8;

		block += blockage(endTile, MapData::O_OBJECT, type, direction);
	}
	else // type == DT_NONE
	{
        if (block <= 127) 
        {
            direction += 4;
            if (direction > 7)
                direction -= 8;

            if (blockage(endTile, MapData::O_OBJECT, type, direction) > 127)
			{
                return -1; // hit bigwall, reveal bigwall tile
            }
        }
	}

	return block;
}

/**
 * Calculates the amount this certain wall or floor-part of the tile blocks.
 * @param startTile The tile where the power starts.
 * @param part The part of the tile the power needs to go through.
 * @param type The type of power/damage.
 * @param direction Direction the power travels.
 * @return Amount of blockage.
 */
int TileEngine::blockage(Tile* tile, const int part, ItemDamageType type, int direction)
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
			switch (direction)
			{
				case 0: // north
					if (wall == Pathfinding::BIGWALLWEST
						|| wall == Pathfinding::BIGWALLEAST
						|| wall == Pathfinding::BIGWALLSOUTH
						|| wall == Pathfinding::BIGWALLEASTANDSOUTH)
					{
						check = false;
					}
				break;
				case 1: // north east
					if (wall == Pathfinding::BIGWALLWEST
						|| wall == Pathfinding::BIGWALLSOUTH)
					{
						check = false;
					}
				break;
				case 2: // east
					if (wall == Pathfinding::BIGWALLNORTH
						|| wall == Pathfinding::BIGWALLSOUTH
						|| wall == Pathfinding::BIGWALLWEST)
					{
						check = false;
					}
				break;
				case 3: // south east
					if (wall == Pathfinding::BIGWALLNORTH
						|| wall == Pathfinding::BIGWALLWEST)
					{
						check = false;
					}
				break;
				case 4: // south
					if (wall == Pathfinding::BIGWALLWEST
						|| wall == Pathfinding::BIGWALLEAST
						|| wall == Pathfinding::BIGWALLNORTH)
					{
						check = false;
					}
				break;
				case 5: // south west
					if (wall == Pathfinding::BIGWALLNORTH
						|| wall == Pathfinding::BIGWALLEAST)
					{
						check = false;
					}
				break;
				case 6: // west
					if (wall == Pathfinding::BIGWALLNORTH
						|| wall == Pathfinding::BIGWALLSOUTH
						|| wall == Pathfinding::BIGWALLEAST
						|| wall == Pathfinding::BIGWALLEASTANDSOUTH)
					{
						check = false;
					}
				break;
				case 7: // north west
					if (wall == Pathfinding::BIGWALLSOUTH
						|| wall == Pathfinding::BIGWALLEAST
						|| wall == Pathfinding::BIGWALLEASTANDSOUTH)
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
			if (type == DT_SMOKE
				&& wall != 0
				&& !tile->isUfoDoorOpen(part))
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
}

/**
 * Opens a door (if any) by rightclick, or by walking through it. The unit has to face in the right direction.
 * @param unit Unit.
 * @param rClick Whether the player right clicked.
 * @param dir Direction.
 * @return -1 there is no door, you can walk through; or you're a tank and can't do sweet shit with a door except blast the fuck out of it.
 *			0 normal door opened, make a squeaky sound and you can walk through;
 *			1 ufo door is starting to open, make a whoosh sound, don't walk through;
 *			3 ufo door is still opening, don't walk through it yet. (have patience, futuristic technology...)
 *			4 not enough TUs
 *			5 would contravene fire reserve
 */
int TileEngine::unitOpensDoor(BattleUnit* unit, bool rClick, int dir)
{
	int TUCost = 0;
	int door = -1;

	int size = unit->getArmor()->getSize();
	if (size > 1 && rClick)
		return door;

	if (dir == -1)
	{
		dir = unit->getDirection();
	}

	int z = unit->getTile()->getTerrainLevel() < -12? 1:0; // if we're standing on stairs, check the tile above instead.
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
			std::vector<std::pair<Position, int>> checkPositions;
			Tile* tile = _save->getTile(unit->getPosition() + Position(x, y, z));

			if (!tile)
				continue;


			switch (dir)
			{
				case 0: // north
					checkPositions.push_back(std::make_pair(Position(0, 0, 0), MapData::O_NORTHWALL));	// origin
				break;
				case 1: // north east
					checkPositions.push_back(std::make_pair(Position(0, 0, 0), MapData::O_NORTHWALL));	// origin
					checkPositions.push_back(std::make_pair(Position(1, -1, 0), MapData::O_WESTWALL));	// one tile north-east
					checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
					checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_NORTHWALL));	// one tile east
				break;
				case 2: // east
					checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
				break;
				case 3: // south-east
					checkPositions.push_back(std::make_pair(Position(1, 0, 0), MapData::O_WESTWALL));	// one tile east
					checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
					checkPositions.push_back(std::make_pair(Position(1, 1, 0), MapData::O_WESTWALL));	// one tile south-east
					checkPositions.push_back(std::make_pair(Position(1, 1, 0), MapData::O_NORTHWALL));	// one tile south-east
				break;
				case 4: // south
					checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
				break;
				case 5: // south-west
					checkPositions.push_back(std::make_pair(Position(0, 0, 0), MapData::O_WESTWALL));	// origin
					checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_WESTWALL));	// one tile south
					checkPositions.push_back(std::make_pair(Position(0, 1, 0), MapData::O_NORTHWALL));	// one tile south
					checkPositions.push_back(std::make_pair(Position(-1, 1, 0), MapData::O_NORTHWALL));	// one tile south-west
				break;
				case 6: // west
					checkPositions.push_back(std::make_pair(Position(0, 0, 0), MapData::O_WESTWALL));	// origin
				break;
				case 7: // north-west
					checkPositions.push_back(std::make_pair(Position(0, 0, 0), MapData::O_WESTWALL));	// origin
					checkPositions.push_back(std::make_pair(Position(0, 0, 0), MapData::O_NORTHWALL));	// origin
					checkPositions.push_back(std::make_pair(Position(0, -1, 0), MapData::O_WESTWALL));	// one tile north
					checkPositions.push_back(std::make_pair(Position(-1, 0, 0), MapData::O_NORTHWALL));	// one tile west
				break;

				default:
				break;
			}

			int part = 0;
			for (std::vector<std::pair<Position, int>>::const_iterator
					i = checkPositions.begin();
					i != checkPositions.end()
						&& door == -1;
					++i)
			{
				tile = _save->getTile(unit->getPosition() + Position(x, y, z) + i->first);
				if (tile)
				{
					door = tile->openDoor(i->second, unit, _save->getBattleGame()->getReservedAction());
					if (door != -1)
					{
						part = i->second;

						if (door == 1)
						{
							checkAdjacentDoors(unit->getPosition() + Position(x, y, z) + i->first, i->second);
						}
					}
				}
			}

			if (door == 0 && rClick)
			{
				if (part == MapData::O_WESTWALL)
				{
					part = MapData::O_NORTHWALL;
				}
				else
				{
					part = MapData::O_WESTWALL;
				}

				TUCost = tile->getTUCost(part, unit->getArmor()->getMovementType());
			}
			else if (door == 1 || door == 4)
			{
				TUCost = tile->getTUCost(part, unit->getArmor()->getMovementType());
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
				std::vector<BattleUnit*>* vunits = unit->getVisibleUnits();
				for (size_t
						i = 0;
						i < vunits->size();
						++i)
				{
					calculateFOV(vunits->at(i));
				}
			}
			else return 4;
		}
		else return 5;
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
 * Opens any doors connected to this part at this position,
 * Keeps processing til it hits a non-ufo-door.
 * @param pos The starting position
 * @param part The part to open, defines which direction to check.
 */
void TileEngine::checkAdjacentDoors(Position pos, int part)
{
	Position offset;
	bool westSide = (part == 1);

	for (int i = 1;; ++i)
	{
		offset = westSide ? Position(0, i, 0):Position(i, 0, 0);
		Tile* tile = _save->getTile(pos + offset);
		if (tile
			&& tile->getMapData(part)
			&& tile->getMapData(part)->isUFODoor())
		{
			tile->openDoor(part);
		}
		else break;
	}

	for (int i = -1;; --i)
	{
		offset = westSide ? Position(0, i, 0):Position(i, 0, 0);
		Tile* tile = _save->getTile(pos + offset);
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
 * @return Whether doors are closed.
 */
int TileEngine::closeUfoDoors()
{
	int doorsclosed = 0;

	// prepare a list of tiles on fire/smoke & close any ufo doors
	for (int i = 0; i < _save->getMapSizeXYZ(); ++i)
	{
		if (_save->getTiles()[i]->getUnit()
			&& _save->getTiles()[i]->getUnit()->getArmor()->getSize() > 1)
		{
			BattleUnit* bu = _save->getTiles()[i]->getUnit();

			Tile* tile = _save->getTiles()[i];
			Tile* oneTileNorth = _save->getTile(tile->getPosition() + Position(0, -1, 0));
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
 * @param doVoxelCheck, Check against voxel or tile blocking? (first one for units visibility and line of fire, second one for terrain visibility).
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

	int x, x0, x1, delta_x, step_x;
	int y, y0, y1, delta_y, step_y;
	int z, z0, z1, delta_z, step_z;

	int swap_xy, swap_xz;
	int drift_xy, drift_xz;
	int cx, cy, cz;

	Position lastPoint(origin);
	int result;

	// start and end points
	x0 = origin.x; x1 = target.x;
	y0 = origin.y; y1 = target.y;
	z0 = origin.z; z1 = target.z;

	// 'steep' xy Line, make longest delta x plane
	swap_xy = abs(y1 - y0) > abs(x1 - x0);
	if (swap_xy)
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
	}

	// do same for xz
	swap_xz = abs(z1 - z0) > abs(x1 - x0);
	if (swap_xz)
	{
		std::swap(x0, z0);
		std::swap(x1, z1);
	}

	// delta is Length in each plane
	delta_x = abs(x1 - x0);
	delta_y = abs(y1 - y0);
	delta_z = abs(z1 - z0);

	// drift controls when to step in 'shallow' planes
	// starting value keeps Line centred
	drift_xy = delta_x / 2;
	drift_xz = delta_x / 2;

	// direction of line
	step_x = 1; if (x0 > x1) {step_x = -1;}
	step_y = 1; if (y0 > y1) {step_y = -1;}
	step_z = 1; if (z0 > z1) {step_z = -1;}

	// starting point
	y = y0;
	z = z0;

	// step through longest delta (which we have swapped to x)
	for (
			x = x0;
			x != x1 + step_x;
			x += step_x)
	{
		// copy position
		cx = x;	cy = y;	cz = z;

		// unswap (in reverse)
		if (swap_xz) std::swap(cx, cz);
		if (swap_xy) std::swap(cx, cy);

		if (storeTrajectory && trajectory)
		{
			trajectory->push_back(Position(cx, cy, cz));
		}

		// passes through this point
		if (doVoxelCheck)
		{
			result = voxelCheck(Position(cx, cy, cz), excludeUnit, false, onlyVisible, excludeAllBut);
			if (result != VOXEL_EMPTY)
			{
				if (trajectory)
				{
					trajectory->push_back(Position(cx, cy, cz)); // store the position of impact
				}

				//Log(LOG_INFO) << "TileEngine::calculateLine(). 1 return " << result;
				return result;
			}
		}
		else
		{
            int result2 = verticalBlockage(_save->getTile(lastPoint), _save->getTile(Position(cx, cy, cz)), DT_NONE);
			result = horizontalBlockage(_save->getTile(lastPoint), _save->getTile(Position(cx, cy, cz)), DT_NONE);
            if (result == -1)
            {
                if (result2 > 127)
                {
                    result = 0;
                }
				else
				{
					//Log(LOG_INFO) << "TileEngine::calculateLine(), odd return1, could be Big Wall = " << result;
					//Log(LOG_INFO) << "TileEngine::calculateLine(). 2 return " << result;
					return result; // We hit a big wall
                }
            }

			result += result2;
			if (result > 127)
			{
				//Log(LOG_INFO) << "TileEngine::calculateLine(), odd return2, could be Big Wall = " << result;
				//Log(LOG_INFO) << "TileEngine::calculateLine(). 3 return " << result;
				return result;
			}

			lastPoint = Position(cx, cy, cz);
		}

		// update progress in other planes
		drift_xy = drift_xy - delta_y;
		drift_xz = drift_xz - delta_z;

		// step in y plane
		if (drift_xy < 0)
		{
			y = y + step_y;
			drift_xy = drift_xy + delta_x;

			// check for xy diagonal intermediate voxel step
			if (doVoxelCheck)
			{
				cx = x;	cz = z; cy = y;
				if (swap_xz) std::swap(cx, cz);
				if (swap_xy) std::swap(cx, cy);

				result = voxelCheck(Position(cx, cy, cz), excludeUnit, false, onlyVisible, excludeAllBut);
				if (result != VOXEL_EMPTY)
				{
					if (trajectory != 0)
					{
						trajectory->push_back(Position(cx, cy, cz)); // store the position of impact
					}

					//Log(LOG_INFO) << "TileEngine::calculateLine(). 4 return " << result;
					return result;
				}
			}
		}

		// same in z
		if (drift_xz < 0)
		{
			z = z + step_z;
			drift_xz = drift_xz + delta_x;

			// check for xz diagonal intermediate voxel step
			if (doVoxelCheck)
			{
				cx = x;	cz = z; cy = y;
				if (swap_xz) std::swap(cx, cz);
				if (swap_xy) std::swap(cx, cy);

				result = voxelCheck(Position(cx, cy, cz), excludeUnit, false, onlyVisible,  excludeAllBut);
				if (result != VOXEL_EMPTY)
				{
					if (trajectory != 0)
					{
						trajectory->push_back(Position(cx, cy, cz)); // store the position of impact
					}

					//Log(LOG_INFO) << "TileEngine::calculateLine(). 5 return " << result;
					return result;
				}
			}
		}
	}

	//Log(LOG_INFO) << ". return -1";
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
//				const Position delta // Wb.131129, supercedes 'acu'
				double acu)
{
	Log(LOG_INFO) << "TileEngine::calculateParabola()";

	double ro = sqrt(static_cast<double>((target.x - origin.x) * (target.x - origin.x)
			+ (target.y - origin.y) * (target.y - origin.y)
			+ (target.z - origin.z) * (target.z - origin.z)));

//	if (AreSame(ro, 0.0)) return VOXEL_EMPTY; // just in case. Wb.131129

	double fi = acos(static_cast<double>(target.z - origin.z) / ro);
	double te = atan2(static_cast<double>(target.y - origin.y), static_cast<double>(target.x - origin.x));

	fi *= acu;
	te *= acu;
//	te += (delta.x / ro) / 2.0 * M_PI;							// horizontal magic value
//	fi += ((delta.z + delta.y) / ro) / 14.0 * M_PI * curvature;	// another magic value (vertical), to make it in line with fire spread

	double zA = sqrt(ro) * arc;
	double zK = 4.0 * zA / ro / ro;

	int x = origin.x;
	int y = origin.y;
	int z = origin.z;
	int i = 8;

//kL	Position lastPosition = Position(x, y, z);

	while (z > 0)
	{
		x = static_cast<int>(static_cast<double>(origin.x) + static_cast<double>(i) * cos(te) * sin(fi));
		y = static_cast<int>(static_cast<double>(origin.y) + static_cast<double>(i) * sin(te) * sin(fi));
		z = static_cast<int>(static_cast<double>(origin.z) + static_cast<double>(i) * cos(fi)
				- zK * (static_cast<double>(i) - ro / 2.0) * (static_cast<double>(i) - ro / 2.0) + zA);

		if (storeTrajectory && trajectory)
		{
			trajectory->push_back(Position(x, y, z));
		}

//kL		Position nextPosition = Position(x, y, z);
//kL		int result = calculateLine(lastPosition, nextPosition, false, 0, excludeUnit);
		int result = voxelCheck(Position(x, y, z), excludeUnit); // Old code, not sure it should be changed to calculateLine()...
		if (result != VOXEL_EMPTY)
		{
//kL			if (lastPosition.z < nextPosition.z)
//kL			{
//kL				result = VOXEL_OUTOFBOUNDS;
//kL			}

			if (!storeTrajectory && trajectory != 0)
			{
//kL				trajectory->push_back(nextPosition);	// store the position of impact
				trajectory->push_back(Position(x, y, z));	// kL
			}

			return result;
		}

//kL		lastPosition = Position(x, y, z);
		++i;
	}

	if (!storeTrajectory && trajectory != 0)
	{
		trajectory->push_back(Position(x, y, z)); // store the position of impact
	}

	return VOXEL_EMPTY;
}

/**
 * Validates a throw action.
 * @param action, The action to validate
 * @return, Validity of action
 */
bool TileEngine::validateThrow(BattleAction* action) // superceded by Wb.131129 below.
{
	Log(LOG_INFO) << "TileEngine::validateThrow(), cf Projectile::calculateThrow()";

	bool found = false;

	Tile* tile = _save->getTile(action->target);
	if (!tile
		|| (action->type == BA_THROW
			&& tile
			&& tile->getMapData(MapData::O_OBJECT)
			&& tile->getMapData(MapData::O_OBJECT)->getTUCost(MT_WALK) == 255))
	{
		return false; // object blocking - can't throw here
	}

	Position originVoxel, targetVoxel;

	Position origin = action->actor->getPosition();
	Tile* tileAbove = _save->getTile(origin + Position(0, 0, 1));

	originVoxel = Position(
			(origin.x * 16) + 8,
			(origin.y * 16) + 8,
			(origin.z * 24));
	originVoxel.z += action->actor->getHeight()
						+ action->actor->getFloatHeight()
						- _save->getTile(origin)->getTerrainLevel()
						- 3;

	if (originVoxel.z >= (origin.z + 1) * 24)
	{
		if (!tileAbove
			|| !tileAbove->hasNoFloor(0))
		{
			while (originVoxel.z > (origin.z + 1) * 24)
			{
				originVoxel.z--;
			}

			originVoxel.z -= 4;
		}
		else
		{
			origin.z++;
		}
	}

	// determine the target voxel; aim at the center of the floor
	targetVoxel = Position(
			(action->target.x * 16) + 8,
			(action->target.y * 16) + 8,
			(action->target.z * 24) + 2);
	targetVoxel.z -= _save->getTile(action->target)->getTerrainLevel();

	if (action->type != BA_THROW)
	{
		BattleUnit* targetUnit;
		if (tile->getUnit())
		{
			targetUnit = tile->getUnit();
		}

		if (!targetUnit
			&& action->target.z > 0
			&& tile->hasNoFloor(0))
		{
			if (_save->getTile(Position(
						action->target.x,
						action->target.y,
						action->target.z - 1))
					->getUnit())
			{
				targetUnit = _save->getTile(Position(
							action->target.x,
							action->target.y,
							action->target.z - 1))
						->getUnit();
			}
		}

		if (targetUnit)
		{
			targetVoxel.z += (targetUnit->getHeight() / 2) + targetUnit->getFloatHeight();
		}
	}


	std::vector<Position> trajectory;

	// we try several different arcs to try and reach our goal.
//	double arc = 0.5; // start with a low traj.5 is a bit too low
	double arc = 1.0;
	if (action->type == BA_THROW)
	{
		arc = std::max(
					0.48,
					(1.73 / sqrt(
								sqrt(
									(static_cast<double>(action->actor->getStats()->strength)
										/ static_cast<double>(action->weapon->getRules()->getWeight())))))
							+ (action->actor->isKneeled()? 0.1: 0.0));
	}

	while (!found && arc < 5.0)
	{
		int checkParab = calculateParabola(
						originVoxel,
						targetVoxel,
						false,
						&trajectory,
						action->actor,
						arc,
						1.0);
		if (checkParab != 5
			&& trajectory.at(0).x / 16 == targetVoxel.x / 16
			&& trajectory.at(0).y / 16 == targetVoxel.y / 16
			&& trajectory.at(0).z / 24 == targetVoxel.z / 24)
		{
			found = true;
		}
		else
		{
			arc += 0.5;
		}

		trajectory.clear();
	}
	Log(LOG_INFO) << ". arc = " << arc;

	if (arc >= 5.0)
	{
		return false;
	}

	return ProjectileFlyBState::validThrowRange(action, originVoxel, tile);
}

/**
 * Validates a throw action.
 * @param action The action to validate.
 * @param originVoxel The origin point of the action.
 * @param targetVoxel The target point of the action.
 * @param curvature The curvature of the throw.
 * @param voxelType The type of voxel at which this parabola terminates.
 * @return Validity of action.
 */
/* Wb.131129
bool TileEngine::validateThrow(BattleAction &action, Position originVoxel, Position targetVoxel, double *curve, int *voxelType)
{
	bool foundCurve = false;
	double curvature = 0.5;
	if (action.type == BA_THROW)
	{
		curvature = std::max(
						0.48,
						(1.73 / sqrt(
									sqrt(
										(static_cast<double>(action->actor->getStats()->strength)
											/ static_cast<double>(action->weapon->getRules()->getWeight())))))
								+ (action->actor->isKneeled()? 0.1: 0.0));
	}
	Tile *targetTile = _save->getTile(action.target);
	// object blocking - can't throw here
	if ((action.type == BA_THROW
		&& targetTile
		&& targetTile->getMapData(MapData::O_OBJECT)
		&& targetTile->getMapData(MapData::O_OBJECT)->getTUCost(MT_WALK) == 255)
		|| ProjectileFlyBState::validThrowRange(&action, originVoxel, targetTile) == false)
	{
		return false;
	}

	// we try 8 different curvatures to try and reach our goal.
	int test = V_OUTOFBOUNDS;

	while (!foundCurve && curvature < 5.0)
	{
		std::vector<Position> trajectory;
		test = calculateParabola(originVoxel, targetVoxel, false, &trajectory, action.actor, curvature, Position(0,0,0));
		if (test != V_OUTOFBOUNDS && (trajectory.at(0) / Position(16, 16, 24)) == (targetVoxel / Position(16, 16, 24)))
		{
			if (voxelType)
			{
				*voxelType = test;
			}
			foundCurve = true;
		}
		else
		{
			curvature += 0.5;
		}
	}
	if (curvature >= 5.0)
	{
		return false;
	}
	if (curve)
	{
		*curve = curvature;
	}

	return true;
} */

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

	for (z = zstart; z > 0; z--)
	{
		tmpVoxel.z = z;
		if (voxelCheck(tmpVoxel, 0) != VOXEL_EMPTY)
			break;
	}

	return z;
}

/**
 * Traces voxel visibility.
 * @param voxel Voxel coordinates.
 * @return True if visible.
 */
bool TileEngine::isVoxelVisible(const Position& voxel)
{
	int zstart = voxel.z + 3; // slight Z adjust
	if (zstart / 24 != voxel.z / 24)
		return true; // visble!

	Position tmpVoxel = voxel;

	int zend = (zstart/24) * 24 + 24;
	for (int z = zstart; z < zend; z++)
	{
		tmpVoxel.z = z;

		// only OBJECT can cause additional occlusion (because of any shape)
		if (voxelCheck(tmpVoxel, 0) == VOXEL_OBJECT) return false;

		++tmpVoxel.x;
		if (voxelCheck(tmpVoxel, 0) == VOXEL_OBJECT) return false;

		++tmpVoxel.y;
		if (voxelCheck(tmpVoxel, 0) == VOXEL_OBJECT) return false;
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

	if (hit) return VOXEL_UNIT;		// kL


//	Tile* tileTest_old = _save->getTile(Position(pTarget_voxel.x / 16, pTarget_voxel.y / 16, pTarget_voxel.z / 24));
	//Log(LOG_INFO) << ". tileTest_old = " << tileTest_old->getPosition();

	Tile* tTarget = _save->getTile(pTarget_voxel / Position(16, 16, 24)); // converts to tilespace -> Tile
	//Log(LOG_INFO) << ". tTarget kL = " << tTarget->getPosition();

	// check if we are out of the map
	if (tTarget == 0
		|| pTarget_voxel.x < 0
		|| pTarget_voxel.y < 0
		|| pTarget_voxel.z < 0)
	{
		//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret 5";
		return VOXEL_OUTOFBOUNDS;
	}

	if (tTarget->isVoid()
		&& tTarget->getUnit() == 0) // <- check tileBelow ? It's done below, but this seems premature!!!
	{
		//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret(1) -1";
		return VOXEL_EMPTY;
	}

	if (pTarget_voxel.z %24 == 0
		&& tTarget->getMapData(MapData::O_FLOOR)
		&& tTarget->getMapData(MapData::O_FLOOR)->isGravLift())
	{
		Tile* tTarget_below = _save->getTile(tTarget->getPosition() + Position(0, 0, -1)); // tileBelow target
		if (tTarget_below
			&& tTarget_below->getMapData(MapData::O_FLOOR)
			&& !tTarget_below->getMapData(MapData::O_FLOOR)->isGravLift())
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
				{
					//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret 4";
					return VOXEL_UNIT;
				}
			}
		}
	}

	//Log(LOG_INFO) << "TileEngine::voxelCheck() EXIT, ret(2) -1"; // massive lag-to-file, Do not use.
	return VOXEL_EMPTY;
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
 * Calculates the distance between 2 points. Rounded down to first INT.
 * @param pos1, Position of first square.
 * @param pos2, Position of second square.
 * @return, Distance.
 */
int TileEngine::distance(const Position& pos1, const Position& pos2) const
{
	int x = pos1.x - pos2.x;
	int y = pos1.y - pos2.y;
	int z = pos1.z - pos2.z;	// kL

//kL	return (int)floor(sqrt(float(x * x + y * y)) + 0.5f);	// kL, why the +0.5 ???
//	return (int)floor(sqrt(float(x * x + y * y)));
	return static_cast<int>(floor(sqrt(static_cast<float>(x * x + y * y + z * z))));	// 3-d
}

/**
 * Calculates the distance squared between 2 points.
 * No sqrt(), not floating point math, and sometimes it's all you need.
 * @param pos1, Position of first square.
 * @param pos2, Position of second square.
 * @param considerZ, Whether to consider the z coordinate.
 * @return, Distance.
 */
int TileEngine::distanceSq(const Position& pos1, const Position& pos2, bool considerZ) const
{
	int x = pos1.x - pos2.x;
	int y = pos1.y - pos2.y;
	int z = considerZ? (pos1.z - pos2.z):0;

	return x * x + y * y + z * z;
}

/**
 * Attempts a panic or mind control action.
 * @param action, Pointer to an action
 * @return, Whether it failed or succeeded
 */
bool TileEngine::psiAttack(BattleAction* action)
{
	Log(LOG_INFO) << "TileEngine::psiAttack()";
	Log(LOG_INFO) << ". attackerID " << action->actor->getId();

//	bool ret = false;

	Tile* t = _save->getTile(action->target);
	if (t
		&& t->getUnit())
	{
		//Log(LOG_INFO) << ". . tile EXISTS, so does Unit";
		BattleUnit* victim = t->getUnit();
		Log(LOG_INFO) << ". . defenderID " << victim->getId();
		//Log(LOG_INFO) << ". . target(pos) " << action->target;


		double attackStr =
				static_cast<double>(action->actor->getStats()->psiStrength)
				* static_cast<double>(action->actor->getStats()->psiSkill) / 50.0;
		Log(LOG_INFO) << ". . . attackStr = " << (int)attackStr;

		double defenseStr =
				static_cast<double>(victim->getStats()->psiStrength)
				+ (static_cast<double>(victim->getStats()->psiSkill) / 5.0);
		Log(LOG_INFO) << ". . . defenseStr = " << (int)defenseStr;

		double d = static_cast<double>(distance(action->actor->getPosition(), action->target));
		Log(LOG_INFO) << ". . . d = " << d;

		attackStr -= d;

		attackStr -= defenseStr;
		if (action->type == BA_MINDCONTROL)
			attackStr += 25.0;
		else
			attackStr += 45.0;

		attackStr *= 100.0;
		attackStr /= 56.0;

		action->actor->addPsiExp();
		int percent = static_cast<int>(attackStr);
		Log(LOG_INFO) << ". . . attackStr Success @ " << percent;
		if (percent > 0
			&& RNG::percent(percent))
		{
			Log(LOG_INFO) << ". . Success";

			action->actor->addPsiExp();
			action->actor->addPsiExp();
			if (action->type == BA_PANIC)
			{
				Log(LOG_INFO) << ". . . action->type == BA_PANIC";

//kL				int moraleLoss = (110 - _save->getTile(action->target)->getUnit()->getStats()->bravery);
				int moraleLoss = (110 - victim->getStats()->bravery);		// kL
				if (moraleLoss > 0)
					_save->getTile(action->target)->getUnit()->moraleChange(-moraleLoss);
			}
			else //if (action->type == BA_MINDCONTROL)
			{
				Log(LOG_INFO) << ". . . action->type == BA_MINDCONTROL";

				victim->convertToFaction(action->actor->getFaction());
				calculateFOV(victim->getPosition());
				calculateUnitLighting();
				victim->setTimeUnits(victim->getStats()->tu);
				victim->allowReselect();
				victim->abortTurn(); // resets unit status to STANDING

				// if all units from either faction are mind controlled - auto-end the mission.
				if (_save->getSide() == FACTION_PLAYER
					&& Options::getBool("battleAutoEnd")
					&& Options::getBool("allowPsionicCapture"))
				{
					//Log(LOG_INFO) << ". . . . inside tallyUnits codeblock";

					int liveAliens = 0;
					int liveSoldiers = 0;

					_save->getBattleGame()->tallyUnits(liveAliens, liveSoldiers, false);

					if (liveAliens == 0 || liveSoldiers == 0)
					{
						_save->setSelectedUnit(0);
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
 * @param t Tile.
 * @return Tile where the items end up in eventually.
 */
Tile* TileEngine::applyGravity(Tile* t)
{
	if (!t) return 0;				// skip this if there is no tile.

	if (t->getInventory()->empty()	// skip this if there are no items;
		&& !t->getUnit())			// skip this if there is no unit in the tile. huh
	{
		return t;
	}


	Position p = t->getPosition();
	Tile* rt = t;
	Tile* rtb;
	BattleUnit* occupant = t->getUnit();

	if (occupant
		&& (occupant->getArmor()->getMovementType() != MT_FLY
			|| occupant->isOut()))
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
					rt = _save->getTile(Position(unitPos.x + x, unitPos.y + y, unitPos.z));
					rtb = _save->getTile(Position(unitPos.x + x, unitPos.y + y, unitPos.z - 1)); // below
					if (!rt->hasNoFloor(rtb))
					{
						canFall = false;
					}
				}
			}

			if (!canFall) break;


			unitPos.z--;
		}

		if (unitPos != occupant->getPosition())
		{
			if (occupant->getHealth() != 0
				&& occupant->getStunlevel() < occupant->getHealth())
			{
				occupant->startWalking(
						Pathfinding::DIR_DOWN,
						occupant->getPosition() + Position(0, 0, -1),
						_save->getTile(occupant->getPosition() + Position(0, 0, -1)),
						true);

				//Log(LOG_INFO) << "TileEngine::applyGravity(), addFallingUnit() ID " << occupant->getId();
				_save->addFallingUnit(occupant);
			}
			else
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
	while (p.z >= 0 && canFall)
	{
		rt = _save->getTile(p);
		rtb = _save->getTile(Position(p.x, p.y, p.z - 1)); // below
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
		{
			rt->addItem(*it, (*it)->getSlot());
		}
	}

	if (t != rt)
	{
		// clear tile
		t->getInventory()->clear();
	}

	return rt;
}

/**
 * Validates the melee range between two units.
 * @param attacker The attacking unit.
 * @param target The unit we want to attack.
 * @param dir Direction to check.
 * @return True when the range is valid.
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
			target);
}

/**
 * Validates the melee range between a tile and a unit.
 * @param pos, Position to check from.
 * @param direction, Direction to check.
 * @param size, For large units, we have to do extra checks.
 * @param target, The unit we want to attack, 0 for any unit.
 * @return, True when the range is valid.
 */
bool TileEngine::validMeleeRange(
		Position pos,
		int direction,
		BattleUnit* attacker,
		BattleUnit* target)
{
	//Log(LOG_INFO) << "TileEngine::validMeleeRange()";
	if (direction < 0 || direction > 7)
		return false;

	Position p;
	Pathfinding::directionToVector(direction, &p);

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
//kL			Tile* tileOrigin (_save->getTile(Position(pos + Position(x, y, 0))));
//kL			Tile* tileTarget (_save->getTile(Position(pos + Position(x, y, 0) + p)));
//kL			Tile* tileTargetAbove (_save->getTile(Position(pos + Position(x, y, 1) + p)));
			Tile* tileOrigin		= _save->getTile(Position(pos + Position(x, y, 0)));		// kL
			Tile* tileTarget		= _save->getTile(Position(pos + Position(x, y, 0) + p));	// kL
			Tile* tileTargetAbove	= _save->getTile(Position(pos + Position(x, y, 1) + p));	// kL

			if (tileOrigin && tileTarget)
			{
				if (tileOrigin->getTerrainLevel() <= -16
					&& tileTargetAbove
					&& !tileTargetAbove->hasNoFloor(tileTarget))
				{
					tileTarget = tileTargetAbove;
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
										attacker->getHeight() + attacker->getFloatHeight() - 4 - tileOrigin->getTerrainLevel());

						Position voxelTarget;
						if (canTargetUnit(&voxelOrigin, tileTarget, &voxelTarget, attacker))
						{
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
int TileEngine::faceWindow(const Position &position)
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
 * @return direction.
 */
int TileEngine::getDirectionTo(const Position& origin, const Position& target) const
{
	if (origin == target) return 0;	// kL. safety

	double offset_x = target.x - origin.x;
	double offset_y = target.y - origin.y;

	// kL_note: atan2() usually takes the y-value first;
	// and that's why things may seem so fucked up.
//kL	double theta = atan2(offset_x, -offset_y);
	double theta = atan2(-offset_y, offset_x); // radians: + = y > 0; - = y < 0;

	// divide the pie in 4 thetas, each at 1/8th before each quarter
//	double m_pi_8 = M_PI_4 / 2.0;
	double m_pi_8 = M_PI / 8.0;			// a circle divided into 16 sections (rads) -> 22.5 deg
	double d = 0.1;						// kL, a bias toward cardinal directions.
	double pie[4] =
	{
//kL		(M_PI_4 * 4.0) - m_pi_8,	// 2.7488935718910690836548129603696
		M_PI - m_pi_8 - d,					// 2.7488935718910690836548129603696	-> 157.5 deg
//kL		(M_PI_4 * 3.0) - m_pi_8,	// 1.9634954084936207740391521145497
		(M_PI * 3.0 / 4.0) - m_pi_8 + d,	// 1.9634954084936207740391521145497	-> 112.5 deg
//kL		(M_PI_4 * 2.0) - m_pi_8,	// 1.1780972450961724644234912687298
		M_PI_2 - m_pi_8 - d,				// 1.1780972450961724644234912687298	-> 67.5 deg
//kL		(M_PI_4 * 1.0) - m_pi_8		// 0.39269908169872415480783042290994
		m_pi_8 + d							// 0.39269908169872415480783042290994	-> 22.5 deg
	};

//kL	int dir = 0;
	int dir = 2;
	if (theta > pie[0] || theta < -pie[0])
	{
//kL		dir = 4;
		dir = 6;
	}
	else if (theta > pie[1])
	{
//kL		dir = 3;
		dir = 7;
	}
	else if (theta > pie[2])
	{
//kL		dir = 2;
		dir = 0;
	}
	else if (theta > pie[3])
	{
//kL		dir = 1;
		dir = 1;
	}
	else if (theta < -pie[1])
	{
//kL		dir = 5;
		dir = 5;
	}
	else if (theta < -pie[2])
	{
//kL		dir = 6;
		dir = 4;
	}
	else if (theta < -pie[3])
	{
//kL		dir = 7;
		dir = 3;
	}
//	else //if (theta < pie[0])
//	{
//		dir = 0;
//	}

	return dir;
}

/**
 *
 */
Position TileEngine::getOriginVoxel(BattleAction& action, Tile* tile)
{
	const int dirYshift[24] = {1,  3,  9, 15, 15, 13, 7, 1, 1,  1,  7, 13, 15, 15, 9, 3, 1,  2,  8, 14, 15, 14, 8, 2};
	const int dirXshift[24] = {9, 15, 15, 13,  8,  1, 1, 3, 7, 13, 15, 15,  9,  3, 1, 1, 8, 14, 15, 14,  8,  2, 1, 2};

	if (!tile)
	{
		tile = action.actor->getTile();
	}

	Position origin = tile->getPosition();
	Position originVoxel = Position(origin.x * 16, origin.y * 16, origin.z * 24);

	Tile* tileAbove = _save->getTile(origin + Position(0, 0, 1));

	// take into account soldier height and terrain level if the projectile is launched from a soldier
	if (action.actor->getPosition() == origin
		|| action.type != BA_LAUNCH)
	{
		// calculate vertical offset of the starting point of the projectile
		originVoxel.z += action.actor->getHeight() + action.actor->getFloatHeight();
		originVoxel.z -= tile->getTerrainLevel();
		originVoxel.z -= 4; // for good luck.
		
/*kL		if (action.type == BA_THROW)
		{
			originVoxel.z -= 3;
		}
		else
		{
			originVoxel.z -= 4;
		} */

		if (originVoxel.z >= (origin.z + 1) * 24)
		{
			if (tileAbove
				&& tileAbove->hasNoFloor(0))
			{
				origin.z++;
			}
			else
			{
				while (originVoxel.z >= (origin.z + 1) * 24)
				{
					originVoxel.z--;
				}

				originVoxel.z -= 4;
			}
		}

		int offset = 0;
		if (action.actor->getArmor()->getSize() > 1)
		{
			offset = 16;
		}
		else if (action.weapon == action.weapon->getOwner()->getItem("STR_LEFT_HAND")
			&& !action.weapon->getRules()->isTwoHanded())
		{
			offset = 8;
		}

		int direction = getDirectionTo(origin, action.target);
		originVoxel.x += dirXshift[direction + offset] * action.actor->getArmor()->getSize();
		originVoxel.y += dirYshift[direction + offset] * action.actor->getArmor()->getSize();
	}
	else // action.type == BA_LAUNCH
	{
		// don't take into account soldier height and terrain level if the
		// projectile is not launched from a soldier (ie. from a waypoint)
		originVoxel.x += 8;
		originVoxel.y += 8;
		originVoxel.z += 12;
	}

	return originVoxel;
}

}
