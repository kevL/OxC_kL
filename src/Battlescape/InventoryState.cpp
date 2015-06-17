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

#include "InventoryState.h"

//#include <sstream>

#include "BattlescapeState.h"
//#include "Camera.h"
#include "Inventory.h"
//#include "Map.h"
//#include "Pathfinding.h"
#include "TileEngine.h"
#include "UnitInfoState.h"

#include "../Engine/Action.h"
//#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
//#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
//#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/BattlescapeButton.h"
#include "../Interface/Text.h"
#include "../Interface/NumberText.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

//static const int _templateBtnX		= 288;
//static const int _createTemplateBtnY	= 67;	//90
//static const int _applyTemplateBtnY	= 90;	//113
//static const int _clearInventoryBtnY	= 113;


/**
 * Initializes all the elements in the Inventory screen.
 * @param tuMode - true if in battle when inventory usage costs time units
 * @param parent - pointer to parent BattlescapeState
 */
InventoryState::InventoryState(
		bool tuMode,
		BattlescapeState* parent)
	:
		_tuMode(tuMode),
		_parent(parent)
{
	_battleSave = _game->getSavedGame()->getSavedBattle();
/*kL
	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	}
	else if (!_battleSave->getTileEngine())
	{
		Screen::updateScale(
						Options::battlescapeScale,
						Options::battlescapeScale,
						Options::baseXBattlescape,
						Options::baseYBattlescape,
						true);
		_game->getScreen()->resetDisplay(false);
	} */

	_bg			= new Surface(320, 200);
	_soldier	= new Surface(320, 200);

	_txtName	= new Text(200, 17, 36, 6);
	_gender		= new Surface(7, 7, 28, 1);

	_txtWeight	= new Text(70, 9, 237, 24); // 237 -> was, 245
	_txtTUs		= new Text(40, 9, 237, 24);
	_txtFAcc	= new Text(40, 9, 237, 32);
	_txtReact	= new Text(40, 9, 237, 40);
	_txtThrow	= new Text(40, 9, 237, 48);
	_txtMelee	= new Text(40, 9, 237, 56);
	_txtPStr	= new Text(40, 9, 237, 64);
	_txtPSkill	= new Text(40, 9, 237, 72);

	_txtUseTU	= new Text(45, 9, 245, 123);
	_txtThrowTU	= new Text(40, 9, 245, 132);
	_txtPsiTU	= new Text(40, 9, 245, 141);

	_numOrder	= new NumberText(7, 5, 228,  4);
	_tuCost		= new NumberText(7, 5, 310, 60);

	_txtItem	= new Text(160, 9, 128, 140);

	_btnOk		= new BattlescapeButton(35, 23, 237, 0);
	_btnPrev	= new BattlescapeButton(23, 23, 273, 0);
	_btnNext	= new BattlescapeButton(23, 23, 297, 0);

	_btnRank	= new BattlescapeButton(26, 23,   0,   0);
	_btnUnload	= new BattlescapeButton(32, 25, 288,  32);
	_btnGround	= new BattlescapeButton(32, 15, 288, 137);

/*	_btnCreateTemplate	= new BattlescapeButton(
											32,
											22,
											_templateBtnX,
											_createTemplateBtnY);
	_btnApplyTemplate	= new BattlescapeButton(
											32,
											22,
											_templateBtnX,
											_applyTemplateBtnY);
	_btnClearInventory	= new BattlescapeButton(
											32,
											22,
											_templateBtnX,
											_clearInventoryBtnY); */

//	_txtAmmo	= new Text(40, 24, 272, 64);
	_txtAmmo	= new Text(40, 24, 288, 64);
	_selAmmo	= new Surface(
						RuleInventory::HAND_W * RuleInventory::SLOT_W,
						RuleInventory::HAND_H * RuleInventory::SLOT_H,
						288, // 272,
						88);

	_inv		= new Inventory(
							_game,
							320,
							200,
							0,0,
							_parent == NULL);

	setPalette("PAL_BATTLESCAPE");

	add(_bg);
	_game->getResourcePack()->getSurface("TAC01.SCR")->blit(_bg);

	add(_gender);
	add(_soldier);
	add(_txtName,		"textName",			"inventory", _bg);

	add(_txtWeight,		"textWeight",		"inventory", _bg);
	add(_txtTUs,		"textTUs",			"inventory", _bg);
	add(_txtFAcc,		"textFiring",		"inventory", _bg);
	add(_txtReact,		"textReaction",		"inventory", _bg);
	add(_txtThrow,		"textThrowing",		"inventory", _bg);
	add(_txtMelee,		"textMelee",		"inventory", _bg);
	add(_txtPStr,		"textPsiStrength",	"inventory", _bg);
	add(_txtPSkill,		"textPsiSkill",		"inventory", _bg);

	add(_numOrder);
	add(_tuCost);
	add(_txtItem,		"textItem",			"inventory", _bg);
	add(_txtAmmo,		"textAmmo",			"inventory", _bg);
	add(_btnOk,			"buttonOK",			"inventory", _bg);
	add(_btnPrev,		"buttonPrev",		"inventory", _bg);
	add(_btnNext,		"buttonNext",		"inventory", _bg);
	add(_btnUnload,		"buttonUnload",		"inventory", _bg);
	add(_btnGround,		"buttonGround",		"inventory", _bg);
	add(_btnRank,		"rank",				"inventory", _bg);
//	add(_btnCreateTemplate,	"buttonCreate",	"inventory", _bg);
//	add(_btnApplyTemplate,	"buttonApply",	"inventory", _bg);
//	add(_btnClearInventory);
	add(_selAmmo);
	add(_inv);
	add(_txtUseTU,		"textTUs",			"inventory", _bg);
	add(_txtThrowTU,	"textTUs",			"inventory", _bg);
	add(_txtPsiTU,		"textTUs",			"inventory", _bg);

	// move the TU display down to make room for the weight display
	if (Options::showMoreStatsInInventoryView == true)
		_txtTUs->setY(_txtTUs->getY() + 8);

	centerAllSurfaces();


	_txtName->setBig();
	_txtName->setHighContrast();

	_txtWeight->setHighContrast();
	_txtTUs->setHighContrast();
	_txtFAcc->setHighContrast();
	_txtReact->setHighContrast();
	_txtThrow->setHighContrast();
	_txtMelee->setHighContrast();
	_txtPStr->setHighContrast();
	_txtPSkill->setHighContrast();
	_txtUseTU->setHighContrast();
	_txtThrowTU->setHighContrast();
	_txtPsiTU->setHighContrast();

	_numOrder->setColor(1);
	_numOrder->setVisible(false);

	_tuCost->setColor(1);
	_tuCost->setVisible(false);

	_txtItem->setHighContrast();

	_txtAmmo->setAlign(ALIGN_LEFT);
	_txtAmmo->setHighContrast();

	_btnOk->onMouseClick((ActionHandler)& InventoryState::btnOkClick);
	_btnOk->onKeyboardPress(
						(ActionHandler)& InventoryState::btnOkClick,
						Options::keyCancel);
	_btnOk->onKeyboardPress(
						(ActionHandler)& InventoryState::btnOkClick,
						Options::keyBattleInventory);
//	_btnOk->setTooltip("STR_OK");
//	_btnOk->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnOk->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnPrev->onMouseClick((ActionHandler)& InventoryState::btnPrevClick);
	_btnPrev->onKeyboardPress(
						(ActionHandler)& InventoryState::btnPrevClick,
						Options::keyBattlePrevUnit);
//	_btnPrev->setTooltip("STR_PREVIOUS_UNIT");
//	_btnPrev->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnPrev->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnNext->onMouseClick((ActionHandler)& InventoryState::btnNextClick);
	_btnNext->onKeyboardPress(
						(ActionHandler)& InventoryState::btnNextClick,
						Options::keyBattleNextUnit);
//	_btnNext->setTooltip("STR_NEXT_UNIT");
//	_btnNext->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnNext->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnUnload->onMouseClick(
					(ActionHandler)& InventoryState::btnUnloadClick,
					SDL_BUTTON_LEFT);
	_btnUnload->onMouseClick(
					(ActionHandler)& InventoryState::btnSaveLayouts,
					SDL_BUTTON_RIGHT);
