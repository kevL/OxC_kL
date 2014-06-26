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

#include "UnitInfoState.h"

#include <sstream>

#include "BattlescapeGame.h"
#include "BattlescapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
//kL #include "../Engine/Screen.h"

#include "../Interface/Bar.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/Unit.h"

#include "../Savegame/Base.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Unit Info screen.
 * @param game			- Pointer to the core game.
 * @param unit			- Pointer to the selected unit.
 * @param parent		- Pointer to parent Battlescape.
 * @param fromInventory	- Is player coming from the inventory?
 * @param mindProbe		- Is player using a Mind Probe?
 */
UnitInfoState::UnitInfoState(
		BattleUnit* unit,
		BattlescapeState* parent,
		bool fromInventory,
		bool mindProbe)
	:
		_unit(unit),
		_parent(parent),
		_fromInventory(fromInventory),
		_mindProbe(mindProbe)
{
/*kL
	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */

	_battleGame = _game->getSavedGame()->getSavedBattle();

	_bg			= new Surface(320, 200, 0, 0);
	_exit		= new InteractiveSurface(320, 180, 0, 20);
	_txtName	= new Text(288, 17, 16, 4);

	int
		step = 9,
		yPos = 38;

	_txtTimeUnits	= new Text(140, 9, 8, yPos);
	_numTimeUnits	= new Text(18, 9, 151, yPos);
	_barTimeUnits	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtEnergy		= new Text(140, 9, 8, yPos);
	_numEnergy		= new Text(18, 9, 151, yPos);
	_barEnergy		= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtHealth		= new Text(140, 9, 8, yPos);
	_numHealth		= new Text(18, 9, 151, yPos);
	_barHealth		= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtFatalWounds	= new Text(140, 9, 8, yPos);
	_numFatalWounds	= new Text(18, 9, 151, yPos);
	_barFatalWounds	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtBravery		= new Text(140, 9, 8, yPos);
	_numBravery		= new Text(18, 9, 151, yPos);
	_barBravery		= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtMorale		= new Text(140, 9, 8, yPos);
	_numMorale		= new Text(18, 9, 151, yPos);
	_barMorale		= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtReactions	= new Text(140, 9, 8, yPos);
	_numReactions	= new Text(18, 9, 151, yPos);
	_barReactions	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtFiring		= new Text(140, 9, 8, yPos);
	_numFiring		= new Text(18, 9, 151, yPos);
	_barFiring		= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtThrowing	= new Text(140, 9, 8, yPos);
	_numThrowing	= new Text(18, 9, 151, yPos);
	_barThrowing	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtMelee		= new Text(140, 9, 8, yPos);
	_numMelee		= new Text(18, 9, 151, yPos);
	_barMelee		= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtStrength	= new Text(140, 9, 8, yPos);
	_numStrength	= new Text(18, 9, 151, yPos);
	_barStrength	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtPsiStrength	= new Text(140, 9, 8, yPos);
	_numPsiStrength	= new Text(18, 9, 151, yPos);
	_barPsiStrength	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtPsiSkill	= new Text(140, 9, 8, yPos);
	_numPsiSkill	= new Text(18, 9, 151, yPos);
	_barPsiSkill	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtFrontArmor	= new Text(140, 9, 8, yPos);
	_numFrontArmor	= new Text(18, 9, 151, yPos);
	_barFrontArmor	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtLeftArmor	= new Text(140, 9, 8, yPos);
	_numLeftArmor	= new Text(18, 9, 151, yPos);
	_barLeftArmor	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtRightArmor	= new Text(140, 9, 8, yPos);
	_numRightArmor	= new Text(18, 9, 151, yPos);
	_barRightArmor	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtRearArmor	= new Text(140, 9, 8, yPos);
	_numRearArmor	= new Text(18, 9, 151, yPos);
	_barRearArmor	= new Bar(170, 5, 170, yPos + 1);

	yPos += step;
	_txtUnderArmor	= new Text(140, 9, 8, yPos);
	_numUnderArmor	= new Text(18, 9, 151, yPos);
	_barUnderArmor	= new Bar(170, 5, 170, yPos + 1);

	if (!_mindProbe)
	{
		_btnPrev	= new TextButton(17, 18, 2, 2);
		_btnNext	= new TextButton(17, 18, 301, 2);
	}

	setPalette("PAL_BATTLESCAPE");

	add(_bg);
	add(_exit);
	add(_txtName);

	add(_txtTimeUnits);
	add(_numTimeUnits);
	add(_barTimeUnits);

	add(_txtEnergy);
	add(_numEnergy);
	add(_barEnergy);

	add(_txtHealth);
	add(_numHealth);
	add(_barHealth);

	add(_txtFatalWounds);
	add(_numFatalWounds);
	add(_barFatalWounds);

	add(_txtBravery);
	add(_numBravery);
	add(_barBravery);

	add(_txtMorale);
	add(_numMorale);
	add(_barMorale);

	add(_txtReactions);
	add(_numReactions);
	add(_barReactions);

	add(_txtFiring);
	add(_numFiring);
	add(_barFiring);

	add(_txtThrowing);
	add(_numThrowing);
	add(_barThrowing);

	add(_txtMelee);
	add(_numMelee);
	add(_barMelee);

	add(_txtStrength);
	add(_numStrength);
	add(_barStrength);

	add(_txtPsiStrength);
	add(_numPsiStrength);
	add(_barPsiStrength);

	add(_txtPsiSkill);
	add(_numPsiSkill);
	add(_barPsiSkill);

	add(_txtFrontArmor);
	add(_numFrontArmor);
	add(_barFrontArmor);

	add(_txtLeftArmor);
	add(_numLeftArmor);
	add(_barLeftArmor);

	add(_txtRightArmor);
	add(_numRightArmor);
	add(_barRightArmor);

	add(_txtRearArmor);
	add(_numRearArmor);
	add(_barRearArmor);

	add(_txtUnderArmor);
	add(_numUnderArmor);
	add(_barUnderArmor);

	if (!_mindProbe)
	{
		add(_btnPrev);
		add(_btnNext);
	}

	centerAllSurfaces();

	_game->getResourcePack()->getSurface("UNIBORD.PCK")->blit(_bg);

	_exit->onMouseClick((ActionHandler)& UnitInfoState::exitClick);
	_exit->onKeyboardPress(
					(ActionHandler)& UnitInfoState::exitClick,
					Options::keyCancel);
	_exit->onKeyboardPress(
					(ActionHandler)& UnitInfoState::exitClick,
					Options::keyBattleStats);

	_txtName->setAlign(ALIGN_CENTER);
	_txtName->setBig();
	_txtName->setColor(Palette::blockOffset(4));
	_txtName->setHighContrast(true);


	_txtTimeUnits->setColor(Palette::blockOffset(3));
	_txtTimeUnits->setHighContrast(true);
	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));

	_numTimeUnits->setColor(Palette::blockOffset(9));
	_numTimeUnits->setHighContrast(true);

	_barTimeUnits->setColor(Palette::blockOffset(4));
	_barTimeUnits->setScale(1.0);


	_txtEnergy->setColor(Palette::blockOffset(3));
	_txtEnergy->setHighContrast(true);
	_txtEnergy->setText(tr("STR_ENERGY"));

	_numEnergy->setColor(Palette::blockOffset(9));
	_numEnergy->setHighContrast(true);

	_barEnergy->setColor(Palette::blockOffset(9));
	_barEnergy->setScale(1.0);


	_txtHealth->setColor(Palette::blockOffset(3));
	_txtHealth->setHighContrast(true);
	_txtHealth->setText(tr("STR_HEALTH"));

	_numHealth->setColor(Palette::blockOffset(9));
	_numHealth->setHighContrast(true);

	_barHealth->setColor(Palette::blockOffset(2));
	_barHealth->setColor2(Palette::blockOffset(5)+2);
	_barHealth->setScale(1.0);


	_txtFatalWounds->setColor(Palette::blockOffset(3));
	_txtFatalWounds->setHighContrast(true);
	_txtFatalWounds->setText(tr("STR_FATAL_WOUNDS"));

	_numFatalWounds->setColor(Palette::blockOffset(9));
	_numFatalWounds->setHighContrast(true);

	_barFatalWounds->setColor(Palette::blockOffset(2));
	_barFatalWounds->setScale(1.0);


	_txtBravery->setColor(Palette::blockOffset(3));
	_txtBravery->setHighContrast(true);
	_txtBravery->setText(tr("STR_BRAVERY"));

	_numBravery->setColor(Palette::blockOffset(9));
	_numBravery->setHighContrast(true);

	_barBravery->setColor(Palette::blockOffset(12));
	_barBravery->setScale(1.0);


	_txtMorale->setColor(Palette::blockOffset(3));
	_txtMorale->setHighContrast(true);
	_txtMorale->setText(tr("STR_MORALE"));

	_numMorale->setColor(Palette::blockOffset(9));
	_numMorale->setHighContrast(true);

	_barMorale->setColor(Palette::blockOffset(12));
	_barMorale->setScale(1.0);


	_txtReactions->setColor(Palette::blockOffset(3));
	_txtReactions->setHighContrast(true);
	_txtReactions->setText(tr("STR_REACTIONS"));

	_numReactions->setColor(Palette::blockOffset(9));
	_numReactions->setHighContrast(true);

	_barReactions->setColor(Palette::blockOffset(9));
	_barReactions->setScale(1.0);


	_txtFiring->setColor(Palette::blockOffset(3));
	_txtFiring->setHighContrast(true);
	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));

	_numFiring->setColor(Palette::blockOffset(9));
	_numFiring->setHighContrast(true);

	_barFiring->setColor(Palette::blockOffset(8));
	_barFiring->setScale(1.0);


	_txtThrowing->setColor(Palette::blockOffset(3));
	_txtThrowing->setHighContrast(true);
	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));

	_numThrowing->setColor(Palette::blockOffset(9));
	_numThrowing->setHighContrast(true);

	_barThrowing->setColor(Palette::blockOffset(6));
	_barThrowing->setScale(1.0);


	_txtMelee->setColor(Palette::blockOffset(3));
	_txtMelee->setHighContrast(true);
	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));

	_numMelee->setColor(Palette::blockOffset(9));
	_numMelee->setHighContrast(true);

	_barMelee->setColor(Palette::blockOffset(14));
	_barMelee->setScale(1.0);


	_txtStrength->setColor(Palette::blockOffset(3));
	_txtStrength->setHighContrast(true);
	_txtStrength->setText(tr("STR_STRENGTH"));

	_numStrength->setColor(Palette::blockOffset(9));
	_numStrength->setHighContrast(true);

	_barStrength->setColor(Palette::blockOffset(3));
	_barStrength->setScale(1.0);


	_txtPsiStrength->setColor(Palette::blockOffset(3));
	_txtPsiStrength->setHighContrast(true);
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));

	_numPsiStrength->setColor(Palette::blockOffset(9));
	_numPsiStrength->setHighContrast(true);

	_barPsiStrength->setColor(Palette::blockOffset(12));
	_barPsiStrength->setScale(1.0);


	_txtPsiSkill->setColor(Palette::blockOffset(3));
	_txtPsiSkill->setHighContrast(true);
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));

	_numPsiSkill->setColor(Palette::blockOffset(9));
	_numPsiSkill->setHighContrast(true);

	_barPsiSkill->setColor(Palette::blockOffset(12));
	_barPsiSkill->setScale(1.0);


	_txtFrontArmor->setColor(Palette::blockOffset(3));
	_txtFrontArmor->setHighContrast(true);
	_txtFrontArmor->setText(tr("STR_FRONT_ARMOR_UC"));

	_numFrontArmor->setColor(Palette::blockOffset(9));
	_numFrontArmor->setHighContrast(true);

	_barFrontArmor->setColor(Palette::blockOffset(5));
	_barFrontArmor->setScale(1.0);


	_txtLeftArmor->setColor(Palette::blockOffset(3));
	_txtLeftArmor->setHighContrast(true);
	_txtLeftArmor->setText(tr("STR_LEFT_ARMOR_UC"));

	_numLeftArmor->setColor(Palette::blockOffset(9));
	_numLeftArmor->setHighContrast(true);

	_barLeftArmor->setColor(Palette::blockOffset(5));
	_barLeftArmor->setScale(1.0);


	_txtRightArmor->setColor(Palette::blockOffset(3));
	_txtRightArmor->setHighContrast(true);
	_txtRightArmor->setText(tr("STR_RIGHT_ARMOR_UC"));

	_numRightArmor->setColor(Palette::blockOffset(9));
	_numRightArmor->setHighContrast(true);

	_barRightArmor->setColor(Palette::blockOffset(5));
	_barRightArmor->setScale(1.0);


	_txtRearArmor->setColor(Palette::blockOffset(3));
	_txtRearArmor->setHighContrast(true);
	_txtRearArmor->setText(tr("STR_REAR_ARMOR_UC"));

	_numRearArmor->setColor(Palette::blockOffset(9));
	_numRearArmor->setHighContrast(true);

	_barRearArmor->setColor(Palette::blockOffset(5));
	_barRearArmor->setScale(1.0);


	_txtUnderArmor->setColor(Palette::blockOffset(3));
	_txtUnderArmor->setHighContrast(true);
	_txtUnderArmor->setText(tr("STR_UNDER_ARMOR_UC"));

	_numUnderArmor->setColor(Palette::blockOffset(9));
	_numUnderArmor->setHighContrast(true);

	_barUnderArmor->setColor(Palette::blockOffset(5));
	_barUnderArmor->setScale(1.0);


	if (!_mindProbe)
	{
		_btnPrev->setText(L"<");
		_btnPrev->setColor(Palette::blockOffset(4)+4);
		_btnPrev->onMouseClick((ActionHandler)& UnitInfoState::btnPrevClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& UnitInfoState::btnPrevClick,
						Options::keyBattlePrevUnit);
		_btnNext->setText(L">");
		_btnNext->setColor(Palette::blockOffset(4)+4);
		_btnNext->onMouseClick((ActionHandler)& UnitInfoState::btnNextClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& UnitInfoState::btnNextClick,
						Options::keyBattleNextUnit);
	}
}

