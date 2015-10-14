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

#include "UnitInfoState.h"

//#include <sstream>

#include "BattlescapeGame.h"
#include "BattlescapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/Screen.h"

#include "../Interface/Bar.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/RuleUnit.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Unit Info screen.
 * @param unit			- pointer to the selected unit
 * @param parent		- pointer to parent Battlescape
 * @param fromInventory	- true if player is here from the inventory (default false)
 * @param mindProbe		- true if player is using a Mind Probe (default false)
 * @param preBattle		- true if preEquip state; ie tuMode not tactical (default false)
 */
UnitInfoState::UnitInfoState(
		const BattleUnit* const unit,
		BattlescapeState* const parent,
		const bool fromInventory,
		const bool mindProbe,
		const bool preBattle)
	:
		_unit(unit),
		_parent(parent),
		_fromInventory(fromInventory),
		_mindProbe(mindProbe),
		_preBattle(preBattle),
		_battleSave(_game->getSavedGame()->getBattleSave())
{
/*	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */

	_bg			= new Surface(320, 200);

	_exit		= new InteractiveSurface(320, 180, 0, 20);
	_txtName	= new Text(288, 17, 16, 4);

	_gender		= new Surface(7, 7, 22, 4);
	_numOrder	= new NumberText(7, 5, 0, 5); // x-value is set in init()

	int
		step = 9,
		yPos = 38;

	_txtTimeUnits	= new Text(140, 9, 8, yPos);
	_numTimeUnits	= new Text(18, 9, 151, yPos);
	_barTimeUnits	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtEnergy		= new Text(140, 9, 8, yPos);
	_numEnergy		= new Text(18, 9, 151, yPos);
	_barEnergy		= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtHealth		= new Text(140, 9, 8, yPos);
	_numHealth		= new Text(18, 9, 151, yPos);
	_numStun		= new Text(18, 9, 127, yPos);
	_barHealth		= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtFatalWounds	= new Text(140, 9, 8, yPos);
	_numFatalWounds	= new Text(18, 9, 151, yPos);
	_barFatalWounds	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtBravery		= new Text(140, 9, 8, yPos);
	_numBravery		= new Text(18, 9, 151, yPos);
	_barBravery		= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtMorale		= new Text(140, 9, 8, yPos);
	_numMorale		= new Text(18, 9, 151, yPos);
	_barMorale		= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtReactions	= new Text(140, 9, 8, yPos);
	_numReactions	= new Text(18, 9, 151, yPos);
	_barReactions	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtFiring		= new Text(140, 9, 8, yPos);
	_numFiring		= new Text(18, 9, 151, yPos);
	_barFiring		= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtThrowing	= new Text(140, 9, 8, yPos);
	_numThrowing	= new Text(18, 9, 151, yPos);
	_barThrowing	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtMelee		= new Text(140, 9, 8, yPos);
	_numMelee		= new Text(18, 9, 151, yPos);
	_barMelee		= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtStrength	= new Text(140, 9, 8, yPos);
	_numStrength	= new Text(18, 9, 151, yPos);
	_barStrength	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtPsiStrength	= new Text(140, 9, 8, yPos);
	_numPsiStrength	= new Text(18, 9, 151, yPos);
	_barPsiStrength	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtPsiSkill	= new Text(140, 9, 8, yPos);
	_numPsiSkill	= new Text(18, 9, 151, yPos);
	_barPsiSkill	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtFrontArmor	= new Text(140, 9, 8, yPos);
	_numFrontArmor	= new Text(18, 9, 151, yPos);
	_barFrontArmor	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtLeftArmor	= new Text(140, 9, 8, yPos);
	_numLeftArmor	= new Text(18, 9, 151, yPos);
	_barLeftArmor	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtRightArmor	= new Text(140, 9, 8, yPos);
	_numRightArmor	= new Text(18, 9, 151, yPos);
	_barRightArmor	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtRearArmor	= new Text(140, 9, 8, yPos);
	_numRearArmor	= new Text(18, 9, 151, yPos);
	_barRearArmor	= new Bar(280, 5, 170, yPos + 1);

	yPos += step;
	_txtUnderArmor	= new Text(140, 9, 8, yPos);
	_numUnderArmor	= new Text(18, 9, 151, yPos);
	_barUnderArmor	= new Bar(280, 5, 170, yPos + 1);

	if (_mindProbe == false)
	{
		_btnPrev	= new TextButton(18, 18, 2, 2);
		_btnNext	= new TextButton(18, 18, 300, 2);
	}

	setPalette("PAL_BATTLESCAPE");

	add(_bg);
	add(_exit);
	add(_txtName, "textName", "stats");

	add(_gender);
	add(_numOrder);

	add(_txtTimeUnits);
	add(_numTimeUnits);
	add(_barTimeUnits, "barTUs", "stats");

	add(_txtEnergy);
	add(_numEnergy);
	add(_barEnergy, "barEnergy", "stats");

	add(_txtHealth);
	add(_numHealth);
	add(_numStun);
	add(_barHealth, "barHealth", "stats");

	add(_txtFatalWounds);
	add(_numFatalWounds);
	add(_barFatalWounds, "barWounds", "stats");

	add(_txtBravery);
	add(_numBravery);
	add(_barBravery, "barBravery", "stats");

	add(_txtMorale);
	add(_numMorale);
	add(_barMorale, "barMorale", "stats");

	add(_txtReactions);
	add(_numReactions);
	add(_barReactions, "barReactions", "stats");

	add(_txtFiring);
	add(_numFiring);
	add(_barFiring, "barFiring", "stats");

	add(_txtThrowing);
	add(_numThrowing);
	add(_barThrowing, "barThrowing", "stats");

	add(_txtMelee);
	add(_numMelee);
	add(_barMelee, "barMelee", "stats");

	add(_txtStrength);
	add(_numStrength);
	add(_barStrength, "barStrength", "stats");

	add(_txtPsiStrength);
	add(_numPsiStrength);
	add(_barPsiStrength, "barPsiStrength", "stats");

	add(_txtPsiSkill);
	add(_numPsiSkill);
	add(_barPsiSkill, "barPsiSkill", "stats");

	add(_txtFrontArmor);
	add(_numFrontArmor);
	add(_barFrontArmor, "barFrontArmor", "stats");

	add(_txtLeftArmor);
	add(_numLeftArmor);
	add(_barLeftArmor, "barLeftArmor", "stats");

	add(_txtRightArmor);
	add(_numRightArmor);
	add(_barRightArmor, "barRightArmor", "stats");

	add(_txtRearArmor);
	add(_numRearArmor);
	add(_barRearArmor, "barRearArmor", "stats");

	add(_txtUnderArmor);
	add(_numUnderArmor);
	add(_barUnderArmor, "barUnderArmor", "stats");

	if (_mindProbe == false)
	{
		add(_btnPrev, "button", "stats");
		add(_btnNext, "button", "stats");
	}

	centerAllSurfaces();


	_game->getResourcePack()->getSurface("UNIBORD.PCK")->blit(_bg);

	_exit->onMouseClick((ActionHandler)& UnitInfoState::exitClick,
					SDL_BUTTON_RIGHT);
	_exit->onKeyboardPress(
					(ActionHandler)& UnitInfoState::exitClick,
					Options::keyCancel);
	_exit->onKeyboardPress(
					(ActionHandler)& UnitInfoState::exitClick,
					Options::keyOk);
	_exit->onKeyboardPress(
					(ActionHandler)& UnitInfoState::exitClick,
					Options::keyOkKeypad);
	_exit->onKeyboardPress(
					(ActionHandler)& UnitInfoState::exitClick,
					Options::keyBattleStats);

	Uint8
		color = static_cast<Uint8>(_game->getRuleset()->getInterface("stats")->getElement("text")->color),
		color2 = static_cast<Uint8>(_game->getRuleset()->getInterface("stats")->getElement("text")->color2);

	_txtName->setAlign(ALIGN_CENTER);
	_txtName->setBig();
	_txtName->setHighContrast();

	_numOrder->setColor(1);
	_numOrder->setVisible(false);

	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));
	_txtTimeUnits->setColor(color);
	_txtTimeUnits->setHighContrast();

	_numTimeUnits->setColor(color2);
	_numTimeUnits->setHighContrast();
