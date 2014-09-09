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

#include "DebriefingState.h"

#include <sstream>

#include "CannotReequipState.h"
#include "CommendationState.h"
#include "NoContainmentState.h"
#include "PromotionsState.h"

#include "../Basescape/ManageAlienContainmentState.h"
#include "../Basescape/SellState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Music.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/RNG.h"
#include "../Engine/Screen.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"
#include "../Menu/MainMenuState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/ResourcePack.h"
#include "../Resource/XcomResourcePack.h" // sza_MusicRules

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h" //

#include "../Savegame/AlienBase.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h" // kL
#include "../Savegame/SoldierDeath.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Debriefing screen.
 * @param game, Pointer to the core game.
 */
DebriefingState::DebriefingState()
	:
		_region(NULL),
		_country(NULL),
		_noContainment(false),
		_manageContainment(false),
		_destroyBase(false),
		_limitsEnforced(0)
{
	//Log(LOG_INFO) << "Create DebriefingState";
	Options::baseXResolution = Options::baseXGeoscape;
	Options::baseYResolution = Options::baseYGeoscape;
	_game->getScreen()->resetDisplay(false);

	// Restore the cursor in case something weird happened
	_game->getCursor()->setVisible();

	// kL_begin: Clean up the leftover states from BattlescapeGame
	// ( was done in ~BattlescapeGame, but that causes CTD under reLoad situation )
	// Now done here and in NextTurnState. (not ideal: should find a safe place
	// when BattlescapeGame is really dTor'd, and not reLoaded ...... ) uh, i guess.
//	if (_game->getSavedGame()->getSavedBattle()->getBattleGame())
//	{
		//Log(LOG_INFO) << "DebriefingState : Saved Battle Game EXISTS";
	_game->getSavedGame()->getSavedBattle()->getBattleGame()->cleanupDeleted(); // kL, delete CTD
//	}


	_missionStatistics = new MissionStatistics();

	if (Options::storageLimitsEnforced)
		_limitsEnforced = 1;

	_window			= new Window(this, 320, 200, 0, 0);

	_txtTitle		= new Text(280, 17, 16, 8);
	_txtBaseLabel	= new Text(80, 9, 216, 8);

	_txtItem		= new Text(184, 9, 16, 24);
	_txtQuantity	= new Text(60, 9, 200, 24);
	_txtScore		= new Text(36, 9, 260, 24);

	_lstStats		= new TextList(288, 80, 16, 32);

	_lstRecovery	= new TextList(288, 80, 16, 32);
	_txtRecovery	= new Text(180, 9, 16, 60);

	_lstTotal		= new TextList(288, 9, 16, 12);

	_btnOk			= new TextButton(176, 16, 16, 177);
	_txtRating		= new Text(100, 9, 212, 180);

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtItem);
	add(_txtQuantity);
	add(_txtScore);
	add(_txtRecovery);
	add(_lstStats);
	add(_lstRecovery);
	add(_lstTotal);
	add(_btnOk);
	add(_txtRating);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& DebriefingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefingState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefingState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setBig();

	_txtItem->setColor(Palette::blockOffset(8)+5);
	_txtItem->setText(tr("STR_LIST_ITEM"));

	_txtQuantity->setColor(Palette::blockOffset(8)+5);
	_txtQuantity->setText(tr("STR_QUANTITY_UC"));
//kL	_txtQuantity->setAlign(ALIGN_RIGHT);

	_txtScore->setColor(Palette::blockOffset(8)+5);
	_txtScore->setText(tr("STR_SCORE"));

	_txtRecovery->setColor(Palette::blockOffset(8)+5);
	_txtRecovery->setText(tr("STR_UFO_RECOVERY"));

	_txtRating->setColor(Palette::blockOffset(8)+5);

	_lstStats->setColor(Palette::blockOffset(15)-1);
	_lstStats->setSecondaryColor(Palette::blockOffset(8)+10);
	_lstStats->setColumns(3, 176, 60, 36);
	_lstStats->setDot();
	_lstStats->setMargin();

	_lstRecovery->setColor(Palette::blockOffset(15)-1);
	_lstRecovery->setSecondaryColor(Palette::blockOffset(8)+10);
	_lstRecovery->setColumns(3, 176, 60, 36);
	_lstRecovery->setDot();
	_lstRecovery->setMargin();

	_lstTotal->setColor(Palette::blockOffset(8)+5);
	_lstTotal->setColumns(2, 244, 36);
	_lstTotal->setDot();


	prepareDebriefing();

	_txtBaseLabel->setColor(Palette::blockOffset(8)+5);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_baseLabel);


	int
		total = 0,
		stats_dy = 0,
		recovery_dy = 0,
		civiliansSaved = 0,
		civiliansDead = 0;

	for (std::vector<DebriefingStat*>::iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->qty == 0)
			continue;


		std::wostringstream
			ss,
			ss2;

		ss << L'\x01' << (*i)->qty << L'\x01';	// quantity of recovered item-type
		ss2 << L'\x01' << (*i)->score;			// score
		total += (*i)->score;					// total score

		if ((*i)->recovery)
		{
			_lstRecovery->addRow(
								3,
								tr((*i)->item).c_str(),
								ss.str().c_str(),
								ss2.str().c_str());
			recovery_dy += 8;
		}
		else
		{
			_lstStats->addRow(
							3,
							tr((*i)->item).c_str(),
							ss.str().c_str(),
							ss2.str().c_str());
			stats_dy += 8;
		}

		if ((*i)->item == "STR_CIVILIANS_SAVED")
			civiliansSaved = (*i)->qty;

		if ((*i)->item == "STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES"
			|| (*i)->item == "STR_CIVILIANS_KILLED_BY_ALIENS")
		{
			civiliansDead += (*i)->qty;
		}
	}

	if (civiliansSaved
		&& !civiliansDead)
	{
		_missionStatistics->valiantCrux = true;
	}

	std::wostringstream ss3;
	ss3 << total;
	_lstTotal->addRow(
					2,
					tr("STR_TOTAL_UC").c_str(),
					ss3.str().c_str());

	if (recovery_dy > 0)
	{
		_txtRecovery->setY(_lstStats->getY() + stats_dy + 5);
		_lstRecovery->setY(_txtRecovery->getY() + 8);
		_lstTotal->setY(_lstRecovery->getY() + recovery_dy + 5);
	}
	else
	{
		_txtRecovery->setText(L"");
		_lstTotal->setY(_lstStats->getY() + stats_dy + 5);
	}

	// add the points to our activity score
	if (_region)
	{
		//Log(LOG_INFO) << ". aLien act.ScoreREGION = " << _region->getActivityAlien().back();
		_region->addActivityXcom(total);

		if (_destroyBase) // kL ->
		{
			int diff = static_cast<int>(_game->getSavedGame()->getDifficulty()) + 1;
			_region->addActivityAlien(diff * 200);
		}
	}

	if (_country)
	{
		//Log(LOG_INFO) << ". aLien act.ScoreCOUNTRY = " << _country->getActivityAlien().back();
		_country->addActivityXcom(total);

		if (_destroyBase) // kL ->
		{
			int diff = static_cast<int>(_game->getSavedGame()->getDifficulty()) + 1;
			_country->addActivityAlien(diff * 200);
		}
	}

	std::wstring rating; // Calculate rating
	if (total < -99)
	{
		rating = tr("STR_RATING_TERRIBLE");
		_missionStatistics->rating = "STR_RATING_TERRIBLE";
	}
	else if (total < 51)
	{
		rating = tr("STR_RATING_POOR");
		_missionStatistics->rating = "STR_RATING_POOR";
	}
	else if (total < 276)
	{
		rating = tr("STR_RATING_OK");
		_missionStatistics->rating = "STR_RATING_OK";
	}
	else if (total < 651)
	{
		rating = tr("STR_RATING_GOOD");
		_missionStatistics->rating = "STR_RATING_GOOD";
	}
	else if (total < 1201)
	{
		rating = tr("STR_RATING_EXCELLENT");
		_missionStatistics->rating = "STR_RATING_EXCELLENT";
	}
	else
	{
		rating = tr("STR_RATING_STUPENDOUS");
		_missionStatistics->rating = "STR_RATING_EXCELLENT";
	}

	_txtRating->setText(tr("STR_RATING").arg(rating));
	_missionStatistics->score = total;

	SavedGame* save = _game->getSavedGame();
	SavedBattleGame* battle = save->getSavedBattle();

	_missionStatistics->daylight = save->getSavedBattle()->getGlobalShade();
	_missionStatistics->id = _game->getSavedGame()->getMissionStatistics()->size();

	for (std::vector<BattleUnit*>::iterator
			j = battle->getUnits()->begin();
			j != battle->getUnits()->end();
			++j)
	{
		if ((*j)->getGeoscapeSoldier())
		{
			(*j)->getStatistics()->daysWounded = (*j)->getGeoscapeSoldier()->getWoundRecovery();
			_missionStatistics->injuryList[(*j)->getGeoscapeSoldier()->getId()] = (*j)->getGeoscapeSoldier()->getWoundRecovery();

			if ((*j)->getStatus() == STATUS_DEAD)
				(*j)->getStatistics()->KIA = true;

			(*j)->getGeoscapeSoldier()->getDiary()->updateDiary(
															(*j)->getStatistics(),
															_missionStatistics);

			if ((*j)->getStatus() != STATUS_DEAD
				&& (*j)->getGeoscapeSoldier()->getDiary()->manageCommendations(_game->getRuleset()))
			{
				_soldiersCommended.push_back((*j)->getGeoscapeSoldier());
			}
			else if ((*j)->getStatus() == STATUS_DEAD
				&& (*j)->getGeoscapeSoldier()->getDiary()->manageCommendations(_game->getRuleset()))
			{
				// Quietly award dead soldiers their commendations as well
			}
		}
	}

	_game->getSavedGame()->getMissionStatistics()->push_back(_missionStatistics);

