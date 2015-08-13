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

#include "Pathfinding.h"
#include "TileEngine.h"
#include "Map.h"

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
#include "../Savegame/BattleUnit.h"
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
	_txtTitle	= new Text(200, 9, 60, 58);

	_lstTarget	= new TextList(234, 33, 42, 80);

	_btnCancel	= new TextButton(120, 14, 100, 125);

	setPalette("PAL_BATTLESCAPE");
//	_game->getRuleset()->getInterface("soldierList")->getElement("palette")->color);

	add(_window,	"messageWindowBorder",	"battlescape");
	add(_txtTitle,	"messageWindows",		"battlescape");
	add(_lstTarget);//,	"messageWindows",		"battlescape");
	add(_btnCancel,	"messageWindowButtons",	"battlescape");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));
	_window->setHighContrast();

	_txtTitle->setText(tr("STR_EXECUTE"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setHighContrast();

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

	BattleUnit* bu;

	const SavedBattleGame* const battleSave = _game->getSavedGame()->getSavedBattle();
	Tile* tile = battleSave->getTile(_action->actor->getPosition());

	for (std::vector<BattleItem*>::const_iterator
			i = tile->getInventory()->begin();
			i != tile->getInventory()->end();
			++i)
	{
		bu = (*i)->getUnit();
		if (bu != NULL
			&& bu->getStatus() == STATUS_UNCONSCIOUS)
		{
			_targetUnits.push_back(bu);
			_lstTarget->addRow(
							1,
							bu->getName(_game->getLanguage()).c_str());
		}
	}

	if ((tile = getTargetTile()) != NULL)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = tile->getInventory()->begin();
				i != tile->getInventory()->end();
				++i)
		{
			bu = (*i)->getUnit();
			if (bu != NULL
				&& bu->getStatus() == STATUS_UNCONSCIOUS)
			{
				_targetUnits.push_back(bu);
				_lstTarget->addRow(
								1,
								bu->getName(_game->getLanguage()).c_str());
			}
		}
	}
}

/**
 * Gets a tile with an unconscious unit in valid range.
 * @return, pointer to a Tile
 */
Tile* ExecuteState::getTargetTile() // private. // TODO: duplicate of ActionMenuState::hasUnconscious()
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
						return tileTarget;
					}
				}
			}
		}
	}

	return NULL;
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
			int sound = -1;

			if (itRule->getBattleType() == BT_MELEE)
			{
				sound = amRule->getMeleeSound();
				if (sound == -1)
				{
					sound = itRule->getMeleeSound();
					if (sound == -1)
						sound = ResourcePack::ITEM_THROW;
				}
			}
			else
			{
				sound = amRule->getFireSound();
				if (sound == -1)
					sound = itRule->getFireSound();
			}

			if (sound != -1)
				_game->getResourcePack()->getSound(
												"BATTLE.CAT",
												sound)
											->play(
												-1,
												_game->getSavedGame()->getSavedBattle()->getBattleGame()->getMap()
													->getSoundAngle(_action->actor->getPosition()));
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
