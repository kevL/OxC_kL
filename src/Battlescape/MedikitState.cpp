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

#include "MedikitState.h"

#include <iostream>
#include <sstream>

#include "MedikitView.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"

#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleItem.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Helper function that returns a string representation of a type (mainly used for numbers).
 * @param t The value to stringify.
 * @return A string representation of the value.
 */
template<typename type>
std::wstring toString(type t)
{
	std::wostringstream ss;
	ss << t;

	return ss.str();
}


/**
 * Helper class for the medikit title.
 */
class MedikitTitle
	:
		public Text
{
	public:
		/// Creates a medikit title.
		MedikitTitle(
				int y,
				const std::wstring& title);
};

/**
 * Initializes a Medikit title.
 * @param y The title's y origin.
 * @param title The title.
 */
MedikitTitle::MedikitTitle(
		int y,
		const std::wstring& title)
	:
		Text(73, 9, 186, y) //(60, 16, 192, y)
{
	this->setText(title);
	this->setHighContrast(true);
	this->setAlign(ALIGN_CENTER);
}

/**
 * Helper class for the medikit value.
 */
class MedikitTxt
	:
		public Text
{
	public:
		/// Creates a medikit text.
		MedikitTxt(int y);
};

/**
 * Initializes a Medikit text.
 * @param y The text's y origin.
 */
MedikitTxt::MedikitTxt(int y)
	:
		Text(33, 17, 220, y) //(30, 22, 220, y)
{
	// Note: we can't set setBig here. The needed font is only set when added to State
	this->setColor(Palette::blockOffset(1));
	this->setHighContrast(true);
	this->setAlign(ALIGN_CENTER);
//	this->setVerticalAlign(ALIGN_MIDDLE); // kL
}

/**
 * Helper class for the medikit button.
 */
class MedikitButton
	:
		public InteractiveSurface
{
	public:
		/// Creates a medikit button.
		MedikitButton(int y);
};

/**
 * Initializes a Medikit button.
 * @param y The button's y origin.
 */
MedikitButton::MedikitButton(int y)
	:
		InteractiveSurface(30, 20, 190, y)
{
}

/**
 * Initializes the Medikit State.
 * @param game Pointer to the core game.
 * @param targetUnit The wounded unit.
 * @param action The healing action.
 */
MedikitState::MedikitState(
		BattleUnit* targetUnit,
		BattleAction* action)
	:
		_targetUnit(targetUnit),
		_action(action)
{
/*kL
	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */

	_unit = action->actor;
	_item = action->weapon;

	_bg = new Surface(320, 200);

	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);

	if (_game->getScreen()->getDY() > 50)
	{
		_screen = false;

/*		SDL_Rect current;
		current.w = 190;
		current.h = 100;
		current.x = 67;
		current.y = 44;
		_bg->drawRect(&current, Palette::blockOffset(15)+15); */
		_bg->drawRect(67, 44, 190, 100, Palette::blockOffset(15)+15);
	}

//	_partTxt	= new Text(50, 15, 90, 120);	// kL
//	_woundTxt	= new Text(10, 15, 145, 120);	// kL
	_partTxt	= new Text(62, 9, 82, 120);
	_woundTxt	= new Text(14, 9, 145, 120);
	_mediView	= new MedikitView(
								52, 58, 95, 60,
								_game,
								_targetUnit,
								_partTxt,
								_woundTxt);

	_stimButton	= new MedikitButton(84);
	_painButton	= new MedikitButton(48);
	_healButton	= new MedikitButton(120);
	_endButton	= new InteractiveSurface(20, 20, 220, 140);