//	_game->getResourcePack()->playMusic("GMMARS");
//	_game->getResourcePack()->getMusic(OpenXcom::XCOM_RESOURCE_MUSIC_GMMARS)->play(); // sza_MusicRules
	_game->getResourcePack()->playMusic(OpenXcom::XCOM_RESOURCE_MUSIC_GMMARS); // kL, sza_MusicRules

	_game->getCursor()->setColor(Palette::blockOffset(15)+12);
	_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);

	//Log(LOG_INFO) << "Create DebriefingState DONE";
}

/**
 * dTor.
 */
DebriefingState::~DebriefingState()
{
	//Log(LOG_INFO) << "Delete DebriefingState";
	if (_game->isQuitting())
		_game->getSavedGame()->setBattleGame(NULL);

	for (std::vector<DebriefingStat*>::iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		delete *i;
	}

	_rounds.clear();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void DebriefingState::btnOkClick(Action*)
{
	//Log(LOG_INFO) << "DebriefingState::btnOkClick()";
	std::vector<Soldier*> participants;
	for (std::vector<BattleUnit*>::const_iterator
			i = _game->getSavedGame()->getSavedBattle()->getUnits()->begin();
			i != _game->getSavedGame()->getSavedBattle()->getUnits()->end();
			++i)
	{
		if ((*i)->getGeoscapeSoldier())
			participants.push_back((*i)->getGeoscapeSoldier());
	}

	_game->getSavedGame()->setBattleGame(NULL);	// kL_note: Would it be better to **delete** that ........
												// It is deleted when SavedGame::setBattleGame() runs the *next Battle*.
	_game->popState();

	if (_game->getSavedGame()->getMonthsPassed() == -1)
		_game->setState(new MainMenuState());
	else
	{
		if (!_destroyBase)
		{
			if (!_soldiersCommended.empty())
				_game->pushState(new CommendationState(_soldiersCommended));

			if (_game->getSavedGame()->handlePromotions(participants))
				_game->pushState(new PromotionsState());

			if (!_missingItems.empty())
				_game->pushState(new CannotReequipState(_missingItems));

			if (_noContainment)
				_game->pushState(new NoContainmentState());
			else if (_manageContainment)
			{
				_game->pushState(new ManageAlienContainmentState(
																_base,
																OPT_BATTLESCAPE,
																false)); // kL_add. Do not allow researchHelp!
				_game->pushState(new ErrorMessageState(
													tr("STR_CONTAINMENT_EXCEEDED")
														.arg(_base->getName()).c_str(),
													_palette,
													Palette::blockOffset(8)+5,
													"BACK01.SCR",
													0));
			}

			if (!_manageContainment
				&& Options::storageLimitsEnforced
				&& _base->storesOverfull())
			{
				_game->pushState(new SellState(
											_base,
											OPT_BATTLESCAPE));
				_game->pushState(new ErrorMessageState(
													tr("STR_STORAGE_EXCEEDED")
														.arg(_base->getName()).c_str(),
													_palette,
													Palette::blockOffset(8)+5,
													"BACK01.SCR",
													0));
			}
		}

		if (_game->getSavedGame()->isIronman()) // Autosave after mission
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
		else if (Options::autosave)
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_AUTO_GEOSCAPE,
											_palette));
	}
	//Log(LOG_INFO) << "DebriefingState::btnOkClick() EXIT";
}

/**
 * Adds to the debriefing stats.
 * @param name		- the untranslated name of the stat
 * @param score		- the score to add
 * @param quantity	- the quantity to add
 */
