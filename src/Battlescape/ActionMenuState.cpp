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

//#include <cmath>
//#include <sstream>

#include "ActionMenuItem.h"
#include "MedikitState.h"
#include "Pathfinding.h"
#include "PrimeGrenadeState.h"
#include "ScannerState.h"
#include "TileEngine.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"

//#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleItem.h"

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
		BattleAction* action,
		int x,
		int y)
	:
		_action(action)
{
	_screen = false;

	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

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

		addItem(
				BA_THROW,
				"STR_THROW",
				&id);
	}

	SavedGame* const gameSave = _game->getSavedGame();
	if (gameSave->getSavedBattle()->getSelectedUnit()->getOriginalFaction() != FACTION_HOSTILE
		&& gameSave->isResearched(itRule->getRequirements()) == false)
	{
		return;
	}

	if (itRule->getBattleType() == BT_GRENADE
		|| itRule->getBattleType() == BT_PROXIMITYGRENADE)
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

	if (itRule->getTUMelee() != 0)
	{
		if (itRule->getBattleType() == BT_MELEE
			&& itRule->getDamageType() == DT_STUN)
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
				BA_MINDCONTROL,
				"STR_MIND_CONTROL",
				&id);
		addItem(
				BA_PANIC,
				"STR_PANIC_UNIT",
				&id);
	}
	else if (itRule->getBattleType() == BT_MINDPROBE)
		addItem(
				BA_USE,
				"STR_USE_MIND_PROBE",
				&id);

	if (itRule->getBattleType() == BT_FIREARM)
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
}

/**
 * Deletes the ActionMenuState.
 */
ActionMenuState::~ActionMenuState()
{}

/**
 * Adds a new menu item for an action.
 * @param baType	- action type (BattlescapeGame.h)
 * @param desc		- reference the action description
 * @param id		- pointer to the new item-action ID
 */
void ActionMenuState::addItem(
		BattleActionType baType,
		const std::string& desc,
		size_t* id)
{
	std::wstring
		wst1, // acu
		wst2; // tu

	if (baType == BA_THROW
		|| baType == BA_AIMEDSHOT
		|| baType == BA_SNAPSHOT
		|| baType == BA_AUTOSHOT
//		|| baType == BA_LAUNCH
		|| baType == BA_HIT)
	{
		int acu;
		if (baType == BA_THROW)
			acu = static_cast<int>(Round(_action->actor->getThrowingAccuracy() * 100.));
		else
			acu = static_cast<int>(Round(_action->actor->getFiringAccuracy(
																		baType,
																		_action->weapon) * 100.));

//		wst1 = tr("STR_ACCURACY_SHORT").arg(Text::formatPercentage(acu));
		wst1 = tr("STR_ACCURACY_SHORT_KL").arg(acu);
	}

	const int tu = _action->actor->getActionTUs(
											baType,
											_action->weapon);
	wst2 = tr("STR_TIME_UNITS_SHORT").arg(tu);

	_menuSelect[*id]->setAction(
							baType,
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

	size_t btnID = MENU_ITEMS;

	for (size_t // find out which button was pressed
			i = 0;
			i != MENU_ITEMS; //sizeof(_menuSelect) / sizeof(_menuSelect[0])
			++i)
	{
		if (action->getSender() == _menuSelect[i])
		{
			btnID = i;
			break;
		}
	}

	if (btnID != MENU_ITEMS)
	{
		const RuleItem* const itRule = _action->weapon->getRules();

		_action->TU = _menuSelect[btnID]->getTUs();
		_action->type = _menuSelect[btnID]->getAction();

		if (_action->type != BA_THROW
			&& _game->getSavedGame()->getSavedBattle()->getDepth() == 0
			&& itRule->isWaterOnly() == true)
		{
			_action->result = "STR_THIS_EQUIPMENT_WILL_NOT_FUNCTION_ABOVE_WATER";
			_game->popState();
		}
		else if (_action->type == BA_PRIME)
		{
			if (itRule->getBattleType() == BT_PROXIMITYGRENADE)
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
		else if (_action->type == BA_DEFUSE)
		{
			_action->value = -1;
			_game->popState();
		}
		else if (_action->type == BA_DROP)
		{
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
		}
		else if (_action->type == BA_USE
			&& itRule->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit* targetUnit = NULL;

			const std::vector<BattleUnit*>* const units = _game->getSavedGame()->getSavedBattle()->getUnits();
			for (std::vector<BattleUnit*>::const_iterator
					i = units->begin();
					i != units->end()
						&& targetUnit == NULL;
					++i)
			{
				if ((*i)->getStatus() == STATUS_UNCONSCIOUS
					&& (*i)->isFearable() == true
//					&& (*i)->isWoundable() == true
					&& (*i)->getPosition() == _action->actor->getPosition())
//					&& *i != _action->actor // note: if unconscious, won't be actor.
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
																						&_action->target,
																						false))
			{
				const Tile* const tile = _game->getSavedGame()->getSavedBattle()->getTile(_action->target);
				if (tile != NULL
					&& tile->getUnit() != NULL
					&& tile->getUnit()->isFearable() == true)
//					&& tile->getUnit()->isWoundable() == true)
				{
					targetUnit = tile->getUnit();
				}
			}

			if (targetUnit == NULL) // kL: if no target TargetSelf.
				targetUnit = _action->actor;

			_game->popState();
			_game->pushState(new MedikitState(
											targetUnit,
											_action));
/*			if (targetUnit != NULL)
			{
				_game->popState();
				_game->pushState(new MedikitState(
												targetUnit,
												_action));
			}
			else // note This will never run due to TargetSelf.
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
				_game->popState();
			} */
		}
		else if (_action->type == BA_USE
			&& itRule->getBattleType() == BT_SCANNER)
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
		else if (_action->type == BA_LAUNCH)
		{
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
		}
		else if (_action->type == BA_HIT)
		{
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
