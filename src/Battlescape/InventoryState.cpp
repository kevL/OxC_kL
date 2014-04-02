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

#include "InventoryState.h"

#include <sstream>

#include "BattlescapeState.h"
#include "Camera.h"
#include "Inventory.h"
#include "Map.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "UnitInfoState.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Inventory screen.
 * @param game Pointer to the core game.
 * @param tu Does Inventory use up Time Units?
 * @param parent Pointer to parent Battlescape.
 */
InventoryState::InventoryState(
		Game* game,
		bool tu,
		BattlescapeState* parent)
	:
		State(game),
		_tu(tu),
		_parent(parent)
{
	//Log(LOG_INFO) << "\nCreate InventoryState";

	_battleGame = _game->getSavedGame()->getSavedBattle();
	//Log(LOG_INFO) << ". _battleGame = " << _battleGame;


	_bg			= new Surface(320, 200, 0, 0);
	_soldier	= new Surface(320, 200, 0, 0);

	_txtName	= new Text(200, 17, 36, 6);

	_txtWeight	= new Text(70, 9, 245, 24);
	_txtTus		= new Text(40, 9, 245, Options::showMoreStatsInInventoryView? 32: 24);
	_txtFAcc	= new Text(40, 9, 245, 32);
	_txtReact	= new Text(40, 9, 245, 40);
	_txtThrow	= new Text(40, 9, 245, 48); // kL
	_txtPStr	= new Text(40, 9, 245, 56);
	_txtPSkill	= new Text(40, 9, 245, 64);

	_txtItem	= new Text(160, 9, 128, 140);
	_txtAmmo	= new Text(66, 24, 254, 64);

	_btnOk		= new InteractiveSurface(35, 22, 237, 0);
	_btnPrev	= new InteractiveSurface(23, 22, 273, 0);
	_btnNext	= new InteractiveSurface(23, 22, 297, 0);

	_btnUnload	= new InteractiveSurface(32, 25, 288, 32);

	_btnGround	= new InteractiveSurface(32, 15, 289, 137);

	_btnRank	= new InteractiveSurface(26, 23, 0, 0);

	_selAmmo	= new Surface(
						RuleInventory::HAND_W * RuleInventory::SLOT_W,
						RuleInventory::HAND_H * RuleInventory::SLOT_H,
						272,
						88);

	_inv		= new Inventory(
							_game,
							320,
							200,
							0,
							0,
							_parent == 0);

	add(_bg);
	add(_soldier);
	add(_txtName);

	add(_txtWeight);
	add(_txtTus);
	add(_txtFAcc);
	add(_txtReact);
	add(_txtThrow); // kL
	add(_txtPStr);
	add(_txtPSkill);

	add(_txtItem);
	add(_txtAmmo);
	add(_btnOk);
	add(_btnPrev);
	add(_btnNext);
	add(_btnUnload);
	add(_btnGround);
	add(_btnRank);
	add(_selAmmo);
	add(_inv);

	centerAllSurfaces();


	_game->getResourcePack()->getSurface("TAC01.SCR")->blit(_bg);

	_txtName->setColor(Palette::blockOffset(4));
	_txtName->setBig();
	_txtName->setHighContrast(true);

	_txtWeight->setColor(Palette::blockOffset(4));
	_txtWeight->setSecondaryColor(Palette::blockOffset(1));
	_txtWeight->setHighContrast(true);

	_txtTus->setColor(Palette::blockOffset(4));
	_txtTus->setSecondaryColor(Palette::blockOffset(1));
	_txtTus->setHighContrast(true);

	_txtFAcc->setColor(Palette::blockOffset(4));
	_txtFAcc->setSecondaryColor(Palette::blockOffset(1));
	_txtFAcc->setHighContrast(true);

	_txtReact->setColor(Palette::blockOffset(4));
	_txtReact->setSecondaryColor(Palette::blockOffset(1));
	_txtReact->setHighContrast(true);

	_txtThrow->setColor(Palette::blockOffset(4));			// kL
	_txtThrow->setSecondaryColor(Palette::blockOffset(1));	// kL
	_txtThrow->setHighContrast(true);						// kL

	_txtPStr->setColor(Palette::blockOffset(4));
	_txtPStr->setSecondaryColor(Palette::blockOffset(1));
	_txtPStr->setHighContrast(true);

	_txtPSkill->setColor(Palette::blockOffset(4));
	_txtPSkill->setSecondaryColor(Palette::blockOffset(1));
	_txtPSkill->setHighContrast(true);

	_txtItem->setColor(Palette::blockOffset(3));
	_txtItem->setHighContrast(true);

	_txtAmmo->setColor(Palette::blockOffset(4));
	_txtAmmo->setSecondaryColor(Palette::blockOffset(1));
	_txtAmmo->setAlign(ALIGN_CENTER);
	_txtAmmo->setHighContrast(true);

	_btnOk->onMouseClick((ActionHandler)& InventoryState::btnOkClick);
	_btnOk->onKeyboardPress(
						(ActionHandler)& InventoryState::btnOkClick,
						Options::keyCancel);
	_btnOk->onKeyboardPress(
						(ActionHandler)& InventoryState::btnOkClick,
						Options::keyBattleInventory);

	_btnPrev->onMouseClick((ActionHandler)& InventoryState::btnPrevClick);
	_btnPrev->onKeyboardPress(
						(ActionHandler)& InventoryState::btnPrevClick,
						Options::keyBattlePrevUnit);
	_btnNext->onMouseClick((ActionHandler)&InventoryState::btnNextClick);
	_btnNext->onKeyboardPress(
						(ActionHandler)& InventoryState::btnNextClick,
						Options::keyBattleNextUnit);

	_btnUnload->onMouseClick((ActionHandler)& InventoryState::btnUnloadClick);
	_btnGround->onMouseClick((ActionHandler)& InventoryState::btnGroundClick);
	_btnRank->onMouseClick((ActionHandler)& InventoryState::btnRankClick);

	_inv->draw();
	_inv->setTuMode(_tu);
	_inv->setSelectedUnit(_game->getSavedGame()->getSavedBattle()->getSelectedUnit());
	_inv->onMouseClick((ActionHandler)& InventoryState::invClick, 0);

	_txtWeight->setVisible(Options::showMoreStatsInInventoryView);
	_txtTus->setVisible(_tu);
	_txtFAcc->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtReact->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtThrow->setVisible(Options::showMoreStatsInInventoryView && !_tu); // kL
	_txtPStr->setVisible(Options::showMoreStatsInInventoryView && !_tu);
	_txtPSkill->setVisible(Options::showMoreStatsInInventoryView && !_tu);

	//Log(LOG_INFO) << "Create InventoryState EXIT";
}