//	_barTimeUnits->setScale();


	_txtEnergy->setText(tr("STR_ENERGY"));
	_txtEnergy->setColor(color);
	_txtEnergy->setHighContrast();

	_numEnergy->setColor(color2);
	_numEnergy->setHighContrast();
//	_barEnergy->setScale();


	_txtHealth->setText(tr("STR_HEALTH"));
	_txtHealth->setColor(color);
	_txtHealth->setHighContrast();

	_numHealth->setColor(color2);
	_numHealth->setHighContrast();

	_numStun->setColor(80); // brown
	_numStun->setHighContrast();
	_numStun->setAlign(ALIGN_RIGHT);
//	_barHealth->setScale();


	_txtFatalWounds->setText(tr("STR_FATAL_WOUNDS"));
	_txtFatalWounds->setColor(color);
	_txtFatalWounds->setHighContrast();

	_numFatalWounds->setColor(color2);
	_numFatalWounds->setHighContrast();
//	_barFatalWounds->setScale();


	_txtBravery->setText(tr("STR_BRAVERY"));
	_txtBravery->setColor(color);
	_txtBravery->setHighContrast();

	_numBravery->setColor(color2);
	_numBravery->setHighContrast();
//	_barBravery->setScale();


	_txtMorale->setText(tr("STR_MORALE"));
	_txtMorale->setColor(color);
	_txtMorale->setHighContrast();

	_numMorale->setColor(color2);
	_numMorale->setHighContrast();
