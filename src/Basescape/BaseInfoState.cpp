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

#include "BaseInfoState.h"

//#include <sstream>

#include "BasescapeState.h"
#include "MiniBaseView.h"
#include "MonthlyCostsState.h"
#include "StoresState.h"
#include "TransfersState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Bar.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleRegion.h"

#include "../Savegame/Base.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Base Info screen.
 * @param base	- pointer to the Base to get info from
 * @param state	- pointer to the BasescapeState
 */
BaseInfoState::BaseInfoState(
		Base* base,
		BasescapeState* state)
	:
		_base(base),
		_state(state)
{
	_bg					= new Surface(320, 200, 0, 0);
	_mini				= new MiniBaseView(128, 16, 182, 8);

	_btnMonthlyCosts	= new TextButton(72, 14, 10, 179);
	_btnTransfers		= new TextButton(72, 14, 86, 179);
	_btnStores			= new TextButton(72, 14, 162, 179);
	_btnOk				= new TextButton(72, 14, 238, 179);

	_edtBase			= new TextEdit(this, 127, 16, 8, 9);

	_txtPersonnel		= new Text(204, 9, 8, 31);
	_txtRegion			= new Text(100, 9, 212, 31);

	_txtSoldiers		= new Text(114, 9, 8, 41);
	_numSoldiers		= new Text(40, 9, 126, 42);
	_barSoldiers		= new Bar(218, 5, 166, 43);
	_txtScientists		= new Text(114, 9, 8, 51);
	_numScientists		= new Text(40, 9, 126, 52);
	_barScientists		= new Bar(218, 5, 166, 53);
	_txtEngineers		= new Text(114, 9, 8, 61);
	_numEngineers		= new Text(40, 9, 126, 62);
	_barEngineers		= new Bar(218, 5, 166, 63);

	_txtSpace			= new Text(300, 9, 8, 72);

	_txtQuarters		= new Text(114, 9, 8, 83);
	_numQuarters		= new Text(40, 9, 126, 84);
	_barQuarters		= new Bar(218, 5, 166, 85);
	_txtLaboratories	= new Text(114, 9, 8, 93);
	_numLaboratories	= new Text(40, 9, 126, 94);
	_barLaboratories	= new Bar(218, 5, 166, 95);
	_txtWorkshops		= new Text(114, 9, 8, 103);
	_numWorkshops		= new Text(40, 9, 126, 104);
	_barWorkshops		= new Bar(218, 5, 166, 105);
	_txtContainment		= new Text(114, 9, 8, 113);
	_numContainment		= new Text(40, 9, 126, 114);
	_barContainment		= new Bar(218, 5, 166, 115);
	_txtStores			= new Text(114, 9, 8, 123);
	_numStores			= new Text(40, 9, 126, 124);
	_barStores			= new Bar(218, 5, 166, 125);
	_txtHangars			= new Text(114, 9, 8, 133);
	_numHangars			= new Text(40, 9, 126, 134);
	_barHangars			= new Bar(218, 5, 166, 135);

	_txtLongRange		= new Text(114, 9, 8, 147);
	_numLongRange		= new Text(40, 9, 126, 148);
	_barLongRange		= new Bar(218, 5, 166, 149);
	_txtShortRange		= new Text(114, 9, 8, 157);
	_numShortRange		= new Text(40, 9, 126, 158);
	_barShortRange		= new Bar(218, 5, 166, 159);
	_txtDefense			= new Text(114, 9, 8, 167);
	_numDefense			= new Text(40, 9, 126, 168);
	_barDefense			= new Bar(218, 5, 166, 169);

	setPalette("PAL_BASESCAPE");

	add(_bg);
	add(_mini, "miniBase", "basescape");
	add(_btnOk, "button", "baseInfo");
	add(_btnTransfers, "button", "baseInfo");
	add(_btnStores, "button", "baseInfo");
	add(_btnMonthlyCosts, "button", "baseInfo");
	add(_edtBase, "text1", "baseInfo");

	add(_txtPersonnel, "text1", "baseInfo");
	add(_txtRegion, "text1", "baseInfo");
	add(_txtSoldiers, "text2", "baseInfo");
	add(_numSoldiers, "numbers", "baseInfo");
	add(_barSoldiers, "personnelBars", "baseInfo");
	add(_txtEngineers, "text2", "baseInfo");
	add(_numEngineers, "numbers", "baseInfo");
	add(_barEngineers, "personnelBars", "baseInfo");
	add(_txtScientists, "text2", "baseInfo");
	add(_numScientists, "numbers", "baseInfo");
	add(_barScientists, "personnelBars", "baseInfo");

	add(_txtSpace, "text1", "baseInfo");
	add(_txtQuarters, "text2", "baseInfo");
	add(_numQuarters, "numbers", "baseInfo");
	add(_barQuarters, "facilityBars", "baseInfo");
	add(_txtStores, "text2", "baseInfo");
	add(_numStores, "numbers", "baseInfo");
	add(_barStores, "facilityBars", "baseInfo");
	add(_txtLaboratories, "text2", "baseInfo");
	add(_numLaboratories, "numbers", "baseInfo");
	add(_barLaboratories, "facilityBars", "baseInfo");
	add(_txtWorkshops, "text2", "baseInfo");
	add(_numWorkshops, "numbers", "baseInfo");
	add(_barWorkshops, "facilityBars", "baseInfo");

	if (Options::storageLimitsEnforced == true)
	{
		add(_txtContainment, "text2", "baseInfo");
		add(_numContainment, "numbers", "baseInfo");
		add(_barContainment, "facilityBars", "baseInfo");
	}

	add(_txtHangars, "text2", "baseInfo");
	add(_numHangars, "numbers", "baseInfo");
	add(_barHangars, "facilityBars", "baseInfo");

	add(_txtDefense, "text2", "baseInfo");
	add(_numDefense, "numbers", "baseInfo");
	add(_barDefense, "defenceBar", "baseInfo");
	add(_txtShortRange, "text2", "baseInfo");
	add(_numShortRange, "numbers", "baseInfo");
	add(_barShortRange, "detectionBars", "baseInfo");
	add(_txtLongRange, "text2", "baseInfo");
	add(_numLongRange, "numbers", "baseInfo");
	add(_barLongRange, "detectionBars", "baseInfo");

	centerAllSurfaces();


	std::ostringstream osts;
	if (Options::storageLimitsEnforced == true)
		osts << "ALT";
	osts << "BACK07.SCR";
	_game->getResourcePack()->getSurface(osts.str())->blit(_bg);

	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_game->getSavedGame()->getBases());
	for (size_t
			i = 0;
			i != _game->getSavedGame()->getBases()->size();
			++i)
	{
		if (_game->getSavedGame()->getBases()->at(i) == base)
		{
			_mini->setSelectedBase(i);
			break;
		}
	}
	_mini->onMouseClick((ActionHandler)& BaseInfoState::miniClick);
	_mini->onKeyboardPress((ActionHandler)& BaseInfoState::handleKeyPress);