//	_btnUnload->onMouseClick((ActionHandler)& InventoryState::btnUnloadClick);
//	_btnUnload->setTooltip("STR_UNLOAD_WEAPON");
//	_btnUnload->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnUnload->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnGround->onMouseClick(
					(ActionHandler)& InventoryState::btnGroundClick,
					SDL_BUTTON_LEFT);
	_btnGround->onMouseClick(
					(ActionHandler)& InventoryState::btnUnloadUnitClick,
					SDL_BUTTON_RIGHT);
//	_btnGround->setTooltip("STR_SCROLL_RIGHT");
//	_btnGround->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnGround->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnRank->onMouseClick((ActionHandler)& InventoryState::btnRankClick);
//	_btnRank->setTooltip("STR_UNIT_STATS");
//	_btnRank->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnRank->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);


/*	_btnCreateTemplate->onMouseClick((ActionHandler)& InventoryState::btnCreateTemplateClick);
	_btnCreateTemplate->onKeyboardPress(
						(ActionHandler)& InventoryState::btnCreateTemplateClick,
						Options::keyInvCreateTemplate); */
//	_btnCreateTemplate->setTooltip("STR_CREATE_INVENTORY_TEMPLATE");
//	_btnCreateTemplate->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnCreateTemplate->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

/*	_btnApplyTemplate->onMouseClick((ActionHandler)& InventoryState::btnApplyTemplateClick);
	_btnApplyTemplate->onKeyboardPress(
						(ActionHandler)& InventoryState::btnApplyTemplateClick,
						Options::keyInvApplyTemplate); */
//	_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
//	_btnApplyTemplate->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnApplyTemplate->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

/*	_btnClearInventory->onMouseClick((ActionHandler)& InventoryState::btnUnloadUnitClick);
	_btnClearInventory->onKeyboardPress(
					(ActionHandler)& InventoryState::btnUnloadUnitClick,
					Options::keyInvClear); */
//	_btnClearInventory->setTooltip("STR_CLEAR_INVENTORY");
//	_btnClearInventory->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnClearInventory->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);


	// only use copy/paste layout-template buttons in setup (i.e. non-tu) mode
/*	if (_tuMode)
	{
		_btnCreateTemplate->setVisible(false);
		_btnApplyTemplate->setVisible(false);
		_btnClearInventory->setVisible(false);
	}
	else
		_updateTemplateButtons(true); */

	_inv->draw();
	_inv->setTuMode(_tuMode);
	_inv->setSelectedUnit(_game->getSavedGame()->getSavedBattle()->getSelectedUnit());
	_inv->onMouseClick((ActionHandler)& InventoryState::invClick, 0);
	_inv->onMouseOver((ActionHandler)& InventoryState::invMouseOver);
	_inv->onMouseOut((ActionHandler)& InventoryState::invMouseOut);

	_txtWeight->setVisible(Options::showMoreStatsInInventoryView);
	_txtTUs->setVisible(_tuMode);
	_txtUseTU->setVisible(_tuMode);

	const bool vis = _tuMode == false
				  && Options::showMoreStatsInInventoryView == true;
	_txtFAcc->setVisible(vis);
	_txtReact->setVisible(vis);
	_txtThrow->setVisible(vis);
	_txtMelee->setVisible(vis);
	_txtPStr->setVisible(vis);
	_txtPSkill->setVisible(vis);
}