void DebriefingState::addStat(
		const std::string& name,
		int score,
		int quantity)
{
	//Log(LOG_INFO) << "DebriefingState::addStat()";
	for (std::vector<DebriefingStat*>::iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->item == name)
		{
			(*i)->score = (*i)->score + score;
			(*i)->qty = (*i)->qty + quantity;

			break;
		}
	}
	//Log(LOG_INFO) << "DebriefingState::addStat() EXIT";
}


/**
 * Clears the alien base from supply missions that use it.
 */
class ClearAlienBase
	:
		public std::unary_function<AlienMission*, void>
{

private:
	const AlienBase* _base;


	public:
		/// Remembers the base.
		ClearAlienBase(const AlienBase* base)
			:
				_base(base)
		{
			//Log(LOG_INFO) << "DebriefingState, ClearAlienBase() cTor";
			/* Empty by design. */
		}

		/// Clears the base if required.
		void operator()(AlienMission* am) const;
};

/**
 * Removes the association between the alien mission and the alien base,
 * if one existed.
 * @param am Pointer to the alien mission.
 */
void ClearAlienBase::operator()(AlienMission* am) const
{
	//Log(LOG_INFO) << "DebriefingState, ClearAlienBase()";
	if (am->getAlienBase() == _base)
		am->setAlienBase(NULL);
}


/**
 * Prepares debriefing: gathers Aliens, Corpses, Artefacts, UFO Components.
 * Adds the items to the craft.
 * Also calculates the soldiers' experience, and possible promotions.
 * If aborted, only the things on the exit area are recovered.
 */
void DebriefingState::prepareDebriefing()
{
	//Log(LOG_INFO) << "DebriefingState::prepareDebriefing()";
	_stats.push_back(new DebriefingStat("STR_ALIENS_KILLED"));
	_stats.push_back(new DebriefingStat("STR_ALIEN_CORPSES_RECOVERED"));
	_stats.push_back(new DebriefingStat("STR_LIVE_ALIENS_RECOVERED"));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ARTIFACTS_RECOVERED"));
	_stats.push_back(new DebriefingStat("STR_ALIEN_BASE_CONTROL_DESTROYED"));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_ALIENS"));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES"));
	_stats.push_back(new DebriefingStat("STR_CIVILIANS_SAVED"));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_KILLED"));
//	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_RETIRED_THROUGH_INJURY"));
	_stats.push_back(new DebriefingStat("STR_XCOM_OPERATIVES_MISSING_IN_ACTION"));
	_stats.push_back(new DebriefingStat("STR_TANKS_DESTROYED"));
	_stats.push_back(new DebriefingStat("STR_XCOM_CRAFT_LOST"));

	_stats.push_back(new DebriefingStat("STR_UFO_POWER_SOURCE", true));
	_stats.push_back(new DebriefingStat("STR_UFO_NAVIGATION", true));
	_stats.push_back(new DebriefingStat("STR_UFO_CONSTRUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_FOOD", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_REPRODUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ENTERTAINMENT", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_SURGERY", true));
	_stats.push_back(new DebriefingStat("STR_EXAMINATION_ROOM", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ALLOYS", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_HABITAT", true));
	_stats.push_back(new DebriefingStat(_game->getRuleset()->getAlienFuel(), true));

	SavedGame* save = _game->getSavedGame();
	SavedBattleGame* battle = save->getSavedBattle();

	bool
		aborted = battle->isAborted(),
		success = !aborted;

	Base* base = NULL;
	Craft* craft = NULL;
	std::vector<Craft*>::iterator craftIterator;

	_missionStatistics->time = *save->getTime();
	_missionStatistics->type = battle->getMissionType();

	int soldierExit = 0; // if this stays 0 the craft is lost...
	int soldierLive = 0; // if this stays 0 the craft is lost...
	int soldierOut = 0;

	for (std::vector<Base*>::iterator
			i = save->getBases()->begin();
			i != save->getBases()->end();
			++i)
	{
		// in case we have a craft - check which craft it is about
		for (std::vector<Craft*>::iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->isInBattlescape())
			{
				double
					craftLon = (*j)->getLongitude(),
					craftLat = (*j)->getLatitude();

				for (std::vector<Region*>::iterator
						k = save->getRegions()->begin();
						k != save->getRegions()->end();
						++k)
				{
					if ((*k)->getRules()->insideRegion(
													craftLon,
													craftLat))
					{
						_region = *k;
						_missionStatistics->region = _region->getRules()->getType();

						break;
					}
				}

				for (std::vector<Country*>::iterator
						k = save->getCountries()->begin();
						k != save->getCountries()->end();
						++k)
				{
					if ((*k)->getRules()->insideCountry(
													craftLon,
													craftLat))
					{
						_country = *k;
						_missionStatistics->country = _country->getRules()->getType();

						break;
					}
				}

				base = *i;
				craft = *j;
				craftIterator = j;
				craft->returnToBase();
				craft->setMissionComplete(true);
				craft->setInBattlescape(false);
			}
			else if ((*j)->getDestination() != NULL)
			{
				Ufo* ufo = dynamic_cast<Ufo*>((*j)->getDestination());
				if (ufo != NULL
					&& ufo->isInBattlescape())
				{
					(*j)->returnToBase();
				}
			}
		}

		if ((*i)->isInBattlescape()) // in case we DON'T have a craft (base defense)
		{
			base = *i;
			base->setInBattlescape(false);

			double
				baseLon = base->getLongitude(),
				baseLat = base->getLatitude();

			for (std::vector<Region*>::iterator
					k = save->getRegions()->begin();
					k != save->getRegions()->end();
					++k)
			{
				if ((*k)->getRules()->insideRegion(
												baseLon,
												baseLat))
				{
					_region = *k;
					_missionStatistics->region = _region->getRules()->getType();

					break;
				}
			}

			for (std::vector<Country*>::iterator
					k = save->getCountries()->begin();
					k != save->getCountries()->end();
					++k)
			{
				if ((*k)->getRules()->insideCountry(
												baseLon,
												baseLat))
				{
					_country = *k;
					_missionStatistics->country = _country->getRules()->getType();

					break;
				}
			}

			if (aborted)
				_destroyBase = true;

			bool facilDestroyed = false;
			for (std::vector<BaseFacility*>::iterator
					k = base->getFacilities()->begin();
					k != base->getFacilities()->end();
					)
			{
				if (battle->getModuleMap()[(*k)->getX()][(*k)->getY()].second == 0) // this facility was demolished
				{
					facilDestroyed = true;
					base->destroyFacility(k);
				}
				else
					++k;
			}

			base->destroyDisconnectedFacilities(); // this may cause the base to become disjointed, destroy the disconnected parts.
		}
	}

	_base = base;
	_baseLabel = _base->getName(_game->getLanguage()); // kL

	// kL_begin: Do all missionTypes here, for SoldierDiary race stat.
	if (save->getMonthsPassed() != -1)
	{
		if (battle->getAlienRace() != "") // safety.
			_missionStatistics->alienRace = battle->getAlienRace();
		else
			_missionStatistics->alienRace = "STR_UNKNOWN";
	} // kL_end.

	// UFO crash/landing site disappears
	for (std::vector<Ufo*>::iterator
			i = save->getUfos()->begin();
			i != save->getUfos()->end();
			++i)
	{
		if ((*i)->isInBattlescape())
		{
			_missionStatistics->ufo = (*i)->getRules()->getType();
//kL		if (save->getMonthsPassed() != -1)
//kL			_missionStatistics->alienRace = (*i)->getAlienRace();

			(*i)->setInBattlescape(false);

			if ((*i)->getStatus() == Ufo::LANDED
				&& aborted)
			{
				(*i)->setSecondsRemaining(15); // UFO lifts off ...
			}
			else if (!aborted) // successful mission ( kL: failed mission leaves UFO still crashed! )
//kL			|| (*i)->getStatus() == Ufo::CRASHED) // UFO can't fly
			{
				delete *i;
				save->getUfos()->erase(i); // UFO disappears.

				break;
			}
		}
	}

	// terror site disappears (even when you abort)
	for (std::vector<TerrorSite*>::iterator
			i = save->getTerrorSites()->begin();
			i != save->getTerrorSites()->end();
			++i)
	{
		if ((*i)->isInBattlescape())
		{
//kL		_missionStatistics->alienRace = (*i)->getAlienRace();

			delete *i;
			save->getTerrorSites()->erase(i);

			break;
		}
	}

	// lets see what happens with units; first,
	// we evaluate how many surviving XCom units there are,
	// and if they are unconscious
	// and how many have died (to use for commendations)
	int deadSoldiers = 0;

	for (std::vector<BattleUnit*>::iterator
			j = battle->getUnits()->begin();
			j != battle->getUnits()->end();
			++j)
	{
		if ((*j)->getOriginalFaction() == FACTION_PLAYER
			&& (*j)->getStatus() != STATUS_DEAD)
		{
			if ((*j)->getStatus() == STATUS_UNCONSCIOUS
				|| (*j)->getFaction() == FACTION_HOSTILE)
			{
				soldierOut++;
			}

			soldierLive++;
		}
		else if ((*j)->getOriginalFaction() == FACTION_PLAYER
			&& (*j)->getStatus() == STATUS_DEAD)
		{
			deadSoldiers++;
		}
	}

	// if all our men are unconscious, the aliens get to have their way with them.
	if (soldierOut == soldierLive)
	{
		soldierLive = 0;

		for (std::vector<BattleUnit*>::iterator
				j = battle->getUnits()->begin();
				j != battle->getUnits()->end();
				++j)
		{
			if ((*j)->getOriginalFaction() == FACTION_PLAYER
				&& (*j)->getStatus() != STATUS_DEAD)
			{
				(*j)->instaKill();
			}
		}
	}

	if (soldierLive == 1)
	{
		for (std::vector<BattleUnit*>::iterator
				j = battle->getUnits()->begin();
				j != battle->getUnits()->end();
				++j)
		{
			// if only one soldier survived, give him a medal! (unless he killed all the others...)
			if ((*j)->getStatus() != STATUS_DEAD
				&& (*j)->getOriginalFaction() == FACTION_PLAYER
				&& !(*j)->getStatistics()->hasFriendlyFired()
				&& deadSoldiers != 0)
			{
				(*j)->getStatistics()->loneSurvivor = true;

				break;
			}
			// if only one soldier survived AND none have died, means only one soldier went on the mission...
			else if ((*j)->getStatus() != STATUS_DEAD
				&& (*j)->getOriginalFaction() == FACTION_PLAYER
				&& deadSoldiers != 0)
			{
				(*j)->getStatistics()->ironMan = true;
			}
		}
	}

	std::string mission = battle->getMissionType();

	// kL_begin: Do all missionTypes here, for SoldierDiary race stat.
