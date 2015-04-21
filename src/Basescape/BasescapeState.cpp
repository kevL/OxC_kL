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

#include "BasescapeState.h"

#include "BaseDetectionState.h"
#include "BaseInfoState.h"
#include "BaseView.h"
#include "BuildFacilitiesState.h"
#include "CraftInfoState.h"
#include "CraftsState.h"
#include "DismantleFacilityState.h"
#include "AlienContainmentState.h"
#include "ManufactureState.h"
#include "MiniBaseView.h"
#include "MonthlyCostsState.h"
#include "PurchaseState.h"
#include "ResearchState.h"
#include "SellState.h"
#include "StoresState.h"
#include "SoldiersState.h"
#include "TransferBaseState.h"
#include "TransfersState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Sound.h"

#include "../Geoscape/AllocatePsiTrainingState.h"
#include "../Geoscape/BuildNewBaseState.h"
#include "../Geoscape/GeoscapeState.h"	// kL_geoMusic
#include "../Geoscape/Globe.h"			// kL_reCenter

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleRegion.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Basescape screen.
 * @param base	- pointer to the Base to get info from
 * @param globe	- pointer to the geoscape Globe
 */
BasescapeState::BasescapeState(
		Base* base,
		Globe* globe)
	:
		_base(base),
		_globe(globe)
{
	_view			= new BaseView(192, 192, 0, 8);
	_mini			= new MiniBaseView(128, 22, 192, 33);

	_txtFacility	= new Text(192, 9);

	_edtBase		= new TextEdit(this, 126, 17, 194, 0);
	_txtRegion		= new Text(126, 9, 194, 15);
	_txtFunds		= new Text(126, 9, 194, 24);

	_btnBaseInfo	= new TextButton(128, 12, 192, 56);
	_btnSoldiers	= new TextButton(128, 12, 192, 68);
	_btnCrafts		= new TextButton(128, 12, 192, 80);
	_btnAliens		= new TextButton(128, 12, 192, 92);
	_btnResearch	= new TextButton(128, 12, 192, 104);
	_btnManufacture	= new TextButton(128, 12, 192, 116);
	_btnPurchase	= new TextButton(128, 12, 192, 128);
	_btnSell		= new TextButton(128, 12, 192, 140);
	_btnTransfer	= new TextButton(128, 12, 192, 152);
	_btnIncTrans	= new TextButton(128, 12, 192, 164);
	_btnFacilities	= new TextButton(128, 12, 192, 176);
	_btnGeoscape	= new TextButton(128, 12, 192, 188);
//	_btnNewBase		= new TextButton(128, 12, 192, 176);

	setInterface("basescape");

	add(_view,				"baseView",		"basescape");
	add(_mini,				"miniBase",		"basescape");

	add(_txtFacility,		"textTooltip",	"basescape");

	add(_edtBase,			"text1",		"basescape");
	add(_txtRegion,			"text2",		"basescape");
	add(_txtFunds,			"text3",		"basescape");

	add(_btnBaseInfo,		"button",		"basescape");
	add(_btnSoldiers,		"button",		"basescape");
	add(_btnCrafts,			"button",		"basescape");
	add(_btnAliens,			"button",		"basescape");
	add(_btnResearch,		"button",		"basescape");
	add(_btnManufacture,	"button",		"basescape");
	add(_btnPurchase,		"button",		"basescape");
	add(_btnSell,			"button",		"basescape");
	add(_btnTransfer,		"button",		"basescape");
	add(_btnIncTrans,		"button",		"basescape");
	add(_btnFacilities,		"button",		"basescape");
	add(_btnGeoscape,		"button",		"basescape");
//	add(_btnNewBase,		"button",		"basescape");

	centerAllSurfaces();


	_view->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_view->setDog(_game->getResourcePack()->getSurface("BASEDOG"));
	_view->onMouseClick(
					(ActionHandler)& BasescapeState::viewLeftClick,
					SDL_BUTTON_LEFT);
	_view->onMouseClick(
					(ActionHandler)& BasescapeState::viewRightClick,
					SDL_BUTTON_RIGHT);
	_view->onMouseOver((ActionHandler)& BasescapeState::viewMouseOver);
	_view->onMouseOut((ActionHandler)& BasescapeState::viewMouseOut);

	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_game->getSavedGame()->getBases());
	_mini->onMouseClick(
					(ActionHandler)& BasescapeState::miniLeftClick,
					SDL_BUTTON_LEFT);
	_mini->onMouseClick(
					(ActionHandler)& BasescapeState::miniRightClick,
					SDL_BUTTON_RIGHT);
	_mini->onKeyboardPress((ActionHandler)& BasescapeState::handleKeyPress);
	_mini->onMouseOver((ActionHandler)& BasescapeState::viewMouseOver);
	_mini->onMouseOut((ActionHandler)& BasescapeState::viewMouseOut);

	_edtBase->setBig();
	_edtBase->onChange((ActionHandler)& BasescapeState::edtBaseChange);

	_txtRegion->setAlign(ALIGN_RIGHT);

	_btnBaseInfo->setText(tr("STR_BASE_INFORMATION"));
	_btnBaseInfo->onMouseClick((ActionHandler)& BasescapeState::btnBaseInfoClick);

	_btnSoldiers->setText(tr("STR_SOLDIERS_UC"));
	_btnSoldiers->onMouseClick((ActionHandler)& BasescapeState::btnSoldiersClick);

	_btnCrafts->setText(tr("STR_EQUIP_CRAFT"));
	_btnCrafts->onMouseClick((ActionHandler)& BasescapeState::btnCraftsClick);

	_btnAliens->setText(tr("STR_ALIENS"));
	_btnAliens->onMouseClick((ActionHandler)& BasescapeState::btnAliens);

	_btnResearch->setText(tr("STR_RESEARCH"));
	_btnResearch->onMouseClick((ActionHandler)& BasescapeState::btnResearchClick);

	_btnManufacture->setText(tr("STR_MANUFACTURE"));
	_btnManufacture->onMouseClick((ActionHandler)& BasescapeState::btnManufactureClick);

	_btnPurchase->setText(tr("STR_PURCHASE_RECRUIT"));
	_btnPurchase->onMouseClick((ActionHandler)& BasescapeState::btnPurchaseClick);

	_btnSell->setText(tr("STR_SELL_SACK_UC"));
	_btnSell->onMouseClick((ActionHandler)& BasescapeState::btnSellClick);

	_btnTransfer->setText(tr("STR_TRANSFER_UC"));
	_btnTransfer->onMouseClick((ActionHandler)& BasescapeState::btnTransferClick);

	_btnIncTrans->setText(tr("STR_TRANSIT_LC"));
	_btnIncTrans->onMouseClick((ActionHandler)& BasescapeState::btnIncTransClick);

	_btnFacilities->setText(tr("STR_BUILD_FACILITIES"));
	_btnFacilities->onMouseClick((ActionHandler)& BasescapeState::btnFacilitiesClick);

	_btnGeoscape->setText(tr("STR_GEOSCAPE_UC"));
	_btnGeoscape->onMouseClick((ActionHandler)& BasescapeState::btnGeoscapeClick);
	_btnGeoscape->onKeyboardPress(
					(ActionHandler)& BasescapeState::btnGeoscapeClick,
					Options::keyCancel);