/**
 * Helper for the dTor.
 */
/* static void _clearInventoryTemplate(std::vector<EquipmentLayoutItem*>& inventoryTemplate)
{
	for (std::vector<EquipmentLayoutItem*>::iterator
			i = inventoryTemplate.begin();
			i != inventoryTemplate.end();
			)
	{
		delete *i;
	}
} */

/**
 * dTor.
 */
InventoryState::~InventoryState()
{
//	_clearInventoryTemplate(_curInventoryTemplate);

	// kL_begin:
	if (_parent != NULL)
	{
		TileEngine* const tileEngine = _battleSave->getTileEngine();

		tileEngine->applyGravity(_battleSave->getSelectedUnit()->getTile());
		tileEngine->calculateTerrainLighting();
		tileEngine->recalculateFOV();
	} // kL_end.
}
//	if (_battleSave->getTileEngine())
//	{
//		if (Options::maximizeInfoScreens)
//		{
//			Screen::updateScale(
//							Options::battlescapeScale,
//							Options::battlescapeScale,
//							Options::baseXBattlescape,
//							Options::baseYBattlescape,
//							true);
//			_game->getScreen()->resetDisplay(false);
//		}

//kL	Tile* tile = _battleSave->getSelectedUnit()->getTile();
//kL	_battleSave->getTileEngine()->applyGravity(tile);
//kL	_battleSave->getTileEngine()->calculateTerrainLighting(); // dropping/picking up flares
//kL	_battleSave->getTileEngine()->recalculateFOV();

//	}
//	else
//	{
//		Screen::updateScale(
//						Options::geoscapeScale,
//						Options::geoscapeScale,
//						Options::baseXGeoscape,
//						Options::baseYGeoscape,
//						true);
//		_game->getScreen()->resetDisplay(false);
//	}

/**
 * Updates all soldier stats when the soldier changes.
 @note parent BattlescapeState is invalid @ BaseEquip screen
 */
void InventoryState::init()
{
	State::init();

	BattleUnit* unit = _battleSave->getSelectedUnit();
	if (unit == NULL) // no selected unit, close inventory
	{
		btnOkClick(NULL);
		return;
	}
	else if (unit->hasInventory() == false) // skip to the first unit with inventory
	{
		if (_parent != NULL)
			_parent->selectNextFactionUnit(
										false,
										false,
										true);
		else
			_battleSave->selectNextFactionUnit(
											false,
											false,
											true);

		if (_battleSave->getSelectedUnit() == NULL
			 || _battleSave->getSelectedUnit()->hasInventory() == false)
		{
			btnOkClick(NULL);
			return; // starting a mission with just vehicles. kL_note: DISALLOWED!!!
		}
		else
			unit = _battleSave->getSelectedUnit();
	}

//	if (_parent)
//		_parent->getMap()->getCamera()->centerOnPosition(unit->getPosition(), false);

	unit->setCache(NULL);

	_soldier->clear();
	_btnRank->clear();
	_gender->clear();

	_txtName->setText(unit->getName(_game->getLanguage()));

	_inv->setSelectedUnit(unit);

	const Soldier* const soldier = unit->getGeoscapeSoldier();
	if (soldier != NULL)
	{
		Surface* gender = NULL;
		if (soldier->getGender() == GENDER_MALE)
			gender = _game->getResourcePack()->getSurface("GENDER_M");
		else
			gender = _game->getResourcePack()->getSurface("GENDER_F");

		if (gender != NULL)
			gender->blit(_gender);

		SurfaceSet* const sstRank = _game->getResourcePack()->getSurfaceSet("SMOKE.PCK");
//		sstRank->getFrame(20 + soldier->getRank())->setX(0);
//		sstRank->getFrame(20 + soldier->getRank())->setY(0);
		sstRank->getFrame(20 + soldier->getRank())->blit(_btnRank);

		std::string look = soldier->getArmor()->getSpriteInventory();
		if (soldier->getGender() == GENDER_MALE)
			look += "M";
		else
			look += "F";

		if		(soldier->getLook() == LOOK_BLONDE)		look += "0";
		else if (soldier->getLook() == LOOK_BROWNHAIR)	look += "1";
		else if (soldier->getLook() == LOOK_ORIENTAL)	look += "2";
		else if (soldier->getLook() == LOOK_AFRICAN)	look += "3";

		look += ".SPK";

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile("UFOGRAPH/" + look)) == false
			&& _game->getResourcePack()->getSurface(look) == NULL)
		{
			look = soldier->getArmor()->getSpriteInventory() + ".SPK";
		}

		_game->getResourcePack()->getSurface(look)->blit(_soldier);
	}
	else
	{
		SurfaceSet* const sstRank = _game->getResourcePack()->getSurfaceSet("SMOKE.PCK");
//		sstRank->getFrame(26)->setX(0);
//		sstRank->getFrame(26)->setY(0);
		sstRank->getFrame(26)->blit(_btnRank);

		Surface* const srfArmor = _game->getResourcePack()->getSurface(unit->getArmor()->getSpriteInventory());
		if (srfArmor != NULL)
			srfArmor->blit(_soldier);
	}

	updateStats();
	refreshMouse();
}

