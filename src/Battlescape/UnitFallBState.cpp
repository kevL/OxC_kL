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

#include "UnitFallBState.h"

#include "BattleAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "ExplosionBState.h"
#include "Map.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up an UnitFallBState.
 * @param parent Pointer to the Battlescape.
 */
UnitFallBState::UnitFallBState(BattlescapeGame* parent)
	:
		BattleState(parent),
		_terrain(0),
		_unitsToMove(0),
		_tilesToFallInto(0)
{
}

/**
 * Deletes the UnitWalkBState.
 */
UnitFallBState::~UnitFallBState()
{
}

/**
 * Initializes the state.
 */
void UnitFallBState::init()
{
	_terrain = _parent->getTileEngine();

	if (_parent->getSave()->getSide() == FACTION_PLAYER)
		_parent->setStateInterval(Options::getInt("battleXcomSpeed"));
	else
		_parent->setStateInterval(Options::getInt("battleAlienSpeed"));
}

/**
 * Runs state functionality every cycle.
 * Progresses the fall, updates the battlescape, ...
 */
void UnitFallBState::think()
{
	Log(LOG_INFO) << "UnitFallBState::think()";

	for (std::list<BattleUnit*>::iterator
			unit = _parent->getSave()->getFallingUnits()->begin();
			unit != _parent->getSave()->getFallingUnits()->end();
			)
	{
		Log(LOG_INFO) << ". falling ID = " << (*unit)->getId();

		if ((*unit)->isOut(true, true)) // kL
//kL		if ((*unit)->getHealth() == 0
//kL			|| (*unit)->getStunlevel() >= (*unit)->getHealth())
		{
			Log(LOG_INFO) << ". dead OR stunned, Erase & cont";
			unit = _parent->getSave()->getFallingUnits()->erase(unit);

			continue;
		}

		if ((*unit)->getStatus() == STATUS_TURNING)
		{
			Log(LOG_INFO) << ". STATUS_TURNING, abortTurn()";
//kL			(*unit)->abortTurn();
			(*unit)->setStatus(STATUS_STANDING); // kL
		}

		bool
			fallCheck = true,
			falling = true;
		int size = (*unit)->getArmor()->getSize() - 1;

		bool onScreen = (*unit)->getVisible()
						&& _parent->getMap()->getCamera()->isOnScreen((*unit)->getPosition());


		Tile* tBelow = 0;

		for (int
				x = size; // units = 0; large = 1
				x > -1;
				x--)
		{
			for (int
					y = size; // units = 0; large = 1
					y > -1;
					y--)
			{
				tBelow = _parent->getSave()->getTile(
												(*unit)->getPosition()
												+ Position(x, y, -1));
				if (!_parent->getSave()->getTile(
											(*unit)->getPosition()
											+ Position(x, y, 0))
										->hasNoFloor(tBelow)
					|| (*unit)->getArmor()->getMovementType() == MT_FLY)
				{
					Log(LOG_INFO) << ". . fallCheck set FALSE";
					fallCheck = false;
				}
			}
		}

		tBelow = _parent->getSave()->getTile(
										(*unit)->getPosition()
										+ Position(0, 0, -1));

		falling = fallCheck
					&& (*unit)->getPosition().z != 0
					&& (*unit)->getTile()->hasNoFloor(tBelow)
//kL					&& (*unit)->getArmor()->getMovementType() != MT_FLY // done above in fallCheck
					&& (*unit)->getWalkingPhase() == 0;

		BattleUnit* uBelow = 0;

		if (falling)
		{
			for (int // tile(s) that unit is falling into:
//kL					x = (*unit)->getArmor()->getSize() - 1;
					x = size;
					x > -1;
					--x)
			{
				for (int
//kL						y = (*unit)->getArmor()->getSize() - 1;
						y = size;
						y > -1;
						--y)
				{
					tBelow = _parent->getSave()->getTile(
													(*unit)->getPosition()
													+ Position(x, y, -1));
					_tilesToFallInto.push_back(tBelow);
				}
			}

			// Check each tile for units that need moving out of the way.
			for (std::vector<Tile*>::iterator
					i = _tilesToFallInto.begin();
					i < _tilesToFallInto.end();
					++i)
			{
				uBelow = (*i)->getUnit();
				if (uBelow
					&& (*unit) != uBelow	// falling units do not fall on themselves
					&& !(std::find(			// already added
								_unitsToMove.begin(),
								_unitsToMove.end(),
								uBelow)
							!= _unitsToMove.end()))
				{
					Log(LOG_INFO) << ". . . Move, ID " << uBelow->getId();
					_unitsToMove.push_back(uBelow);
				}
			}
		}

		if ((*unit)->getStatus() == STATUS_WALKING
			|| (*unit)->getStatus() == STATUS_FLYING)
		{
			Log(LOG_INFO) << ". . call keepWalking()";

			(*unit)->keepWalking(tBelow, true);		// advances the phase
			_parent->getMap()->cacheUnit(*unit);	// make sure the unit sprites are up to date
		}											// kL_note: might need set cache invalid...

		falling = fallCheck
					&& (*unit)->getPosition().z != 0
					&& (*unit)->getTile()->hasNoFloor(tBelow)
//					&& (*unit)->getArmor()->getMovementType() != MT_FLY // done above in fallCheck
					&& (*unit)->getWalkingPhase() == 0;

		Log(LOG_INFO) << ". new fallCheck = " << fallCheck;


		// The unit has moved from one tile to the other.
		// kL_note: Can prob. use _tileSwitchDone around here...
		if ((*unit)->getPosition() != (*unit)->getLastPosition())
		{
			// Reset tiles moved from
			for (int
					x = size;
					x > -1;
					x--)
			{
				for (int
						y = size;
						y > -1;
						y--)
				{
					// A falling unit might have already taken up this position so check that this unit is still here.
					if (_parent->getSave()->getTile(
												(*unit)->getLastPosition()
												+ Position(x, y, 0))->getUnit()
											== *unit)
					{
						Log(LOG_INFO) << ". Tile is not occupied";
						_parent->getSave()->getTile(
												(*unit)->getLastPosition()
												+ Position(x, y, 0))->setUnit(0);
					}
				}
			}

			// Update tiles moved to.
			for (int
					x = size;
					x > -1;
					x--)
			{
				for (int
						y = size;
						y > -1;
						y--)
				{
					Log(LOG_INFO) << ". setUnit to belowTile";
					_parent->getSave()->getTile(
											(*unit)->getPosition()
											+ Position(x, y, 0))->setUnit(
																		*unit,
																		_parent->getSave()->getTile(
																								(*unit)->getPosition()
																								+ Position(x, y, -1)));
				}
			}

			// Find somewhere to move the unit(s) in danger of being squashed.
			if (!_unitsToMove.empty())
			{
				Log(LOG_INFO) << ". unitsToMove not empty";

				std::vector<Tile*> escapeTiles;
				for (std::vector<BattleUnit*>::iterator
						u = _unitsToMove.begin();
						u < _unitsToMove.end();
						)
				{
					Log(LOG_INFO) << ". moving unit ID " << (*u)->getId();

					uBelow = *u;
					bool escape = false;

					// We need to move all sections of the unit out of the way.
					int uSize = uBelow->getArmor()->getSize() - 1;
					std::vector<Position> bodySections;
					for (int
							x = uSize;
							x > -1;
							--x)
					{
						for (int
								y = uSize;
								y > -1;
								--y)
						{
							Log(LOG_INFO) << ". body size + 1";
							Position pBody = uBelow->getPosition() + Position(x, y, 0);
							bodySections.push_back(pBody);
						}
					}

					for (int // Check in each compass direction.
							dir = 0;
							dir < Pathfinding::DIR_UP
								&& !escape;
							dir++)
					{
						Log(LOG_INFO) << ". . checking directions to move";

						Position offset;
						Pathfinding::directionToVector(dir, &offset);

						for (std::vector<Position>::iterator
								body = bodySections.begin();
								body < bodySections.end();
								)
						{
							Log(LOG_INFO) << ". . . checking bodysections";

							Position originalPosition = *body;
							Tile* t = _parent->getSave()->getTile(originalPosition + offset);
							Tile* tCurrent = _parent->getSave()->getTile(originalPosition);
							Tile* tBelow2 = _parent->getSave()->getTile(originalPosition + offset + Position(0, 0, -1));

							bool aboutToBeOccupiedFromAbove = t
																&& std::find(
																			_tilesToFallInto.begin(),
																			_tilesToFallInto.end(),
																			t)
																		!= _tilesToFallInto.end();
							bool alreadyTaken = t
												&& std::find(
															escapeTiles.begin(),
															escapeTiles.end(),
															t)
														!= escapeTiles.end();
							bool alreadyOccupied = t
													&& t->getUnit()
													&& t->getUnit() != uBelow;
							bool hasFloor = t
											&& !t->hasNoFloor(tBelow2);
							bool movementBlocked = _parent->getSave()->getPathfinding()->isBlocked(
																								tCurrent,
																								t,
																								dir,
																								uBelow);
							bool unitCanFly = uBelow->getArmor()->getMovementType() == MT_FLY;
							bool canMoveToTile = t
												&& !alreadyOccupied
												&& !alreadyTaken
												&& !aboutToBeOccupiedFromAbove
												&& !movementBlocked
												&& (hasFloor || unitCanFly);
							if (canMoveToTile)
								++body; // Check next section of the unit.
							else
								break; // Try next direction.

							// If all sections of the unit-fallen-onto can be moved, then move it.
							if (body == bodySections.end())
							{
								Log(LOG_INFO) << ". . . . move unit";

								if (_parent->getSave()->addFallingUnit(uBelow))
								{
									Log(LOG_INFO) << ". . . . . add Falling Unit";

									escape = true;
									// Now ensure no other unit escapes to here too.
									for (int
											x = uSize;
											x > -1;
											--x)
									{
										for (int
												y = uSize;
												y > -1;
												--y)
										{
											Log(LOG_INFO) << ". . . . . . check for more escape units?";

											Tile* tEscape = _parent->getSave()->getTile(t->getPosition() + Position(x, y, 0));
											escapeTiles.push_back(tEscape);
										}
									}

									Log(LOG_INFO) << ". . . . startWalking() out of the way?";

									Tile* tBelow3 = _parent->getSave()->getTile(originalPosition + Position(0, 0, -1));
									uBelow->startWalking(
													dir,
													uBelow->getPosition() + offset,
													tBelow3,
													onScreen);

									u = _unitsToMove.erase(u);
								}
							}
						}
					}

					if (!escape)
					{
						Log(LOG_INFO) << ". . . NOT escape";

						uBelow->knockOut(_parent);
						u = _unitsToMove.erase(u);
					}
				}

				Log(LOG_INFO) << ". . checkForCasualties()";
				_parent->checkForCasualties(0, *unit);
			}
		}

		if ((*unit)->getStatus() == STATUS_STANDING) // just standing around, done falling.
		{
			Log(LOG_INFO) << ". STATUS_STANDING";

			if (falling)
			{
				Log(LOG_INFO) << ". . falling (again?) -> startWalking()";

				Position destination = (*unit)->getPosition() + Position(0, 0, -1);

				tBelow = _parent->getSave()->getTile(destination);
				(*unit)->startWalking(
									Pathfinding::DIR_DOWN,
									destination,
									tBelow,
									onScreen);

				(*unit)->setCache(0);
				_parent->getMap()->cacheUnit(*unit);

				++unit;
			}
			else
			{
				Log(LOG_INFO) << ". . burnFloors, checkProxies, Erase.unit";

				if ((*unit)->getSpecialAbility() == SPECAB_BURNFLOOR) // if the unit burns floortiles, burn floortiles
				{
					// kL_add: Put burnedBySilacoid() here! etc
					(*unit)->getTile()->ignite(1);
					Position here = ((*unit)->getPosition() * Position(
																	16,
																	16,
																	24))
									+ Position(
												8,
												8,
												-(*unit)->getTile()->getTerrainLevel());
					_parent->getTileEngine()->hit(
												here,
												(*unit)->getStats()->strength,
												DT_IN,
												*unit);
				}

				_terrain->calculateUnitLighting(); // move personal lighting

				_parent->getMap()->cacheUnit(*unit);
				(*unit)->setCache(0);

				_terrain->calculateFOV(*unit);

				_parent->checkForProximityGrenades(*unit);
				// kL_add: Put checkForSilacoid() here!

				if (_parent->getTileEngine()->checkReactionFire(*unit))
					_parent->getPathfinding()->abortPath();

				unit = _parent->getSave()->getFallingUnits()->erase(unit);
			}
		}
		else
		{
			Log(LOG_INFO) << ". not STATUS_STANDING, next unit";

			++unit;
		}
	}


	Log(LOG_INFO) << ". done main recursion";

	if (_parent->getSave()->getFallingUnits()->empty())
	{
		Log(LOG_INFO) << ". Falling units EMPTY";

		_tilesToFallInto.clear();
		_unitsToMove.clear();

		_parent->popState();

		return;
	}

	Log(LOG_INFO) << "UnitFallBState::think() EXIT";
}

}