//	_btnNewBase->setText(tr("STR_BUILD_NEW_BASE_UC"));
//	_btnNewBase->onMouseClick((ActionHandler)& BasescapeState::btnNewBaseClick);
}

/**
 * dTor.
 */
BasescapeState::~BasescapeState()
{
	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		if (*i == _base)
			return;
	}

	delete _base;
}

/**
 * The player can change the selected base or change info on other screens.
 */
void BasescapeState::init()
{
	State::init();

	setBase(_base);

	_view->setBase(_base);
	_mini->draw();
	_edtBase->setText(_base->getName());

	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										_base->getLongitude(),
										_base->getLatitude()) == true)
		{
			_txtRegion->setText(tr((*i)->getRules()->getType()));
			break;
		}
	}

	_txtFunds->setText(tr("STR_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

//	_btnNewBase->setVisible(_game->getSavedGame()->getBases()->size() < MiniBaseView::MAX_BASES);

	bool
		hasFunds		= (_game->getSavedGame()->getFunds() > 0),
		hasQuarters		= false,
		hasSoldiers		= false,
		hasScientists	= false,
		hasEngineers	= false,
		hasHangar		= false,
		hasCraft		= false,
		hasAlienCont	= false,
		hasLabs			= false,
		hasProd			= false,
		hasStores		= false;

	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->getRules()->getPersonnel() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasQuarters = true;

			if (_base->getSoldiers()->size() > 0)
				hasSoldiers = true;

			if (_base->getScientists() > 0
				|| _base->getAllocatedScientists() > 0)
			{
				hasScientists = true;
			}

			if (_base->getEngineers() > 0
				|| _base->getAllocatedEngineers() > 0)
			{
				hasEngineers = true;
			}
		}

		if ((*i)->getRules()->getCrafts() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasHangar = true;

			if (_base->getCrafts()->size() > 0)
				hasCraft = true;
		}

		if ((*i)->getRules()->getAliens() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasAlienCont = true;
		}

		if ((*i)->getRules()->getLaboratories() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasLabs = true;
		}

		if ((*i)->getRules()->getWorkshops() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasProd = true;
		}

		if ((*i)->getRules()->getStorage() > 0
			&& (*i)->getBuildTime() == 0)
		{
			hasStores = true;
		}
	}

	_btnSoldiers->setVisible(hasSoldiers);
	_btnCrafts->setVisible(hasCraft);
	_btnAliens->setVisible(hasAlienCont); // ie. hasLiveAliens
	_btnResearch->setVisible(hasLabs && hasScientists);
	_btnManufacture->setVisible(hasProd && hasEngineers);
	_btnPurchase->setVisible(hasFunds && (hasStores || hasQuarters || hasHangar)); // ie, hasFree... space
	_btnSell->setVisible(hasStores || hasQuarters || hasCraft || hasAlienCont); // ie. hasFreeSoldiers || hasFreeScientists || hasFreeEngineers || hasLiveAliens || hasItemsInStorage
	_btnTransfer->setVisible(hasFunds && (hasStores || hasQuarters || hasCraft || hasAlienCont)); // ditto.
	_btnFacilities->setVisible(hasFunds);

	_btnIncTrans->setVisible(_base->getTransfers()->empty() == false);


	std::string track = OpenXcom::res_MUSIC_GEO_GLOBE;
	if (_game->getResourcePack()->isMusicPlaying(track) == false) // stop Dogfight music, Pls.
	{
		_game->getResourcePack()->fadeMusic(_game, 345);
		_game->getResourcePack()->playMusic(track);
	}
}