//	_painText	= new MedikitTxt(50);
//	_stimTxt	= new MedikitTxt(85);
//	_healTxt	= new MedikitTxt(120);
	_painText	= new MedikitTxt(52);
	_stimTxt	= new MedikitTxt(88);
	_healTxt	= new MedikitTxt(124);

	add(_bg);
	add(_mediView);
	add(_endButton);

	add(new MedikitTitle(37, tr("STR_PAIN_KILLER")));
	add(new MedikitTitle(73, tr("STR_STIMULANT")));
	add(new MedikitTitle(109, tr("STR_HEAL")));

	add(_healButton);
	add(_stimButton);
	add(_painButton);
	add(_painText);
	add(_stimTxt);
	add(_healTxt);
	add(_partTxt);
	add(_woundTxt);

	centerAllSurfaces();

	_game->getResourcePack()->getSurface("MEDIBORD.PCK")->blit(_bg);
	_painText->setBig();
	_stimTxt->setBig();
	_healTxt->setBig();
	_partTxt->setColor(Palette::blockOffset(2));
	_partTxt->setHighContrast(true);
	_woundTxt->setColor(Palette::blockOffset(2));
	_woundTxt->setHighContrast(true);

	_endButton->onMouseClick((ActionHandler)& MedikitState::onEndClick);
	_endButton->onKeyboardPress(
					(ActionHandler)& MedikitState::onEndClick,
					Options::keyCancel);
	_healButton->onMouseClick((ActionHandler)& MedikitState::onHealClick);
	_stimButton->onMouseClick((ActionHandler)& MedikitState::onStimulantClick);
	_painButton->onMouseClick((ActionHandler)& MedikitState::onPainKillerClick);

	update();
}

/**
 * Closes the window on right-click.
 * @param action Pointer to an action.
 */
void MedikitState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		onEndClick(NULL);
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void MedikitState::onEndClick(Action*)
{
/*kL
	if (Options::maximizeInfoScreens)
	{
		Screen::updateScale(
						Options::battlescapeScale,
						Options::battlescapeScale,
						Options::baseXBattlescape,
						Options::baseYBattlescape,
						true);
		_game->getScreen()->resetDisplay(false);
	} */

	_game->popState();
}

/**
 * Handler for clicking on the heal button.
 * @param action Pointer to an action.
 */
void MedikitState::onHealClick(Action*)
{
	RuleItem* rule = _item->getRules();

	int heal = _item->getHealQuantity();
	if (heal == 0)
		return;

	if (_unit->spendTimeUnits(rule->getTUUse()))
	{
		_targetUnit->heal(
					_mediView->getSelectedPart(),
					rule->getWoundRecovery(),
					rule->getHealthRecovery());

		_item->setHealQuantity(--heal);
		_mediView->updateSelectedPart();
		_mediView->invalidate();

		update();
	}
	else
	{
		_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
//kL	onEndClick(0);
	}
}

/**
 * Handler for clicking on the stimulant button.
 * @param action Pointer to an action.
 */
void MedikitState::onStimulantClick(Action*)
{
	int stimulant = _item->getStimulantQuantity();
	if (stimulant == 0)
		return;

	RuleItem* rule = _item->getRules();
	if (_unit->spendTimeUnits(rule->getTUUse()))
	{
		_targetUnit->stimulant(
							rule->getEnergyRecovery(),
							rule->getStunRecovery());
		_item->setStimulantQuantity(--stimulant);

		update();

		// if the unit has revived we quit this screen automatically
		if (_targetUnit->getStatus() == STATUS_UNCONSCIOUS
			&& _targetUnit->getStunlevel() < _targetUnit->getHealth())
		{
			onEndClick(NULL);
		}
	}
	else
	{
		_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
//kL	onEndClick(0);
	}
}

/**
 * Handler for clicking on the pain killer button.
 * @param action Pointer to an action.
 */
void MedikitState::onPainKillerClick(Action*)
{
	int pain = _item->getPainKillerQuantity();
	if (pain == 0)
		return;

	RuleItem* rule = _item->getRules();
	if (_unit->spendTimeUnits(rule->getTUUse()))
	{
		_targetUnit->painKillers();
		_item->setPainKillerQuantity(--pain);

		update();
	}
	else
	{
		_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
//kL	onEndClick(0);
	}
}

/**
 * Updates the medikit state.
 */
void MedikitState::update()
{
	_painText->setText(toString(_item->getPainKillerQuantity()));
	_stimTxt->setText(toString(_item->getStimulantQuantity()));
	_healTxt->setText(toString(_item->getHealQuantity()));
}

}