//	if (mission == "STR_BASE_DEFENSE")
/*	if (battle->getAlienRace() != "") // safety.
		_missionStatistics->alienRace = battle->getAlienRace();
	else
		_missionStatistics->alienRace = "STR_UNKNOWN";
	// */ // kL_end.

	// alien base disappears (if you didn't abort)
	if (mission == "STR_ALIEN_BASE_ASSAULT")
	{
		_txtRecovery->setText(tr("STR_ALIEN_BASE_RECOVERY"));
		bool destroyAlienBase = true;

		if (aborted
			|| soldierLive == 0)
		{
			for (int
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				// get recoverable map data objects from the battlescape map
				if (battle->getTiles()[i]->getMapData(3)
					&& battle->getTiles()[i]->getMapData(3)->getSpecialType() == UFO_NAVIGATION)
				{
					destroyAlienBase = false;

					break;
				}
			}
		}

		success = destroyAlienBase;

		for (std::vector<AlienBase*>::iterator
				i = save->getAlienBases()->begin();
				i != save->getAlienBases()->end();
				++i)
		{
			if ((*i)->isInBattlescape())
			{
//kL			_missionStatistics->alienRace = (*i)->getAlienRace();

				if (destroyAlienBase)
				{
					addStat(
							"STR_ALIEN_BASE_CONTROL_DESTROYED",
							300);

					// Take care to remove supply missions for this base.
					std::for_each(
							save->getAlienMissions().begin(),
							save->getAlienMissions().end(),
							ClearAlienBase(*i));

					delete *i;
					save->getAlienBases()->erase(i);

					break;
				}
				else
				{
					(*i)->setInBattlescape(false);

					break;
				}
			}
		}
	}

	// time to care about units.
	for (std::vector<BattleUnit*>::iterator
			j = battle->getUnits()->begin();
			j != battle->getUnits()->end();
			++j)
	{
		if ((*j)->getTile() == NULL) // handle unconscious units; This unit is not on a tile...
		{
			Position pos = (*j)->getPosition();
			if (pos == Position(-1,-1,-1)) // in fact, this Unit is in limbo...
			{
				for (std::vector<BattleItem*>::iterator // so look for its corpse...
						k = battle->getItems()->begin();
						k != battle->getItems()->end();
						++k)
				{
					if ((*k)->getUnit()
						&& (*k)->getUnit() == *j) // found it: corpse is a dead BattleUnit!!
					{
						if ((*k)->getOwner()) // corpse of BattleUnit has an Owner (ie. is being carried by another BattleUnit)
							pos = (*k)->getOwner()->getPosition(); // Put the corpse down.. slowly.
						else if ((*k)->getTile()) // corpse of BattleUnit is laying around somewhere
							pos = (*k)->getTile()->getPosition(); // you're not dead yet, Get up.
					}
				}
			}

			(*j)->setTile(battle->getTile(pos));
		}


		int value = (*j)->getValue();

		UnitFaction origFact = (*j)->getOriginalFaction();

		UnitStatus status = (*j)->getStatus();
		if (status == STATUS_DEAD) // so this is a dead unit
		{
			if (origFact == FACTION_HOSTILE
				&& (*j)->killedBy() == FACTION_PLAYER)
			{
				addStat(
						"STR_ALIENS_KILLED",
						value);
			}
			else if (origFact == FACTION_PLAYER)
			{
				Soldier* soldier = save->getSoldier((*j)->getId());
				if (soldier != NULL) // xCom soldier.
				{
					addStat(
							"STR_XCOM_OPERATIVES_KILLED",
							-value);

					for (std::vector<Soldier*>::iterator
							i = base->getSoldiers()->begin();
							i != base->getSoldiers()->end();
							++i)
					{
						if (*i == soldier) // the specific soldier at Base..
						{
							(*j)->updateGeoscapeStats(*i);

/*							SoldierDeath* death = new SoldierDeath();
							death->setTime(*save->getTime());

//kL							(*i)->die(death);
//kL							save->getDeadSoldiers()->push_back(*i);
							SoldierDead* dead = (*i)->die(death);		// kL, converts Soldier to SoldierDead class instance.
							save->getDeadSoldiers()->push_back(dead);	// kL

							base->getSoldiers()->erase(i);			// erase Base soldier
							delete save->getSoldier((*j)->getId()); // kL, delete SavedGame soldier
																	// uh, what 'bout err GeoscapeSoldier etc.
*/
							// kL_begin: sorta mirror GeoscapeState::time1Day()
							SoldierDeath* death = new SoldierDeath();
							death->setTime(*save->getTime());

							SoldierDead* dead = (*i)->die(death); // converts Soldier to SoldierDead class instance.
							save->getDeadSoldiers()->push_back(dead);

							int iD = (*j)->getId(); // get ID from battleunit; could also use (*i) instead.

							base->getSoldiers()->erase(i); // erase Soldier from Base_soldiers vector.

							delete save->getSoldier(iD); // delete Soldier instance.

							// note: Could return any armor the soldier was wearing to Stores. CHEATER!!!!!
							break;
						}
					}
				}
				else // not soldier -> tank
					addStat(
							"STR_TANKS_DESTROYED",
							-value);
			}
			else if (origFact == FACTION_NEUTRAL)
			{
				if ((*j)->killedBy() == FACTION_PLAYER)
					addStat(
							"STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES",
							-(value * 2)); // kL
				else
					// if civilians happen to kill themselves XCOM shouldn't get penalty for it
					addStat(
							"STR_CIVILIANS_KILLED_BY_ALIENS",
							-value); // kL
			}
		}
		else // so this unit is NOT dead...
		{
			UnitFaction faction = (*j)->getFaction();
			std::string type = (*j)->getType();

			if ((*j)->getSpawnUnit() != "")
				type = (*j)->getSpawnUnit();

			if (origFact == FACTION_PLAYER)
			{
				Soldier* soldier = save->getSoldier((*j)->getId());

				if (((*j)->isInExitArea()
						&& (mission != "STR_BASE_DEFENSE"
							|| success))
					|| !aborted)
				{
					// so game is not aborted, or aborted and unit is on exit area
					(*j)->postMissionProcedures(save);

					soldierExit++;

					if (soldier != NULL)
					{
						recoverItems(
								(*j)->getInventory(),
								base);

						soldier->calcStatString( // calculate new statString
											_game->getRuleset()->getStatStrings(),
											Options::psiStrengthEval
												&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements()));
					}
					else // not soldier -> tank
					{
						base->getItems()->addItem(type);

						RuleItem* tankRule = _game->getRuleset()->getItem(type);
						BattleItem* ammoItem = (*j)->getItem("STR_RIGHT_HAND")->getAmmoItem();

						if (tankRule->getCompatibleAmmo()->empty() == false
							&& ammoItem != NULL
							&& ammoItem->getAmmoQuantity() > 0)
						{
							base->getItems()->addItem(
													tankRule->getCompatibleAmmo()->front(),
													ammoItem->getAmmoQuantity());
						}
					}
				}
				else // so game is aborted and unit is not on exit area
				{
					addStat(
							"STR_XCOM_OPERATIVES_MISSING_IN_ACTION",
							-value);

					if (soldier != NULL)
					{
						for (std::vector<Soldier*>::iterator
								i = base->getSoldiers()->begin();
								i != base->getSoldiers()->end();
								++i)
						{
							if (*i == soldier)
							{
								(*j)->updateGeoscapeStats(*i);

/*								SoldierDeath* death = new SoldierDeath();
								death->setTime(*save->getTime());

//kL								(*i)->die(death);
//kL								save->getDeadSoldiers()->push_back(*i);
								SoldierDead* dead = (*i)->die(death);		// kL, converts Soldier to SoldierDead class instance.
								save->getDeadSoldiers()->push_back(dead);	// kL

								base->getSoldiers()->erase(i);			// erase Base soldier
								delete save->getSoldier((*j)->getId()); // kL, delete SavedGame soldier
																		// uh, what 'bout err GeoscapeSoldier etc.

								break; */
								// kL_begin: sorta mirror GeoscapeState::time1Day()
								SoldierDeath* death = new SoldierDeath();
								death->setTime(*save->getTime());

								SoldierDead* dead = (*i)->die(death); // converts Soldier to SoldierDead class instance.
								save->getDeadSoldiers()->push_back(dead);

								int iD = (*j)->getId(); // get ID from battleunit; could also use (*i) instead.

								base->getSoldiers()->erase(i); // erase Soldier from Base_soldiers vector.

								delete save->getSoldier(iD); // delete Soldier instance.

								// note: Could return any armor the soldier was wearing to Stores. CHEATER!!!!!
								break;
							}
						}
					}
				}
			}
			else if (origFact == FACTION_HOSTILE	// Mc'd unit,
				&& faction == FACTION_PLAYER		// counts as unconscious
				&& (!aborted
					|| (*j)->isInExitArea())
				&& !(*j)->isOut()) // not Unconscious ... dead handled above.
				// kL_note: so, this never actually runs unless early psi-exit
				// when all aliens are dead or mind-controlled is turned on ....
				// Except the two-stage Cydonia mission, in which case all this
				// does not matter.
			{
				for (std::vector<BattleItem*>::iterator
						k = (*j)->getInventory()->begin();
						k != (*j)->getInventory()->end();
						++k)
				{
					if ((*k)->getRules()->isFixed() == false)
						(*j)->getTile()->addItem(
												*k,
												_game->getRuleset()->getInventory("STR_GROUND"));
				}

				std::string corpseItem = (*j)->getArmor()->getCorpseGeoscape();
				if ((*j)->getSpawnUnit() != "")
				{
					Ruleset* rule = _game->getRuleset();
					corpseItem = rule
									->getArmor(rule->getUnit((*j)->getSpawnUnit())->getArmor())
									->getCorpseGeoscape();
				}

				if (base->getAvailableContainment() > 0)
					addStat(
							"STR_LIVE_ALIENS_RECOVERED",
							value); // kL, duplicated in function below.
				else // kL->
					addStat(
							"STR_ALIENS_KILLED",
							value); // kL, duplicated in function below.

				RuleResearch* research = _game->getRuleset()->getResearch(type);
				if (research != NULL
					&& save->isResearchAvailable(
											research,
											save->getDiscoveredResearch(),
											_game->getRuleset()))
				{
					if (base->getAvailableContainment() == 0)
					{
						_noContainment = true;
						base->getItems()->addItem(corpseItem);
					}
					else
					{
						addStat( // more points if it's not researched
								"STR_LIVE_ALIENS_RECOVERED",
								value, // kL, duplicated in function below.
								0);

						base->getItems()->addItem(type);
						_manageContainment = base->getAvailableContainment()
												- (base->getUsedContainment() * _limitsEnforced) < 0;
					}
				}
				else
				{
					if (Options::canSellLiveAliens)
						_game->getSavedGame()->setFunds(
													_game->getSavedGame()->getFunds()
													+ _game->getRuleset()->getItem(type)->getSellCost());
					else
						base->getItems()->addItem(corpseItem);
				}
			}
			else if (origFact == FACTION_NEUTRAL)
			{
				if (aborted
					|| soldierLive == 0) // if mission fails, all civilians die
				{
					addStat(
							"STR_CIVILIANS_KILLED_BY_ALIENS",
							-value); // kL
				}
				else
					addStat(
							"STR_CIVILIANS_SAVED",
							value); // kL_note: duplicated below.
			}
		}
	}

	if (craft != NULL
		&& ((soldierExit == 0
				&& aborted)
			|| soldierLive == 0))
	{
		addStat(
				"STR_XCOM_CRAFT_LOST",
				-craft->getRules()->getScore());

		for (std::vector<Soldier*>::iterator
				i = base->getSoldiers()->begin();
				i != base->getSoldiers()->end();
				)
		{
			if ((*i)->getCraft() == craft)
			{
				delete *i;
				i = base->getSoldiers()->erase(i);
			}
			else
				 ++i;
		}

		// Since this is not a base defense mission, we can safely erase the craft,
		// without worrying its vehicles' destructor calling double (on base defense missions
		// all vehicle objects in the craft are also referenced by base->getVehicles() !!)
		delete craft;

		craft = NULL; // To avoid a crash down there!!
		base->getCrafts()->erase(craftIterator);
		_txtTitle->setText(tr("STR_CRAFT_IS_LOST"));

		return;
	}

	if (aborted
		&& mission == "STR_BASE_DEFENSE"
		&& !base->getCrafts()->empty())
	{
		//Log(LOG_INFO) << ". aborted BASE_DEFENSE";

		for (std::vector<Craft*>::iterator
				i = base->getCrafts()->begin();
				i != base->getCrafts()->end();
				++i)
		{
			addStat(
					"STR_XCOM_CRAFT_LOST",
					-(*i)->getRules()->getScore());
		}
	}


	if ((!aborted
			|| success)		// RECOVER UFO:
		&& soldierLive > 0)	// Run through all tiles to recover UFO components and items.
	{
		if (mission == "STR_BASE_DEFENSE")
			_txtTitle->setText(tr("STR_BASE_IS_SAVED"));
		else if (mission == "STR_TERROR_MISSION")
			_txtTitle->setText(tr("STR_ALIENS_DEFEATED"));
		else if (mission == "STR_ALIEN_BASE_ASSAULT"
			|| mission == "STR_MARS_CYDONIA_LANDING"
			|| mission == "STR_MARS_THE_FINAL_ASSAULT")
		{
			_txtTitle->setText(tr("STR_ALIEN_BASE_DESTROYED"));
		}
		else
			_txtTitle->setText(tr("STR_UFO_IS_RECOVERED"));


		if (!aborted)
		{
			for (int // get recoverable map data objects from the battlescape map
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				for (int
						part = 0;
						part < 4;
						part++)
				{
					if (battle->getTiles()[i]->getMapData(part))
					{
						switch (battle->getTiles()[i]->getMapData(part)->getSpecialType())
						{
							case UFO_POWER_SOURCE:
								addStat(
										"STR_UFO_POWER_SOURCE",
										20); // kL
							break;
							case UFO_NAVIGATION:
								addStat(
										"STR_UFO_NAVIGATION",
										5);
							break;
							case UFO_CONSTRUCTION:
								addStat(
										"STR_UFO_CONSTRUCTION",
										2);
							break;
							case ALIEN_FOOD:
								addStat(
										"STR_ALIEN_FOOD",
										1); // kL
							break;
							case ALIEN_REPRODUCTION:
								addStat(
										"STR_ALIEN_REPRODUCTION",
										3); // kL
							break;
							case ALIEN_ENTERTAINMENT:
								addStat(
										"STR_ALIEN_ENTERTAINMENT",
										1); // kL
							break;
							case ALIEN_SURGERY:
								addStat(
										"STR_ALIEN_SURGERY",
										2);
							break;
							case EXAM_ROOM:
								addStat(
										"STR_EXAMINATION_ROOM",
										2);
							break;
							case ALIEN_ALLOYS:
								addStat(
										"STR_ALIEN_ALLOYS",
										1);
							break;
							case ALIEN_HABITAT:
								addStat(
										"STR_ALIEN_HABITAT",
										1);
							break;
							case MUST_DESTROY: // this is the brain!
							break;

							default:
							break;
						}
					}
				}

				recoverItems( // recover items from the floor
						battle->getTiles()[i]->getInventory(),
						base);
			}
		}
		else
		{
			for (int
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				if (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)
					&& (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT))
				{
					recoverItems(
							battle->getTiles()[i]->getInventory(),
							base);
				}
			}
		}
	}
	else // mission FAIL.
	{
		if (mission == "STR_BASE_DEFENSE")
		{
			//Log(LOG_INFO) << ". failed BASE_DEFENSE";
			_txtTitle->setText(tr("STR_BASE_IS_LOST"));
			_destroyBase = true;
		}
		else if (mission == "STR_TERROR_MISSION")
			_txtTitle->setText(tr("STR_TERROR_CONTINUES"));
		else if (mission == "STR_ALIEN_BASE_ASSAULT"
			|| mission == "STR_MARS_CYDONIA_LANDING"
			|| mission == "STR_MARS_THE_FINAL_ASSAULT")
		{
			_txtTitle->setText(tr("STR_ALIEN_BASE_STILL_INTACT"));
		}
		else
			_txtTitle->setText(tr("STR_UFO_IS_NOT_RECOVERED"));

		if (soldierLive > 0
			&& !_destroyBase)
		{
			for (int // recover items from the ground
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				if (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)
					&& (battle->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT))
				{
					recoverItems(
							battle->getTiles()[i]->getInventory(),
							base);
				}
			}
		}
	}

	// calculate the clips for each type based on the recovered rounds.
	for (std::map<RuleItem*, int>::const_iterator
			i = _rounds.begin();
			i != _rounds.end();
			++i)
	{
		if (i->first->getClipSize() > 0) // kL
		{
			int total_clips = i->second / i->first->getClipSize();
			if (total_clips > 0)
				base->getItems()->addItem(
										i->first->getType(),
										total_clips);
		}
	}

	if (soldierLive > 0) // recover all our goodies
	{
		for (std::vector<DebriefingStat*>::iterator
				i = _stats.begin();
				i != _stats.end();
				++i)
		{
			// alien alloys recovery values are divided by 10 or divided by 150 in case of an alien base
			// kL_note: change to divided by 6 or divided by 120 in case of an alien base.
			if ((*i)->item == "STR_ALIEN_ALLOYS")
			{
//kL			int alloy = 10;
				int alloy = 6; // kL
				if (mission == "STR_ALIEN_BASE_ASSAULT")
//kL				alloy = 100;
					alloy = 120; // kL

				(*i)->qty = (*i)->qty / alloy;
				(*i)->score = (*i)->score / alloy;
			}

			// recoverable battlescape tiles are now converted to items and put in base inventory
			if ((*i)->recovery
				&& (*i)->qty > 0)
			{
				base->getItems()->addItem(
										(*i)->item,
										(*i)->qty);
			}
		}
	}

	// reequip craft after a non-base-defense mission;
	// of course only if it's not lost already, that case craft=0
	if (craft != NULL)
		reequipCraft(
					base,
					craft,
					true);