/**
 * Updates the selected unit's info - weight, TU, etc.
 */
void InventoryState::updateStats() // private.
{
	const BattleUnit* const unit = _battleSave->getSelectedUnit();

	if (unit->getGeoscapeSoldier() != NULL)
	{
		_numOrder->setValue(unit->getBattleOrder());
		_numOrder->setVisible();
//		_numOrder->setVisible(unit->getOriginalFaction() == FACTION_PLAYER);
	}
	else
		_numOrder->setVisible(false);

	if (_tuMode == true)
		_txtTUs->setText(tr("STR_TIME_UNITS_SHORT").arg(unit->getTimeUnits()));

	if (Options::showMoreStatsInInventoryView == true)
	{
		const int
			weight = unit->getCarriedWeight(_inv->getSelectedItem()),
			strength = static_cast<int>(Round(
					   static_cast<double>(unit->getBaseStats()->strength)
					   * (unit->getAccuracyModifier() / 2. + 0.5)));

		_txtWeight->setText(tr("STR_WEIGHT").arg(weight).arg(strength));
		if (weight > strength)
			_txtWeight->setSecondaryColor(static_cast<Uint8>(
							_game->getRuleset()->getInterface("inventory")->getElement("weight")->color2));
		else
			_txtWeight->setSecondaryColor(static_cast<Uint8>(
							_game->getRuleset()->getInterface("inventory")->getElement("weight")->color));

		const int psiSkill = unit->getBaseStats()->psiSkill;

		if (_tuMode == true)
		{
			_txtThrowTU->setText(tr("STR_THROW_").arg(unit->getActionTUs(BA_THROW)));

			if (unit->getOriginalFaction() == FACTION_HOSTILE
				&& psiSkill > 0)
			{
				_txtPsiTU->setVisible();
				_txtPsiTU->setText(tr("STR_PSI_").arg(unit->getActionTUs(
																	BA_PANIC,
																	_parent->getBattleGame()->getAlienPsi())));
			}
			else
				_txtPsiTU->setVisible(false);
		}
		else
		{
			_txtFAcc->setText(tr("STR_ACCURACY_SHORT").arg(unit->getBaseStats()->firing));
			_txtReact->setText(tr("STR_REACTIONS_SHORT").arg(unit->getBaseStats()->reactions));
			_txtThrow->setText(tr("STR_THROWACC_SHORT").arg(unit->getBaseStats()->throwing));
			_txtMelee->setText(tr("STR_MELEEACC_SHORT").arg(unit->getBaseStats()->melee));

			if (psiSkill > 0)
			{
				_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(unit->getBaseStats()->psiStrength));
				_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(psiSkill));
			}
			else
			{
				_txtPStr->setText(L"");
				_txtPSkill->setText(L"");
			}
/*			int minPsi;
			if (unit->getGeoscapeSoldier() != NULL)
				minPsi = unit->getGeoscapeSoldier()->getRules()->getMinStats().psiSkill;
			else
				minPsi = 0;

			if (psiSkill >= minPsi)
				_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(psiSkill));
			else
				_txtPSkill->setText(L"");

			if (psiSkill >= minPsi
//				|| (Options::psiStrengthEval == true
//					&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements())))
			{
				_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(unit->getBaseStats()->psiStrength));
			}
			else
				_txtPStr->setText(L""); */
		}
	}
}

/**
 * Jogs the mouse cursor to refresh appearances.
 */
void InventoryState::refreshMouse() // private.
{
	int
		x,y;
	SDL_GetMouseState(&x,&y);

	SDL_WarpMouse(
			static_cast<Uint16>(x + 1),
			static_cast<Uint16>(y));	// send a mouse motion event to refresh any hover actions
	SDL_WarpMouse(
			static_cast<Uint16>(x),
			static_cast<Uint16>(y));	// move the mouse back to avoid cursor creep
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void InventoryState::btnOkClick(Action*)
{
	if (_inv->getSelectedItem() == NULL)
	{
		_game->popState();

		if (_tuMode == false	// pre-Battle but
			&& _parent != NULL)	// going into Battlescape!
		{
			_battleSave->resetUnitTiles();

			Tile* const invTile = _battleSave->getBattleInventory();
			_battleSave->randomizeItemLocations(invTile);	// This doesn't seem to happen on second stage of Multi-State MISSIONS.
															// In fact, none of this !_tuMode InventoryState appears to run for 2nd staged missions.
															// and BattlescapeGenerator::nextStage() has its own bu->prepUnit() call ....
/*			if (_battleSave->getTurn() == 1) // Leaving this out could be troublesome for Multi-Stage MISSIONS.
			{
				//Log(LOG_INFO) << ". turn = 1";
				_battleSave->randomizeItemLocations(invTile);
				if (invTile->getUnit())
				{
					// make sure the unit closest to the ramp is selected.
					_battleSave->setSelectedUnit(invTile->getUnit());
				}
			} */

			for (std::vector<BattleUnit*>::const_iterator
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getFaction() == FACTION_PLAYER)
					(*i)->initTU(true);
			}
		}
	}
}

/**
 * Selects the previous soldier.
 * @param action - pointer to an Action
 */
void InventoryState::btnPrevClick(Action*)
{
	if (_inv->getSelectedItem() != NULL)
		return;

	if (_parent != NULL)
		_parent->selectPreviousFactionUnit(
										false,
										false,
										true);
	else
		_battleSave->selectPreviousFactionUnit(
											false,
											false,
											true);
	init();
}

