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

//#include <sstream>

#include "CannotReequipState.h"
#include "CommendationState.h"
#include "CommendationDeadState.h"
#include "NoContainmentState.h"
#include "PromotionsState.h"

#include "../Basescape/ManageAlienContainmentState.h"
#include "../Basescape/SellState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Music.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/RNG.h"
//#include "../Engine/Screen.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"
#include "../Menu/MainMenuState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"

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
#include "../Savegame/SoldierDead.h"
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
 */
DebriefingState::DebriefingState()
	:
		_rules(_game->getRuleset()),
		_savedGame(_game->getSavedGame()),
		_region(NULL),
		_country(NULL),
		_noContainment(false),
		_manageContainment(false),
		_destroyXCOMBase(false),
		_limitsEnforced(0),
		_aliensControlled(0),
		_aliensKilled(0),
		_aliensStunned(0)
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
//	if (_savedGame->getSavedBattle()->getBattleGame())
//	{
		//Log(LOG_INFO) << "DebriefingState : Saved Battle Game EXISTS";
	_savedGame->getSavedBattle()->getBattleGame()->cleanupDeleted();
//	}


	_missionStatistics = new MissionStatistics();

	if (Options::storageLimitsEnforced == true)
		_limitsEnforced = 1;

	_window			= new Window(this, 320, 200, 0, 0);

	_txtTitle		= new Text(280, 17, 16, 8);
	_txtBaseLabel	= new Text(80, 9, 216, 8);

	_txtItem		= new Text(184, 9, 16, 24);
	_txtQuantity	= new Text(60, 9, 200, 24);
	_txtScore		= new Text(36, 9, 260, 24);

	_lstStats		= new TextList(288, 81, 16, 32);

	_lstRecovery	= new TextList(288, 81, 16, 32);
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
//	_txtQuantity->setAlign(ALIGN_RIGHT);

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


	prepareDebriefing(); /*/ <- -- GATHER ALL DATA HERE <- < */


	_txtBaseLabel->setColor(Palette::blockOffset(8)+5);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_baseLabel);


	int
		total = 0,
		stats_offY = 0,
		recov_offY = 0,
		civiliansSaved = 0,
		civiliansDead = 0;

	//Log(LOG_INFO) << ". iterate _stats";
	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		//Log(LOG_INFO) << ". . item: " << (*i)->item;
		if ((*i)->qty == 0)
			continue;
		//Log(LOG_INFO) << ". . add item";


		std::wostringstream
			ss,
			ss2;

		ss << L'\x01' << (*i)->qty << L'\x01';	// quantity of recovered item-type
		ss2 << L'\x01' << (*i)->score;			// score
		total += (*i)->score;					// total score

		if ((*i)->recover == true)
		{
			_lstRecovery->addRow(
								3,
								tr((*i)->item).c_str(),
								ss.str().c_str(),
								ss2.str().c_str());
			recov_offY += 8;
		}
		else
		{
			_lstStats->addRow(
							3,
							tr((*i)->item).c_str(),
							ss.str().c_str(),
							ss2.str().c_str());
			stats_offY += 8;
		}

		if ((*i)->item == "STR_CIVILIANS_SAVED")
			civiliansSaved = (*i)->qty;

		if ((*i)->item == "STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES"
			|| (*i)->item == "STR_CIVILIANS_KILLED_BY_ALIENS")
		{
			civiliansDead += (*i)->qty;
		}

		if ((*i)->item == "STR_ALIENS_KILLED")
			_aliensKilled += (*i)->qty;
	}
	//Log(LOG_INFO) << ". iterate _stats DONE";

	if (civiliansSaved > 0
		&& civiliansDead == 0
		&& _missionStatistics->success == true)
	{
		_missionStatistics->valiantCrux = true;
	}

	std::wostringstream ss3;
	ss3 << total;
	_lstTotal->addRow(
					2,
					tr("STR_TOTAL_UC").c_str(),
					ss3.str().c_str());

	if (recov_offY > 0)
	{
		_txtRecovery->setY(_lstStats->getY() + stats_offY + 5);
		_lstRecovery->setY(_txtRecovery->getY() + 8);
		_lstTotal->setY(_lstRecovery->getY() + recov_offY + 5);
	}
	else
	{
		_txtRecovery->setText(L"");
		_lstTotal->setY(_lstStats->getY() + stats_offY + 5);
	}

	// add the points to activity scores
	if (_region != NULL)
	{
		//Log(LOG_INFO) << ". aLien act.ScoreREGION = " << _region->getActivityAlien().back();
		if (_destroyXCOMBase == true)
		{
			const int diff = static_cast<int>(_savedGame->getDifficulty()) + 1;
			_region->addActivityAlien(diff * 235);
			_region->recentActivity();
		}
		else
		{
			_region->addActivityXcom(total);
			_region->recentActivityXCOM();
		}
	}

	if (_country != NULL)
	{
		//Log(LOG_INFO) << ". aLien act.ScoreCOUNTRY = " << _country->getActivityAlien().back();
		if (_destroyXCOMBase == true)
		{
			const int diff = static_cast<int>(_savedGame->getDifficulty()) + 1;
			_country->addActivityAlien(diff * 235);
			_country->recentActivity();
		}
		else
		{
			_country->addActivityXcom(total);
			_country->recentActivityXCOM();
		}
	}

	std::wstring rating; // Calculate rating
	if (total < -99)
	{
		rating = tr("STR_RATING_TERRIBLE");
		_missionStatistics->rating = "STR_RATING_TERRIBLE";
	}
	else if (total < 101)
	{
		rating = tr("STR_RATING_POOR");
		_missionStatistics->rating = "STR_RATING_POOR";
	}
	else if (total < 351)
	{
		rating = tr("STR_RATING_OK");
		_missionStatistics->rating = "STR_RATING_OK";
	}
	else if (total < 751)
	{
		rating = tr("STR_RATING_GOOD");
		_missionStatistics->rating = "STR_RATING_GOOD";
	}
	else if (total < 1251)
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


	// Soldier Diary ->
	SavedBattleGame* const battle = _savedGame->getSavedBattle();

	_missionStatistics->id = _savedGame->getMissionStatistics()->size();
	_missionStatistics->shade = battle->getGlobalShade();

