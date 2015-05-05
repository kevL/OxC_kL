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
		_terrain(parent->getTileEngine())
//		_terrain(NULL),
//		_unitsToMove(),
//		_tilesToFallInto()
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
//	_terrain = _parent->getTileEngine();

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
	BattleUnit* unitBelow;
	Tile
		* tile,
		* tileBelow;
	Position startPos;

	for (std::list<BattleUnit*>::const_iterator
			i = _parent->getSave()->getFallingUnits()->begin();
			i != _parent->getSave()->getFallingUnits()->end();
			)
	{
		//Log(LOG_INFO) << ". falling ID " << (*i)->getId();
		if ((*i)->isOut(true, true) == true)
//		if ((*i)->getHealth() == 0
//			|| (*i)->getStun() >= (*i)->getHealth())
		{
			//Log(LOG_INFO) << ". dead OR stunned, Erase & cont";
			i = _parent->getSave()->getFallingUnits()->erase(i);
			continue;
		}

		if ((*i)->getStatus() == STATUS_TURNING)
		{
			//Log(LOG_INFO) << ". STATUS_TURNING, abortTurn()";
			(*i)->setStatus(STATUS_STANDING);
		}

		bool
			fallCheck = true,
			falling = true;
//			onScreen = (*i)->getUnitVisible()
//					&& _parent->getMap()->getCamera()->isOnScreen((*i)->getPosition());
//		bool onScreen = ((*i)->getVisible() && _parent->getMap()->getCamera()->isOnScreen((*i)->getPosition(), true, size, false));

		const int unitSize = (*i)->getArmor()->getSize() - 1;
		for (int
				x = unitSize;
				x != -1;
				--x)
		{
			for (int
					y = unitSize;
					y != -1;
					--y)
			{
				tileBelow = _parent->getSave()->getTile((*i)->getPosition() + Position(x,y,-1));
				if (_parent->getSave()->getTile((*i)->getPosition() + Position(x,y,0))
												->hasNoFloor(tileBelow) == false
					|| (*i)->getMoveTypeUnit() == MT_FLY)
				{
					//Log(LOG_INFO) << ". . fallCheck set FALSE";
					fallCheck = false;
				}
			}
		}

		tileBelow = _parent->getSave()->getTile((*i)->getPosition() + Position(0,0,-1));

		falling = fallCheck
			   && (*i)->getPosition().z != 0
			   && (*i)->getTile()->hasNoFloor(tileBelow)
//			   && (*i)->getMoveTypeUnit() != MT_FLY // done above in fallCheck
			   && (*i)->getWalkingPhase() == 0;

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
					tileBelow = _parent->getSave()->getTile((*i)->getPosition() + Position(x,y,-1));
					_tilesToFallInto.push_back(tileBelow);
				}
			}

			// Check each tile for units that need moving out of the way.
			for (std::vector<Tile*>::const_iterator
					j = _tilesToFallInto.begin();
					j != _tilesToFallInto.end();
					++j)
			{
				unitBelow = (*j)->getUnit();
				if (unitBelow != NULL
					&& *i != unitBelow	// falling units do not fall on themselves
					&& std::find(
							_unitsToMove.begin(),
							_unitsToMove.end(),
							unitBelow) == _unitsToMove.end())
				{
					//Log(LOG_INFO) << ". . . Move, ID " << unitBelow->getId();
					_unitsToMove.push_back(unitBelow);
				}
			}
		}

		if ((*i)->getStatus() == STATUS_WALKING
			|| (*i)->getStatus() == STATUS_FLYING)
		{
			//Log(LOG_INFO) << ". . call keepWalking()";
			(*i)->keepWalking(tileBelow, true);	// advances the phase

			(*i)->setCache(NULL);				// kL
			_parent->getMap()->cacheUnit(*i);	// make sure the fallUnit sprites are up to date
		}

		falling = fallCheck
			   && (*i)->getPosition().z != 0
			   && (*i)->getTile()->hasNoFloor(tileBelow)