/**
 * Selects the next soldier.
 * @param action - pointer to an Action
 */
void InventoryState::btnNextClick(Action*)
{
	if (_inv->getSelectedItem() != NULL)
		return;

	if (_parent != NULL)
		_parent->selectNextFactionUnit(
									false,
									false,
									true);
	else
		_battleSave->selectNextFactionUnit(
										false,
										false,
										true);
	init();
}

/**
 * Unloads the selected weapon or saves soldier's equipment layout.
 * @param action - pointer to an Action
 */
void InventoryState::btnUnloadClick(Action*)
{
	if (_inv->unload() == true)
	{
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");
		_txtUseTU->setText(L"");

		_selAmmo->clear();

		updateStats();
//		_game->getResourcePack()->getSoundByDepth(
//											0,
		_game->getResourcePack()->getSound(
										"BATTLE.CAT",
										ResourcePack::ITEM_UNLOAD_HQ)
									->play();
	}
	else if (_tuMode == false
		&& _inv->getSelectedItem() == NULL)
	{
		if (saveLayout(_battleSave->getSelectedUnit()))
			_inv->showWarning(tr("STR_EQUIP_LAYOUT_SAVED"));
	}
}

/**
 * Saves a Soldier's equipment layout.
 * @note Called from btnUnloadClick() if in pre-battle.
 * @param unit - pointer to a BattleUnit
 * @return, true if layout saved
 */
bool InventoryState::saveLayout(BattleUnit* const unit) const // private.
{
	bool ret = false;

	if (unit->getGeoscapeSoldier() != NULL)
	{
		ret = true;
		std::vector<EquipmentLayoutItem*>* const layoutItems = unit->getGeoscapeSoldier()->getEquipmentLayout();

		if (layoutItems->empty() == false) // clear Soldier's items
		{
			for (std::vector<EquipmentLayoutItem*>::const_iterator
					i = layoutItems->begin();
					i != layoutItems->end();
					++i)
			{
				delete *i;
			}

			layoutItems->clear();
		}

		// Note: when using getInventory() the loaded ammos are skipped because
		// they're not owned by the unit; ammo is handled separately by the weapon.
		for (std::vector<BattleItem*>::const_iterator // save Soldier's items
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end();
				++i)
		{
			std::string ammo;
			if ((*i)->usesAmmo() == true
				&& (*i)->getAmmoItem() != NULL)
			{
				ammo = (*i)->getAmmoItem()->getRules()->getType();
			}
			else
				ammo = "NONE";

			layoutItems->push_back(new EquipmentLayoutItem(
													(*i)->getRules()->getType(),
													(*i)->getSlot()->getId(),
													(*i)->getSlotX(),
													(*i)->getSlotY(),
													ammo,
													(*i)->getFuseTimer()));
		}
	}

	return ret;
}

/**
 * Saves all Soldiers' equipment layouts in pre-battle.
 * @param action - pointer to an Action
 */
void InventoryState::btnSaveLayouts(Action*) // private.
{
	if (_tuMode == false
		&& _inv->getSelectedItem() == NULL)
	{
		if (saveAllLayouts() == true)
			_inv->showWarning(tr("STR_EQUIP_LAYOUTS_SAVED"));
	}
}

/**
 * Saves all Soldiers' equipment layouts.
 * @return, true if a layout is saved
 */
bool InventoryState::saveAllLayouts() const // private.
{
	bool ret = false;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (saveLayout(*i) == true)
			ret = true;
	}

	return ret;
}

/**
 * Shows more ground items / rearranges them.
 * @param action - pointer to an Action
 */
void InventoryState::btnGroundClick(Action*)
{
	_inv->arrangeGround();
}

/**
 * Clears the current unit's inventory and moves all items to the ground.
 * @param action - pointer to an Action
 */
void InventoryState::btnUnloadUnitClick(Action*)
{
	if (_tuMode == false					// don't accept clicks in battlescape because this doesn't cost TU.
		&& _inv->getSelectedItem() == NULL) // or when mouse is holding an item
	{
		BattleUnit* const unit = _battleSave->getSelectedUnit();
		std::vector<BattleItem*>* const unitInvent = unit->getInventory();
		Tile* const unitTile = unit->getTile();
		RuleInventory* const grdRule = _game->getRuleset()->getInventory("STR_GROUND");

		for (std::vector<BattleItem*>::const_iterator
				i = unitInvent->begin();
				i != unitInvent->end();
				)
		{
			(*i)->setOwner(NULL);
			unitTile->addItem(
							*i,
							grdRule);

			i = unitInvent->erase(i);
		}

		_inv->arrangeGround(false); // refresh ui
		updateStats();
		refreshMouse();

//		_game->getResourcePack()->getSoundByDepth(
//											_battleSave->getDepth(),
		_game->getResourcePack()->getSound(
										"BATTLE.CAT",
										ResourcePack::ITEM_DROP)
									->play();
	}
}
/* void InventoryState::btnUnloadUnitClick(Action*)
{
	if (_tuMode == true						// don't accept clicks in battlescape
		&& _inv->getSelectedItem() == NULL) // or when moving items
	{
		BattleUnit* const unit = _battleSave->getSelectedUnit();
		std::vector<BattleItem*>* const unitInv = unit->getInventory();
		Tile* const groundTile = unit->getTile();

		clearInventory(
					_game,
					unitInv,
					groundTile);

		_inv->arrangeGround(false); // refresh ui
		updateStats();
		refreshMouse();

//		_game->getResourcePack()->getSoundByDepth(
//											_battleSave->getDepth(),
		_game->getResourcePack()->getSound(
										"BATTLE.CAT",
										ResourcePack::ITEM_DROP)
									->play();
	}
} */
/**
 * Clears unit's inventory - move everything to the ground.
 * @note Helper for btnUnloadUnitClick().
 * @param game			- pointer to the core Game to get the Ruleset
 * @param unitInv		- pointer to a vector of pointers to BattleItems
 * @param groundTile	- pointer to the ground Tile
 */
