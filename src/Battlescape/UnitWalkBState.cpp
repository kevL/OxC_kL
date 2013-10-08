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

#include "UnitWalkBState.h"
#include "ProjectileFlyBState.h"
#include "TileEngine.h"
#include "Pathfinding.h"
#include "BattlescapeState.h"
#include "Map.h"
#include "Camera.h"
#include "BattleAIState.h"
#include "ExplosionBState.h"
#include "../Engine/Game.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
//#include "../Savegame/Soldier.h"		// kL
#include "../Savegame/Tile.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Sound.h"
#include "../Engine/Options.h"
#include "../Ruleset/Armor.h"
#include "../Engine/Logger.h"
#include "UnitFallBState.h"


namespace OpenXcom
{

/**
 * Sets up an UnitWalkBState.
 */
UnitWalkBState::UnitWalkBState(BattlescapeGame* parent, BattleAction action)
	:
	BattleState(parent, action),
	_unit(0),
	_pf(0),
	_terrain(0),
	_falling(false),
	_beforeFirstStep(false),
	_unitsSpotted(0),
	_preMovementCost(0)
{
	Log(LOG_INFO) << "Create UnitWalkBState";
}

/**
 * Deletes the UnitWalkBState.
 */
UnitWalkBState::~UnitWalkBState()
{
	Log(LOG_INFO) << "Delete UnitWalkBState";
}

/**
 * Initializes the state.
 */
void UnitWalkBState::init()
{
	Log(LOG_INFO) << "UnitWalkBState::init()";

	_unit = _action.actor;
	_unitsSpotted = _unit->getUnitsSpottedThisTurn().size();

	setNormalWalkSpeed();
	_pf = _parent->getPathfinding();
	_terrain = _parent->getTileEngine();
	_target = _action.target;

	if (_parent->getSave()->getTraceSetting())
	{
		Log(LOG_INFO) << "Walking from: "
				<< _unit->getPosition().x << "," << _unit->getPosition().y << "," << _unit->getPosition().z
				<< " to " << _target.x << "," << _target.y << "," << _target.z;
	}

	int dir = _pf->getStartDirection();
	if (!_action.strafe
		&& dir != -1
		&& dir != _unit->getDirection())
	{
		// kL_note: if unit is not facing in the direction that it's about to walk toward...
		_beforeFirstStep = true;
	}

	Log(LOG_INFO) << "UnitWalkBState::init() EXIT";
}

/**
 * Runs state functionality every cycle.
 */
void UnitWalkBState::think()
{
	Log(LOG_INFO) << "UnitWalkBState::think() : " << _unit->getId() << " Phase = " << _unit->getWalkingPhase();	// kL

	if (_unit->isOut())
	{
		Log(LOG_INFO) << ". . unit isOut, abort.";	// kL

		_pf->abortPath();
		_parent->popState();

		return;
	}
	else
	{
		Log(LOG_INFO) << ". . unit !isOut, continue - health:" << _unit->getHealth();	// kL
	}


//	int visUnits = _unit->getVisibleUnits().size();		// kL
	bool newUnitSpotted = false;
	bool onScreen = _unit->getVisible()
			&& _parent->getMap()->getCamera()->isOnScreen(_unit->getPosition());

	int dir = _pf->getStartDirection();		// kL: also below, in STATUS_STANDING!
//	Log(LOG_INFO) << ". StartDirection = " << dir;	// kL

	if (_unit->isKneeled()
//		&& !_pf->validateUpDown(_unit, _unit->getPosition(), _pf->DIR_UP)		// kL
//		&& !_pf->validateUpDown(_unit, _unit->getPosition(), _pf->DIR_DOWN))	// kL
//		&& !_pf->DIR_UP			// kL
//		&& !_pf->DIR_DOWN)		// kL
		&& dir > -1 && dir < 8)	// kL: ie. *not* up or down
	{
//		Log(LOG_INFO) << ". kneeled, and path UpDown INVALID";	// kL

		if (_parent->kneel(_unit))
		{
//			Log(LOG_INFO) << ". . Stand up";	// kL

//kL			_unit->setCache(0);
			_terrain->calculateFOV(_unit);

			_unit->setCache(0);		// kL
			_parent->getMap()->cacheUnit(_unit);


			newUnitSpotted = !_action.desperate		// kL
				&& _parent->getPanicHandled()		// kL
//				&& _unitsSpotted != _unit->getUnitsSpottedThisTurn().size();	// kL
				&& _unitsSpotted < _unit->getUnitsSpottedThisTurn().size();		// kL
			if (newUnitSpotted)							// kL
			{
//				_unit->setCache(0);						// kL
//				_parent->getMap()->cacheUnit(_unit);	// kL
				_pf->abortPath();						// kL
				_parent->popState();					// kL
			}

			return;
		}
		else
		{
			_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			_pf->abortPath();
			_parent->popState();

			return;
		}
	}
//	else if (_unit->isKneeled())	// kL
//	{
//		Log(LOG_INFO) << ". kneeled, and path UpDown VALID";	// kL
//	}

//kL_below	Tile* tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0, 0, -1));

	if (_unit->getStatus() == STATUS_WALKING
		|| _unit->getStatus() == STATUS_FLYING)
	{
		Log(LOG_INFO) << "STATUS_WALKING or FLYING : " << _unit->getId();	// kL

		Tile* tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0, 0, -1));	// kL_above