//	BattleUnitStatistics* statistics = NULL; // BattleUnit::getStatistics()
//	Soldier* soldier = NULL; // BattleUnit::getGeoscapeSoldier() const

	for (std::vector<BattleUnit*>::const_iterator
			i = battle->getUnits()->begin();
			i != battle->getUnits()->end();
			++i)
	{
		//Log(LOG_INFO) << "UNIT id " << (*i)->getId();
		Soldier* soldier = (*i)->getGeoscapeSoldier();
		// NOTE: In the case of a dead soldier, this pointer is Valid but points to garbage.
		// Use that.
		if (soldier != NULL)
		{
			//Log(LOG_INFO) << ". soldier VALID";
			BattleUnitStatistics* const statistics = (*i)->getStatistics();

			int soldierAlienKills = 0;

			for (std::vector<BattleUnitKills*>::const_iterator
					j = statistics->kills.begin();
					j != statistics->kills.end();
					++j)
			{
				if ((*j)->_faction == FACTION_HOSTILE)
					++soldierAlienKills;
			}

			// NOTE re. Nike Cross:
			// This can be exploited by MC'ing a bunch of aLiens
			// while having Option "psi-control ends battle" TRUE.
			// ... Patched
			// NOTE: This can still be exploited by MC'ing and
			// executing a bunch of aLiens with a single Soldier.
			if (_aliensControlled == 0
				&& _aliensKilled + _aliensStunned > 4 + static_cast<int>(_savedGame->getDifficulty())
				&& _aliensKilled + _aliensStunned == soldierAlienKills
				&& _missionStatistics->success == true)
			{
				statistics->nikeCross = true;
			}


			if ((*i)->getStatus() == STATUS_DEAD)
			{
				soldier = NULL;	// Zero out the BattleUnit from the geoscape Soldiers list
								// in this State; it's already gone from his/her former Base.
								// This makes them ineligible for promotion.
								// PS, there is no 'geoscape Soldiers list' really; it's
								// just a variable stored in each xCom-agent/BattleUnit ....
				SoldierDead* deadSoldier;

				for (std::vector<SoldierDead*>::const_iterator
						j = _savedGame->getDeadSoldiers()->begin();
						j != _savedGame->getDeadSoldiers()->end();
						++j)
				{
					if ((*j)->getId() == (*i)->getId())
					{
						deadSoldier = *j;
						break;
					}
				}

				_missionStatistics->injuryList[deadSoldier->getId()] = -1;

				statistics->daysWounded = 0;
				statistics->KIA = true;

				//Log(LOG_INFO) << ". . DEAD updateDiary()";
				deadSoldier->getDiary()->updateDiary(
												statistics,
												_missionStatistics,
												_rules);
				//Log(LOG_INFO) << ". . DEAD updateDiary() DONE";

				deadSoldier->getDiary()->manageAwards(_rules);

				_soldiersKIA.push_back(deadSoldier);
			}
			else
			{
				_missionStatistics->injuryList[soldier->getId()] = soldier->getWoundRecovery();

				statistics->daysWounded = soldier->getWoundRecovery();

				//Log(LOG_INFO) << ". . updateDiary()";
				soldier->getDiary()->updateDiary(
											statistics,
											_missionStatistics,
											_rules);
				//Log(LOG_INFO) << ". . updateDiary() DONE";

				if (soldier->getDiary()->manageAwards(_rules) == true)
					_soldiersCommended.push_back(soldier);
			}
		}
	}

	_savedGame->getMissionStatistics()->push_back(_missionStatistics); // Soldier Diary_end.


	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_TAC_DEBRIEFING);

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
	if (_game->isQuitting() == true)
		_savedGame->setBattleGame(NULL);

	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		delete *i;
	}

	for (std::map<int, SpecialType*>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		delete i->second;
	}
	_specialTypes.clear();

	_rounds.clear();
	//Log(LOG_INFO) << "Delete DebriefingState EXIT";
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void DebriefingState::btnOkClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 900);

	std::vector<Soldier*> participants;
	for (std::vector<BattleUnit*>::const_iterator
			i = _savedGame->getSavedBattle()->getUnits()->begin();
			i != _savedGame->getSavedBattle()->getUnits()->end();
			++i)
	{
		if ((*i)->getGeoscapeSoldier() != NULL)
			participants.push_back((*i)->getGeoscapeSoldier());
	}

	_savedGame->setBattleGame(NULL);
	_game->popState();

	if (_savedGame->getMonthsPassed() == -1)
		_game->setState(new MainMenuState());
	else
	{
		if (_destroyXCOMBase == false)
		{
			bool playAwardMusic = false;

			if (_soldiersKIA.empty() == false)
			{
				playAwardMusic = true;
				_game->pushState(new CommendationDeadState(_soldiersKIA));
			}

			if (_soldiersCommended.empty() == false)
			{
				playAwardMusic = true;
				_game->pushState(new CommendationState(_soldiersCommended));
			}

			if (_savedGame->handlePromotions(participants))
			{
				playAwardMusic = true;
				_game->pushState(new PromotionsState());
			}

			if (_missingItems.empty() == false)
			{
//				playAwardMusic = true;
				_game->pushState(new CannotReequipState(_missingItems));
			}

			if (_noContainment == true)
			{
//				playAwardMusic = true;
				_game->pushState(new NoContainmentState());
			}
			else if (_manageContainment == true)
			{
//				playAwardMusic = true;
				_game->pushState(new ManageAlienContainmentState(
															_base,
															OPT_BATTLESCAPE,
															false)); // Do not allow researchHelp!
				_game->pushState(new ErrorMessageState(
													tr("STR_CONTAINMENT_EXCEEDED")
														.arg(_base->getName()).c_str(),
													_palette,
													Palette::blockOffset(8)+5,
													"BACK01.SCR",
													0));
			}

			if (_manageContainment == false
				&& Options::storageLimitsEnforced == true
				&& _base->storesOverfull() == true)
			{
//				playAwardMusic = true;
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

			if (playAwardMusic == true)
				_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_TAC_AWARDS);
		}

		if (_savedGame->isIronman() == true) // Autosave after mission
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
		else if (Options::autosave == true)
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_AUTO_GEOSCAPE,
											_palette));
	}
}