/**
 *
 */
UnitInfoState::~UnitInfoState()
{
}

/**
 * Updates unit info which can change after going into other screens.
 */
void UnitInfoState::init()
{
	State::init();

	std::wostringstream ss;

	int minPsi = 0;
	if (_unit->getType() == "SOLDIER")
	{
		ss << tr(_unit->getRankString());
		ss << " ";

		minPsi = _game->getSavedGame()->getSoldier(_unit->getId())->getRules()->getMinStats().psiSkill - 1; // kL, shit..
	}
	ss << _unit->getName(_game->getLanguage(), BattlescapeGame::_debugPlay);
	_txtName->setBig();
	_txtName->setText(ss.str());

	int stat = _unit->getTimeUnits();
	ss.str(L"");
	ss << stat;
	_numTimeUnits->setText(ss.str());
	_barTimeUnits->setMax(static_cast<double>(_unit->getStats()->tu));
	_barTimeUnits->setValue(static_cast<double>(stat));

	stat = _unit->getEnergy();
	ss.str(L"");
	ss << stat;
	_numEnergy->setText(ss.str());
	_barEnergy->setMax(static_cast<double>(_unit->getStats()->stamina));
	_barEnergy->setValue(static_cast<double>(stat));

	stat = _unit->getHealth();
	ss.str(L"");
	ss << stat;
	_numHealth->setText(ss.str());
	_barHealth->setMax(static_cast<double>(_unit->getStats()->health));
	_barHealth->setValue(static_cast<double>(stat));
	_barHealth->setValue2(static_cast<double>(_unit->getStunlevel()));

	stat = _unit->getFatalWounds();
	ss.str(L"");
	ss << stat;
	_numFatalWounds->setText(ss.str());
	_barFatalWounds->setMax(static_cast<double>(stat));
	_barFatalWounds->setValue(static_cast<double>(stat));

	stat = _unit->getStats()->bravery;
	ss.str(L"");
	ss << stat;
	_numBravery->setText(ss.str());
	_barBravery->setMax(static_cast<double>(stat));
	_barBravery->setValue(static_cast<double>(stat));

	stat = _unit->getMorale();
	ss.str(L"");
	ss << stat;
	_numMorale->setText(ss.str());
	_barMorale->setMax(100);
	_barMorale->setValue(static_cast<double>(stat));

	stat = _unit->getStats()->reactions;
	ss.str(L"");
	ss << stat;
	_numReactions->setText(ss.str());
	_barReactions->setMax(static_cast<double>(stat));
	_barReactions->setValue(static_cast<double>(stat));

	double arbitraryVariable = static_cast<double>(_unit->getStats()->firing);
	stat = static_cast<int>(arbitraryVariable * _unit->getAccuracyModifier());
	ss.str(L"");
	ss << stat;
	_numFiring->setText(ss.str());
	_barFiring->setMax(arbitraryVariable);
	_barFiring->setValue(static_cast<double>(stat));

	arbitraryVariable = static_cast<double>(_unit->getStats()->throwing);
	stat = static_cast<int>(arbitraryVariable * _unit->getAccuracyModifier());
	ss.str(L"");
	ss << stat;
	_numThrowing->setText(ss.str());
	_barThrowing->setMax(arbitraryVariable);
	_barThrowing->setValue(static_cast<double>(stat));

	arbitraryVariable = static_cast<double>(_unit->getStats()->melee);
	stat = static_cast<int>(arbitraryVariable * _unit->getAccuracyModifier());
	ss.str(L"");
	ss << stat;
	_numMelee->setText(ss.str());
	_barMelee->setMax(arbitraryVariable);
	_barMelee->setValue(static_cast<double>(stat));

	stat = _unit->getStats()->strength;
	ss.str(L"");
	ss << stat;
	_numStrength->setText(ss.str());
	_barStrength->setMax(static_cast<double>(stat));
	_barStrength->setValue(static_cast<double>(stat));


	if (_unit->getStats()->psiSkill > minPsi
		|| (Options::psiStrengthEval
			&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements())))
	{
		stat = _unit->getStats()->psiStrength;
		ss.str(L"");
		ss << stat;
		_numPsiStrength->setText(ss.str());
		_barPsiStrength->setMax(static_cast<double>(stat));
		_barPsiStrength->setValue(static_cast<double>(stat));

		_txtPsiStrength->setVisible(true);
		_numPsiStrength->setVisible(true);
		_barPsiStrength->setVisible(true);
	}
	else
	{
		_txtPsiStrength->setVisible(false);
		_numPsiStrength->setVisible(false);
		_barPsiStrength->setVisible(false);
	}

	if (_unit->getStats()->psiSkill > minPsi)
	{
		stat = _unit->getStats()->psiSkill;
		ss.str(L"");
		ss << stat;
		_numPsiSkill->setText(ss.str());
		_barPsiSkill->setMax(static_cast<double>(stat));
		_barPsiSkill->setValue(static_cast<double>(stat));

		_txtPsiSkill->setVisible(true);
		_numPsiSkill->setVisible(true);
		_barPsiSkill->setVisible(true);
	}
	else
	{
		_txtPsiSkill->setVisible(false);
		_numPsiSkill->setVisible(false);
		_barPsiSkill->setVisible(false);
	}

	stat = _unit->getArmor(SIDE_FRONT);
	ss.str(L"");
	ss << stat;
	_numFrontArmor->setText(ss.str());
	_barFrontArmor->setMax(static_cast<double>(_unit->getArmor()->getFrontArmor()));
	_barFrontArmor->setValue(static_cast<double>(stat));

	arbitraryVariable = _unit->getArmor()->getSideArmor(); // haha!!
	stat = _unit->getArmor(SIDE_LEFT);
	ss.str(L"");
	ss << stat;
	_numLeftArmor->setText(ss.str());
	_barLeftArmor->setMax(arbitraryVariable);
	_barLeftArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_RIGHT);
	ss.str(L"");
	ss << stat;
	_numRightArmor->setText(ss.str());
	_barRightArmor->setMax(arbitraryVariable);
	_barRightArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_REAR);
	ss.str(L"");
	ss << stat;
	_numRearArmor->setText(ss.str());
	_barRearArmor->setMax(_unit->getArmor()->getRearArmor());
	_barRearArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_UNDER);
	ss.str(L"");
	ss << stat;
	_numUnderArmor->setText(ss.str());
	_barUnderArmor->setMax(_unit->getArmor()->getUnderArmor());
	_barUnderArmor->setValue(static_cast<double>(stat));
}