		if (_parent->getSave()->getTile(_unit->getDestination())->getUnit() == 0  // next tile must be not occupied
			// kL_note: and, if not flying, the position directly below the tile must not be occupied...
			// Had that happen with a sectoid left standing in the air because a cyberdisc was 2 levels below it.
			|| _parent->getSave()->getTile(_unit->getDestination())->getUnit() == _unit)
		{
			playMovementSound();
			_unit->keepWalking(tileBelow, onScreen); // advances the phase
		}
		else if (!_falling)
		{
			Log(LOG_INFO) << "UnitWalkBState::think() 1";

			_unit->lookAt(_unit->getDestination(), _unit->getTurretType() != -1);	// turn to undiscovered unit
			_pf->abortPath();
		}

		// unit moved from one tile to the other, update the tiles
		if (_unit->getPosition() != _unit->getLastPosition())
		{
			int size = _unit->getArmor()->getSize() - 1;
			bool largeCheck = true;

			for (int x = size; x >= 0; x--)
			{
				for (int y = size; y >= 0; y--)
				{
					Tile* otherTileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x, y, -1));

					if (!_parent->getSave()->getTile(_unit->getPosition() + Position(x, y, 0))->hasNoFloor(otherTileBelow)
						|| _unit->getArmor()->getMovementType() == MT_FLY)
					{
						largeCheck = false;
					}

					_parent->getSave()->getTile(_unit->getLastPosition() + Position(x, y, 0))->setUnit(0);
				}
			}

			for (int x = size; x >= 0; x--)
			{
				for (int y = size; y >= 0; y--)
				{
					_parent->getSave()->getTile(_unit->getPosition()
						+ Position(x, y, 0))->setUnit(_unit, _parent->getSave()->getTile(_unit->getPosition() + Position(x, y, -1)));
				}
			}

			_falling = largeCheck
					&& _unit->getPosition().z != 0
					&& _unit->getTile()->hasNoFloor(tileBelow)
					&& _unit->getArmor()->getMovementType() != MT_FLY
					&& _unit->getWalkingPhase() == 0;
			if (_falling)
			{
				for (int x = _unit->getArmor()->getSize() - 1; x >= 0; --x)
				{
					for (int y = _unit->getArmor()->getSize() - 1; y >= 0; --y)
					{
						Tile* otherTileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x, y, -1));

						if (otherTileBelow
							&& otherTileBelow->getUnit())
						{
							_falling = false;

							_pf->dequeuePath();
							_parent->getSave()->addFallingUnit(_unit);
							_parent->statePushFront(new UnitFallBState(_parent));

							return;
						}
					}
				}
			}