//	_edtBase->setColor(Palette::blockOffset(15)+1);
	_edtBase->setBig();
	_edtBase->onChange((ActionHandler)& BaseInfoState::edtBaseChange);

//	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& BaseInfoState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& BaseInfoState::btnOkClick,
					Options::keyCancel);

//	_btnTransfers->setColor(Palette::blockOffset(15)+6);
	_btnTransfers->setText(tr("STR_TRANSFERS"));
	_btnTransfers->onMouseClick((ActionHandler)& BaseInfoState::btnTransfersClick);

//	_btnStores->setColor(Palette::blockOffset(15)+6);
	_btnStores->setText(tr("STR_STORES_UC"));
	_btnStores->onMouseClick((ActionHandler)& BaseInfoState::btnStoresClick);

//	_btnMonthlyCosts->setColor(Palette::blockOffset(15)+6);
	_btnMonthlyCosts->setText(tr("STR_MONTHLY_COSTS"));
	_btnMonthlyCosts->onMouseClick((ActionHandler)& BaseInfoState::btnMonthlyCostsClick);

//	_txtPersonnel->setColor(Palette::blockOffset(15)+1);
	_txtPersonnel->setText(tr("STR_PERSONNEL_AVAILABLE_PERSONNEL_TOTAL"));

//	_txtRegion->setColor(Palette::blockOffset(15)+1);
	_txtRegion->setAlign(ALIGN_RIGHT);