/* void InventoryState::clearInventory( // private.
		Game* game,
		std::vector<BattleItem*>* unitInv,
		Tile* groundTile)
{
	RuleInventory* const groundRule = game->getRuleset()->getInventory("STR_GROUND");

	for (std::vector<BattleItem*>::const_iterator
			i = unitInv->begin();
			i != unitInv->end();
			)
	{
		(*i)->setOwner(NULL);
		groundTile->addItem(
						*i,
						groundRule);

		i = unitInv->erase(i);
	}
} */

/**
 * Shows the unit info screen.
 * @param action - pointer to an Action
 */
void InventoryState::btnRankClick(Action*)
{
	_game->pushState(new UnitInfoState(
									_battleSave->getSelectedUnit(),
									_parent,
									true,
									false,
									_tuMode == false));
}

/**
 * Updates item info.
 * @param action - pointer to an Action
 */
void InventoryState::invClick(Action*)
{
	_tuCost->setVisible(false);

	// kL_note: This function has only updateStats() in the stock code,
	// since induction of Copy/Paste Inventory Layouts ... that is, the
	// vast majority of this function has been subsumed into invMouseOver().
	// But i'm leaving it in anyway ...

	const BattleItem* const item = _inv->getSelectedItem();
	if (item != NULL)
	{
		const RuleItem* const itRule = item->getRules();
		const BattleItem* const ammo = item->getAmmoItem();

		setExtraInfo(
					item,
					itRule,
					ammo);

		std::wstring wst;

		if (ammo != NULL
			&& item->usesAmmo() == true)
		{
			wst = tr("STR_AMMO_ROUNDS_LEFT").arg(ammo->getAmmoQuantity());

			SDL_Rect rect;
			rect.x =
			rect.y = 0;
			rect.w = static_cast<Sint16>(RuleInventory::HAND_W * RuleInventory::SLOT_W);
			rect.h = static_cast<Sint16>(RuleInventory::HAND_H * RuleInventory::SLOT_H);
			_selAmmo->drawRect(
							&rect,
							static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")->getElement("grid")->color));

			++rect.x;
			++rect.y;
			rect.w -= 2;
			rect.h -= 2;
			_selAmmo->drawRect(
							&rect,
							15);

			ammo->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_selAmmo);
		}
		else if (item->getAmmoQuantity() != 0
			&& item->usesAmmo() == true)
		{
			wst = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoQuantity());
		}
		else if (itRule->getBattleType() == BT_MEDIKIT)
			wst = tr("STR_MEDI_KIT_QUANTITIES_LEFT")
						.arg(item->getPainKillerQuantity())
						.arg(item->getStimulantQuantity())
						.arg(item->getHealQuantity());

		_txtAmmo->setText(wst);
	}
/*	else
	{
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");

		_selAmmo->clear();
	} */

	updateStats();
}

/**
 * Shows item info.
 * @param action - pointer to an Action
 */
void InventoryState::invMouseOver(Action*)
{
	if (_inv->getSelectedItem() != NULL)
	{
		_tuCost->setValue(static_cast<unsigned>(_inv->getTUCostInventory()));
		_tuCost->setVisible(_tuMode
						 && _inv->getTUCostInventory() > 0);

//		_updateTemplateButtons(false);
		return;
	}

	const BattleItem* const item = _inv->getMouseOverItem();
	if (item != NULL)
	{
//		_updateTemplateButtons(false);
//		_tuCost->setVisible(false);

		const RuleItem* const itRule = item->getRules();
		const BattleItem* const ammo = item->getAmmoItem();

		setExtraInfo(
				item,
				itRule,
				ammo);

		std::wstring wst;

		if (ammo != NULL
			&& item->usesAmmo() == true)
		{
			wst = tr("STR_AMMO_ROUNDS_LEFT").arg(ammo->getAmmoQuantity());

			SDL_Rect rect;
			rect.x =
			rect.y = 0;
			rect.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			rect.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
			_selAmmo->drawRect(&rect, static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")->getElement("grid")->color));

			++rect.x;
			++rect.y;
			rect.w -= 2;
			rect.h -= 2;
			_selAmmo->drawRect(
							&rect,
							15);

			ammo->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_selAmmo);
//			_updateTemplateButtons(false);
		}
		else
		{
			_selAmmo->clear();
//			_updateTemplateButtons(!_tuMode);
		}

		if (item->getAmmoQuantity() != 0
			&& item->usesAmmo() == true)
		{
			wst = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoQuantity());
		}
		else if (itRule->getBattleType() == BT_MEDIKIT)
		{
			wst = tr("STR_MEDI_KIT_QUANTITIES_LEFT")
					.arg(item->getPainKillerQuantity())
					.arg(item->getStimulantQuantity())
					.arg(item->getHealQuantity());
		}

		_txtAmmo->setText(wst);
	}
	else
	{
//		if (_currentTooltip.empty())
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");
		_txtUseTU->setText(L"");

		_selAmmo->clear();
//		_updateTemplateButtons(!_tuMode);
	}
}

/**
 * Hides item info.
 * @param action - pointer to an Action
 */