/*kL			if (!_parent->getMap()->getCamera()->isOnScreen(_unit->getPosition())
				&& _unit->getFaction() != FACTION_PLAYER
				&& _unit->getVisible()) */
			// kL_note: Let's try this, maintain camera focus centered on Visible aliens during (un)hidden movement
			if (_unit->getVisible()								// kL
				&& _unit->getFaction() != FACTION_PLAYER)		// kL
			{
				_parent->getMap()->getCamera()->centerOnPosition(_unit->getPosition());
			}

			// if the unit changed level, camera changes level with it
			_parent->getMap()->getCamera()->setViewLevel(_unit->getPosition().z);
		}

		// is the step finished?
		if (_unit->getStatus() == STATUS_STANDING)
		{
			Log(LOG_INFO) << "Hey we got to STATUS_STANDING in UnitWalkBState _WALKING or _FLYING !!!" ;	// kL

			// if the unit burns floortiles, burn floortiles
			if (_unit->getSpecialAbility() == SPECAB_BURNFLOOR)
			{
				_unit->getTile()->ignite(1);

				Position here = (_unit->getPosition() * Position(16, 16, 24)) + Position(8, 8, -(_unit->getTile()->getTerrainLevel()));
				_parent->getTileEngine()->hit(here, _unit->getStats()->strength, DT_IN, _unit);
			}

			// move our personal lighting with us
			_terrain->calculateUnitLighting();

			if (_unit->getFaction() != FACTION_PLAYER)
			{
				_unit->setVisible(false);
			}

//			Log(LOG_INFO) << ". . getVisibleUnits() pre" ;					// kL
//			std::vector<BattleUnit* >* vunits = _unit->getVisibleUnits();	// kL
//			int preVisUnits = vunits->size();								// kL
//			Log(LOG_INFO) << ". . getVisibleUnits() " << preVisUnits ;		// kL

			_terrain->calculateFOV(_unit->getPosition());
			newUnitSpotted = !_action.desperate
				&& _parent->getPanicHandled()
//kL				&& _unitsSpotted != _unit->getUnitsSpottedThisTurn().size();
				&& _unitsSpotted < _unit->getUnitsSpottedThisTurn().size();

			// check for proximity grenades (1 tile around the unit in every direction)
			// For large units, we need to check every tile it occupies
			int size = _unit->getArmor()->getSize() - 1;
			for (int x = size; x >= 0; x--)
			{
				for (int y = size; y >= 0; y--)
				{
					for (int tx = -1; tx < 2; tx++)
					{
						for (int ty = -1; ty < 2; ty++)
						{
							Tile* t = _parent->getSave()->getTile(_unit->getPosition() + Position(x, y, 0) + Position(tx, ty, 0));
							if (t)
							{
								for (std::vector<BattleItem* >::iterator i = t->getInventory()->begin(); i != t->getInventory()->end(); ++i)
								{
									if ((*i)->getRules()->getBattleType() == BT_PROXIMITYGRENADE
										&& (*i)->getExplodeTurn() > 0)
									{
										Position p;
										p.x = t->getPosition().x * 16 + 8;
										p.y = t->getPosition().y * 16 + 8;
										p.z = t->getPosition().z * 24 + t->getTerrainLevel();
										_parent->statePushNext(new ExplosionBState(_parent, p, *i, (*i)->getPreviousOwner()));

										t->getInventory()->erase(i);
										_unit->setCache(0);
										_parent->getMap()->cacheUnit(_unit);
										_parent->popState();

//kL_TEST
										return;
									}
								}
							}
						}
					}
				}
			}

			// kL_note: I think this is the place to stop my soldiers from halting vs. already-seen alien units.
			// ie, do a check for !_alreadySpotted ( more cases are further down below )
//			newUnitSpotted = _parent->getPanicHandled()
//					&& _unitsSpotted != _unit->getUnitsSpottedThisTurn().size();	// kL: from above.
			// kL_note: this actually seems to be redundant w/ TileEngine::calculateFOV() ...

			// kL_begin: recalculation of SpottedUnit.
/*			int postVisUnits = vunits->size();
			if (_unit->getFaction() == FACTION_PLAYER)
			{
				if (postVisUnits <= preVisUnits)
				{
					newUnitSpotted = false;
				}
			} */
			// kL_end.


			if (newUnitSpotted)
			{
				_unit->setCache(0);
				_parent->getMap()->cacheUnit(_unit);
				_pf->abortPath();
				_parent->popState();

//kL_TEST
				return;
			}

			// check for reaction fire
//kL_TEST
			if (!_falling)
			{
				if (_terrain->checkReactionFire(_unit))
				{
					// unit got fired upon - stop walking
					_unit->setCache(0);
					_parent->getMap()->cacheUnit(_unit);
					_pf->abortPath();
					_parent->popState();

//kL_TEST
					return;
				}
			}

			// kL_note: I *think* this spot here qualifies as a Return;
			// ( finish _walkphase cycle, from one tile to a next tile )
			// ->
//			return;		// kL_TEST - confuses strafe moves ?
		}
		else if (onScreen) // still walking....
		{
			if (_pf->getStrafeMove())
			{
				// This is where we fake out the strafe movement direction so the unit "moonwalks"
				int dirTemp = _unit->getDirection();
				_unit->setDirection(_unit->getFaceDirection());
				_unit->setDirection(dirTemp);
			}

			// make sure the unit sprites are up to date
			_parent->getMap()->cacheUnit(_unit);
		}
	}

	// we are just standing around, shouldn't we be walking?
	if (_unit->getStatus() == STATUS_STANDING
		|| _unit->getStatus() == STATUS_PANICKING)
	{
		Log(LOG_INFO) << "STATUS_STANDING or PANICKING : " << _unit->getId();	// kL

		// check if we did spot new units
		if (newUnitSpotted
			&& !_action.desperate
			&& _unit->getCharging() == 0
			&& !_falling)
		{
			if (_parent->getSave()->getTraceSetting()) { Log(LOG_INFO) << "Uh-oh! Company!"; }
			Log(LOG_INFO) << "Uh-oh! STATUS_STANDING or PANICKING Company!";		// kL

			_unit->_hidingForTurn = false; // clearly we're not hidden now
			_parent->getMap()->cacheUnit(_unit);

			postPathProcedures();

			return;
		}

		if (onScreen
			|| _parent->getSave()->getDebugMode())
		{
			setNormalWalkSpeed();
		}
		else
			_parent->setStateInterval(0);
			// kL_note: mute footstep sounds. Trying...

		int dir = _pf->getStartDirection();
		if (_falling)
			dir = _pf->DIR_DOWN;

//		Log(LOG_INFO) << ". pos 0";			// kL
		Log(LOG_INFO) << "direction = " << dir;	// kL

		if (dir != -1)
		{
			Log(LOG_INFO) << "enter (dir!=-1) : " << _unit->getId();	// kL

			if (_pf->getStrafeMove())
			{
				_unit->setFaceDirection(_unit->getDirection());
			}

			Log(LOG_INFO) << ". pos 1";	// kL

			// gets tu cost, but also gets the destination position.
			Position destination;
			int tu = _pf->getTUCost(_unit->getPosition(), dir, &destination, _unit, 0, false);

			// we artificially inflate the TU cost by 32 points in getTUCost
			// under these conditions, so we have to deflate it here.
			if (_unit->getFaction() == FACTION_HOSTILE
				&& ((_parent->getSave()->getTile(destination)->getUnit()
					&& _parent->getSave()->getTile(destination)->getUnit()->getFaction() == FACTION_HOSTILE
					&& _parent->getSave()->getTile(destination)->getUnit() != _unit)
						|| _parent->getSave()->getTile(destination)->getFire() > 0))
			{
				tu -= 32;
			}

			Log(LOG_INFO) << ". pos 2";	// kL

			if (_falling) tu = 0;
			int energy = tu;

			if (_action.run)
			{
				tu *= 0.75;
				energy *= 1.5;
			}

			if (tu > _unit->getTimeUnits())
			{
				if (_parent->getPanicHandled() && tu < 255)
				{
					_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
				}

				_pf->abortPath();

				_unit->setCache(0);
				_parent->getMap()->cacheUnit(_unit);

				_parent->popState();

				return;
			}
			else if (energy / 2 > _unit->getEnergy())
			{
				Log(LOG_INFO) << ". pos 3";	// kL

				if (_parent->getPanicHandled())
				{
					_action.result = "STR_NOT_ENOUGH_ENERGY";
				}

				_pf->abortPath();

				_unit->setCache(0);
				_parent->getMap()->cacheUnit(_unit);

				_parent->popState();

				return;
			}
			else if (_parent->getPanicHandled()
				&& _parent->checkReservedTU(_unit, tu) == false)
			{
				Log(LOG_INFO) << ". pos 4";	// kL

				_pf->abortPath();

				_unit->setCache(0);
				_parent->getMap()->cacheUnit(_unit);

				return;
			}
			// we are looking in the wrong way, turn first (unless strafing)
			// we are not using the turn state, because turning during walking costs no tu
			// kL_note: have this turn to face the nearest (last spotted?) xCom soldier
			else if (dir != _unit->getDirection()
				&& dir < _pf->DIR_UP
				&& !_pf->getStrafeMove())
			{
				Log(LOG_INFO) << ". pos 5";	// kL
				Log(LOG_INFO) << "UnitWalkBState::think() 2";

				_unit->lookAt(dir);

				_unit->setCache(0);
				_parent->getMap()->cacheUnit(_unit);

				return;
			}
			else if (dir < _pf->DIR_UP) // now open doors (if any)
			{
				Log(LOG_INFO) << ". open doors";	// kL

				int door = _terrain->unitOpensDoor(_unit, false, dir);
				if (door == 3)
				{
					Log(LOG_INFO) << ". . door #3";	// kL

					return; // don't start walking yet, wait for the ufo door to open
				}
				else if (door == 0)
				{
					Log(LOG_INFO) << ". . door #0";	// kL

					_parent->getResourcePack()->getSound("BATTLE.CAT", 3)->play(); // normal door

//					return; // kL. don't start walking yet, wait for the normal door to open
				}
				else if (door == 1)
				{
					Log(LOG_INFO) << ". . door #1";	// kL

					_parent->getResourcePack()->getSound("BATTLE.CAT", 20)->play(); // ufo door

					return; // don't start walking yet, wait for the ufo door to open
				}
			}

			Log(LOG_INFO) << ". pos 7";	// kL

			for (int x = _unit->getArmor()->getSize() - 1; x >= 0; --x)
			{
				for (int y = _unit->getArmor()->getSize() - 1; y >= 0; --y)
				{
					BattleUnit* unitInMyWay = _parent->getSave()->getTile(destination + Position(x, y, 0))->getUnit();
					BattleUnit* unitBelowMyWay = 0;

					Tile* belowDest = _parent->getSave()->getTile(destination + Position(x, y, -1));
					if (belowDest)
					{
						unitBelowMyWay = belowDest->getUnit();
					}

					// can't walk into units in this tile, or on top of other units sticking their head into this tile
					if (!_falling
						&& ((unitInMyWay
								&& unitInMyWay != _unit)
							|| (belowDest
								&& unitBelowMyWay
								&& unitBelowMyWay != _unit
								&& -belowDest->getTerrainLevel() + unitBelowMyWay->getFloatHeight() + unitBelowMyWay->getHeight()) >= 28))
								// 4+ voxels poking into the tile above, we don't kick people in the head here at XCom.
					{
						_action.TU = 0;
						_pf->abortPath();

						_unit->setCache(0);
						_parent->getMap()->cacheUnit(_unit);

						_parent->popState();

						return;
					}
				}
			}

			Log(LOG_INFO) << ". pos 8";	// kL

			dir = _pf->dequeuePath(); // now start moving
			if (_falling) dir = _pf->DIR_DOWN;		// kL_note: set above, if it hasn't changed...

			if (_unit->spendTimeUnits(tu))
			{
				if (_unit->spendEnergy(energy))
				{
					Log(LOG_INFO) << ". WalkBState: spend TU & Energy";	// kL

					Tile* tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0, 0, -1));
//					if (!isKneeled)															// kL
//					if (_unit->getWalkingPhase() < 0)										// kL
//					{
//						Log(LOG_INFO) << ". WalkBState: startWalking()";	// kL

					_unit->startWalking(dir, destination, tileBelow, onScreen);
	
					_beforeFirstStep = false;
//					}
//					else													// kL
//					{
//						Log(LOG_INFO) << ". WalkBState: keepWalking()";		// kL

//						_unit->keepWalking(tileBelow, onScreen);			// kL
//					}
				}
			}

			// make sure the unit sprites are up to date
			if (onScreen)
			{
				Log(LOG_INFO) << ". . . onScreen -> cacheUnit()";	// kL

				if (_pf->getStrafeMove())
				{
					Log(LOG_INFO) << ". . . (_pf->getStrafeMove()";	// kL

					// This is where we fake out the strafe movement direction so the unit "moonwalks"
					int dirTemp = _unit->getDirection();
					_unit->setDirection(_unit->getFaceDirection());
					_unit->setDirection(dirTemp);

					Log(LOG_INFO) << ". . . end (_pf->getStrafeMove()";	// kL
				}

//				Log(LOG_INFO) << ". . mid (onScreen)";	// kL

				_parent->getMap()->cacheUnit(_unit);

//				Log(LOG_INFO) << ". . end (onScreen)";	// kL
			}

			Log(LOG_INFO) << "exit (dir!=-1) : " << _unit->getId();	// kL
		}
		else
		{
			Log(LOG_INFO) << ". . postPathProcedures()";	// kL

			postPathProcedures();

			return;
		}
	}

	// turning during walking costs no tu
	if (_unit->getStatus() == STATUS_TURNING)
	{
		Log(LOG_INFO) << "STATUS_TURNING : " << _unit->getId();	// kL

		// except before the first step.
		if (_beforeFirstStep)
		{
			_preMovementCost++;
		}

		_unit->turn();

		// calculateFOV() is unreliable for setting the newUnitSpotted bool, as it can be called from
		// various other places in the code, ie: doors opening, and that messes up the result.
		_terrain->calculateFOV(_unit);
		newUnitSpotted = !_action.desperate
			&& _parent->getPanicHandled()
//kL			&& _unitsSpotted != _unit->getUnitsSpottedThisTurn().size();
			&& _unitsSpotted < _unit->getUnitsSpottedThisTurn().size();		// kL

		// make sure the unit sprites are up to date
		if (onScreen)
		{
			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);
		}

		if (newUnitSpotted
			&& !(_action.desperate || _unit->getCharging())
			&& !_falling)
		{
			if (_beforeFirstStep)
			{
				_unit->spendTimeUnits(_preMovementCost);
			}

			if (_parent->getSave()->getTraceSetting()) { Log(LOG_INFO) << "Egads! A turn reveals new units! I must pause!"; }
			Log(LOG_INFO) << "Egads! STATUS_TURNING reveals new units!!! I must pause!";	// kL

			_unit->_hidingForTurn = false; // not hidden, are we...

			_pf->abortPath();
			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);
			_parent->popState();
		}
	}

	Log(LOG_INFO) << "****** think() : " << _unit->getId() << " : end ******";	// kL
}