/**
 * Adds to the debriefing stats.
 * @param name		- reference the untranslated name of the stat
 * @param score		- the score to add
 * @param quantity	- the quantity to add (default 1)
 */
void DebriefingState::addStat(
		const std::string& name,
		int score,
		int quantity)
{
	//Log(LOG_INFO) << "DebriefingState::addStat()";
	for (std::vector<DebriefingStat*>::const_iterator
			i = _stats.begin();
			i != _stats.end();
			++i)
	{
		if ((*i)->item == name)
		{
			(*i)->score += score;
			(*i)->qty += quantity;

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
 * Removes the association between the alien mission and the alien base if one existed.
 * @param am - pointer to the AlienMission
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
	for (std::vector<std::string>::const_iterator
			i = _rules->getItemsList().begin();
			i != _rules->getItemsList().end();
			++i)
	{
		if (_rules->getItem(*i)->getSpecialType() > 1)
		{
			SpecialType* const item = new SpecialType();
			item->name = *i;
			item->value = _rules->getItem(*i)->getRecoveryPoints();

			_specialTypes[_rules->getItem(*i)->getSpecialType()] = item;
		}
	}

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

	for (std::map<int, SpecialType*>::const_iterator
			i = _specialTypes.begin();
			i != _specialTypes.end();
			++i)
	{
		_stats.push_back(new DebriefingStat(
										(*i).second->name,
										true));
	}
/*	_stats.push_back(new DebriefingStat("STR_UFO_POWER_SOURCE", true));
	_stats.push_back(new DebriefingStat("STR_UFO_NAVIGATION", true));
	_stats.push_back(new DebriefingStat("STR_UFO_CONSTRUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_FOOD", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_REPRODUCTION", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ENTERTAINMENT", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_SURGERY", true));
	_stats.push_back(new DebriefingStat("STR_EXAMINATION_ROOM", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_ALLOYS", true));
	_stats.push_back(new DebriefingStat("STR_ALIEN_HABITAT", true)); */

	_stats.push_back(new DebriefingStat(
									_rules->getAlienFuel(),
									true));

	SavedBattleGame* const battle = _savedGame->getSavedBattle();

	const bool aborted = battle->isAborted();
	bool missionAccomplished = !aborted;

	Base* base = NULL;
	Craft* craft = NULL;

	std::vector<Craft*>::const_iterator craftIterator;


	_missionStatistics->time = *_savedGame->getTime();
	_missionStatistics->type = battle->getMissionType();

	int
		soldierExit = 0, // if this stays 0 the craft is lost...
		soldierLive = 0, // if this stays 0 the craft is lost...
		soldierDead = 0, // Soldier Diary.
		soldierOut = 0;

	for (std::vector<Base*>::const_iterator
			i = _savedGame->getBases()->begin();
			i != _savedGame->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator // in case there is a craft - check which craft it is about
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->isInBattlescape() == true)
			{
				const double
					craftLon = (*j)->getLongitude(),
					craftLat = (*j)->getLatitude();

				for (std::vector<Region*>::const_iterator
						k = _savedGame->getRegions()->begin();
						k != _savedGame->getRegions()->end();
						++k)
				{
					if ((*k)->getRules()->insideRegion(
													craftLon,
													craftLat) == true)
					{
						_region = *k;
						_missionStatistics->region = _region->getRules()->getType();
						break;
					}
				}

				for (std::vector<Country*>::const_iterator
						k = _savedGame->getCountries()->begin();
						k != _savedGame->getCountries()->end();
						++k)
				{
					if ((*k)->getRules()->insideCountry(
													craftLon,
													craftLat) == true)
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
				const Ufo* const ufo = dynamic_cast<Ufo*>((*j)->getDestination());
				if (ufo != NULL
					&& ufo->isInBattlescape() == true)
				{
					(*j)->returnToBase();
				}
			}
		}

		if ((*i)->isInBattlescape() == true) // in case we DON'T have a craft (ie. baseDefense)
		{
			base = *i;
			base->setInBattlescape(false);
			base->cleanupDefenses(false);

			const double
				baseLon = base->getLongitude(),
				baseLat = base->getLatitude();

			for (std::vector<Region*>::const_iterator
					k = _savedGame->getRegions()->begin();
					k != _savedGame->getRegions()->end();
					++k)
			{
				if ((*k)->getRules()->insideRegion(
												baseLon,
												baseLat) == true)
				{
					_region = *k;
					_missionStatistics->region = _region->getRules()->getType();
					break;
				}
			}

			for (std::vector<Country*>::const_iterator
					k = _savedGame->getCountries()->begin();
					k != _savedGame->getCountries()->end();
					++k)
			{
				if ((*k)->getRules()->insideCountry(
												baseLon,
												baseLat) == true)
				{
					_country = *k;
					_missionStatistics->country = _country->getRules()->getType();
					break;
				}
			}

			if (aborted == true)
				_destroyXCOMBase = true;

			bool facDestroyed = false;
			for (std::vector<BaseFacility*>::const_iterator
					k = base->getFacilities()->begin();
					k != base->getFacilities()->end();
					)
			{
				if (battle->getModuleMap()[(*k)->getX()][(*k)->getY()].second == 0) // this facility was demolished
				{
					facDestroyed = true;
					base->destroyFacility(k);
				}
				else
					++k;
			}

			base->destroyDisconnectedFacilities(); // this may cause the base to become disjointed, destroy the disconnected parts.
		}
	}

	_base = base;
	_baseLabel = _base->getName(_game->getLanguage());


	// kL_begin: Do all aLienRace types here for SoldierDiary stat.
	if (_savedGame->getMonthsPassed() != -1)
	{
		if (battle->getAlienRace().empty() == false) // safety.
			_missionStatistics->alienRace = battle->getAlienRace();
		else
			_missionStatistics->alienRace = "STR_UNKNOWN";
	} // kL_end.


	// UFO crash/landing site disappears
	for (std::vector<Ufo*>::const_iterator
			i = _savedGame->getUfos()->begin();
			i != _savedGame->getUfos()->end();
			++i)
	{
		if ((*i)->isInBattlescape() == true)
		{
			(*i)->setInBattlescape(false);

			_missionStatistics->ufo = (*i)->getRules()->getType();

			if (aborted == true
				&& (*i)->getStatus() == Ufo::LANDED)
			{
				(*i)->setSecondsRemaining(15); // UFO lifts off ...
			}
			else if (aborted == false) // successful mission ( kL: failed mission leaves UFO still crashed! )
//kL			|| (*i)->getStatus() == Ufo::CRASHED) // UFO can't fly
			{
				delete *i;
				_savedGame->getUfos()->erase(i); // UFO disappears.
			}

			break;
		}
	}

	// terror site disappears (even when you abort)
	for (std::vector<TerrorSite*>::const_iterator
			i = _savedGame->getTerrorSites()->begin();
			i != _savedGame->getTerrorSites()->end();
			++i)
	{
		if ((*i)->isInBattlescape() == true)
		{
			delete *i;
			_savedGame->getTerrorSites()->erase(i);

			break;
		}
	}

	// lets see what happens with units; first,
	// we evaluate how many surviving XCom units there are,
	// and if they are unconscious
	// and how many have died (to use for commendations)
	for (std::vector<BattleUnit*>::const_iterator
			i = battle->getUnits()->begin();
			i != battle->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->getStatus() != STATUS_DEAD)
		{
			if ((*i)->getStatus() == STATUS_UNCONSCIOUS
				|| (*i)->getFaction() == FACTION_HOSTILE)
			{
				++soldierOut;
			}

			++soldierLive;
		}
		else if ((*i)->getOriginalFaction() == FACTION_PLAYER
			&& (*i)->getStatus() == STATUS_DEAD)
		{
			++soldierDead;
		}
	}

	// if all our men are unconscious, the aliens get to have their way with them.
	if (soldierOut == soldierLive)
	{
		soldierLive = 0;

		for (std::vector<BattleUnit*>::const_iterator
				i = battle->getUnits()->begin();
				i != battle->getUnits()->end();
				++i)
		{
			if ((*i)->getOriginalFaction() == FACTION_PLAYER
				&& (*i)->getStatus() != STATUS_DEAD)
			{
				(*i)->instaKill();
			}
		}
	}

	if (soldierLive == 1)
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = battle->getUnits()->begin();
				i != battle->getUnits()->end();
				++i)
		{
			if ((*i)->getGeoscapeSoldier() != NULL
				&& (*i)->getStatus() != STATUS_DEAD)
			{
				// if only one soldier survived AND none have died, means only one soldier went on the mission...
				if (soldierDead == 0
					&& _aliensControlled == 0
					&& _aliensKilled + _aliensStunned > 2 + static_cast<int>(_savedGame->getDifficulty())
					&& aborted == false)
				{
					(*i)->getStatistics()->ironMan = true;
					break;
				}
				// if only one soldier survived, give him a medal! (unless he killed all the others...)
				else if ((*i)->getStatistics()->hasFriendlyFired() == false
					&& soldierDead > 0)
				{
					(*i)->getStatistics()->loneSurvivor = true;
					break;
				}
			}
		}
	}

	const std::string mission = battle->getMissionType();

	// alien base disappears (if you didn't abort)
	if (mission == "STR_ALIEN_BASE_ASSAULT")
	{
		_txtRecovery->setText(tr("STR_ALIEN_BASE_RECOVERY"));
		bool destroyAlienBase = true;

		if (aborted == true
			|| soldierLive == 0)
		{
			for (int
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				if (battle->getTiles()[i]->getMapData(3) != NULL
					&& battle->getTiles()[i]->getMapData(3)->getSpecialType() == UFO_NAVIGATION)
				{
					destroyAlienBase = false;
					break;
				}
			}
		}

		missionAccomplished = destroyAlienBase;

		for (std::vector<AlienBase*>::const_iterator
				i = _savedGame->getAlienBases()->begin();
				i != _savedGame->getAlienBases()->end();
				++i)
		{
			if ((*i)->isInBattlescape() == true)
			{
				if (destroyAlienBase == true)
				{
					addStat(
						"STR_ALIEN_BASE_CONTROL_DESTROYED",
						300);

					// Take care to remove supply missions for this base.
					std::for_each(
							_savedGame->getAlienMissions().begin(),
							_savedGame->getAlienMissions().end(),
							ClearAlienBase(*i));

					delete *i;
					_savedGame->getAlienBases()->erase(i);

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
	for (std::vector<BattleUnit*>::const_iterator
			i = battle->getUnits()->begin();
			i != battle->getUnits()->end();
			++i)
	{
		if ((*i)->getTile() == NULL) // handle unconscious units; This unit is not on a tile...
		{
			Position pos = (*i)->getPosition();
			if (pos == Position(-1,-1,-1)) // in fact, this Unit is in limbo...
			{
				for (std::vector<BattleItem*>::const_iterator // so look for its body or corpse...
						j = battle->getItems()->begin();
						j != battle->getItems()->end();
						++j)
				{
					if ((*j)->getUnit() != NULL
						&& (*j)->getUnit() == *i) // found it: corpse is a dead BattleUnit!!
					{
						if ((*j)->getOwner() != NULL) // corpse of BattleUnit has an Owner (ie. is being carried by another BattleUnit)
							pos = (*j)->getOwner()->getPosition(); // Put the corpse down.. slowly.
						else if ((*j)->getTile() != NULL) // corpse of BattleUnit is laying around somewhere
							pos = (*j)->getTile()->getPosition(); // you're not dead yet, Get up.
					}
				}
			}

			(*i)->setTile(battle->getTile(pos));
		}


		const int value = (*i)->getValue();
		const UnitFaction origFact = (*i)->getOriginalFaction();
		const UnitStatus status = (*i)->getStatus();

		if (status == STATUS_DEAD) // so this is a dead unit
		{
			if (origFact == FACTION_HOSTILE
				&& (*i)->killedBy() == FACTION_PLAYER)
			{
				addStat(
					"STR_ALIENS_KILLED",
					value);
			}
			else if (origFact == FACTION_PLAYER)
			{
				const Soldier* const soldier = _savedGame->getSoldier((*i)->getId());
				if (soldier != NULL) // xCom soldier.
				{
					addStat(
						"STR_XCOM_OPERATIVES_KILLED",
						-value);

					for (std::vector<Soldier*>::const_iterator
							j = base->getSoldiers()->begin();
							j != base->getSoldiers()->end();
							++j)
					{
						if (*j == soldier) // the specific soldier at Base..
						{
							(*i)->updateGeoscapeStats(*j);

							(*j)->die(_savedGame);

							delete *j;
							base->getSoldiers()->erase(j);

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
				if ((*i)->killedBy() == FACTION_PLAYER)
					addStat(
						"STR_CIVILIANS_KILLED_BY_XCOM_OPERATIVES",
						-(value * 2));
				else
					// if civilians happen to kill themselves XCOM shouldn't get penalty for it
					addStat(
						"STR_CIVILIANS_KILLED_BY_ALIENS",
						-value);
			}
		}
		else // so this unit is NOT dead...
		{
			const UnitFaction faction = (*i)->getFaction();
			std::string type = (*i)->getType();

			if ((*i)->getSpawnUnit().empty() == false)
				type = (*i)->getSpawnUnit();

			if (origFact == FACTION_PLAYER)
			{
				const Soldier* const soldier = _savedGame->getSoldier((*i)->getId());

				if (((*i)->isInExitArea() == true
						&& (mission != "STR_BASE_DEFENSE"
							|| missionAccomplished == true))
					|| aborted == false)
				{
					// so game is not aborted, or aborted and unit is on exit area
					(*i)->postMissionProcedures(_savedGame);

					++soldierExit;

					if (soldier != NULL)
					{
						recoverItems(
								(*i)->getInventory(),
								base);

//						soldier->calcStatString( // calculate new statString
//											_rules->getStatStrings(),
//											Options::psiStrengthEval
//												&& _savedGame->isResearched(_rules->getPsiRequirements()));
					}
					else // not soldier -> tank
					{
						base->getItems()->addItem(type);

						const RuleItem* const tankRule = _rules->getItem(type);
						const BattleItem* const ammoItem = (*i)->getItem("STR_RIGHT_HAND")->getAmmoItem();

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
						for (std::vector<Soldier*>::const_iterator
								j = base->getSoldiers()->begin();
								j != base->getSoldiers()->end();
								++j)
						{
							if (*j == soldier)
							{
								(*i)->updateGeoscapeStats(*j);
								(*i)->setStatus(STATUS_DEAD);

								(*j)->die(_savedGame);

								delete *j;
								base->getSoldiers()->erase(j);

								// note: Could return any armor the soldier was wearing to Stores. CHEATER!!!!!
								break;
							}
						}
					}
				}
			}
			else if (origFact == FACTION_HOSTILE	// This section is for units still standing.
				&& faction == FACTION_PLAYER		// MC'd aLiens count as unconscious
				&& (*i)->isOut() == false
				&& (aborted == false
					|| (*i)->isInExitArea() == true))
				// kL_note: so, this never actually runs unless early psi-exit
				// when all aliens are dead or mind-controlled is turned on ....
				// Except the two-stage Cydonia mission, in which case all this
				// does not matter.
			{
				// ADD ALIENS MC'D COUNTER HERE_kL
				++_aliensControlled;

				for (std::vector<BattleItem*>::const_iterator
						j = (*i)->getInventory()->begin();
						j != (*i)->getInventory()->end();
						++j)
				{
					if ((*j)->getRules()->isFixed() == false)
						(*i)->getTile()->addItem(
											*j,
											_rules->getInventory("STR_GROUND"));
				}

				std::string corpseItem = (*i)->getArmor()->getCorpseGeoscape();
				if ((*i)->getSpawnUnit().empty() == false)
				{
					corpseItem = _rules->getArmor(_rules->getUnit((*i)->getSpawnUnit())->getArmor())
								 ->getCorpseGeoscape();
				}

				if (base->getAvailableContainment() > 0) // should this do +1 containment per loop ...
					addStat(
						"STR_LIVE_ALIENS_RECOVERED",
						value); // duplicated in function below.
				else
					addStat(
						"STR_ALIENS_KILLED",
						value); // duplicated in function below.

				RuleResearch* const research = _rules->getResearch(type);
				if (research != NULL
					&& _savedGame->isResearchAvailable(
													research,
													_savedGame->getDiscoveredResearch(),
													_rules) == true)
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
							value, // duplicated in function below.
							0);

						base->getItems()->addItem(type);
						_manageContainment = base->getAvailableContainment()
										   - (base->getUsedContainment() * _limitsEnforced) < 0;
					}
				}
				else
				{
/*					if (Options::canSellLiveAliens == true)
						_savedGame->setFunds(
										_savedGame->getFunds()
										+ _rules->getItem(type)->getSellCost());
					else */ // Don't sell unconscious aLiens post-battle. (see recoverItems() below_ also)
					base->getItems()->addItem(corpseItem);
				}
			}
			else if (origFact == FACTION_NEUTRAL)
			{
				if (soldierLive == 0
					|| (aborted == true						// if mission fails, all civilians die
						&& (*i)->isInExitArea() == false))	// unless they're on an Exit_Tile. kL
				{
					addStat(
						"STR_CIVILIANS_KILLED_BY_ALIENS",
						-value);
				}
				else
					addStat(
						"STR_CIVILIANS_SAVED",
						value); // duplicated below.
			}
		}
	}

	if (craft != NULL
		&& ((soldierExit == 0
				&& aborted == true)
			|| soldierLive == 0))
	{
		addStat(
			"STR_XCOM_CRAFT_LOST",
			-craft->getRules()->getScore());

		for (std::vector<Soldier*>::const_iterator
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

		craft = NULL; // To avoid a crash down there!! uh, not after return; it won't.
		base->getCrafts()->erase(craftIterator);
		_txtTitle->setText(tr("STR_CRAFT_IS_LOST"));

		return;
	}

	if (aborted == true
		&& mission == "STR_BASE_DEFENSE"
		&& base->getCrafts()->empty() == false)
	{
		//Log(LOG_INFO) << ". aborted BASE_DEFENSE";
		for (std::vector<Craft*>::const_iterator
				i = base->getCrafts()->begin();
				i != base->getCrafts()->end();
				++i)
		{
			addStat(
				"STR_XCOM_CRAFT_LOST",
				-(*i)->getRules()->getScore());
		}
	}


	if ((aborted == false
			|| missionAccomplished == true)	// RECOVER UFO:
		&& soldierLive > 0)					// Run through all tiles to recover UFO components and items.
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


		if (aborted == false)
		{
			for (int // get recoverable map data objects from the battlescape map
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				for (int
						part = 0;
						part < 4;
						++part)
				{
					if (battle->getTiles()[i]->getMapData(part))
					{
						const size_t specialType = battle->getTiles()[i]->getMapData(part)->getSpecialType();
						if (_specialTypes.find(specialType) != _specialTypes.end())
							addStat(
								_specialTypes[specialType]->name,
								_specialTypes[specialType]->value);
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
				if (battle->getTiles()[i]->getMapData(MapData::O_FLOOR) != NULL
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
			_destroyXCOMBase = true;
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
			&& _destroyXCOMBase == false)
		{
			for (int // recover items from the ground
					i = 0;
					i < battle->getMapSizeXYZ();
					++i)
			{
				if (battle->getTiles()[i]->getMapData(MapData::O_FLOOR) != NULL
					&& battle->getTiles()[i]->getMapData(MapData::O_FLOOR)->getSpecialType() == START_POINT)
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
		if (i->first->getClipSize() > 0)
		{
			const int total_clips = i->second / i->first->getClipSize();
			if (total_clips > 0)
				base->getItems()->addItem(
										i->first->getType(),
										total_clips);
		}
	}

	if (soldierLive > 0) // recover all our goodies
	{
		for (std::vector<DebriefingStat*>::const_iterator
				i = _stats.begin();
				i != _stats.end();
				++i)
		{
			// alien alloys recovery values are divided by 10 or divided by 150 in case of an alien base
			if ((*i)->item == _specialTypes[ALIEN_ALLOYS]->name)
			{
				int alloys = 10;
				if (mission == "STR_ALIEN_BASE_ASSAULT")
					alloys = 150;

				(*i)->qty = (*i)->qty / alloys;
				(*i)->score = (*i)->score / alloys;
			}

			// recoverable battlescape tiles are now converted to items and put in base inventory
			if ((*i)->recover == true
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

	if (mission == "STR_BASE_DEFENSE")
	{
		if (_destroyXCOMBase == false)
		{
			// reequip crafts (only those on the base) after a base defense mission;
			for (std::vector<Craft*>::const_iterator
					i = base->getCrafts()->begin();
					i != base->getCrafts()->end();
					++i)
			{
				if ((*i)->getStatus() != "STR_OUT")
					reequipCraft(
								base,
								*i,
								false);
			}

			// clear base->getVehicles() objects, they not needed anymore.
			for (std::vector<Vehicle*>::const_iterator
					i = base->getVehicles()->begin();
					i != base->getVehicles()->end();
					++i)
			{
				delete *i;
			}

			base->getVehicles()->clear();
		}
		else if (_savedGame->getMonthsPassed() != -1)
		{
			for (std::vector<Base*>::const_iterator
					i = _savedGame->getBases()->begin();
					i != _savedGame->getBases()->end();
					++i)
			{
				if (*i == base)
				{
					delete *i;
					_savedGame->getBases()->erase(i);

					break;
				}
			}
		}

		if (_region != NULL)
		{
			const AlienMission* const am = _savedGame->getAlienMission(
																_region->getRules()->getType(),
																"STR_ALIEN_RETALIATION");
			for (std::vector<Ufo*>::const_iterator
					i = _savedGame->getUfos()->begin();
					i != _savedGame->getUfos()->end();
					)
			{
				if ((*i)->getMission() == am)
				{
					delete *i;
					i = _savedGame->getUfos()->erase(i);
				}
				else
					++i;
			}

			for (std::vector<AlienMission*>::const_iterator
					i = _savedGame->getAlienMissions().begin();
					i != _savedGame->getAlienMissions().end();
					++i)
			{
				if (dynamic_cast<AlienMission*>(*i) == am)
				{
					delete *i;
					_savedGame->getAlienMissions().erase(i);

					break;
				}
			}
		}
	}

	_missionStatistics->success = missionAccomplished;
	//Log(LOG_INFO) << "DebriefingState::prepareDebriefing() EXIT";
}

/**
 * Reequips a craft after a mission.
 * @param base					- pointer to a Base to reequip from
 * @param craft					- pointer to a Craft to reequip
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

	const std::map<std::string, int> craftItems = *craft->getItems()->getContents();
	for (std::map<std::string, int>::const_iterator
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

			const ReequipStat stat =
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

	// the vehicles
	ItemContainer craftVehicles;
	for (std::vector<Vehicle*>::const_iterator
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
	if (vehicleDestruction == true)
	{
		for (std::vector<Vehicle*>::const_iterator
				i = craft->getVehicles()->begin();
				i != craft->getVehicles()->end();
				++i)
		{
			delete *i;
		}
	}

	craft->getVehicles()->clear();

	int canBeAdded = 0;

	for (std::map<std::string, int>::const_iterator // Ok, now read those vehicles
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
			const ReequipStat stat =
			{
				i->first,
				lost,
				craft->getName(_game->getLanguage())
			};

			_missingItems.push_back(stat);
		}


		RuleItem* const tankRule = _rules->getItem(i->first);
		const Unit* const tankUnit = _rules->getUnit(tankRule->getType());

		int tankSize = 4;
		if (tankUnit != NULL)
		{
			tankSize = _rules->getArmor(tankUnit->getArmor())->getSize();
			tankSize *= tankSize;
		}

		if (tankRule->getCompatibleAmmo()->empty() == true) // so this tank does NOT require ammo
		{
			for (int
					j = 0;
					j < canBeAdded;
					++j)
			{
				craft->getVehicles()->push_back(new Vehicle(
														tankRule,
														tankRule->getClipSize(),
														tankSize));
			}

			base->getItems()->removeItem(
										i->first,
										canBeAdded);
		}
		else // so this tank requires ammo
		{
			const RuleItem* const ammoRule = _rules->getItem(tankRule->getCompatibleAmmo()->front());
			const int ammoPerVehicle = ammoRule->getClipSize();

			const int baseQty = base->getItems()->getItem(ammoRule->getType()); // Ammo Quantity for this vehicle-type on the base

			if (baseQty < i->second * ammoPerVehicle)
			{
				lost = (i->second * ammoPerVehicle) - baseQty; // missing ammo
				const ReequipStat stat =
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
															tankSize));

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
 * Converts the battlescape inventory into a geoscape itemcontainer.
 * @param battleItems	- pointer to a vector of pointers to BattleItems on the battlescape
 * @param base			- base to add recovered items to
 */
void DebriefingState::recoverItems(
		std::vector<BattleItem*>* battleItems,
		Base* base)
{
	//Log(LOG_INFO) << "DebriefingState::recoverItems()";
	for (std::vector<BattleItem*>::const_iterator
			i = battleItems->begin();
			i != battleItems->end();
			++i)
	{
		if ((*i)->getRules()->getName() == _rules->getAlienFuel())
			addStat( // special case of an item counted as a stat
				_rules->getAlienFuel(),
				100,
				50);
		else
		{
			if ((*i)->getRules()->getRecoveryPoints() != 0
				&& (*i)->getXCOMProperty() == false)
			{
				if ((*i)->getRules()->getBattleType() == BT_CORPSE)
				{
					if ((*i)->getUnit()->getStatus() == STATUS_DEAD)
					{
						addStat(
							"STR_ALIEN_CORPSES_RECOVERED",
							(*i)->getUnit()->getValue() / 3);

						base->getItems()->addItem((*i)->getUnit()->getArmor()->getCorpseGeoscape());
					}
					else if ((*i)->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
					{
						if ((*i)->getUnit()->getOriginalFaction() == FACTION_HOSTILE)
						{
							// ADD STUNNED ALIEN COUNTING HERE_kL
							++_aliensStunned; // for Nike Cross determination.

							if (base->getAvailableContainment() > 0) // should this do +1 containment per loop ...
								addStat(
									"STR_LIVE_ALIENS_RECOVERED",
									(*i)->getUnit()->getValue()); // duplicated above.
							else
								addStat(
									"STR_ALIENS_KILLED",
									(*i)->getUnit()->getValue()); // duplicated above.

							RuleResearch* const research = _rules->getResearch((*i)->getUnit()->getType());
							if (research != NULL
								&& _savedGame->isResearchAvailable(
															research,
															_savedGame->getDiscoveredResearch(),
															_rules) == true)
							{
								if (base->getAvailableContainment() == 0)
								{
									_noContainment = true;
									base->getItems()->addItem((*i)->getUnit()->getArmor()->getCorpseGeoscape());
								}
								else
								{
									addStat( // more points if item's not researched
										"STR_LIVE_ALIENS_RECOVERED",
										(*i)->getUnit()->getValue(), // duplicated above.
										0);

									base->getItems()->addItem((*i)->getUnit()->getType());
									_manageContainment = base->getAvailableContainment()
													   - (base->getUsedContainment() * _limitsEnforced) < 0;
								}
							}
							else // Don't sell unconscious aLiens post-battle. (see prepareDebriefing() above^ also)
								base->getItems()->addItem((*i)->getUnit()->getArmor()->getCorpseGeoscape());
						}
						else if ((*i)->getUnit()->getOriginalFaction() == FACTION_NEUTRAL)
							addStat(
								"STR_CIVILIANS_SAVED",
								(*i)->getUnit()->getValue()); // duplicated above.
					}
				}
				else if (_savedGame->isResearched((*i)->getRules()->getType()) == false)
					addStat( // add pts. for unresearched items only
						"STR_ALIEN_ARTIFACTS_RECOVERED",
						(*i)->getRules()->getRecoveryPoints());
			}


			if ((*i)->getRules()->isRecoverable() == true
				&& (*i)->getRules()->isFixed() == false)
			{
				switch ((*i)->getRules()->getBattleType()) // put items back in the base
				{
					case BT_CORPSE:
					break;
					case BT_AMMO: // item's a clip, count rounds left
						_rounds[(*i)->getRules()] += (*i)->getAmmoQuantity();
					break;
					case BT_FIREARM:
					case BT_MELEE: // item's a weapon, count rounds in clip
					{
						const BattleItem* const clip = (*i)->getAmmoItem();
						if (clip != NULL
							&& clip != *i
							&& clip->getRules()->getClipSize() > 0)
						{
							_rounds[clip->getRules()] += clip->getAmmoQuantity();
						}
					}

					default: // fall-through, recover the weapon itself
						base->getItems()->addItem((*i)->getRules()->getType());
					break;
				}
			}
		}
	}
	//Log(LOG_INFO) << "DebriefingState::recoverItems() EXIT";
}

}
