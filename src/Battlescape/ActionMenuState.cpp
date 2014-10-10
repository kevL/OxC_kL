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

#include "ActionMenuState.h"

#include <cmath>
#include <sstream>

#include "ActionMenuItem.h"
#include "MedikitState.h"
#include "Pathfinding.h"
#include "PrimeGrenadeState.h"
#include "ScannerState.h"
#include "TileEngine.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleItem.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Action Menu window.
 * @param action	- pointer to BattleAction (BattlescapeGame.h)
 * @param x			- position on the x-axis
 * @param y			- position on the y-axis
 */
ActionMenuState::ActionMenuState(
		BattleAction* action,
		int x,
		int y)
	:
		_action(action)
{
	_screen = false;

	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	for (int
			i = 0;
			i < 6;
			++i)
	{
		_actionMenu[i] = new ActionMenuItem(
										i,
										_game,
										x,
										y);
		add(_actionMenu[i]);

		_actionMenu[i]->setVisible(false);
		_actionMenu[i]->onMouseClick((ActionHandler)& ActionMenuState::btnActionMenuItemClick);
	}

	// Build the popup menu
	RuleItem* weapon = _action->weapon->getRules();
	int id = 0;

	if (weapon->isFixed() == false) // throwing (if not a fixed weapon)
		addItem(
				BA_THROW,
				"STR_THROW",
				&id);

	// kL_begin:
	SavedGame* save = _game->getSavedGame();
	if (save->getSavedBattle()->getSelectedUnit()->getOriginalFaction() != FACTION_HOSTILE
		&& save->isResearched(_action->weapon->getRules()->getRequirements()) == false)
	{
		return;
	} // kL_end.

	if (weapon->getBattleType() == BT_GRENADE
		|| weapon->getBattleType() == BT_PROXIMITYGRENADE)
	{
		if (_action->weapon->getFuseTimer() == -1) // canPrime
			addItem(
					BA_PRIME,
					"STR_PRIME_GRENADE",
					&id);
		else // kL_add:
			addItem(
					BA_DEFUSE,
					"STR_DEFUSE_GRENADE",
					&id);
	}

	if (weapon->getTUMelee())
	{
		if (weapon->getBattleType() == BT_MELEE
			&& weapon->getDamageType() == DT_STUN)
		{
			addItem( // stun rod
					BA_HIT,
					"STR_STUN",
					&id);
		}
		else
			addItem( // melee weapon
					BA_HIT,
					"STR_HIT_MELEE",
					&id);
	}
	else if (weapon->getBattleType() == BT_MEDIKIT) // special items
		addItem(
				BA_USE,
				"STR_USE_MEDI_KIT",
				&id);
	else if (weapon->getBattleType() == BT_SCANNER)
		addItem(
				BA_USE,
				"STR_USE_SCANNER",
				&id);
	else if (weapon->getBattleType() == BT_PSIAMP
		&& _action->actor->getStats()->psiSkill > 0)
	{
		addItem(
				BA_MINDCONTROL,
				"STR_MIND_CONTROL",
				&id);
		addItem(
				BA_PANIC,
				"STR_PANIC_UNIT",
				&id);
	}
	else if (weapon->getBattleType() == BT_MINDPROBE)
		addItem(
				BA_USE,
				"STR_USE_MIND_PROBE",
				&id);

	if (weapon->getBattleType() == BT_FIREARM)
	{
		if (weapon->isWaypoint()
			|| (_action->weapon->getAmmoItem()
				&& _action->weapon->getAmmoItem()->getRules()->isWaypoint()))
		{
			addItem(
					BA_LAUNCH,
					"STR_LAUNCH_MISSILE",
					&id);
		}

		if (_action->weapon->getAmmoItem()) // kL
		{
			if (weapon->getAccuracySnap() != 0)
				addItem(
						BA_SNAPSHOT,
						"STR_SNAP_SHOT",
						&id);

			if (weapon->getAccuracyAuto() != 0)
				addItem(
						BA_AUTOSHOT,
						"STR_AUTO_SHOT",
						&id);

			if (weapon->getAccuracyAimed() != 0)
				addItem(
						BA_AIMEDSHOT,
						"STR_AIMED_SHOT",
						&id);
		}
	}
}

/**
 * Deletes the ActionMenuState.
 */
ActionMenuState::~ActionMenuState()
{
}

/**
 * Adds a new menu item for an action.
 * @param baType	- action type (BattlescapeGame.h)
 * @param desc		- reference the action description
 * @param id		- pointer to the new item-action ID
 */