void InventoryState::invMouseOut(Action*)
{
	_txtItem->setText(L"");
	_txtAmmo->setText(L"");
	_txtUseTU->setText(L"");

	_selAmmo->clear();

//	_updateTemplateButtons(!_tuMode);
	_tuCost->setVisible(false);
}

/**
 * Takes care of any events from the core game engine.
 * @param action - pointer to an Action
 */
void InventoryState::handle(Action* action)
{
	State::handle(action);

#ifndef __MORPHOS__
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_X1)
			btnNextClick(action);
		else if (action->getDetails()->button.button == SDL_BUTTON_X2)
			btnPrevClick(action);
	}
#endif
}

/**
 * Sets the extra-info fields on mouseover and mouseclicks.
 * @param item		- pointer to an item
 * @param itRule	- pointer to item's RuleItem
 * @param ammo		- pointer to item's ammo
 */
void InventoryState::setExtraInfo( // private.
		const BattleItem* const item,
		const RuleItem* const itRule,
		const BattleItem* const ammo)
{
		std::wostringstream label;
		bool isArt = false;

		if (item->getUnit() != NULL)
		{
			if (item->getUnit()->getType().compare(0,11, "STR_FLOATER") == 0) // TODO: require Floater autopsy research; also, BattlescapeState::mapOver()
			{
				label << tr("STR_FLOATER") // STR_FLOATER_CORPSE
					  << L" (status doubtful)";
			}
			if (item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				label << item->getUnit()->getName(_game->getLanguage());
			else
			{
				label << tr(itRule->getType());

				if (item->getUnit()->getOriginalFaction() == FACTION_PLAYER)
					label << L" (" + item->getUnit()->getName(_game->getLanguage()) + L")";
			}
		}
		else if (_game->getSavedGame()->isResearched(itRule->getRequirements()) == true)
			label << tr(itRule->getType());
		else
		{
			label << tr("STR_ALIEN_ARTIFACT");
			isArt = true;
		}

		int weight = itRule->getWeight();
		if (ammo != NULL
			&& ammo != item)
		{
			weight += ammo->getRules()->getWeight();
		}

		label << L" (" << weight << L")";
		_txtItem->setText(label.str());


		const BattleUnit* const unit = _battleSave->getSelectedUnit();
		const BattleActionType bat = itRule->getDefaultAction(item->getFuseTimer() > -1);

		if (unit != NULL
			&& isArt == false
			&& (bat != BA_NONE
				|| itRule->getBattleType() == BT_AMMO))
		{
			std::string actionType;
			int tuUse;

			if (itRule->getBattleType() == BT_AMMO)
			{
				actionType = "STR_RELOAD_";
				tuUse = 15;
			}
			else
			{
				switch (bat)
				{
					case BA_LAUNCH:		actionType = "STR_LAUNCH_";	break;
					case BA_SNAPSHOT:	actionType = "STR_SNAP_";	break;
					case BA_AUTOSHOT:	actionType = "STR_BURST_";	break;
					case BA_AIMEDSHOT:	actionType = "STR_SCOPE_";	break;
					case BA_PRIME:		actionType = "STR_PRIME_";	break;
					case BA_DEFUSE:		actionType = "STR_DEFUSE_";	break;
					case BA_USE:		actionType = "STR_USE_";	break;
					case BA_PANIC:		actionType = "STR_PSI_";	break;
					case BA_HIT:		actionType = "STR_ATTACK_";
				}

				tuUse = unit->getActionTUs(
										bat,
										item);
			}

			_txtUseTU->setText(tr(actionType).arg(tuUse));
		}
		else
			_txtUseTU->setText(L"");
}

}

/**
* Shows a tooltip for the appropriate button.
* @param action - pointer to an Action
*/
/* void InventoryState::txtTooltipIn(Action* action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtItem->setText(tr(_currentTooltip));
	}
} */

/**
* Clears the tooltip text.
* @param action - pointer to an Action
*/
/* void InventoryState::txtTooltipOut(Action* action)
{
	if (_inv->getSelectedItem() == 0 && Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_currentTooltip = "";
			_txtItem->setText(L"");
		}
	}
} */

/**
 *
 */
/* void InventoryState::_updateTemplateButtons(bool isVisible)
{
	if (isVisible)
	{
		_game->getResourcePack()->getSurface("InvClear")->blit(_btnClearInventory);

		if (_curInventoryTemplate.empty()) // use "empty template" icons
		{
			_game->getResourcePack()->getSurface("InvCopy")->blit(_btnCreateTemplate);
			_game->getResourcePack()->getSurface("InvPasteEmpty")->blit(_btnApplyTemplate);
		}
		else // use "active template" icons
		{
			_game->getResourcePack()->getSurface("InvCopyActive")->blit(_btnCreateTemplate);
			_game->getResourcePack()->getSurface("InvPaste")->blit(_btnApplyTemplate);
		}

		_btnCreateTemplate->initSurfaces();
		_btnApplyTemplate->initSurfaces();
		_btnClearInventory->initSurfaces();
	}
	else
	{
		_btnCreateTemplate->clear();
		_btnApplyTemplate->clear();
		_btnClearInventory->clear();
	}
} */

/**
 *
 */