//	_barMorale->setScale();


	_txtReactions->setText(tr("STR_REACTIONS"));
	_txtReactions->setColor(color);
	_txtReactions->setHighContrast();

	_numReactions->setColor(color2);
	_numReactions->setHighContrast();
//	_barReactions->setScale();


	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));
	_txtFiring->setColor(color);
	_txtFiring->setHighContrast();

	_numFiring->setColor(color2);
	_numFiring->setHighContrast();
//	_barFiring->setScale();


	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));
	_txtThrowing->setColor(color);
	_txtThrowing->setHighContrast();

	_numThrowing->setColor(color2);
	_numThrowing->setHighContrast();
//	_barThrowing->setScale();


	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));
	_txtMelee->setColor(color);
	_txtMelee->setHighContrast();

	_numMelee->setColor(color2);
	_numMelee->setHighContrast();
//	_barMelee->setScale();


	_txtStrength->setText(tr("STR_STRENGTH"));
	_txtStrength->setColor(color);
	_txtStrength->setHighContrast();

	_numStrength->setColor(color2);
	_numStrength->setHighContrast();
//	_barStrength->setScale();


	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));
	_txtPsiStrength->setColor(color);
	_txtPsiStrength->setHighContrast();

	_numPsiStrength->setColor(color2);
	_numPsiStrength->setHighContrast();
//	_barPsiStrength->setScale();


	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));
	_txtPsiSkill->setColor(color);
	_txtPsiSkill->setHighContrast();

	_numPsiSkill->setColor(color2);
	_numPsiSkill->setHighContrast();
//	_barPsiSkill->setScale();


	_txtFrontArmor->setText(tr("STR_FRONT_ARMOR_UC"));
	_txtFrontArmor->setColor(color);
	_txtFrontArmor->setHighContrast();

	_numFrontArmor->setColor(color2);
	_numFrontArmor->setHighContrast();
//	_barFrontArmor->setScale();


	_txtLeftArmor->setText(tr("STR_LEFT_ARMOR_UC"));
	_txtLeftArmor->setColor(color);
	_txtLeftArmor->setHighContrast();

	_numLeftArmor->setColor(color2);
	_numLeftArmor->setHighContrast();