/**
 * Changes the base currently displayed on screen.
 * @param base - pointer to new base to display
 */
void BasescapeState::setBase(Base* base)
{
	if (_game->getSavedGame()->getBases()->empty() == false)
	{
		bool exists = false; // check if base still exists

		for (size_t
				i = 0;
				i != _game->getSavedGame()->getBases()->size();
				++i)
		{
			if (base == _game->getSavedGame()->getBases()->at(i))
			{
				_base = base;
				_mini->setSelectedBase(i);
//				_game->getSavedGame()->setSelectedBase(i);

				exists = true;
				break;
			}
		}

		if (exists == false)
		{
			_base = _game->getSavedGame()->getBases()->front();
			_mini->setSelectedBase(0);
//			_game->getSavedGame()->setSelectedBase(0);
		}
	}
	else
	{
		_base = new Base(_game->getRuleset());
		_mini->setSelectedBase(0);
//		_game->getSavedGame()->setSelectedBase(0);
	}
}

/**
 * Goes to the Build New Base screen.
 * @param action - pointer to an Action
 */
/* void BasescapeState::btnNewBaseClick(Action*)
{
	Base* base = new Base(_game->getRuleset());

	_game->popState();
	_game->pushState(new BuildNewBaseState(
										base,
										_globe,
										false));
} */

/**
 * Goes to the Base Info screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnBaseInfoClick(Action*)
{
	_game->pushState(new BaseInfoState(
									_base,
									this));
}

/**
 * Goes to the Soldiers screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnSoldiersClick(Action*)
{
	_game->pushState(new SoldiersState(_base));
}

/**
 * Goes to the Crafts screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnCraftsClick(Action*)
{
	_game->pushState(new CraftsState(_base));
}

/**
 * Goes to the Manage Alien Containment screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnAliens(Action*)
{
	_game->pushState(new AlienContainmentState(
											_base,
											OPT_GEOSCAPE));
}

/**
 * Goes to the Research screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnResearchClick(Action*)
{
	_game->pushState(new ResearchState(
									_base,
									this));
}

/**
 * Goes to the Manufacture screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnManufactureClick(Action*)
{
	_game->pushState(new ManufactureState(
									_base,
									this));
}

/**
 * Goes to the Purchase screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnPurchaseClick(Action*)
{
	_game->pushState(new PurchaseState(_base));
}

/**
 * Goes to the Sell screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnSellClick(Action*)
{
	_game->pushState(new SellState(_base));
}

/**
 * Goes to the Select Destination Base window.
 * @param action - pointer to an Action
 */