//	_txtSoldiers->setColor(Palette::blockOffset(13)+5);
	_txtSoldiers->setText(tr("STR_SOLDIERS"));
//	_numSoldiers->setColor(Palette::blockOffset(13));
//	_barSoldiers->setColor(Palette::blockOffset(1));
	_barSoldiers->setScale();

//	_txtEngineers->setColor(Palette::blockOffset(13)+5);
	_txtEngineers->setText(tr("STR_ENGINEERS"));
//	_numEngineers->setColor(Palette::blockOffset(13));
//	_barEngineers->setColor(Palette::blockOffset(1));
	_barEngineers->setScale();

//	_txtScientists->setColor(Palette::blockOffset(13)+5);
	_txtScientists->setText(tr("STR_SCIENTISTS"));
//	_numScientists->setColor(Palette::blockOffset(13));
//	_barScientists->setColor(Palette::blockOffset(1));
	_barScientists->setScale();


//	_txtSpace->setColor(Palette::blockOffset(15)+1);
	_txtSpace->setText(tr("STR_SPACE_USED_SPACE_AVAILABLE"));

//	_txtQuarters->setColor(Palette::blockOffset(13)+5);
	_txtQuarters->setText(tr("STR_LIVING_QUARTERS_PLURAL"));
//	_numQuarters->setColor(Palette::blockOffset(13));
//	_barQuarters->setColor(Palette::blockOffset(3));
	_barQuarters->setScale(0.5);

//	_txtStores->setColor(Palette::blockOffset(13)+5);
	_txtStores->setText(tr("STR_STORES"));
//	_numStores->setColor(Palette::blockOffset(13));
//	_barStores->setColor(Palette::blockOffset(3));
	_barStores->setScale(0.25); //0.5

//	_txtLaboratories->setColor(Palette::blockOffset(13)+5);
	_txtLaboratories->setText(tr("STR_LABORATORIES"));
//	_numLaboratories->setColor(Palette::blockOffset(13));
//	_barLaboratories->setColor(Palette::blockOffset(3));
	_barLaboratories->setScale(0.5);

//	_txtWorkshops->setColor(Palette::blockOffset(13)+5);
	_txtWorkshops->setText(tr("STR_WORK_SHOPS"));
//	_numWorkshops->setColor(Palette::blockOffset(13));
//	_barWorkshops->setColor(Palette::blockOffset(3));
	_barWorkshops->setScale(0.5);

	if (Options::storageLimitsEnforced == true)
	{
//		_txtContainment->setColor(Palette::blockOffset(13)+5);
		_txtContainment->setText(tr("STR_ALIEN_CONTAINMENT"));
//		_numContainment->setColor(Palette::blockOffset(13));
//		_barContainment->setColor(Palette::blockOffset(3));
		_barContainment->setScale(); //0.5
	}

//	_txtHangars->setColor(Palette::blockOffset(13)+5);
	_txtHangars->setText(tr("STR_HANGARS"));
//	_numHangars->setColor(Palette::blockOffset(13));
//	_barHangars->setColor(Palette::blockOffset(3));
	_barHangars->setScale(18.);

//	_txtDefense->setColor(Palette::blockOffset(13)+5);
	_txtDefense->setText(tr("STR_DEFENSE_STRENGTH"));