//	_barLeftArmor->setScale();


	_txtRightArmor->setText(tr("STR_RIGHT_ARMOR_UC"));
	_txtRightArmor->setColor(color);
	_txtRightArmor->setHighContrast();

	_numRightArmor->setColor(color2);
	_numRightArmor->setHighContrast();
//	_barRightArmor->setScale();


	_txtRearArmor->setText(tr("STR_REAR_ARMOR_UC"));
	_txtRearArmor->setColor(color);
	_txtRearArmor->setHighContrast();

	_numRearArmor->setColor(color2);
	_numRearArmor->setHighContrast();
//	_barRearArmor->setScale();


	_txtUnderArmor->setText(tr("STR_UNDER_ARMOR_UC"));
	_txtUnderArmor->setColor(color);
	_txtUnderArmor->setHighContrast();

	_numUnderArmor->setColor(color2);
	_numUnderArmor->setHighContrast();
//	_barUnderArmor->setScale();


	if (_mindProbe == false)
	{
		_btnPrev->setText(L"<");
		_btnPrev->getTextPtr()->setX(-1);
		_btnPrev->onMouseClick((ActionHandler)& UnitInfoState::btnPrevClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& UnitInfoState::btnPrevClick,
						Options::keyBattlePrevUnit);
//		_btnPrev->onKeyboardPress(
//						(ActionHandler)& UnitInfoState::btnPrevClick,
//						SDLK_KP4);

		_btnNext->setText(L">");
		_btnNext->onMouseClick((ActionHandler)& UnitInfoState::btnNextClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& UnitInfoState::btnNextClick,
						Options::keyBattleNextUnit);
//		_btnNext->onKeyboardPress(
//						(ActionHandler)& UnitInfoState::btnNextClick,
//						SDLK_KP6);

//		_timer = new Timer(300);
//		_timer->onTimer((StateHandler)& UnitInfoState::keyRepeat);
//		_timer->start();
	}
}

/**
 * dTor.
 */
UnitInfoState::~UnitInfoState()
{
//	delete _timer;
}

/*
 * Hits the think timer.
 *
void UnitInfoState::think()
{
	if (_mindProbe == false)
	{
		State::think();
		_timer->think(this, NULL);
	}
} */

/*
 * Advances to the next/previous Unit when right/left key is depressed.
 *
void UnitInfoState::keyRepeat() // private.
{
	Uint8* keystate = SDL_GetKeyState(NULL);
	if (keystate[Options::keyBattleNextUnit] == 1
		|| keystate[SDLK_KP6] == 1)
	{
		btnNextClick(NULL);
	}
	else if (keystate[Options::keyBattlePrevUnit] == 1
		|| keystate[SDLK_KP4] == 1)
	{
		btnPrevClick(NULL);
	}
} */

/**
 * Updates unit info which can change after going into other screens.
 */
