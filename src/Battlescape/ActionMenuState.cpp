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

#include "ActionMenuState.h"

#include "ActionMenuItem.h"
#include "ExecuteState.h"
#include "Map.h"
#include "MedikitState.h"
#include "MediTargetState.h"
#include "Pathfinding.h"
#include "PrimeGrenadeState.h"
#include "ScannerState.h"
#include "TileEngine.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
#include "../Engine/Sound.h"

#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleItem.h"
//#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Action Menu window.
 * @param action	- pointer to a BattleAction (BattlescapeGame.h)
 * @param x			- position on the x-axis
 * @param y			- position on the y-axis
 */
ActionMenuState::ActionMenuState(
		BattleAction* const action,
		int x,
		int y)
	:
		_action(action)
{
	_screen = false;

	setPalette("PAL_BATTLESCAPE");

	for (size_t
			i = 0;
			i != MENU_ITEMS;
			++i)
	{
		_menuSelect[i] = new ActionMenuItem(
										i,
										_game,
										x,y);
		add(_menuSelect[i]);

		_menuSelect[i]->setVisible(false);
		_menuSelect[i]->onMouseClick((ActionHandler)& ActionMenuState::btnActionMenuClick);
	}

	// Build the popup menu
	const RuleItem* const itRule = _action->weapon->getRules();
	size_t id = 0;

	if (itRule->isFixed() == false) // Throw & Drop (if not a fixed weapon)
	{
		addItem(
				BA_DROP,
				"STR_DROP",
				&id);

		if (_action->actor->getBaseStats()->throwing != 0)
			addItem(
					BA_THROW,
					"STR_THROW",
					&id);
	}

	if (_game->getSavedGame()->isResearched(itRule->getRequirements()) == true)
	{
		if (itRule->getTUMelee() != 0) // TODO: remove 'HIT' action-items if no target in range.
		{
			if (itRule->getBattleType() == BT_MELEE
				&& itRule->getDamageType() == DT_STUN)
			{
				addItem( // stun rod
						BA_HIT,
						"STR_STUN",
						&id);
			}
			else if (itRule->getType() == "STR_DOGE")
			{
				addItem( // doggie bite
						BA_HIT,
						"STR_DOGE_BITE",
						&id);
				addItem( // doggie bark
						BA_NONE,
						"STR_DOGE_BARK",
						&id);
			}
			else
				addItem( // melee weapon
						BA_HIT,
						"STR_HIT_MELEE",
						&id);
		}

		if (itRule->isGrenade() == true)
		{
			if (_action->weapon->getFuseTimer() == -1) // canPrime
				addItem(
						BA_PRIME,
						"STR_PRIME_GRENADE",
						&id);
			else
				addItem(
						BA_DEFUSE,
						"STR_DEFUSE_GRENADE",
						&id);
		}
		else if (itRule->getBattleType() == BT_MEDIKIT) // special items
			addItem(
					BA_USE,
					"STR_USE_MEDI_KIT",
					&id);
		else if (itRule->getBattleType() == BT_SCANNER)
			addItem(
					BA_USE,
					"STR_USE_SCANNER",
					&id);
		else if (itRule->getBattleType() == BT_PSIAMP
			&& _action->actor->getBaseStats()->psiSkill != 0)
		{
			addItem(
					BA_PSICONTROL,
					"STR_MIND_CONTROL",
					&id);
			addItem(
					BA_PSIPANIC,
					"STR_PANIC_UNIT",
					&id);
			addItem(
					BA_PSICONFUSE,
					"STR_CONFUSE_UNIT",
					&id);
			addItem(
					BA_PSICOURAGE,
					"STR_ENCOURAGE_UNIT",
					&id);
		}
		else if (itRule->getBattleType() == BT_MINDPROBE)
			addItem(
					BA_USE,
					"STR_USE_MIND_PROBE",
					&id);
		else if (itRule->getBattleType() == BT_FIREARM)
		{
			if (_action->weapon->getAmmoItem() != NULL)
			{
				if (itRule->getAccuracySnap() != 0)
					addItem(
							BA_SNAPSHOT,
							"STR_SNAP_SHOT",
							&id);

				if (itRule->getAccuracyAuto() != 0)
					addItem(
							BA_AUTOSHOT,
							"STR_AUTO_SHOT",
							&id);

				if (itRule->getAccuracyAimed() != 0)
					addItem(
							BA_AIMEDSHOT,
							"STR_AIMED_SHOT",
							&id);
			}

			if (itRule->isWaypoints() != 0
				|| (_action->weapon->getAmmoItem() != NULL
					&& _action->weapon->getAmmoItem()->getRules()->isWaypoints() != 0))
			{
				addItem(
						BA_LAUNCH,
						"STR_LAUNCH_MISSILE",
						&id);
			}
		}

		if (_action->weapon->getAmmoItem() != NULL // is loaded or self-loaded.
			&& _action->weapon->getAmmoItem()->getRules()->canExecute() == true
			&& canExecuteTarget() == true)
		{
			addItem(
					BA_EXECUTE,
					"STR_EXECUTE",
					&id);
		}
	}
}