//	_numDefense->setColor(Palette::blockOffset(13));
//	_barDefense->setColor(Palette::blockOffset(2));
//	_barDefense->setScale(0.125);
	_barDefense->setScale(0.018); // kL

//	_txtShortRange->setColor(Palette::blockOffset(13)+5);
	_txtShortRange->setText(tr("STR_SHORT_RANGE_DETECTION"));
//	_numShortRange->setColor(Palette::blockOffset(13));
//	_barShortRange->setColor(Palette::blockOffset(8));
//	_barShortRange->setScale(25.);
	_barShortRange->setScale(1.25); // kL

//	_txtLongRange->setColor(Palette::blockOffset(13)+5);
	_txtLongRange->setText(tr("STR_LONG_RANGE_DETECTION"));
//	_numLongRange->setColor(Palette::blockOffset(13));
//	_barLongRange->setColor(Palette::blockOffset(8));
//	_barLongRange->setScale(25.);
	_barLongRange->setScale(1.25); // kL
}

/**
 * dTor.
 */
BaseInfoState::~BaseInfoState()
{}

/**
 * The player can change the selected base.
 */
void BaseInfoState::init()
{
	State::init();

	_edtBase->setText(_base->getName());

	for (std::vector<Region*>::iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										_base->getLongitude(),
										_base->getLatitude()))
		{
			_txtRegion->setText(tr((*i)->getRules()->getType()));
			break;
		}
	}

	std::wostringstream
		ss1,
		ss2,
		ss3,
		ss4,
		ss5,
		ss6,
		ss7,
		ss8,
		ss9,
		ss10,
		ss11,
		ss12;

	int var,
		var2;

	var = _base->getTotalSoldiers();
	var2 = _base->getAvailableSoldiers(true);
//kL	ss << _base->getAvailableSoldiers() << ":" << _base->getTotalSoldiers();
	ss1 << var2 << ":" << var; // kL
	_numSoldiers->setText(ss1.str());
	_barSoldiers->setMax(var);
//kL	_barSoldiers->setValue(_base->getAvailableSoldiers());
	_barSoldiers->setValue(var2); // kL

	var = _base->getTotalEngineers();
	var2 = _base->getEngineers();
//kL	ss2 << _base->getAvailableEngineers() << ":" << _base->getTotalEngineers();
	ss2 << var2 << ":" << var; // kL
	_numEngineers->setText(ss2.str());
	_barEngineers->setMax(var);
//kL	_barEngineers->setValue(_base->getAvailableEngineers());
	_barEngineers->setValue(var2); // kL

	var = _base->getTotalScientists();
	var2 = _base->getScientists();
//kL	ss3 << _base->getAvailableScientists() << ":" << _base->getTotalScientists();
	ss3 << var2 << ":" << var; // kL
	_numScientists->setText(ss3.str());
	_barScientists->setMax(var);
//kL	_barScientists->setValue(_base->getAvailableScientists());
	_barScientists->setValue(var2); // kL

	var = _base->getAvailableQuarters();
	var2 = _base->getUsedQuarters();
	ss4 << var2 << ":" << var;
	_numQuarters->setText(ss4.str());
	_barQuarters->setMax(var);
	_barQuarters->setValue(var2);

	var = _base->getAvailableStores();
	var2 = static_cast<int>(floor(_base->getUsedStores() + 0.05));
	ss5 << var2 << ":" << var;
	_numStores->setText(ss5.str());
	_barStores->setMax(var);
	_barStores->setValue(var2);

	var = _base->getAvailableLaboratories();
	var2 = _base->getUsedLaboratories();
	ss6 << var2 << ":" << var;
	_numLaboratories->setText(ss6.str());
	_barLaboratories->setMax(var);
	_barLaboratories->setValue(var2);

	var = _base->getAvailableWorkshops();
	var2 = _base->getUsedWorkshops();
	ss7 << var2 << ":" << var;
	_numWorkshops->setText(ss7.str());
	_barWorkshops->setMax(var);
	_barWorkshops->setValue(var2);

	if (Options::storageLimitsEnforced)
	{
		var = _base->getAvailableContainment();
		var2 = _base->getUsedContainment();
		ss8 << var2 << ":" << var;
		_numContainment->setText(ss8.str());
		_barContainment->setMax(var);
		_barContainment->setValue(var2);
	}

	var = _base->getAvailableHangars();
	var2 = _base->getUsedHangars();
	ss9 << var2 << ":" << var;
	_numHangars->setText(ss9.str());
	_barHangars->setMax(var);
	_barHangars->setValue(var2);

	var = _base->getDefenseValue();
	ss10 << var;
	_numDefense->setText(ss10.str());
	_barDefense->setMax(var);
	_barDefense->setValue(var);

	var = _base->getShortRangeValue();