void UnitInfoState::init()
{
	State::init();
	_gender->clear();

	std::wostringstream woststr;

	// Sprites & Order ->
	const Soldier* const sol = _unit->getGeoscapeSoldier();
	if (sol != NULL)
	{
		woststr << tr(_unit->getRankString())
				<< L" ";

		Surface* gender = NULL;
		if (sol->getGender() == GENDER_MALE)
			gender = _game->getResourcePack()->getSurface("GENDER_M");
		else
			gender = _game->getResourcePack()->getSurface("GENDER_F");

		if (gender != NULL)
			gender->blit(_gender);

		const size_t order = _unit->getBattleOrder();
		if (order < 10)
			_numOrder->setX(_btnNext->getX() - 6);
		else
			_numOrder->setX(_btnNext->getX() - 10);

		_numOrder->setValue(order);
		_numOrder->setVisible();
	}
	else
		_numOrder->setVisible(false);

	// NAME ->
	woststr << _unit->getName(_game->getLanguage(), BattlescapeGame::_debugPlay);
	_txtName->setText(woststr.str());
	_txtName->setBig();

	// TU, Health, & Bravery ->
	int stat = _unit->getTimeUnits();
	_numTimeUnits->setText(Text::intWide(stat));
	_barTimeUnits->setMaxValue(static_cast<double>(_unit->getBaseStats()->tu));
	_barTimeUnits->setValue(static_cast<double>(stat));

	stat = _unit->getEnergy();
	_numEnergy->setText(Text::intWide(stat));
	_barEnergy->setMaxValue(static_cast<double>(_unit->getBaseStats()->stamina));
	_barEnergy->setValue(static_cast<double>(stat));

	stat = _unit->getHealth();
	_numHealth->setText(Text::intWide(stat));
	const int stat2 = _unit->getStun();
	if (stat2 != 0)
	{
		_numStun->setText(Text::intWide(stat2));
		_numStun->setVisible();
	}
	else
		_numStun->setVisible(false);
	_barHealth->setMaxValue(static_cast<double>(_unit->getBaseStats()->health));
	_barHealth->setValue(static_cast<double>(stat));
	_barHealth->setValue2(static_cast<double>(stat2));

	woststr.str(L"");
	if (_unit->isWoundable() == true)
	{
		stat = _unit->getFatalWounds();
		woststr << stat;

		_barFatalWounds->setMaxValue(static_cast<double>(stat));
		_barFatalWounds->setValue(static_cast<double>(stat));
		_barFatalWounds->setVisible();
	}
	else
	{
		woststr << L"-";
		_barFatalWounds->setVisible(false);
	}
	_numFatalWounds->setText(woststr.str());

	woststr.str(L"");
//	std::wostringstream woststr1;
	stat = _unit->getBaseStats()->bravery;
	if (stat != 110)
	{
		woststr << stat;
		_barBravery->setMaxValue(static_cast<double>(stat));
		_barBravery->setValue(static_cast<double>(stat));
		_barBravery->setVisible();

//		stat = _unit->getMorale();
//		woststr1 << stat;
//		_barMorale->setValue(static_cast<double>(stat));
//		_barMorale->setVisible();
	}
	else
	{
		woststr << L"oo";
//		woststr1 << L"-";
		_barBravery->setVisible(false);
//		_barMorale->setVisible(false);
	}
	_numBravery->setText(woststr.str());
//	_numMorale->setText(woststr1.str());
	// note Let Morale show until further tested to ensure it never goes down
	// for units w/ 110 Bravery. (I think it might/should when they get damaged)
	stat = _unit->getMorale();
	_barMorale->setValue(static_cast<double>(stat));
	_numMorale->setText(Text::intWide(stat));

	// Primary Abilities ->
	const double acuModi = _unit->getAccuracyModifier();

	double arbitraryVariable = static_cast<double>(_unit->getBaseStats()->reactions);
	_barReactions->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barReactions->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_numReactions->setText(Text::intWide(stat));

	arbitraryVariable = static_cast<double>(_unit->getBaseStats()->firing);
	_barFiring->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barFiring->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_numFiring->setText(Text::intWide(stat));

	arbitraryVariable = static_cast<double>(_unit->getBaseStats()->throwing);
	_barThrowing->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barThrowing->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_numThrowing->setText(Text::intWide(stat));

	arbitraryVariable = static_cast<double>(_unit->getBaseStats()->melee);
	_barMelee->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barMelee->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_numMelee->setText(Text::intWide(stat));

	arbitraryVariable = static_cast<double>(_unit->getBaseStats()->strength);
	_barStrength->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi / 2. + 0.5;
	_barStrength->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_numStrength->setText(Text::intWide(stat));

	// Psionics ->
	const int psiSkill = _unit->getBaseStats()->psiSkill;
	if (psiSkill > 0 || sol == NULL)
	{
		woststr.str(L"");
		if (_unit->isFearable() == true)
		{
			stat = _unit->getBaseStats()->psiStrength;
			woststr << stat;
			_barPsiStrength->setMaxValue(static_cast<double>(stat));
			_barPsiStrength->setValue(static_cast<double>(stat));
			_barPsiStrength->setVisible();
		}
		else
		{
			woststr << L"oo";
			_barPsiStrength->setVisible(false);
		}
		_numPsiStrength->setText(woststr.str());
		_numPsiStrength->setVisible();

		if (psiSkill > 0)
		{
			_numPsiSkill->setText(Text::intWide(psiSkill));
			_barPsiSkill->setMaxValue(static_cast<double>(psiSkill));
			_barPsiSkill->setValue(static_cast<double>(psiSkill));

			_numPsiSkill->setVisible();
			_barPsiSkill->setVisible();
		}
		else
		{
			_numPsiSkill->setVisible(false);
			_barPsiSkill->setVisible(false);
		}
	}
	else
	{
		_numPsiStrength->setVisible(false);
		_barPsiStrength->setVisible(false);
	}

	// Armor ->
	stat = _unit->getArmor(SIDE_FRONT);
	_numFrontArmor->setText(Text::intWide(stat));
	_barFrontArmor->setMaxValue(static_cast<double>(_unit->getArmor()->getFrontArmor()));
	_barFrontArmor->setValue(static_cast<double>(stat));

	arbitraryVariable = _unit->getArmor()->getSideArmor(); // haha!!
	stat = _unit->getArmor(SIDE_LEFT);
	_numLeftArmor->setText(Text::intWide(stat));
	_barLeftArmor->setMaxValue(arbitraryVariable);
	_barLeftArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_RIGHT);
	_numRightArmor->setText(Text::intWide(stat));
	_barRightArmor->setMaxValue(arbitraryVariable);
	_barRightArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_REAR);
	_numRearArmor->setText(Text::intWide(stat));
	_barRearArmor->setMaxValue(_unit->getArmor()->getRearArmor());
	_barRearArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_UNDER);
	_numUnderArmor->setText(Text::intWide(stat));
	_barUnderArmor->setMaxValue(_unit->getArmor()->getUnderArmor());
	_barUnderArmor->setValue(static_cast<double>(stat));
}

