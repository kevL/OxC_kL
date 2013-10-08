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

#include "ActionMenuState.h"
#include <sstream>
#include <cmath>
#include "../Engine/Game.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Action.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Ruleset/RuleItem.h"
#include "ActionMenuItem.h"
#include "PrimeGrenadeState.h"
#include "MedikitState.h"
#include "ScannerState.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "../Interface/Text.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Action Menu window.
 * @param game Pointer to the core game.
 * @param action Pointer to the action.
 * @param x Position on the x-axis.
 * @param y position on the y-axis.
 */
ActionMenuState::ActionMenuState(Game* game, BattleAction* action, int x, int y)
	:
	State(game),
	_action(action)
{
	_screen = false;

	for (int i = 0; i < 6; ++i)
	{
		_actionMenu[i] = new ActionMenuItem(i, _game->getResourcePack()->getFont("FONT_BIG"), _game->getResourcePack()->getFont("FONT_SMALL"), x, y);
		add(_actionMenu[i]);
		_actionMenu[i]->setVisible(false);
		_actionMenu[i]->onMouseClick((ActionHandler)&ActionMenuState::btnActionMenuItemClick);
	}

	// Build up the popup menu
	int id = 0;
	RuleItem* weapon = _action->weapon->getRules();

	if (!weapon->isFixed()) // throwing (if not a fixed weapon)
	{
		addItem(BA_THROW, "STR_THROW", &id);
	}

	if ((weapon->getBattleType() == BT_GRENADE
			|| weapon->getBattleType() == BT_PROXIMITYGRENADE)
		&& _action->weapon->getExplodeTurn() == 0) // priming
	{
		addItem(BA_PRIME, "STR_PRIME_GRENADE", &id);
	}

	if (weapon->getBattleType() == BT_FIREARM)
	{
		if (weapon->isWaypoint()	// kL_note: removing this might muck w/ hovertank-fusion actionMenu.
			|| (_action->weapon->getAmmoItem() && _action->weapon->getAmmoItem()->getRules()->isWaypoint()))
//		if (_action->weapon->getAmmoItem() && _action->weapon->getAmmoItem()->getRules()->isWaypoint())		// kL: This really screws BL; turns launch to scope.
		{
			addItem(BA_LAUNCH, "STR_LAUNCH_MISSILE", &id);
		}
//kL		else
		if (_action->weapon->getAmmoItem())		// kL
		{
			if (weapon->getAccuracyAuto() != 0)
			{
				addItem(BA_AUTOSHOT, "STR_AUTO_SHOT", &id);
			}
			if (weapon->getAccuracySnap() != 0)
			{
				addItem(BA_SNAPSHOT, "STR_SNAP_SHOT", &id);
			}
			if (weapon->getAccuracyAimed() != 0)
			{
				addItem(BA_AIMEDSHOT, "STR_AIMED_SHOT", &id);
			}
		}
	}
	else if (weapon->getBattleType() == BT_MELEE)
	{
		if (weapon->getDamageType() == DT_STUN)		// stun rod
		{
			addItem(BA_HIT, "STR_STUN", &id);
		}
		else
		{
			addItem(BA_HIT, "STR_HIT_MELEE", &id);	// melee weapon
		}
	}
	/** special items */
	else if (weapon->getBattleType() == BT_MEDIKIT)
	{
		addItem(BA_USE, "STR_USE_MEDI_KIT", &id);
	}
	else if (weapon->getBattleType() == BT_SCANNER)
	{
		addItem(BA_USE, "STR_USE_SCANNER", &id);
	}
	else if (weapon->getBattleType() == BT_PSIAMP && _action->actor->getStats()->psiSkill > 0)
	{
		addItem(BA_MINDCONTROL, "STR_MIND_CONTROL", &id);
		addItem(BA_PANIC, "STR_PANIC_UNIT", &id);
	}
	else if (weapon->getBattleType() == BT_MINDPROBE)
	{
		addItem(BA_USE, "STR_USE_MIND_PROBE", &id);
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
 * @param ba Action type.
 * @param name Action description.
 * @param id Pointer to the new item ID.
 */
void ActionMenuState::addItem(BattleActionType ba, const std::string& name, int* id)
{
	std::wstring s1, s2;
	int acc = (int)floor(_action->actor->getFiringAccuracy(ba, _action->weapon) * 100);

	if (ba == BA_THROW)
		acc = (int)floor(_action->actor->getThrowingAccuracy() * 100);

	int tu = _action->actor->getActionTUs(ba, _action->weapon);

	if (ba == BA_THROW
		|| ba == BA_AIMEDSHOT
		|| ba == BA_SNAPSHOT
		|| ba == BA_AUTOSHOT
//kL		|| ba == BA_LAUNCH
		|| ba == BA_HIT)
	{
//kL		s1 = tr("STR_ACC").arg(Text::formatPercentage(acc));
		s1 = tr("STR_ACC".arg(acc));		// kL
	}

	s2 = tr("STR_TUS").arg(tu);
	_actionMenu[*id]->setAction(ba, tr(name), s1, s2, tu);
	_actionMenu[*id]->setVisible(true);

	(*id)++; // kL_note: huh.
}

/**
 * Closes the window on right-click.
 * @param action Pointer to an action.
 */
void ActionMenuState::handle(Action* action)
{
	State::handle(action);
//kL	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN
//kL		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)		// kL
	{
		_game->popState();
	}
}

/**
 * Executes the action corresponding to this action menu item.
 * @param action Pointer to an action.
 */
void ActionMenuState::btnActionMenuItemClick(Action* action)
{
	_game->getSavedGame()->getSavedBattle()->getPathfinding()->removePreview();

	int btnID = -1;
	RuleItem* weapon = _action->weapon->getRules();

	// got to find out which button was pressed
	for (size_t i = 0; i < sizeof(_actionMenu) / sizeof(_actionMenu[0]) && btnID == -1; ++i)
	{
		if (action->getSender() == _actionMenu[i])
		{
			btnID = i;
		}
	}

	if (btnID != -1)
	{
		_action->TU = _actionMenu[btnID]->getTUs();

		_action->type = _actionMenu[btnID]->getAction();
		if (_action->type == BA_PRIME)
		{
			if (weapon->getBattleType() == BT_PROXIMITYGRENADE)
			{
				_action->value = 1;
				_game->popState();
			}
			else
			{
				_game->pushState(new PrimeGrenadeState(_game, _action, false, 0));
			}
		}
		else if (_action->type == BA_USE
			&& weapon->getBattleType() == BT_MEDIKIT)
		{
			BattleUnit* targetUnit = NULL;

			std::vector<BattleUnit* >* const units (_game->getSavedGame()->getSavedBattle()->getUnits());
			for (std::vector<BattleUnit* >::const_iterator i = units->begin (); i != units->end () && !targetUnit; ++i)
			{
				// we can heal a unit that is at the same position, unconscious and healable(=woundable)
				if ((*i)->getPosition() == _action->actor->getPosition()
					&& *i != _action->actor
					&& (*i)->getStatus () == STATUS_UNCONSCIOUS
					&& (*i)->isWoundable())
				{
					targetUnit = *i;
				}
			}

			if (!targetUnit)
			{
				Position p;
				Pathfinding::directionToVector(_action->actor->getDirection(), &p);

				Tile* tile(_game->getSavedGame()->getSavedBattle()->getTile(_action->actor->getPosition() + p));
				if (tile != 0
					&& tile->getUnit()
					&& tile->getUnit()->isWoundable())
				{
					targetUnit = tile->getUnit();
				}
			}

			if (targetUnit)
			{
				_game->popState();
				_game->pushState(new MedikitState(_game, targetUnit, _action));
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
				_game->pushState(new ScannerState(_game, _action));
			}
			else
			{
				_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
				_game->popState();
			}
		}
		else if (_action->type == BA_LAUNCH)
		{
			if (_action->TU > _action->actor->getTimeUnits()) // check beforehand if we have enough time units
			{
				_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
			}
			else if (_action->weapon->getAmmoItem() == 0
				|| (_action->weapon->getAmmoItem() && _action->weapon->getAmmoItem()->getAmmoQuantity() == 0))
			{
				_action->result = "STR_NO_AMMUNITION_LOADED";
			}
			else
			{
				_action->targeting = true;
			}

			_game->popState();
		}
		else if ((_action->type == BA_STUN || _action->type == BA_HIT)
			&& weapon->getBattleType() == BT_MELEE)
		{

			if (!_game->getSavedGame()->getSavedBattle()->getTileEngine()->validMeleeRange(_action->actor->getPosition(), _action->actor->getDirection(), _action->actor, 0))
			{
				_action->result = "STR_THERE_IS_NO_ONE_THERE";
			}

			_game->popState();
		}
		else
		{
			_action->targeting = true;

			_game->popState();
		}
	}
}

}