/**
 * Closes the window on right-click.
 * @param action Pointer to an action.
 */
void UnitInfoState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT) // kL_note: why this include LMB
			exitClick(action);
		else if (!_mindProbe)
		{
			if (action->getDetails()->button.button == SDL_BUTTON_X1)
				btnNextClick(action);
			else if (action->getDetails()->button.button == SDL_BUTTON_X2)
				btnPrevClick(action);
		}
	}
}

/**
* Selects the previous unit.
* @param action Pointer to an action.
*/
void UnitInfoState::btnPrevClick(Action* action)
{
	if (_parent) // so we are here from a Battlescape Game
		_parent->selectPreviousFactionUnit(
										false,
										false);
//kL										_fromInventory);
	else // so we are here from the Craft Equipment screen
		_battleGame->selectPreviousFactionUnit(
											false,
											false,
											true); // no tanks.

	_unit = _battleGame->getSelectedUnit();

	if (_unit != NULL)
		init();
	else
		exitClick(action);
}

/**
 * Selects the next unit.
 * @param action Pointer to an action.
 */
void UnitInfoState::btnNextClick(Action* action)
{
	if (_parent) // so we are here from a Battlescape Game
		_parent->selectNextFactionUnit(
									false,
									false);
//kL									_fromInventory);
	else // so we are here from the Craft Equipment screen
		_battleGame->selectNextFactionUnit(
										false,
										false,
										true); // no tanks.

	_unit = _battleGame->getSelectedUnit();

	if (_unit != NULL)
		init();
	else
		exitClick(action);
}

/**
 * Exits the screen.
 * @param action Pointer to an action.
 */
void UnitInfoState::exitClick(Action*)
{
	if (!_fromInventory)
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
	}
	else if (!_unit->hasInventory())	// kL
		_game->popState();				// kL, tanks require double pop here.

	_game->popState();
}

}