//			   && (*i)->getMovementType() != MT_FLY // done above in fallCheck
			   && (*i)->getWalkingPhase() == 0;

		//Log(LOG_INFO) << ". new fallCheck = " << fallCheck;

		// The unit has moved from one tile to the other.
		// kL_note: Can prob. use _tileSwitchDone around here...
		if ((*i)->getPosition() != (*i)->getLastPosition())
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
					if (*i == _parent->getSave()->getTile((*i)->getLastPosition() + Position(x,y,0))->getUnit())
					{
						//Log(LOG_INFO) << ". Tile is not occupied";
						_parent->getSave()->getTile((*i)->getLastPosition() + Position(x,y,0))->setUnit(NULL);
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
					_parent->getSave()->getTile((*i)->getPosition() + Position(x,y,0))
									->setUnit(
											*i,
											_parent->getSave()->getTile((*i)->getPosition() + Position(x,y,-1)));
				}
			}

			// Find somewhere to move the unit(s) in danger of being squashed.
			if (_unitsToMove.empty() == false)
			{
				//Log(LOG_INFO) << ". unitsToMove not empty";
				std::vector<Tile*> escapeTiles;

				for (std::vector<BattleUnit*>::const_iterator
						j = _unitsToMove.begin();
						j != _unitsToMove.end();
						)
				{
					//Log(LOG_INFO) << ". moving unit ID " << (*j)->getId();
					unitBelow = *j;
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
						Position unitVect;
						Pathfinding::directionToVector(
													dir,
													&unitVect);

						for (std::vector<Position>::const_iterator
								k = bodyPositions.begin();
								k != bodyPositions.end();
								)
						{
							//Log(LOG_INFO) << ". . . checking bodysections";
							startPos = *k;
							tile = _parent->getSave()->getTile(startPos + unitVect);
							tileBelow = _parent->getSave()->getTile(startPos + unitVect + Position(0,0,-1));

//							_parent->getSave()->getPathfinding()->setPathingUnit(unitBelow); // kL

							bool
								aboutToBeOccupiedFromAbove = tile != NULL
														  && std::find(
																	_tilesToFallInto.begin(),
																	_tilesToFallInto.end(),
																	tile) != _tilesToFallInto.end(),
								alreadyTaken = tile != NULL
											&& std::find(
													escapeTiles.begin(),
													escapeTiles.end(),
													tile) != escapeTiles.end(),
								alreadyOccupied = tile != NULL
											   && tile->getUnit() != NULL
											   && tile->getUnit() != unitBelow,
								hasFloor = tile != NULL
										&& tile->hasNoFloor(tileBelow) == false,
								blocked = _parent->getSave()->getPathfinding()->isBlockedPath(
																						_parent->getSave()->getTile(startPos),
//																						tile,
																						dir,
																						unitBelow),
								unitCanFly = unitBelow->getMoveTypeUnit() == MT_FLY,
								canMoveToTile = tile != NULL
											 && alreadyOccupied == false
											 && alreadyTaken == false
											 && aboutToBeOccupiedFromAbove == false
											 && blocked == false
											 && (hasFloor == true
												|| unitCanFly == true);

							if (canMoveToTile == true)
								++k; // Check next section of the unit.
							else
								break; // Try next direction.


							// If all sections of the unit-fallen-onto can be moved, then move it.
							if (k == bodyPositions.end())
							{
								//Log(LOG_INFO) << ". . . . move unit";
								if (_parent->getSave()->addFallingUnit(unitBelow) == true)
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
														unitBelow->getPosition() + unitVect,
														_parent->getSave()->getTile(startPos + Position(0,0,-1)));
//														onScreen);

									j = _unitsToMove.erase(j);
								}
							}
						}
					}

					if (escape == false)
					{
						//Log(LOG_INFO) << ". . . NOT escape";
						unitBelow->knockOut(_parent);
						j = _unitsToMove.erase(j);
					}
				}

				//Log(LOG_INFO) << ". . checkForCasualties()";
				_parent->checkForCasualties(NULL, *i);
			}
		}

		if ((*i)->getStatus() == STATUS_STANDING) // done falling, just standing around.
		{
			//Log(LOG_INFO) << ". STATUS_STANDING";
			if (falling == true)
			{
				//Log(LOG_INFO) << ". . still falling -> startWalking()";
				Position destination = (*i)->getPosition() + Position(0,0,-1);

				tileBelow = _parent->getSave()->getTile(destination);
				(*i)->startWalking(
									Pathfinding::DIR_DOWN,
									destination,
									tileBelow);
//									onScreen);

				(*i)->setCache(NULL);
				_parent->getMap()->cacheUnit(*i);

				++i;
			}
			else // done falling, just standing around ...
			{
				//Log(LOG_INFO) << ". . burnFloors, checkProxies, Erase.i";
				if ((*i)->getSpecialAbility() == SPECAB_BURNFLOOR // if the unit burns floortiles, burn floortiles
					|| (*i)->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE)
				{
					// kL_add: Put burnedBySilacoid() here! etc
					(*i)->getTile()->ignite(1);
					Position pos = ((*i)->getPosition() * Position(16,16,24))
									+ Position(
											8,8,
											-(*i)->getTile()->getTerrainLevel());
					_parent->getTileEngine()->hit(
												pos,
												(*i)->getBaseStats()->strength, // * (*i)->getAccuracyModifier(),
												DT_IN,
												*i);
				}

				_terrain->calculateUnitLighting(); // move personal lighting

				(*i)->setCache(NULL);
				_parent->getMap()->cacheUnit(*i);

//				_terrain->calculateFOV(*i);
				_terrain->calculateFOV((*i)->getPosition());

				_parent->checkForProximityGrenades(*i);
				// kL_add: Put checkForSilacoid() here!

				if (_parent->getTileEngine()->checkReactionFire(*i))
					_parent->getPathfinding()->abortPath();

				i = _parent->getSave()->getFallingUnits()->erase(i);
			}
		}
		else
		{
			//Log(LOG_INFO) << ". not STATUS_STANDING, next unit";
			++i;
		}
	}


	//Log(LOG_INFO) << ". done main recursion";
	if (_parent->getSave()->getFallingUnits()->empty() == true)
	{
		//Log(LOG_INFO) << ". Falling units EMPTY";
//		_tilesToFallInto.clear();
//		_unitsToMove.clear();

		_parent->popState();
//		return;
	}
	//Log(LOG_INFO) << "UnitFallBState::think() EXIT";
}

}