//kL	var = _base->getShortRangeDetection();
//kL	ss11 << _base->getShortRangeDetection();
//	ss11 << _base->getShortRangeValue(); // kL
	ss11 << var; // kL
	_numShortRange->setText(ss11.str());
	_barShortRange->setMax(var);
	_barShortRange->setValue(var);


	if (_base->getHyperDetection())
		_barLongRange->setColor(Palette::blockOffset(4)+4);

	var = _base->getLongRangeValue();
//kL	var = _base->getLongRangeDetection();
//kL	ss12 << _base->getLongRangeDetection();
//	ss12 << _base->getLongRangeValue(); // kL
	ss12 << var; // kL
	_numLongRange->setText(ss12.str());
	_barLongRange->setMax(var);
	_barLongRange->setValue(var);
}

/**
 * Changes the base name.
 * @param action - pointer to an Action
 */
void BaseInfoState::edtBaseChange(Action*)
{
	_base->setName(_edtBase->getText());
}

/**
 * Selects a new base to display.
 * @param action - pointer to an Action
 */
void BaseInfoState::miniClick(Action*)
{
	const size_t baseID = _mini->getHoveredBase();

	if (baseID < _game->getSavedGame()->getBases()->size()
		&& _base != _game->getSavedGame()->getBases()->at(baseID))
	{
		_mini->setSelectedBase(baseID);

		_base = _game->getSavedGame()->getBases()->at(baseID);
		_state->setBase(_base);

		init();
	}
}

/**
 * Selects a new base to display.
 * @param action - pointer to an Action
 */
void BaseInfoState::handleKeyPress(Action* action)
{
	if (action->getDetails()->type == SDL_KEYDOWN)
	{
		const SDLKey baseKeys[] =
		{
			Options::keyBaseSelect1,
			Options::keyBaseSelect2,
			Options::keyBaseSelect3,
			Options::keyBaseSelect4,
			Options::keyBaseSelect5,
			Options::keyBaseSelect6,
			Options::keyBaseSelect7,
			Options::keyBaseSelect8
		};

		const SDLKey key = action->getDetails()->key.keysym.sym;

		for (size_t
				i = 0;
				i != _game->getSavedGame()->getBases()->size();
				++i)
		{
			if (key == baseKeys[i])
			{
				_mini->setSelectedBase(i);
				_base = _game->getSavedGame()->getBases()->at(i);
				_state->setBase(_base);
				init();

				break;
			}
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void BaseInfoState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the Transfers window.
 * @param action - pointer to an Action
 */
void BaseInfoState::btnTransfersClick(Action*)
{
	_game->pushState(new TransfersState(_base));
}

/**
 * Goes to the Stores screen.
 * @param action - pointer to an Action
 */
void BaseInfoState::btnStoresClick(Action*)
{
	_game->pushState(new StoresState(_base));
}

/**
 * Goes to the Monthly Costs screen.
 * @param action - pointer to an Action
 */
void BaseInfoState::btnMonthlyCostsClick(Action*)
{
	_game->pushState(new MonthlyCostsState(_base));
}

}