void ActionMenuState::addItem(
		BattleActionType baType,
		const std::string& desc,
		int* id)
{
	std::wstring
		s1, // acu
		s2; // tu

	if (baType == BA_THROW
		|| baType == BA_AIMEDSHOT
		|| baType == BA_SNAPSHOT
		|| baType == BA_AUTOSHOT
//kL	|| baType == BA_LAUNCH
		|| baType == BA_HIT)
	{
		int acu = 0;

		if (baType == BA_THROW)
			acu = static_cast<int>(Round(_action->actor->getThrowingAccuracy() * 100.0));
		else
			acu = static_cast<int>(Round(_action->actor->getFiringAccuracy(
																		baType,
																		_action->weapon)
																	* 100.0));

//kL	s1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acu));
		s1 = tr("STR_ACCURACY_SHORT_KL").arg(acu); // kL
	}

	int tu = _action->actor->getActionTUs(
										baType,
										_action->weapon);
	s2 = tr("STR_TIME_UNITS_SHORT").arg(tu);



	_actionMenu[*id]->setAction(
							baType,
							tr(desc),
							s1,
							s2,
							tu);
	_actionMenu[*id]->setVisible();

	(*id)++; // kL_note: huh. oh yeah: &reference
}

/**
 * Closes the window on click.
 * @param action - pointer to an action
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
 * @param action - pointer to an action
 */
void ActionMenuState::btnActionMenuItemClick(Action* action)
{
	_game->getSavedGame()->getSavedBattle()->getPathfinding()->removePreview();

	RuleItem* weapon = _action->weapon->getRules();
	int btnID = -1;

	for (size_t // got to find out which button was pressed
			i = 0;
			i < sizeof(_actionMenu) / sizeof(_actionMenu[0])
				&& btnID == -1;
			++i)
	{
		if (action->getSender() == _actionMenu[i])
			btnID = i;
	}

	if (btnID != -1)
	{
		_action->TU		= _actionMenu[btnID]->getTUs();
		_action->type	= _actionMenu[btnID]->getAction();

		if (_action->type != BA_THROW
			&& _game->getSavedGame()->getSavedBattle()->getDepth() == 0
			&& weapon->isWaterOnly())
		{
			_action->result = "STR_THIS_EQUIPMENT_WILL_NOT_FUNCTION_ABOVE_WATER";
			_game->popState();
		}
		else if (_action->type == BA_PRIME)
		{
			if (weapon->getBattleType() == BT_PROXIMITYGRENADE)
			{
				_action->value = 0;
				_game->popState();
			}
			else
				_game->pushState(new PrimeGrenadeState(
													_action,
													false,
													NULL));
		}
		else if (_action->type == BA_DEFUSE) // kL_add.
		{
			_action->value = -1;
			_game->popState();
		}
		else if (_action->type == BA_USE
			&& weapon->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit* targetUnit = NULL;

			std::vector<BattleUnit*>* const units = _game->getSavedGame()->getSavedBattle()->getUnits();
			for (std::vector<BattleUnit*>::const_iterator
					i = units->begin();
					i != units->end()
						&& targetUnit == NULL;
					++i)
			{
				// we can heal a unit that is at the same position, unconscious and healable(=woundable)
				if ((*i)->getPosition() == _action->actor->getPosition()
					&& *i != _action->actor
					&& (*i)->getStatus() == STATUS_UNCONSCIOUS
					&& (*i)->isWoundable())
				{
					targetUnit = *i;
				}
			}

			if (targetUnit == NULL
				&& _game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
																						_action->actor->getPosition(),
																						_action->actor->getDirection(),
																						_action->actor,
																						NULL,
																						&_action->target))
			{
				Tile* tile = _game->getSavedGame()->getSavedBattle()->getTile(_action->target);
				if (tile != NULL
					&& tile->getUnit()
					&& tile->getUnit()->isWoundable())
				{
					targetUnit = tile->getUnit();
				}
			}

			if (targetUnit)
			{
				_game->popState();
				_game->pushState(new MedikitState(
												targetUnit,
												_action));
			}
			else
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
				_game->popState();
			}
		}
		else if (_action->type == BA_USE
			&& weapon->getBattleType() == BT_SCANNER)
		{
			if (_action->actor->spendTimeUnits(_action->TU)) // spend TUs first, then show the scanner
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
		else if (_action->type == BA_LAUNCH)
		{
			if (_action->TU > _action->actor->getTimeUnits())
				_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
			else if (_action->weapon->getAmmoItem() == NULL
				|| (_action->weapon->getAmmoItem()
					&& _action->weapon->getAmmoItem()->getAmmoQuantity() == 0))
			{
				_action->result = "STR_NO_AMMUNITION_LOADED";
			}
			else
				_action->targeting = true;

			_game->popState();
		}
		else if (_action->type == BA_STUN
				|| _action->type == BA_HIT)
		{
			if (_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(
																					_action->actor->getPosition(),
																					_action->actor->getDirection(),
																					_action->actor,
																					NULL,
																					&_action->target)
																				== false)
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
			}

			_game->popState();
		}
		else // shoot, throw, psi-attack, mind-probe
		{
			_action->targeting = true;
			_game->popState();
		}
	}
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