/**
 *
 */
InventoryState::~InventoryState()
{
}

/**
 * Updates all soldier stats when the soldier changes.
 */
void InventoryState::init()
{
	//Log(LOG_INFO) << "InventoryState::init()";

	BattleUnit* unit = _battleGame->getSelectedUnit();
	if (unit == 0) // no selected unit, close inventory
	{
		btnOkClick(0);

		//Log(LOG_INFO) << ". early out <- no selectedUnit";
		return;
	}
	else if (!unit->hasInventory()) // skip to the first unit with inventory
	{
		//Log(LOG_INFO) << ". . unit doesn't have Inventory";
		if (_parent)
			//Log(LOG_INFO) << ". . . _parent: select next unit";
			_parent->selectNextFactionUnit(
										false,
										false,
										true);
		else
			//Log(LOG_INFO) << ". . . NO _parent: select next unit";
			_battleGame->selectNextFactionUnit(
											false,
											false,
											true);

		if (//kL _battleGame->getSelectedUnit() == 0 ||			// no available unit, Checked above.
			 !_battleGame->getSelectedUnit()->hasInventory())	// so close inventory
		{
			btnOkClick(0);

			//Log(LOG_INFO) << ". . . early out <- no selectedUnit w/ Inventory";
			return; // starting a mission with just vehicles
		}
		else
			//Log(LOG_INFO) << ". . . unit = selectedUnit";
			unit = _battleGame->getSelectedUnit();
	}

//kL	if (_parent)
//kL		_parent->getMap()->getCamera()->centerOnPosition(unit->getPosition(), false);

	unit->setCache(0);
	_soldier->clear();
	_btnRank->clear();


	_txtName->setBig();
	_txtName->setText(unit->getName(_game->getLanguage()));

	_inv->setSelectedUnit(unit);


	Soldier* s = _game->getSavedGame()->getSoldier(unit->getId());
	if (s)
	{
		SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
		texture->getFrame(s->getRankSprite())->setX(0);
		texture->getFrame(s->getRankSprite())->setY(0);
		texture->getFrame(s->getRankSprite())->blit(_btnRank);

		std::string look = s->getArmor()->getSpriteInventory();
		if (s->getGender() == GENDER_MALE)
			look += "M";
		else
			look += "F";

		if (s->getLook() == LOOK_BLONDE)
			look += "0";
		else if (s->getLook() == LOOK_BROWNHAIR)
			look += "1";
		else if (s->getLook() == LOOK_ORIENTAL)
			look += "2";
		else if (s->getLook() == LOOK_AFRICAN)
			look += "3";

		look += ".SPK";

		if (!CrossPlatform::fileExists(CrossPlatform::getDataFile("UFOGRAPH/" + look))
			&& !_game->getResourcePack()->getSurface(look))
		{
			look = s->getArmor()->getSpriteInventory() + ".SPK";
		}

		_game->getResourcePack()->getSurface(look)->blit(_soldier);
	}
	else
	{
		Surface* armorSurface = _game->getResourcePack()->getSurface(unit->getArmor()->getSpriteInventory());
		if (armorSurface)
			armorSurface->blit(_soldier);
	}

	updateStats();

	//Log(LOG_INFO) << "InventoryState::init() EXIT";
}