/**
 * Deletes the ActionMenuState.
 */
ActionMenuState::~ActionMenuState()
{}

/**
 * Adds a new menu item for an action.
 * @param batType	- action type (BattlescapeGame.h)
 * @param desc		- reference the action description
 * @param id		- pointer to the new item-action ID
 */
void ActionMenuState::addItem( // private.
		BattleActionType batType,
		const std::string& desc,
		size_t* id)
{
	std::wstring
		wst1, // acu
		wst2; // tu

	if (batType == BA_THROW
		|| batType == BA_AIMEDSHOT
		|| batType == BA_SNAPSHOT
		|| batType == BA_AUTOSHOT
		|| batType == BA_HIT)
	{
		int acu;
		if (batType == BA_THROW)
			acu = static_cast<int>(Round(_action->actor->getThrowingAccuracy() * 100.));
		else
			acu = static_cast<int>(Round(_action->actor->getFiringAccuracy(
																		batType,
																		_action->weapon) * 100.));

		wst1 = tr("STR_ACCURACY_SHORT_KL").arg(acu);
	}

	const int tu = _action->actor->getActionTUs(
											batType,
											_action->weapon);

	if (batType != BA_NONE) // ie. everything but doggie bark
		wst2 = tr("STR_TIME_UNITS_SHORT").arg(tu);

	_menuSelect[*id]->setAction(
							batType,
							tr(desc),
							wst1,
							wst2,
							tu);
	_menuSelect[*id]->setVisible();

	++(*id);
}

/**
 * Closes the window on click.
 * @param action - pointer to an Action
 */
void ActionMenuState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT
		|| (action->getDetails()->type == SDL_KEYDOWN
			&& (action->getDetails()->key.keysym.sym == Options::keyCancel
				|| action->getDetails()->key.keysym.sym == Options::keyBattleUseLeftHand
				|| action->getDetails()->key.keysym.sym == Options::keyBattleUseRightHand)))
	{
		_game->popState();
	}
}

/**
 * Executes the action corresponding to this action menu item.
 * @param action - pointer to an Action
 */
void ActionMenuState::btnActionMenuClick(Action* action)
{
	_game->getSavedGame()->getSavedBattle()->getPathfinding()->removePreview();

	size_t btnId = MENU_ITEMS;

	for (size_t // find out which button was pressed
			i = 0;
			i != MENU_ITEMS;
			++i)
	{
		if (action->getSender() == _menuSelect[i])
		{
			btnId = i;
			break;
		}
	}


	if (btnId != MENU_ITEMS)
	{
		const RuleItem* const itRule = _action->weapon->getRules();

		_action->TU = _menuSelect[btnId]->getTUs();
		_action->type = _menuSelect[btnId]->getAction();

		switch (_action->type)
		{
			case BA_NONE: // doggie bark
				_game->popState();
				_game->getResourcePack()->getSound(
												"BATTLE.CAT",
												itRule->getMeleeSound())
											->play(
												-1,
												_game->getSavedGame()->getSavedBattle()->getBattleGame()->getMap()
													->getSoundAngle(_action->actor->getPosition()));
			break;

			case BA_PRIME:
				if (itRule->getBattleType() == BT_PROXYGRENADE)
				{
					_action->value = 0;
					_game->popState();
				}
				else
					_game->pushState(new PrimeGrenadeState(
														_action,
														false,
														NULL));
			break;

			case BA_DEFUSE:
				_action->value = -1;
				_game->popState();
			break;

			case BA_DROP:
				if (_action->actor->spendTimeUnits(_action->TU) == true)
				{
					_action->weapon->moveToOwner(NULL);
					_action->actor->getTile()->addItem(
													_action->weapon,
													_game->getRuleset()->getInventory("STR_GROUND"));

					if (_action->actor->getItem("STR_LEFT_HAND") == NULL)
						_action->actor->setActiveHand("STR_RIGHT_HAND");
					else
						_action->actor->setActiveHand("STR_LEFT_HAND");

					_action->actor->invalidateCache();
				}
				else
					_action->result = "STR_NOT_ENOUGH_TIME_UNITS";

				_game->popState();
			break;

			case BA_USE:
				if (itRule->getBattleType() == BT_MEDIKIT)
				{
					_game->popState();
					_game->pushState(new MediTargetState(_action));
				}
				else if (itRule->getBattleType() == BT_SCANNER)
				{
					if (_action->actor->spendTimeUnits(_action->TU) == true)
					{
						_game->popState();
						_game->pushState(new ScannerState(_action));
					}
					else
					{
						_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
						_game->popState();
					}
				}
				else if (itRule->getBattleType() == BT_MINDPROBE)
				{
					_action->targeting = true;
					_game->popState();
				}
			break;

			case BA_LAUNCH:
				if (_action->TU > _action->actor->getTimeUnits())
					_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
				else if (_action->weapon->getAmmoItem() == NULL
					|| _action->weapon->getAmmoItem()->getAmmoQuantity() == 0)
				{
					_action->result = "STR_NO_AMMUNITION_LOADED";
				}
				else
					_action->targeting = true;

				_game->popState();
			break;

			case BA_HIT:
				if (_action->TU > _action->actor->getTimeUnits())
					_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
				else if (_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
																							_action->actor->getPosition(),
																							_action->actor->getDirection(),
																							_action->actor,
																							NULL,
																							&_action->target) == false)
				{
					_action->result = "STR_THERE_IS_NO_ONE_THERE";
				}

				_game->popState();
			break;

			case BA_EXECUTE:
				_game->popState();
				_game->pushState(new ExecuteState(_action));
			break;

			default: // shoot, throw, psi-attack
				_action->targeting = true;
				_game->popState();
		}
	}
}

