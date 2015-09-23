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

#include "ExecuteState.h"

#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
#include "../Engine/Sound.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleItem.h"
//#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ExecuteState screen.
 * @param action - pointer to a BattleAction (BattlescapeGame.h)
 */
ExecuteState::ExecuteState(BattleAction* const action)
	:
		_action(action)
{
	_screen = false;

	_window		= new Window(this, 270, 96, 25, 50);
	_txtTitle	= new Text(200, 17, 60, 58);
	_lstTarget	= new TextList(234, 33, 42, 80);
	_btnCancel	= new TextButton(120, 14, 100, 125);

	setPalette("PAL_BATTLESCAPE");
//	_game->getRuleset()->getInterface("soldierList")->getElement("palette")->color);

	add(_window,	"messageWindowBorder",	"battlescape");
	add(_txtTitle,	"messageWindows",		"battlescape");
	add(_lstTarget);//,	"messageWindows",	"battlescape");
	add(_btnCancel,	"messageWindowButtons",	"battlescape");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));
	_window->setHighContrast();

	_txtTitle->setText(tr("STR_EXECUTE"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setHighContrast();
	_txtTitle->setBig();

	_lstTarget->setColumns(1, 234);
	_lstTarget->setColor(Palette::blockOffset(11));
	_lstTarget->setBackground(_window);
	_lstTarget->setSelectable();
	_lstTarget->setMargin(12);
	_lstTarget->setHighContrast();
	_lstTarget->onMousePress((ActionHandler)& ExecuteState::lstTargetPress);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->setHighContrast();
	_btnCancel->onMouseClick((ActionHandler)& ExecuteState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& ExecuteState::btnCancelClick,
					Options::keyCancel);
}

/**
 * dTor.
 */
ExecuteState::~ExecuteState()
{}

/**
 * Resets the palette and adds targets to the TextList.
 */
void ExecuteState::init()
{
	State::init();

	BattleUnit* targetUnit;

	const SavedBattleGame* const battleSave = _game->getSavedGame()->getBattleSave();
	const Position pos = _action->actor->getPosition();
	Tile* tile = battleSave->getTile(pos);

	const int actorSize = _action->actor->getArmor()->getSize();
	for (int
			x = 0;
			x != actorSize;
			++x)
	{
		for (int
				y = 0;
				y != actorSize;
				++y)
		{
			tile = battleSave->getTile(pos + Position(x,y,0));
			for (std::vector<BattleItem*>::const_iterator
					i = tile->getInventory()->begin();
					i != tile->getInventory()->end();
					++i)
			{
				if ((targetUnit = (*i)->getUnit()) != NULL
					&& targetUnit->getUnitStatus() == STATUS_UNCONSCIOUS)
				{
					_targetUnits.push_back(targetUnit);
					_lstTarget->addRow(
									1,
									targetUnit->getName(_game->getLanguage()).c_str());
				}
			}
		}
	}

	if ((tile = battleSave->getTileEngine()->getExecutionTile(_action->actor)) != NULL)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = tile->getInventory()->begin();
				i != tile->getInventory()->end();
				++i)
		{
			if ((targetUnit = (*i)->getUnit()) != NULL
				&& targetUnit->getUnitStatus() == STATUS_UNCONSCIOUS)
			{
				_targetUnits.push_back(targetUnit);
				_lstTarget->addRow(
								1,
								targetUnit->getName(_game->getLanguage()).c_str());
			}
		}
	}
}

/**
 * Chooses a unit to apply execution to.
 * @param action - pointer to an Action
 */
void ExecuteState::lstTargetPress(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_action->TU > _action->actor->getTimeUnits())
			_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
		else
		{
			BattleUnit* const targetUnit = _targetUnits[_lstTarget->getSelectedRow()];
			_action->target = targetUnit->getPosition(); // jic.
			_action->targetUnit = targetUnit;

			const RuleItem
				* const itRule = _action->weapon->getRules(),
				* const amRule = _action->weapon->getAmmoItem()->getRules();
			int soundId = -1;

			if (itRule->getBattleType() == BT_MELEE)
			{
				soundId = amRule->getMeleeSound();
				if (soundId == -1)
				{
					soundId = itRule->getMeleeSound();
					if (soundId == -1)
						soundId = ResourcePack::ITEM_THROW;
				}
			}
			else
			{
				soundId = amRule->getFireSound();
				if (soundId == -1)
					soundId = itRule->getFireSound();
			}

			if (soundId != -1)
			{
				const Map* const battleMap = _game->getSavedGame()->getBattleSave()->getBattleGame()->getMap();
				_game->getResourcePack()->getSound("BATTLE.CAT", soundId)
											->play(-1, battleMap->getSoundAngle(_action->actor->getPosition()));
			}
		}

		_game->popState();
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ExecuteState::btnCancelClick(Action*)
{
	_game->popState();
}

}