//	else // kL, maybe...
	// reequip crafts (only which is on the base) after a base defense mission;
	// we MUST check the missionType here, to avoid non-base-defense missions case
	if (mission == "STR_BASE_DEFENSE"
		&& !_destroyBase)
	{
		for (std::vector<Craft*>::iterator
				c = base->getCrafts()->begin();
				c != base->getCrafts()->end();
				++c)
		{
			if ((*c)->getStatus() != "STR_OUT")
				reequipCraft(
							base,
							*c,
							false);
		}

		// Clearing base->getVehicles() objects, they're not needed anymore.
		for (std::vector<Vehicle*>::iterator
				i = base->getVehicles()->begin();
				i != base->getVehicles()->end();
				++i)
		{
			delete *i;
		}

		base->getVehicles()->clear();
	}

	if (_destroyBase
		&& save->getMonthsPassed() != -1)
	{
		for (std::vector<Base*>::iterator
				i = save->getBases()->begin();
				i != save->getBases()->end();
				++i)
		{
			if (*i == base)
			{
				delete *i;
				save->getBases()->erase(i);

				break;
			}
		}

		AlienMission* am = save->getAlienMission(
											_region->getRules()->getType(),
											"STR_ALIEN_RETALIATION");
		for (std::vector<Ufo*>::iterator
				i = save->getUfos()->begin();
				i != save->getUfos()->end();
				)
		{
			if ((*i)->getMission() == am)
			{
				delete *i;
				i = save->getUfos()->erase(i);
			}
			else
				++i;
		}

		for (std::vector<AlienMission*>::iterator
				i = save->getAlienMissions().begin();
				i != save->getAlienMissions().end();
				++i)
		{
//kL		if ((AlienMission*)(*i) == am)
			if (dynamic_cast<AlienMission*>(*i) == am) // kL
			{
				delete *i;
				save->getAlienMissions().erase(i);

				break;
			}
		}
	}

	_missionStatistics->success = success;
	//Log(LOG_INFO) << "DebriefingState::prepareDebriefing() EXIT";
}