/* void InventoryState::btnCreateTemplateClick(Action* action)
{
	if (_inv->getSelectedItem() != NULL) // don't accept clicks when moving items
		return;

	_clearInventoryTemplate(_curInventoryTemplate); // clear current template

	// Copy inventory instead of just keeping a pointer to it. This way
	// create/apply can be used as an undo button for a single unit and will
	// also work as expected if inventory is modified after 'create' is clicked.
	std::vector<BattleItem*>* unitInv = _battleSave->getSelectedUnit()->getInventory();
	for (std::vector<BattleItem*>::iterator
			j = unitInv->begin();
			j != unitInv->end();
			++j)
	{
		std::string ammo;
		if ((*j)->usesAmmo()
			&& (*j)->getAmmoItem())
		{
			ammo = (*j)->getAmmoItem()->getRules()->getType();
		}
		else
			ammo = "NONE";

		_curInventoryTemplate.push_back(new EquipmentLayoutItem(
															(*j)->getRules()->getType(),
															(*j)->getSlot()->getId(),
															(*j)->getSlotX(),
															(*j)->getSlotY(),
															ammo,
															(*j)->getFuseTimer()));
	}

	_game->getResourcePack()->getSoundByDepth(
											_battleSave->getDepth(),
											ResourcePack::ITEM_DROP)->play();
	refreshMouse();
} */

/**
 *
 */
/* void InventoryState::btnApplyTemplateClick(Action* action)
{
	if (_inv->getSelectedItem() != NULL)	// don't accept clicks when moving items
//		|| _curInventoryTemplate.empty())	// or when the template is empty ->
											// if the template is empty -- it will just result in clearing the unit's inventory
	{
		return;
	}

	BattleUnit* unit = _battleSave->getSelectedUnit();
	std::vector<BattleItem*>* unitInv = unit->getInventory();

	Tile* groundTile = unit->getTile();
	std::vector<BattleItem*>* groundInv = groundTile->getInventory();

	RuleInventory* groundRuleInv = _game->getRuleset()->getInventory("STR_GROUND");

	_clearInventory(
				_game,
				unitInv,
				groundTile);

	// attempt to replicate inventory template by grabbing corresponding items
	// from the ground. if any item is not found on the ground, display warning
	// message, but continue attempting to fulfill the template as best we can
	bool itemMissing = false;

	std::vector<EquipmentLayoutItem*>::iterator templateIt;
	for (
			templateIt = _curInventoryTemplate.begin();
			templateIt != _curInventoryTemplate.end();
			++templateIt)
	{
		// search for template item in ground inventory
		std::vector<BattleItem*>::iterator groundItem;
		const bool usesAmmo = !_game->getRuleset()->getItem((*templateIt)->getItemType())->getCompatibleAmmo()->empty();
		bool
			found = false,
			rescan = true;

		while (rescan)
		{
			rescan = false;

			const std::string targetAmmo = (*templateIt)->getAmmoItem();
			BattleItem* matchedWeapon = NULL;
			BattleItem* matchedAmmo = NULL;

			for (
					groundItem = groundInv->begin();
					groundItem != groundInv->end();
					++groundItem)
			{
				// if we find the appropriate ammo, remember it for later for if we find
				// the right weapon but with the wrong ammo
				const std::string groundItemName = (*groundItem)->getRules()->getType();
				if (usesAmmo
					&& targetAmmo == groundItemName)
				{
					matchedAmmo = *groundItem;
				}

				if ((*templateIt)->getItemType() == groundItemName)
				{
					// if the loaded ammo doesn't match the template item's,
					// remember the weapon for later and continue scanning
					BattleItem* loadedAmmo = (*groundItem)->getAmmoItem();
					if ((usesAmmo
							&& loadedAmmo
							&& targetAmmo != loadedAmmo->getRules()->getType())
						|| (usesAmmo
							&& !loadedAmmo))
					{
						// remember the last matched weapon for simplicity (but prefer empty weapons if any are found)
						if (!matchedWeapon
							|| matchedWeapon->getAmmoItem())
						{
							matchedWeapon = *groundItem;
						}

						continue;
					}

					// move matched item from ground to the appropriate inv slot
					(*groundItem)->setOwner(unit);
					(*groundItem)->setSlot(_game->getRuleset()->getInventory((*templateIt)->getSlot()));
					(*groundItem)->setSlotX((*templateIt)->getSlotX());
					(*groundItem)->setSlotY((*templateIt)->getSlotY());
					(*groundItem)->setFuseTimer((*templateIt)->getFuseTimer());

					unitInv->push_back(*groundItem);
					groundInv->erase(groundItem);

					found = true;
					break;
				}
			}

			// if we failed to find an exact match, but found unloaded ammo and
			// the right weapon, unload the target weapon, load the right ammo, and use it
			if (!found
				&& matchedWeapon
				&& (!usesAmmo
					|| matchedAmmo))
			{
				// unload the existing ammo (if any) from the weapon
				BattleItem* loadedAmmo = matchedWeapon->getAmmoItem();
				if (loadedAmmo)
				{
					groundTile->addItem(loadedAmmo, groundRuleInv);
					matchedWeapon->setAmmoItem(NULL);
				}

				// load the correct ammo into the weapon
				if (matchedAmmo)
				{
					matchedWeapon->setAmmoItem(matchedAmmo);
					groundTile->removeItem(matchedAmmo);
				}

				// rescan and pick up the newly-loaded/unloaded weapon
				rescan = true;
			}
		}

		if (!found)
			itemMissing = true;
	}

	if (itemMissing)
		_inv->showWarning(tr("STR_NOT_ENOUGH_ITEMS_FOR_TEMPLATE"));


	_inv->arrangeGround(false); // refresh ui
	updateStats();
	refreshMouse();

	_game->getResourcePack()->getSoundByDepth(
											_battleSave->getDepth(),
											ResourcePack::ITEM_DROP)->play();
} */