/**
 * Aborts unit walking.
 */
void UnitWalkBState::cancel()
{
	if (_parent->getSave()->getSide() == FACTION_PLAYER
		&& _parent->getPanicHandled())
	{
		_pf->abortPath();
	}
}

/**
 * Handles some calculations when the path is finished.
 */
void UnitWalkBState::postPathProcedures()
{
	Log(LOG_INFO) << "postPathProcedures() : " << _unit->getId();	// kL

	_action.TU = 0;

	if (_unit->getFaction() != FACTION_PLAYER)
	{
		int dir = _action.finalFacing;
		if (_action.finalAction)
		{
			_unit->dontReselect();
		}

		if (_unit->getCharging() != 0)
		{
			dir = _parent->getTileEngine()->getDirectionTo(_unit->getPosition(), _unit->getCharging()->getPosition());
			// kL_notes (pre-above):
			// put an appropriate facing direction here
			// don't stare at a wall. Get if aggro, face closest xCom op <- might be done somewhere already.
			// Cheat: face closest xCom op based on a percentage (perhaps alien 'value' or rank)
			// cf. void AggroBAIState::setAggroTarget(BattleUnit *unit)
			// and bool TileEngine::calculateFOV(BattleUnit *unit)
			Log(LOG_INFO) << "UnitWalkBState::postPathProcedures() 1";

			if (_parent->getTileEngine()->validMeleeRange(_unit, _action.actor->getCharging(), _unit->getDirection()))
			{
				BattleAction action;
				action.actor = _unit;
				action.target = _unit->getCharging()->getPosition();
				action.weapon = _unit->getMainHandWeapon();
				action.type = BA_HIT;
				action.TU = _unit->getActionTUs(action.type, action.weapon);
				action.targeting = true;

				_unit->setCharging(0);
				_parent->statePushBack(new ProjectileFlyBState(_parent, action));
			}
		}
		else if (_unit->_hidingForTurn)
		{
			dir = _unit->getDirection() + 4;
		}

		if (dir != -1)
		{
			if (dir >= 8)
			{
				dir -= 8;
			}
			_unit->lookAt(dir);

			while (_unit->getStatus() == STATUS_TURNING)
			{
				_unit->turn();
				_parent->getTileEngine()->calculateFOV(_unit);
			}

			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);

		}
	}
	else if (!_parent->getPanicHandled())
	{
		// todo: set the unit to aggrostate and try to find cover?
		_unit->setTimeUnits(0);
	}

	_unit->setCache(0);
	_terrain->calculateUnitLighting();
	_terrain->calculateFOV(_unit);
	_parent->getMap()->cacheUnit(_unit);

	if (!_falling)
		_parent->popState();
}