/**
 * Reequips a craft after a mission.
 * @param base					- pointer to a base to reequip from
 * @param craft					- pointer to a craft to reequip
 * @param vehicleDestruction	- true if vehicles on the craft can be destroyed
 */
void DebriefingState::reequipCraft(
		Base* base,
		Craft* craft,
		bool vehicleDestruction)
{
	//Log(LOG_INFO) << "DebriefingState::reequipCraft()";
	int
		qty,
		lost;
//	int used; // kL

	std::map<std::string, int> craftItems = *craft->getItems()->getContents();
	for (std::map<std::string, int>::iterator
			i = craftItems.begin();
			i != craftItems.end();
			++i)
	{
		qty = base->getItems()->getItem(i->first);

		if (qty >= i->second)
		{
			base->getItems()->removeItem(
										i->first,
										i->second);

//			used = i->second; // kL
		}
		else
		{
			base->getItems()->removeItem(
										i->first,
										qty);

			lost = i->second - qty;
			craft->getItems()->removeItem(
										i->first,
										lost);
//										i->second - qty); // kL

//			used = lost; // kL

			ReequipStat stat =
			{
				i->first,
				lost,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		}

		// kL_begin:
		// NOTE: quantifying what was Used on the battlefield is difficult,
		// because *all items recovered* on the 'field, including the craft's
		// stores, have already been added to base->Stores before this function
		// runs. An independent vector of either base->Stores or craft->Stores
		// is NOT maintained.

//		used = i->second - qty;
//		used = qty - i->second;
//		if (used > 0)
/*		{
			ReequipStat stat =
			{
				i->first,
//				i->second,
				used,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		} */
		// kL_end.
	}

	// Now let's see the vehicles
	ItemContainer craftVehicles;
	for (std::vector<Vehicle*>::iterator
			i = craft->getVehicles()->begin();
			i != craft->getVehicles()->end();
			++i)
	{
		craftVehicles.addItem((*i)->getRules()->getType());
	}

	// Now we know how many vehicles (separated by types) we have to read.
	// Erase the current vehicles, because we have to
	// reAdd them ('cause we want to redistribute their ammo)
	// kL_note: and generally weave our way through this spaghetti ....
	if (vehicleDestruction)
	{
		for (std::vector<Vehicle*>::iterator
				i = craft->getVehicles()->begin();
				i != craft->getVehicles()->end();
				++i)
		{
			delete *i;
		}
	}

	craft->getVehicles()->clear();

	int canBeAdded;

	for (std::map<std::string, int>::iterator // Ok, now read those vehicles
			i = craftVehicles.getContents()->begin();
			i != craftVehicles.getContents()->end();
			++i)
	{
		qty = base->getItems()->getItem(i->first);
		canBeAdded = std::min(
							qty,
							i->second);

		if (qty < i->second)
		{
			lost = i->second - qty; // missing tanks
			ReequipStat stat =
			{
				i->first,
				lost,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		}


		RuleItem* tankRule = _game->getRuleset()->getItem(i->first);
		Unit* tankUnit = _game->getRuleset()->getUnit(tankRule->getType());

		int size = 4;
		if (tankUnit)
		{
			size = _game->getRuleset()->getArmor(tankUnit->getArmor())->getSize();
			size *= size;
		}

		if (tankRule->getCompatibleAmmo()->empty()) // so this tank does NOT require ammo
		{
			for (int
					j = 0;
					j < canBeAdded;
					++j)
			{
				craft->getVehicles()->push_back(new Vehicle(
														tankRule,
														tankRule->getClipSize(),
														size));
			}

			base->getItems()->removeItem(
										i->first,
										canBeAdded);
		}
		else // so this tank requires ammo
		{
			RuleItem* ammoRule = _game->getRuleset()->getItem(tankRule->getCompatibleAmmo()->front());
			int ammoPerVehicle = ammoRule->getClipSize();

			int baseQty = base->getItems()->getItem(ammoRule->getType()); // Ammo Quantity for this vehicle-type on the base

			if (baseQty < i->second * ammoPerVehicle)
			{
				lost = (i->second * ammoPerVehicle) - baseQty; // missing ammo
				ReequipStat stat =
				{
					ammoRule->getType(),
					lost,
					craft->getName(_game->getLanguage())
				};

				_missingItems.push_back(stat);
			}

			canBeAdded = std::min(
								canBeAdded,
								baseQty / ammoPerVehicle);

			if (canBeAdded > 0)
			{
				for (int
						j = 0;
						j < canBeAdded;
						++j)
				{
					craft->getVehicles()->push_back(new Vehicle(
															tankRule,
															ammoPerVehicle,
															size));

					base->getItems()->removeItem(
												ammoRule->getType(),
												ammoPerVehicle);
				}

				base->getItems()->removeItem(
											i->first,
											canBeAdded);
			}
		}

		// kL_begin:
/*		ReequipStat stat =
		{
			i->first,
			canBeAdded,
			craft->getName(_game->getLanguage())
		};

		_missingItems.push_back(stat); */
		// kL_end.
	}
	//Log(LOG_INFO) << "DebriefingState::reequipCraft() EXIT";
}

/**
 * Recovers items from the battlescape.
 *
 * Converts the battlescape inventory into a geoscape itemcontainer.
 * @param battleItems	- items recovered from the battlescape
 * @param base			- base to add the items to
 */
void DebriefingState::recoverItems(
		std::vector<BattleItem*>* battleItems,
		Base* base)
{
	//Log(LOG_INFO) << "DebriefingState::recoverItems()";
	for (std::vector<BattleItem*>::iterator
			item = battleItems->begin();
			item != battleItems->end();
			++item)
	{
		if ((*item)->getRules()->getName() == _game->getRuleset()->getAlienFuel())
			addStat( // special case of an item counted as a stat
					_game->getRuleset()->getAlienFuel(),
					100,
					50); // kL
		else
		{
			if ((*item)->getRules()->getRecoveryPoints()
				&& !(*item)->getXCOMProperty())
			{
				if ((*item)->getRules()->getBattleType() == BT_CORPSE
					&& (*item)->getUnit()->getStatus() == STATUS_DEAD)
				{
					addStat(
							"STR_ALIEN_CORPSES_RECOVERED",
							(*item)->getUnit()->getValue() / 3); // kL

					base->getItems()->addItem((*item)->getUnit()->getArmor()->getCorpseGeoscape());
				}
				else if ((*item)->getRules()->getBattleType() == BT_CORPSE
					&& (*item)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					if ((*item)->getUnit()->getOriginalFaction() == FACTION_HOSTILE)
					{
						if (base->getAvailableContainment() > 0)
							addStat(
									"STR_LIVE_ALIENS_RECOVERED",
									(*item)->getUnit()->getValue()); // kL, duplicated above.
						else // kL->
							addStat(
									"STR_ALIENS_KILLED",
									(*item)->getUnit()->getValue()); // kL, duplicated above.

						if (_game->getSavedGame()->isResearchAvailable(
								_game->getRuleset()->getResearch((*item)->getUnit()->getType()),
								_game->getSavedGame()->getDiscoveredResearch(),
								_game->getRuleset()))
						{
							if (base->getAvailableContainment() == 0)
							{
								_noContainment = true;
								base->getItems()->addItem((*item)->getUnit()->getArmor()->getCorpseGeoscape());
							}
							else
							{
								addStat( // more points if item's not researched
										"STR_LIVE_ALIENS_RECOVERED",
										(*item)->getUnit()->getValue(), // kL, duplicated above.
										0);

								base->getItems()->addItem((*item)->getUnit()->getType());
								_manageContainment = base->getAvailableContainment()
														- (base->getUsedContainment() * _limitsEnforced) < 0;
							}
						}
						else
							base->getItems()->addItem((*item)->getUnit()->getArmor()->getCorpseGeoscape());
					}
					else if ((*item)->getUnit()->getOriginalFaction() == FACTION_NEUTRAL)
						addStat(
								"STR_CIVILIANS_SAVED",
								(*item)->getUnit()->getValue()); // kL_note: duplicated above.
				}
				else if (!_game->getSavedGame()->isResearched((*item)->getRules()->getType()))
					addStat( // add pts. for unresearched items only
							"STR_ALIEN_ARTIFACTS_RECOVERED",
							(*item)->getRules()->getRecoveryPoints());
			}

			if ((*item)->getRules()->isRecoverable()
				&& (*item)->getRules()->isFixed() == false)
			{
				// put items back in the base
				switch ((*item)->getRules()->getBattleType())
				{
					case BT_CORPSE:
					break;
					case BT_AMMO: // item's a clip, count rounds left
						_rounds[(*item)->getRules()] += (*item)->getAmmoQuantity();
					break;
					case BT_FIREARM:
					case BT_MELEE: // item's a weapon, count rounds in clip
					{
						BattleItem* clip = (*item)->getAmmoItem();
						if (clip
							&& clip->getRules()->getClipSize() > 0
							&& clip != *item)
						{
							_rounds[clip->getRules()] += clip->getAmmoQuantity();
						}
					}

					default: // fall-through, recover the weapon itself
						base->getItems()->addItem((*item)->getRules()->getType());
					break;
				}
			}
		}
	}
	//Log(LOG_INFO) << "DebriefingState::recoverItems() EXIT";
}

}