/**
 * Closes the window on right-click.
 * @param action - pointer to an Action
 */
void UnitInfoState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
//		if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
//			exitClick(); else
		if (_mindProbe == false)
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
 * @param action - pointer to an Action
*/
void UnitInfoState::btnPrevClick(Action*)
{
	if (_parent != NULL)	// so you are here from a Battlescape Game
		_parent->selectPreviousFactionUnit(false,false);
	else					// so you are here from the Craft Equipment screen
		_battleSave->selectPreviousFactionUnit(false,false,true); // no tanks.

	_unit = _battleSave->getSelectedUnit();

	if (_unit != NULL)
		init();
	else
		exitClick();
}

/**
 * Selects the next unit.
 * @param action - pointer to an Action
 */
void UnitInfoState::btnNextClick(Action*)
{
	if (_parent != NULL)	// this is from a Battlescape Game
		_parent->selectNextFactionUnit(false,false);
	else					// this is from the Craft Equipment screen
		_battleSave->selectNextFactionUnit(false,false,true); // no tanks.

	_unit = _battleSave->getSelectedUnit();

	if (_unit != NULL)
		init();
	else
		exitClick();
}

/**
 * Exits the screen.
 * @param action - pointer to an Action (default NULL)
 */
void UnitInfoState::exitClick(Action*) const // private.
{
	if (_mindProbe == true)
	{
		const BattleUnit* const unit (_battleSave->getSelectedUnit());
		if (unit->getTimeUnits() < unit->getActionTu(
												BA_USE,
												_game->getRuleset()->getItem("STR_MIND_PROBE")))
		{
			_battleSave->getBattleGame()->cancelCurrentAction();
		}
	}

/*	if (_fromInventory == false)
	{
		if (Options::maximizeInfoScreens)
		{
			Screen::updateScale(Options::battlescapeScale, Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
			_game->getScreen()->resetDisplay(false);
		}
	} else */
	if (_mindProbe == true || _unit->hasInventory() == true)
		_game->popState();
	else if (_preBattle == false)
	{
		if (_fromInventory == true)
			_game->popState();

		_game->popState();
	}
}

}
