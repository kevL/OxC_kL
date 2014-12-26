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

//#include <iostream>
//#include <sstream>

#include "MedikitView.h"

#include "../Engine/Action.h"
//#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"

#include "../Interface/Bar.h"
#include "../Interface/NumberText.h"
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
 * @param t - the value to stringify
 * @return, a string representation of the value
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
 * @param y		- the title's y origin
 * @param title	- reference the title
 */
MedikitTitle::MedikitTitle(
		int y,
		const std::wstring& title)
	:
		Text(73, 9, 186, y)
{
	this->setText(title);
	this->setHighContrast();
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
 * @param y - the text's y origin
 */
MedikitTxt::MedikitTxt(int y)
	:
		Text(33, 17, 220, y)
{
	// Note: we can't set setBig here. The needed font is only set when added to State
	this->setColor(Palette::blockOffset(1));
	this->setHighContrast();
	this->setAlign(ALIGN_CENTER);
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
 * @param y - the button's y origin
 */
MedikitButton::MedikitButton(int y)
	:
		InteractiveSurface(25, 21, 192, y)
{}

/**
 * Initializes the Medikit State.
 * @param targetUnit	- pointer to a wounded BattleUnit
 * @param action		- pointer to BattleAction (BattlescapeGame.h)
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
		_bg->drawRect(67, 44, 190, 100, Palette::blockOffset(15)+15);
	}

	_partTxt	= new Text(16, 9, 89, 120);
	_woundTxt	= new Text(16, 9, 145, 120);
	_mediView	= new MedikitView(
								52, 58, 95, 60,
								_game,
								_targetUnit,
								_partTxt,
								_woundTxt);

	_stimButton	= new MedikitButton(84);
	_painButton	= new MedikitButton(48);
	_healButton	= new MedikitButton(120);
	_endButton	= new InteractiveSurface(7, 7, 222, 148);

	_painText	= new MedikitTxt(51);
	_stimTxt	= new MedikitTxt(87);
	_healTxt	= new MedikitTxt(123);

	// kL_begin:
/*	_numTimeUnits	= new NumberText(15, 5, 60, 12); // add x+32 to center these.
	_barTimeUnits	= new Bar(102, 3, 94, 12);

	_numEnergy		= new NumberText(15, 5, 78, 12);
	_barEnergy		= new Bar(102, 3, 94, 16);

	_numHealth		= new NumberText(15, 5, 60, 20);
	_barHealth		= new Bar(102, 3, 94, 20);

	_numMorale		= new NumberText(15, 5, 78, 20);
	_barMorale		= new Bar(102, 3, 94, 24); */

	_numHealth		= new NumberText(15, 5, 90, 8);
	_numStun		= new NumberText(15, 5, 105, 8);
	_barHealth		= new Bar(102, 3, 120, 9);

	_numEnergy		= new NumberText(15, 5, 90, 15);
	_barEnergy		= new Bar(102, 3, 120, 16);

	_numMorale		= new NumberText(15, 5, 90, 22);
	_barMorale		= new Bar(102, 3, 120, 23);

	_numTimeUnits	= new NumberText(15, 5, 90, 164); // add x+32 to center these.
	_barTimeUnits	= new Bar(102, 3, 120, 165);


	add(_numHealth);
	add(_numStun);
	add(_numEnergy);
	add(_numMorale);
	add(_numTimeUnits);

	add(_barHealth);
	add(_barEnergy);
	add(_barMorale);
	add(_barTimeUnits);

	_numHealth->setColor(Palette::blockOffset(2)+2);
	_numStun->setColor(Palette::blockOffset(0)+1);
	_numEnergy->setColor(Palette::blockOffset(1));
	_numMorale->setColor(Palette::blockOffset(12));
	_numTimeUnits->setColor(Palette::blockOffset(4));

	_barHealth->setColor(Palette::blockOffset(2)+2);
	_barHealth->setSecondaryColor(Palette::blockOffset(5)+2);
	_barHealth->setScale();
	_barEnergy->setColor(Palette::blockOffset(1));
	_barEnergy->setScale();
	_barMorale->setColor(Palette::blockOffset(12));
	_barMorale->setScale();
	_barTimeUnits->setColor(Palette::blockOffset(4));
	_barTimeUnits->setScale();
	// kL_end.

	add(_bg);
	add(_mediView, "body", "medikit", _bg);
	add(_endButton);

	add(new MedikitTitle(36, tr("STR_PAIN_KILLER")), "textPK", "medikit", _bg);
	add(new MedikitTitle(72, tr("STR_STIMULANT")), "textStim", "medikit", _bg);
	add(new MedikitTitle(108, tr("STR_HEAL")), "textHeal", "medikit", _bg);

	add(_healButton, "buttonHeal", "medikit", _bg);
	add(_stimButton, "buttonStim", "medikit", _bg);
	add(_painButton, "buttonPK", "medikit", _bg);
	add(_painText, "numPK", "medikit", _bg);
	add(_stimTxt, "numStim", "medikit", _bg);
	add(_healTxt, "numHeal", "medikit", _bg);
	add(_partTxt, "textPart", "medikit", _bg);
	add(_woundTxt, "numWounds", "medikit", _bg);

	centerAllSurfaces();


	_game->getResourcePack()->getSurface("MEDIBORD.PCK")->blit(_bg);

	_painText->setBig();
	_stimTxt->setBig();
	_healTxt->setBig();

//	_partTxt->setColor(Palette::blockOffset(2));
	_partTxt->setHighContrast();

//	_woundTxt->setColor(Palette::blockOffset(2));
	_woundTxt->setHighContrast();

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
 * @param action - pointer to an Action
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
 * @param action - pointer to an Action
 */
void MedikitState::onEndClick(Action*)
{
/* 	if (Options::maximizeInfoScreens)
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
 * @param action - pointer to an Action
 */
void MedikitState::onHealClick(Action*)
{
	int heal = _item->getHealQuantity();
	if (heal == 0)
		return;

	const RuleItem* const rule = _item->getRules();
	if (_unit->spendTimeUnits(rule->getTUUse()) == true)
	{
		++_unit->getStatistics()->medikitApplications;

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
//		onEndClick(0);
	}
}

/**
 * Handler for clicking on the stimulant button.
 * @param action - pointer to an Action
 */
void MedikitState::onStimulantClick(Action*)
{
	int stimulant = _item->getStimulantQuantity();
	if (stimulant == 0)
		return;

	const RuleItem* const rule = _item->getRules();
	if (_unit->spendTimeUnits(rule->getTUUse()) == true)
	{
		++_unit->getStatistics()->medikitApplications;

		_targetUnit->stimulant(
							rule->getEnergyRecovery(),
							rule->getStunRecovery());
		_item->setStimulantQuantity(--stimulant);

		update();

		// if the unit has revived we quit this screen automatically
		if (_targetUnit->getStatus() == STATUS_UNCONSCIOUS
			&& _targetUnit->getStun() < _targetUnit->getHealth())
		{
			onEndClick(NULL);
		}
	}
	else
	{
		_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
//		onEndClick(0);
	}
}

/**
 * Handler for clicking on the pain killer button.
 * @param action - pointer to an Action
 */
void MedikitState::onPainKillerClick(Action*)
{
	int pain = _item->getPainKillerQuantity();
	if (pain == 0)
		return;

	const RuleItem* const rule = _item->getRules();
	if (_unit->spendTimeUnits(rule->getTUUse()) == true)
	{
		++_unit->getStatistics()->medikitApplications;

		_targetUnit->painKillers();
		_item->setPainKillerQuantity(--pain);

		update();
	}
	else
	{
		_action->result = "STR_NOT_ENOUGH_TIME_UNITS";
//		onEndClick(0);
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

	// kL_begin:
	double stat = static_cast<double>(_targetUnit->getBaseStats()->health);
	const int health = _targetUnit->getHealth();
	_numHealth->setValue(static_cast<unsigned>(health));
	_numStun->setValue(static_cast<unsigned>(_targetUnit->getStun()));
	_barHealth->setMax(100.0);
	_barHealth->setValue(ceil(
							static_cast<double>(health) / stat * 100.0));
	_barHealth->setValue2(ceil(
							static_cast<double>(_targetUnit->getStun()) / stat * 100.0));

	stat = static_cast<double>(_targetUnit->getBaseStats()->stamina); // stats of the recipient
	const int energy = _targetUnit->getEnergy();
	_numEnergy->setValue(static_cast<unsigned>(energy));
	_barEnergy->setMax(100.0);
	_barEnergy->setValue(ceil(
							static_cast<double>(energy) / stat * 100.0));

	const int morale = _targetUnit->getMorale();
	_numMorale->setValue(static_cast<unsigned>(morale));
	_barMorale->setMax(100.0);
	_barMorale->setValue(morale);

	stat = static_cast<double>(_unit->getBaseStats()->tu); // TU of the MedKit user
	const int tu = _unit->getTimeUnits();
	_numTimeUnits->setValue(static_cast<unsigned>(tu));
	_barTimeUnits->setMax(100.0);
	_barTimeUnits->setValue(ceil(
							static_cast<double>(tu) / stat * 100.0));
	// kL_end.
}

}