/**
 * Updates the soldier stats (Weight, TU).
 */
void InventoryState::updateStats()
{
	//Log(LOG_INFO) << "InventoryState::updateStats()";

	BattleUnit* unit = _battleGame->getSelectedUnit();

	if (_tu) // kL
		_txtTus->setText(tr("STR_TIME_UNITS_SHORT").arg(unit->getTimeUnits()));

	if (Options::showMoreStatsInInventoryView) // kL
	{
		int str = unit->getStats()->strength;
		int weight = unit->getCarriedWeight(_inv->getSelectedItem());
		_txtWeight->setText(tr("STR_WEIGHT").arg(weight).arg(str));
		if (weight > str)
			_txtWeight->setSecondaryColor(Palette::blockOffset(2)+5);
		else
			_txtWeight->setSecondaryColor(Palette::blockOffset(3));

		if (!_tu) // kL
		{
			_txtFAcc->setText(tr("STR_ACCURACY_SHORT").arg(unit->getStats()->firing));
			_txtReact->setText(tr("STR_REACTIONS_SHORT").arg(unit->getStats()->reactions));
			_txtThrow->setText(tr("STR_THROWACC_SHORT").arg(unit->getStats()->throwing)); // kL


			int minPsi = 0;
			if (unit->getType() == "SOLDIER")
				minPsi = _game->getSavedGame()->getSoldier(unit->getId())->getRules()->getMinStats().psiSkill - 1;

			int psiSkill = unit->getStats()->psiSkill;
			if (psiSkill > minPsi)
				_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(psiSkill));
			else
				_txtPSkill->setText(L"");

			if (psiSkill > minPsi
				|| (Options::psiStrengthEval
					&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements())))
			{
				_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(unit->getStats()->psiStrength));
			}
			else
				_txtPStr->setText(L"");
		}
	}
	//Log(LOG_INFO) << "InventoryState::updateStats() EXIT";
}

/**
 * Saves the soldiers' equipment-layouts.
 */