/**
 * Checks if there is a viable execution target nearby.
 * @return, true if can execute
 */
bool ActionMenuState::canExecuteTarget() // private.
{
	const SavedBattleGame* const battleSave = _game->getSavedGame()->getSavedBattle();
	const Tile* const tile = battleSave->getTile(_action->actor->getPosition()); // TODO: check quads of large units.

	if (tile->hasUnconsciousUnit(false) != 0
		|| hasUnconscious() == true)
	{
		return true;
	}

	return false;
}

/**
 * Checks for an unconscious unit in valid range.
 * @note Helper for canExecute().
 * @return, true if there's an unconscious unit in the direction of facing
 */
bool ActionMenuState::hasUnconscious() // private. // TODO: duplicate of ExecuteState::getTargetTile()
{
	SavedBattleGame* const battleSave = _game->getSavedGame()->getSavedBattle();

	const int dir = _action->actor->getDirection();
	Position posTarget;
	Pathfinding::directionToVector(
								dir,
								&posTarget);

	const Position origin = _action->actor->getPosition();
	Tile
		* tileOrigin,
		* tileTarget,
		* tileTarget_above,
		* tileTarget_below;

	const int unitSize = _action->actor->getArmor()->getSize() - 1;
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
			tileOrigin = battleSave->getTile(Position(origin + Position(x,y,0)));
			tileTarget = battleSave->getTile(Position(origin + Position(x,y,0) + posTarget));

			if (tileOrigin != NULL
				&& tileTarget != NULL)
			{
				if (tileTarget->hasUnconsciousUnit(false) == 0)
				{
					tileTarget_above = battleSave->getTile(Position(origin + Position(x,y, 1) + posTarget));
					tileTarget_below = battleSave->getTile(Position(origin + Position(x,y,-1) + posTarget));

					if (tileTarget_above != NULL // standing on a rise only 1/3 up z-axis reaches adjacent tileAbove.
						&& std::abs(tileTarget_above->getTerrainLevel() - (tileOrigin->getTerrainLevel() + 24)) < 9)
					{
						tileTarget = tileTarget_above;
					}
					else if (tileTarget_below != NULL // can reach targetUnit standing on a rise only 1/3 up z-axis on adjacent tileBelow.
						&& std::abs((tileTarget_below->getTerrainLevel() + 24) + tileOrigin->getTerrainLevel()) < 9)
					{
						tileTarget = tileTarget_below;
					}
				}

				if (tileTarget->hasUnconsciousUnit(false) != 0)
				{
					const Position voxelOrigin = Position(tileOrigin->getPosition() * Position(16,16,24))
											   + Position(
														8,8,
														_action->actor->getHeight(true)
															- tileOrigin->getTerrainLevel()
															- 4);
					Position scanVoxel;
					if (battleSave->getTileEngine()->canTargetTile(
																&voxelOrigin,
																tileOrigin,
																O_FLOOR,
																&scanVoxel,
																_action->actor) == true)
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
 * Updates the scale.
 * @param dX - delta of X
 * @param dY - delta of Y
 */
void ActionMenuState::resize(
		int& dX,
		int& dY)
{
	State::recenter(dX, dY * 2);
}

}