/**
 * Sets the animation speed of soldiers or aliens.
 */
void UnitWalkBState::setNormalWalkSpeed()
{
	if (_unit->getFaction() == FACTION_PLAYER)
		_parent->setStateInterval(Options::getInt("battleXcomSpeed"));
	else
		_parent->setStateInterval(Options::getInt("battleAlienSpeed"));
}


/**
 * Handles the stepping sounds.
 */
void UnitWalkBState::playMovementSound()
{
	if ((!_unit->getVisible() && !_parent->getSave()->getDebugMode())
		|| !_parent->getMap()->getCamera()->isOnScreen(_unit->getPosition()))
	{
		return;
	}

	if (_unit->getMoveSound() != -1)
	{
		// if a sound is configured in the ruleset, play that one
		if (_unit->getWalkingPhase() == 0)
		{
			_parent->getResourcePack()->getSound("BATTLE.CAT", _unit->getMoveSound())->play();
		}
	}
	else
	{
		if (_unit->getStatus() == STATUS_WALKING)
		{
			Tile* tile = _unit->getTile();
			Tile* tileBelow = _parent->getSave()->getTile(tile->getPosition() + Position(0, 0, -1));

			// play footstep sound 1
			if (_unit->getWalkingPhase() == 3)
			{
				if (tile->getFootstepSound(tileBelow)
					&& _unit->getRaceString() != "STR_ETHEREAL")	// kL: and not an ethereal
				{
					_parent->getResourcePack()->getSound("BATTLE.CAT", 22 + (tile->getFootstepSound(tileBelow) * 2))->play();
				}
			}

			// play footstep sound 2
			if (_unit->getWalkingPhase() == 7)
			{
				if (tile->getFootstepSound(tileBelow)
					&& _unit->getRaceString() != "STR_ETHEREAL")	// kL: and not an ethereal
				{
					_parent->getResourcePack()->getSound("BATTLE.CAT", 23 + (tile->getFootstepSound(tileBelow) * 2))->play();
				}
			}
		}
		else
		{
			// play default flying sound
			if (_unit->getWalkingPhase() == 0
				&& !_falling)
			{
				_parent->getResourcePack()->getSound("BATTLE.CAT", 15)->play();
			}
		}
	}
}

}
