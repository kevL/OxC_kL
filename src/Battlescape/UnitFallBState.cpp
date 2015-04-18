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

#include "UnitFallBState.h"

#include "BattleAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
//#include "ExplosionBState.h"
#include "Map.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up an UnitFallBState.
 * @param parent - pointer to the BattlescapeGame
 */
UnitFallBState::UnitFallBState(BattlescapeGame* parent)
	:
		BattleState(parent),
		_terrain(NULL),
		_unitsToMove(),
		_tilesToFallInto()
{}

/**
 * Deletes the UnitWalkBState.
 */
UnitFallBState::~UnitFallBState()
{}

/**
 * Initializes the state.
 */
void UnitFallBState::init()
{
	_terrain = _parent->getTileEngine();

	if (_parent->getSave()->getSide() == FACTION_PLAYER)
		_parent->setStateInterval(Options::battleXcomSpeed);
	else
		_parent->setStateInterval(Options::battleAlienSpeed);
}

/**
 * Runs state functionality every cycle.
 * Progresses the fall, updates the battlescape, ...
 */
void UnitFallBState::think()
{
	//Log(LOG_INFO) << "UnitFallBState::think()";
	for (std::list<BattleUnit*>::const_iterator
			unitFalling = _parent->getSave()->getFallingUnits()->begin();
			unitFalling != _parent->getSave()->getFallingUnits()->end();
			)
	{
		//Log(LOG_INFO) << ". falling ID = " << (*unitFalling)->getId();
		if ((*unitFalling)->isOut(true, true) == true)
//		if ((*unitFalling)->getHealth() == 0
//			|| (*unitFalling)->getStun() >= (*unitFalling)->getHealth())
		{
			//Log(LOG_INFO) << ". dead OR stunned, Erase & cont";
			unitFalling = _parent->getSave()->getFallingUnits()->erase(unitFalling);
			continue;
		}

		if ((*unitFalling)->getStatus() == STATUS_TURNING)
		{
			//Log(LOG_INFO) << ". STATUS_TURNING, abortTurn()";
			(*unitFalling)->setStatus(STATUS_STANDING);
		}

		bool
			fallCheck = true,
			falling = true;
//			onScreen = (*unitFalling)->getUnitVisible()
//					&& _parent->getMap()->getCamera()->isOnScreen((*unitFalling)->getPosition());
//		bool onScreen = ((*unitFalling)->getVisible() && _parent->getMap()->getCamera()->isOnScreen((*unitFalling)->getPosition(), true, size, false));

		BattleUnit* unitBelow;
		Tile* tileBelow;

		const int unitSize = (*unitFalling)->getArmor()->getSize() - 1;

		for (int
				x = unitSize; // units = 0; large = 1
				x != -1;
				--x)
		{
			for (int
					y = unitSize; // units = 0; large = 1
					y != -1;
					--y)
			{
				tileBelow = _parent->getSave()->getTile((*unitFalling)->getPosition() + Position(x,y,-1));
				if (_parent->getSave()->getTile((*unitFalling)->getPosition() + Position(x,y,0))
												->hasNoFloor(tileBelow) == false
					|| (*unitFalling)->getMovementType() == MT_FLY)
				{
					//Log(LOG_INFO) << ". . fallCheck set FALSE";
					fallCheck = false;
				}
			}
		}

		tileBelow = _parent->getSave()->getTile((*unitFalling)->getPosition() + Position(0,0,-1));

		falling = fallCheck
			   && (*unitFalling)->getPosition().z != 0
			   && (*unitFalling)->getTile()->hasNoFloor(tileBelow)
//			   && (*unitFalling)->getMovementType() != MT_FLY // done above in fallCheck
			   && (*unitFalling)->getWalkingPhase() == 0;

		if (falling == true)
		{
			for (int // tile(s) that unit is falling into:
					x = unitSize;
					x != -1;
					--x)
			{
				for (int
						y = unitSize;
						y != -1;
						--y)
				{
					tileBelow = _parent->getSave()->getTile((*unitFalling)->getPosition() + Position(x,y,-1));
					_tilesToFallInto.push_back(tileBelow);
				}
			}

			// Check each tile for units that need moving out of the way.
			for (std::vector<Tile*>::const_iterator
					i = _tilesToFallInto.begin();
					i != _tilesToFallInto.end();
					++i)
			{
				unitBelow = (*i)->getUnit();
				if (unitBelow != NULL
					&& *unitFalling != unitBelow	// falling units do not fall on themselves
					&& std::find(					// already added
							_unitsToMove.begin(),
							_unitsToMove.end(),
							unitBelow) == _unitsToMove.end())
				{
					//Log(LOG_INFO) << ". . . Move, ID " << unitBelow->getId();
					_unitsToMove.push_back(unitBelow);
				}
			}
		}

		if ((*unitFalling)->getStatus() == STATUS_WALKING
			|| (*unitFalling)->getStatus() == STATUS_FLYING)
		{
			//Log(LOG_INFO) << ". . call keepWalking()";
			(*unitFalling)->keepWalking(tileBelow, true);	// advances the phase
			_parent->getMap()->cacheUnit(*unitFalling);		// make sure the unitFalling sprites are up to date
		}													// kL_note: might need set cache invalid...

		falling = fallCheck
					&& (*unitFalling)->getPosition().z != 0
					&& (*unitFalling)->getTile()->hasNoFloor(tileBelow)
//					&& (*unitFalling)->getMovementType() != MT_FLY // done above in fallCheck
					&& (*unitFalling)->getWalkingPhase() == 0;

		//Log(LOG_INFO) << ". new fallCheck = " << fallCheck;

		// The unit has moved from one tile to the other.
		// kL_note: Can prob. use _tileSwitchDone around here...
		if ((*unitFalling)->getPosition() != (*unitFalling)->getLastPosition())
		{
			for (int // reset tiles moved from
					x = unitSize;
					x != -1;
					--x)
			{
				for (int
						y = unitSize;
						y != -1;
						--y)
				{
					// Another falling unit might have already taken up this position so check that that unit is still there.
					if (_parent->getSave()->getTile((*unitFalling)->getLastPosition() + Position(x,y,0))->getUnit() == *unitFalling)
					{
						//Log(LOG_INFO) << ". Tile is not occupied";
						_parent->getSave()->getTile((*unitFalling)->getLastPosition() + Position(x,y,0))->setUnit(NULL);
					}
				}
			}

			for (int // update tiles moved to.
					x = unitSize;
					x != -1;
					--x)
			{
				for (int
						y = unitSize;
						y != -1;
						--y)
				{
					//Log(LOG_INFO) << ". setUnit to belowTile";
					_parent->getSave()->getTile((*unitFalling)->getPosition() + Position(x,y,0))
										->setUnit(
												*unitFalling,
												_parent->getSave()->getTile((*unitFalling)->getPosition() + Position(x,y,-1)));
				}
			}

			// Find somewhere to move the unit(s) in danger of being squashed.
			if (_unitsToMove.empty() == false)
			{
				//Log(LOG_INFO) << ". unitsToMove not empty";
				std::vector<Tile*> escapeTiles;

				for (std::vector<BattleUnit*>::const_iterator
						unitToMove = _unitsToMove.begin();
						unitToMove != _unitsToMove.end();
						)
				{
					//Log(LOG_INFO) << ". moving unit ID " << (*unitToMove)->getId();
					unitBelow = *unitToMove;
					bool escape = false;

					// need to move all sections of unitBelow out of the way.
					const int belowSize = unitBelow->getArmor()->getSize() - 1;
					std::vector<Position> bodyPositions;
					for (int
							x = belowSize;
							x != -1;
							--x)
					{
						for (int
								y = belowSize;
								y != -1;
								--y)
						{
							//Log(LOG_INFO) << ". body size + 1";
							bodyPositions.push_back(unitBelow->getPosition() + Position(x,y,0));
						}
					}

					for (int // Check in each compass direction.
							dir = 0;
							dir != Pathfinding::DIR_UP
								&& escape == false;
							++dir)
					{
						//Log(LOG_INFO) << ". . checking directions to move";
						Position offset;
						Pathfinding::directionToVector(
													dir,
													&offset);

						for (std::vector<Position>::const_iterator
								posBody = bodyPositions.begin();
								posBody != bodyPositions.end();
								)
						{
							//Log(LOG_INFO) << ". . . checking bodysections";
							const Position posOrigin = *posBody;
							Tile
								* const tile = _parent->getSave()->getTile(posOrigin + offset),
								* const tileBelow2 = _parent->getSave()->getTile(posOrigin + offset + Position(0,0,-1));

							bool
								aboutToBeOccupiedFromAbove = tile
														  && std::find(
																	_tilesToFallInto.begin(),
																	_tilesToFallInto.end(),
																	tile) != _tilesToFallInto.end(),
								alreadyTaken = tile
											&& std::find(
													escapeTiles.begin(),
													escapeTiles.end(),
													tile) != escapeTiles.end(),
								alreadyOccupied = tile
											   && tile->getUnit()
											   && tile->getUnit() != unitBelow,
								hasFloor = tile
										&& tile->hasNoFloor(tileBelow2) == false,
								movementBlocked = _parent->getSave()->getPathfinding()->isBlocked(
																								_parent->getSave()->getTile(posOrigin),
																								tile,
																								dir,
																								unitBelow),
								unitCanFly = unitBelow->getMovementType() == MT_FLY,
								canMoveToTile = tile
											 && alreadyOccupied == false
											 && alreadyTaken == false
											 && aboutToBeOccupiedFromAbove == false
											 && movementBlocked == false
											 && (hasFloor == true
												|| unitCanFly == true);

							if (canMoveToTile == true)
								++posBody; // Check next section of the unit.
							else
								break; // Try next direction.

							// If all sections of the unit-fallen-onto can be moved, then move it.
							if (posBody == bodyPositions.end())
							{
								//Log(LOG_INFO) << ". . . . move unit";
								if (_parent->getSave()->addFallingUnit(unitBelow))
								{
									//Log(LOG_INFO) << ". . . . . add Falling Unit";
									escape = true;

									// Now ensure no other unit escapes to here too.
									for (int
											x = belowSize;
											x != -1;
											--x)
									{
										for (int
												y = belowSize;
												y != -1;
												--y)
										{
											//Log(LOG_INFO) << ". . . . . . check for more escape units?";
											escapeTiles.push_back(_parent->getSave()->getTile(tile->getPosition() + Position(x,y,0)));
										}
									}

									//Log(LOG_INFO) << ". . . . startWalking() out of the way?";
									unitBelow->startWalking(
													dir,
													unitBelow->getPosition() + offset,
													_parent->getSave()->getTile(posOrigin + Position(0,0,-1)));
//													onScreen);

									unitToMove = _unitsToMove.erase(unitToMove);
								}
							}
						}
					}

					if (escape == false)
					{
						//Log(LOG_INFO) << ". . . NOT escape";
						unitBelow->knockOut(_parent);
						unitToMove = _unitsToMove.erase(unitToMove);
					}
				}

				//Log(LOG_INFO) << ". . checkForCasualties()";
				_parent->checkForCasualties(NULL, *unitFalling);
			}
		}

		if ((*unitFalling)->getStatus() == STATUS_STANDING) // done falling, just standing around.
		{
			//Log(LOG_INFO) << ". STATUS_STANDING";
			if (falling == true)
			{
				//Log(LOG_INFO) << ". . falling (again?) -> startWalking()";
				Position destination = (*unitFalling)->getPosition() + Position(0,0,-1);

				tileBelow = _parent->getSave()->getTile(destination);
				(*unitFalling)->startWalking(
									Pathfinding::DIR_DOWN,
									destination,
									tileBelow);
//									onScreen);

				(*unitFalling)->setCache(NULL);
				_parent->getMap()->cacheUnit(*unitFalling);

				++unitFalling;
			}
			else
			{
				//Log(LOG_INFO) << ". . burnFloors, checkProxies, Erase.unitFalling";
				if ((*unitFalling)->getSpecialAbility() == SPECAB_BURNFLOOR // if the unit burns floortiles, burn floortiles
					|| (*unitFalling)->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE)
				{
					// kL_add: Put burnedBySilacoid() here! etc
					(*unitFalling)->getTile()->ignite(1, true);
					Position pos = ((*unitFalling)->getPosition() * Position(16,16,24))
									+ Position(
											8,8,
											-(*unitFalling)->getTile()->getTerrainLevel());
					_parent->getTileEngine()->hit(
												pos,
												(*unitFalling)->getBaseStats()->strength, // * (*unitFalling)->getAccuracyModifier(),
												DT_IN,
												*unitFalling);
				}

				_terrain->calculateUnitLighting(); // move personal lighting

				_parent->getMap()->cacheUnit(*unitFalling);
				(*unitFalling)->setCache(NULL);

				_terrain->calculateFOV(*unitFalling);

				_parent->checkForProximityGrenades(*unitFalling);
				// kL_add: Put checkForSilacoid() here!

				if (_parent->getTileEngine()->checkReactionFire(*unitFalling))
					_parent->getPathfinding()->abortPath();

				unitFalling = _parent->getSave()->getFallingUnits()->erase(unitFalling);
			}
		}
		else
		{
			//Log(LOG_INFO) << ". not STATUS_STANDING, next unit";
			++unitFalling;
		}
	}


	//Log(LOG_INFO) << ". done main recursion";

	if (_parent->getSave()->getFallingUnits()->empty() == true)
	{
		//Log(LOG_INFO) << ". Falling units EMPTY";
		_tilesToFallInto.clear();
		_unitsToMove.clear();

		_parent->popState();
		return;
	}
	//Log(LOG_INFO) << "UnitFallBState::think() EXIT";
}

}