void BasescapeState::btnTransferClick(Action*)
{
	_game->pushState(new TransferBaseState(_base));
}

/**
 * Goes to the incoming Transfers window.
 * @param action - pointer to an Action
 */
void BasescapeState::btnIncTransClick(Action*)
{
	_game->pushState(new TransfersState(_base));
}

/**
 * Opens the Build Facilities window.
 * @param action - pointer to an Action
 */
void BasescapeState::btnFacilitiesClick(Action*)
{
	_game->pushState(new BuildFacilitiesState(
										_base,
										this));
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnGeoscapeClick(Action*)
{
	kL_geoMusic = false;
	_game->popState();
}

/**
 * Processes left clicking on facilities.
 * @param action - pointer to an Action
 */
void BasescapeState::viewLeftClick(Action*)
{
	bool bPop = false; // play "wha-wha" sound

	const BaseFacility* const fac = _view->getSelectedFacility();

	if (fac == NULL) // dirt.
	{
		bPop = true;
		_game->pushState(new MonthlyCostsState(_base));
//		_game->pushState(new BaseInfoState(_base, this));
	}
	else if (fac->getBuildTime() > 0)
		return;
	else if (fac->getRules()->getCrafts() > 0)
	{
		for (size_t
				i = 0;
				i != _base->getCrafts()->size();
				++i)
		{
			if (fac->getCraft() == NULL
				|| fac->getCraft()->getStatus() == "STR_OUT")
			{
				bPop = true;
				_game->pushState(new CraftsState(_base));
				break;
			}
			else if (fac->getCraft() == _base->getCrafts()->at(i))
			{
				_game->pushState(new CraftInfoState(
												_base,
												i));
				break;
			}
		}
	}
	else if (fac->getRules()->getStorage() > 0)
	{
		bPop = true;
		_game->pushState(new StoresState(_base));
//		_game->pushState(new SellState(_base));
	}
	else if (fac->getRules()->getPersonnel() > 0
		&& _base->getSoldiers()->empty() == false)
	{
		bPop = true;
		_game->pushState(new SoldiersState(_base));
	}
	else if (fac->getRules()->getPsiLaboratories() > 0
			&& Options::anytimePsiTraining == true)
	{
		bPop = true;
		_game->pushState(new AllocatePsiTrainingState(_base));
	}
	else if (fac->getRules()->getLaboratories() > 0)
	{
		bPop = true;
		_game->pushState(new ResearchState(
										_base,
										this));
	}
	else if (fac->getRules()->getWorkshops() > 0)
	{
		bPop = true;
		_game->pushState(new ManufactureState(
											_base,
											this));
	}
	else if (fac->getRules()->getAliens() > 0)
	{
		bPop = true;
		_game->pushState(new AlienContainmentState(
												_base,
												OPT_GEOSCAPE));
	}
	else if (fac->getRules()->isLift() == true) // Lift has radar range ... (cf. next)
		_game->pushState(new BaseDetectionState(_base));

/*	else if (fac->getRules()->getRadarRange() > 0
		|| fac->getRules()->isMindShield() == true
		|| fac->getRules()->isHyperwave() == true)
	{
		bPop = true;
		_game->getSavedGame()->setGlobeLongitude(_base->getLongitude());
		_game->getSavedGame()->setGlobeLatitude(_base->getLatitude());
		kL_reCenter = true;
		_game->popState();
	} */


	if (bPop == true)
		kL_soundPop->play(Mix_GroupAvailable(0));
}

/**
 * Processes right clicking on facilities.
 * @param action - pointer to an Action
 */
void BasescapeState::viewRightClick(Action*)
{
	BaseFacility* const fac = _view->getSelectedFacility();
	if (fac != NULL)
	{
		if (fac->inUse() == true)
			_game->pushState(new ErrorMessageState(
											tr("STR_FACILITY_IN_USE"),
											_palette,
											_game->getRuleset()->getInterface("basescape")->getElement("errorMessage")->color,
											"BACK13.SCR",
											_game->getRuleset()->getInterface("basescape")->getElement("errorPalette")->color));
		else if (_base->getDisconnectedFacilities(fac).empty() == false)
			_game->pushState(new ErrorMessageState(
											tr("STR_CANNOT_DISMANTLE_FACILITY"),
											_palette,
											_game->getRuleset()->getInterface("basescape")->getElement("errorMessage")->color,
											"BACK13.SCR",
											_game->getRuleset()->getInterface("basescape")->getElement("errorPalette")->color));
		else
			_game->pushState(new DismantleFacilityState(
													_base,
													_view,
													fac));
	}
}

/**
 * Displays the name of the facility the mouse is over.
 * @param action - pointer to an Action
 */
void BasescapeState::viewMouseOver(Action*)
{
	std::wostringstream woststr;

	const BaseFacility* const fac = _view->getSelectedFacility();
	const size_t base = _mini->getHoveredBase();

	if (fac != NULL)
	{
		_txtFacility->setAlign(ALIGN_LEFT);

		if (fac->getRules()->getCrafts() == 0
			|| fac->getBuildTime() > 0)
		{
			woststr << tr(fac->getRules()->getType());
		}
		else
		{
			woststr << tr(fac->getRules()->getType());

			if (fac->getCraft() != NULL)
				woststr << L" " << tr("STR_CRAFT_")
									.arg(fac->getCraft()->getName(_game->getLanguage()));
		}
	}
	else if (base < _game->getSavedGame()->getBases()->size()
		&& _base != _game->getSavedGame()->getBases()->at(base))
	{
		_txtFacility->setAlign(ALIGN_RIGHT);
		woststr << _game->getSavedGame()->getBases()->at(base)->getName(_game->getLanguage()).c_str();
	}

	_txtFacility->setText(woststr.str());
}

/**
 * Clears the facility name.
 * @param action - pointer to an Action
 */
void BasescapeState::viewMouseOut(Action*)
{
	_txtFacility->setText(L"");
}

/**
 * Selects a new base to display.
 * @param action - pointer to an Action
 */
void BasescapeState::miniLeftClick(Action*)
{
	const size_t base = _mini->getHoveredBase();

	if (base < _game->getSavedGame()->getBases()->size()
		&& _base != _game->getSavedGame()->getBases()->at(base))
	{
		_base = _game->getSavedGame()->getBases()->at(base);
		_txtFacility->setText(L"");

		init();
	}
	else if (base == _game->getSavedGame()->getBases()->size())
	{
		kL_geoMusic = false;

		// aka: btnNewBaseClick();
		// courtesy kkmic, http://openxcom.org/forum/index.php?topic=1558.msg32461#msg32461
		Base* const base = new Base(_game->getRuleset());

		_game->popState();
		_game->pushState(new BuildNewBaseState(
											base,
											_globe,
											false));
	}
}

/**
 * Pops to globe with selected base centered.
 * @param action - pointer to an Action
 */
void BasescapeState::miniRightClick(Action*)
{
	const size_t base = _mini->getHoveredBase();

	if (base < _game->getSavedGame()->getBases()->size())
	{
		const Base* const centBase = _game->getSavedGame()->getBases()->at(base);
		_game->getSavedGame()->setGlobeLongitude(centBase->getLongitude());
		_game->getSavedGame()->setGlobeLatitude(centBase->getLatitude());

		kL_reCenter = true;
		kL_geoMusic = false;

		_game->popState();
		kL_soundPop->play(Mix_GroupAvailable(0));
	}
}

/**
 * Selects a new base to display.
 * @param action - pointer to an Action
 */
void BasescapeState::handleKeyPress(Action* action)
{
	if (action->getDetails()->type == SDL_KEYDOWN)
	{
		SDLKey baseKeys[] =
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

		const int key = action->getDetails()->key.keysym.sym;
		for (size_t
				i = 0;
				i != _game->getSavedGame()->getBases()->size();
				++i)
		{
			if (key == baseKeys[i])
			{
				_base = _game->getSavedGame()->getBases()->at(i);
				init();

				break;
			}
		}
	}
}

/**
 * Changes the Base name.
 * @param action - pointer to an Action
 */
void BasescapeState::edtBaseChange(Action*)
{
	_base->setName(_edtBase->getText());
}

}