void InventoryState::saveEquipmentLayout()
{
	//Log(LOG_INFO) << "InventoryState::saveEquipmentLayout()";

	for (std::vector<BattleUnit*>::iterator
			i = _battleGame->getUnits()->begin();
			i != _battleGame->getUnits()->end();
			++i)
	{
		if ((*i)->getGeoscapeSoldier() == 0) // x-Com soldiers only
			continue;


		std::vector<EquipmentLayoutItem*>* layoutItems = (*i)->getGeoscapeSoldier()->getEquipmentLayout();
		if (!layoutItems->empty()) // clear the previous save
		{
			for (std::vector<EquipmentLayoutItem*>::iterator
					j = layoutItems->begin();
					j != layoutItems->end();
					++j)
			{
				delete *j;
			}

			layoutItems->clear();
		}

		// save the soldier's items
		// note: with using getInventory() we are skipping the ammos loaded,
		// (they're not owned) because we handle the loaded-ammos separately (inside)
		for (std::vector<BattleItem*>::iterator
				j = (*i)->getInventory()->begin();
				j != (*i)->getInventory()->end();
				++j)
		{
			std::string ammo;
			if ((*j)->needsAmmo()
				&& (*j)->getAmmoItem() != 0)
			{
				ammo = (*j)->getAmmoItem()->getRules()->getType();
			}
			else
				ammo = "NONE";

			layoutItems->push_back(new EquipmentLayoutItem(
														(*j)->getRules()->getType(),
														(*j)->getSlot()->getId(),
														(*j)->getSlotX(),
														(*j)->getSlotY(),
														ammo));
//														(*j)->getExplodeTurn()));
		}
	}

	//Log(LOG_INFO) << "InventoryState::saveEquipmentLayout() EXIT";
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnOkClick(Action*)
{
	//Log(LOG_INFO) << "InventoryState::btnOkClick()";

	if (_inv->getSelectedItem() != 0)
		return;

	_game->popState();

	if (!_tu) // pre-Battle inventory equip.
	{
		saveEquipmentLayout();

		// kL_begin: This for early exit because access is via CraftEquip screen.
		if (_parent == 0)
		{
			//Log(LOG_INFO) << ". early out <- CraftEquip ( no BattlescapeState )";
			return;
		} // kL_end.

		_battleGame->randomizeItemLocations(_battleGame->getSelectedUnit()->getTile());
		_battleGame->resetUnitTiles();

/*		for (std::vector<BattleUnit*>::iterator
				i = _battleGame->getUnits()->begin();
				i != _battleGame->getUnits()->end();
				++i)
		{
			if ((*i)->getFaction() == _battleGame->getSide())
				(*i)->prepareNewTurn();
		} */
	}


	TileEngine* tileEngine = _battleGame->getTileEngine();
//	if (tileEngine) // kL_note: not needed, CraftEquip is handled above w/ early return.
//	{
	tileEngine->applyGravity(_battleGame->getSelectedUnit()->getTile());
	tileEngine->calculateTerrainLighting(); // dropping / picking up flares
	tileEngine->recalculateFOV();

		// from BattlescapeGame::dropItem() but can't really use this because I don't know exactly what dropped...
		// could figure it out via what's on Ground but meh.
/*		if (item->getRules()->getBattleType() == BT_FLARE)
		{
			getTileEngine()->calculateTerrainLighting();
			getTileEngine()->calculateFOV(position);
		} */
//	}

	//Log(LOG_INFO) << "InventoryState::btnOkClick() EXIT";
}

/**
 * Selects the previous soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnPrevClick(Action*)
{
	if (_inv->getSelectedItem() != 0)
		return;

	if (_parent)
		_parent->selectPreviousFactionUnit(
										false,
										false,
										true);
	else
		_battleGame->selectPreviousFactionUnit(
											false,
											false,
											true);

	init();
}

/**
 * Selects the next soldier.
 * @param action Pointer to an action.
 */
void InventoryState::btnNextClick(Action*)
{
	if (_inv->getSelectedItem() != 0)
		return;

	if (_parent)
		_parent->selectNextFactionUnit(
									false,
									false,
									true);
	else
		_battleGame->selectNextFactionUnit(
										false,
										false,
										true);

	init();
}

/**
 * Unloads the selected weapon.
 * @param action Pointer to an action.
 */
void InventoryState::btnUnloadClick(Action*)
{
	if (_inv->getSelectedItem() != 0
		&& _inv->getSelectedItem()->getAmmoItem() != 0
		&& _inv->getSelectedItem()->needsAmmo())
	{
		_inv->unload();
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");
		_selAmmo->clear();

		updateStats();
	}
}

/**
 * Shows more ground items / rearranges them.
 * @param action Pointer to an action.
 */
void InventoryState::btnGroundClick(Action*)
{
	_inv->arrangeGround();
}

/**
 * Shows the unit info screen.
 * @param action Pointer to an action.
 */
void InventoryState::btnRankClick(Action*)
{
	_game->pushState(new UnitInfoState(
									_game,
									_battleGame->getSelectedUnit(),
									_parent,
									true,
									false));
}

/**
 * Updates item info.
 * @param action, Pointer to an action.
 */
void InventoryState::invClick(Action*)
{
	BattleItem* item = _inv->getSelectedItem();

	_txtItem->setText(L"");
	_txtAmmo->setText(L"");

	_selAmmo->clear();

	if (item != 0)
	{
		if (item->getUnit()
			&& item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
		{
			_txtItem->setText(item->getUnit()->getName(_game->getLanguage()));
		}
		else
		{
			if (_game->getSavedGame()->isResearched(item->getRules()->getRequirements()))
				_txtItem->setText(tr(item->getRules()->getName()));
			else
				_txtItem->setText(tr("STR_ALIEN_ARTIFACT"));
		}


		std::wstring sAmmo;

		if (item->getAmmoItem() != 0
			&& item->needsAmmo())
		{
			sAmmo = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoItem()->getAmmoQuantity());

			SDL_Rect r;
			r.x = 0;
			r.y = 0;
			r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;

			_selAmmo->drawRect(&r, Palette::blockOffset(0)+8);

			r.x++;
			r.y++;
			r.w -= 2;
			r.h -= 2;

			_selAmmo->drawRect(&r, 0);

			item->getAmmoItem()->getRules()->drawHandSprite(
														_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
														_selAmmo);
		}
		else if (item->getAmmoQuantity() != 0
			&& item->needsAmmo())
		{
			sAmmo = tr("STR_AMMO_ROUNDS_LEFT").arg(item->getAmmoQuantity());
		}
		else if (item->getRules()->getBattleType() == BT_MEDIKIT)
			sAmmo = tr("STR_MEDI_KIT_QUANTITIES_LEFT")
						.arg(item->getPainKillerQuantity())
						.arg(item->getStimulantQuantity())
						.arg(item->getHealQuantity());

		_txtAmmo->setText(sAmmo);
	}

	updateStats();
}

/**
 * Takes care of any events from the core game engine.
 * @param action, Pointer to an action.
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

}
