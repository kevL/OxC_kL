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

#include "MonthlyReportState.h"

//#define _USE_MATH_DEFINES
//#include <cmath>
//#include <sstream>

#include "DefeatState.h"

#include "../Battlescape/CeremonyState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Menu/SaveGameState.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Monthly Report screen.
 */
MonthlyReportState::MonthlyReportState()
	:
		_gameOver(false),
		_ratingTotal(0),
		_deltaFunds(0),
		_ratingLast(0),
		_gameSave(_game->getSavedGame())
{
	_window		= new Window(this, 320, 200);
	_txtTitle	= new Text(300, 17, 10, 8);

	_txtMonth	= new Text(110, 9,  16, 24);
	_txtRating	= new Text(178, 9, 126, 24);

	_txtChange	= new Text(288,   9, 16, 32);
	_txtDesc	= new Text(288, 140, 16, 40);

	_btnOk		= new TextButton(288, 16, 16, 177);

	_txtFailure	= new Text(288, 160, 16, 10);
	_btnBigOk	= new TextButton(120, 18, 100, 175);

//	_txtIncome = new Text(300, 9, 16, 32);
//	_txtMaintenance = new Text(130, 9, 16, 40);
//	_txtBalance = new Text(160, 9, 146, 40);

	setInterface("monthlyReport");

	add(_window,		"window",	"monthlyReport");
	add(_txtTitle,		"text1",	"monthlyReport");
	add(_txtMonth,		"text1",	"monthlyReport");
	add(_txtRating,		"text1",	"monthlyReport");
	add(_txtChange,		"text1",	"monthlyReport");
	add(_txtDesc,		"text2",	"monthlyReport");
	add(_btnOk,			"button",	"monthlyReport");
	add(_txtFailure,	"text2",	"monthlyReport");
	add(_btnBigOk,		"button",	"monthlyReport");

//	add(_txtIncome,			"text1", "monthlyReport");
//	add(_txtMaintenance,	"text1", "monthlyReport");
//	add(_txtBalance,		"text1", "monthlyReport");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_txtTitle->setText(tr("STR_XCOM_PROJECT_MONTHLY_REPORT"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();


	calculateChanges(); // <- sets Rating etc.

	int
		month = _gameSave->getTime()->getMonth() - 1,
		year = _gameSave->getTime()->getYear();

	if (month == 0)
	{
		month = 12;
		--year;
	}

	std::string st;
	switch (month)
	{
		case  1: st = "STR_JAN"; break;
		case  2: st = "STR_FEB"; break;
		case  3: st = "STR_MAR"; break;
		case  4: st = "STR_APR"; break;
		case  5: st = "STR_MAY"; break;
		case  6: st = "STR_JUN"; break;
		case  7: st = "STR_JUL"; break;
		case  8: st = "STR_AUG"; break;
		case  9: st = "STR_SEP"; break;
		case 10: st = "STR_OCT"; break;
		case 11: st = "STR_NOV"; break;
		case 12: st = "STR_DEC";
	}
	_txtMonth->setText(tr("STR_MONTH").arg(tr(st)).arg(year));

	const int
		diff = static_cast<int>(_gameSave->getDifficulty()),
		difficulty_threshold = 250 * (diff - 4);
		// 0 -> -1000
		// 1 -> -750
		// 2 -> -500
		// 3 -> -250
		// 4 -> 0

	std::string music = OpenXcom::res_MUSIC_GEO_MONTHLYREPORT;

	std::wstring wst; // do rating
	if (_ratingTotal > 10000)
		wst = tr("STR_RATING_STUPENDOUS");
	else if (_ratingTotal > 5000)
		wst = tr("STR_RATING_EXCELLENT");
	else if (_ratingTotal > 2500)
		wst = tr("STR_RATING_GOOD");
	else if (_ratingTotal > 1000)
		wst = tr("STR_RATING_OK");
	else if (_ratingTotal > difficulty_threshold)
		wst = tr("STR_RATING_POOR");
	else
	{
		wst = tr("STR_RATING_TERRIBLE");
		music = OpenXcom::res_MUSIC_GEO_MONTHLYREPORT_BAD;
	}

	_txtRating->setText(tr("STR_MONTHLY_RATING").arg(_ratingTotal).arg(wst));

/*	std::wostringstream ss; // ADD:
	ss << tr("STR_INCOME") << L"> \x01" << Text::formatFunding(_game->getSavedGame()->getCountryFunding());
	ss << L" (";
	if (_deltaFunds > 0)
		ss << '+';
	ss << Text::formatFunding(_deltaFunds) << L")";
	_txtIncome->setText(ss.str());

	std::wostringstream ss2;
	ss2 << tr("STR_MAINTENANCE") << L"> \x01" << Text::formatFunding(_game->getSavedGame()->getBaseMaintenances());
	_txtMaintenance->setText(ss2.str());

	std::wostringstream ss3;
	ss3 << tr("STR_BALANCE") << L"> \x01" << Text::formatFunding(_game->getSavedGame()->getFunds());
	_txtBalance->setText(ss3.str());
// end ADD. */

	std::wostringstream woststr;
	if (_deltaFunds > 0) woststr << '+';
	woststr << Text::formatFunding(_deltaFunds);
	_txtChange->setText(tr("STR_FUNDING_CHANGE").arg(woststr.str()));


	if (_ratingLast <= difficulty_threshold // calculate satisfaction
		&& _ratingTotal <= difficulty_threshold)
	{
		wst = tr("STR_YOU_HAVE_NOT_SUCCEEDED");

		_pactList.erase(
					_pactList.begin(),
					_pactList.end());
		_happyList.erase(
					_happyList.begin(),
					_happyList.end());
		_sadList.erase(
					_sadList.begin(),
					_sadList.end());

		_gameOver = true; // you lose.
	}
	else if (_ratingTotal > 1000 + (diff * 2000)) // was 1500 flat.
		wst = tr("STR_COUNCIL_IS_VERY_PLEASED");
	else if (_ratingTotal > difficulty_threshold)
		wst = tr("STR_COUNCIL_IS_GENERALLY_SATISFIED");
	else
		wst = tr("STR_COUNCIL_IS_DISSATISFIED");

	woststr.str(L"");
	woststr << wst;

	bool resetWarning = true;
	if (_gameOver == false)
	{
		if (_gameSave->getFunds() < -999999)
		{
			if (_gameSave->getWarned() == true)
			{
				woststr.str(L"");
				woststr << tr("STR_YOU_HAVE_NOT_SUCCEEDED");

				_pactList.erase(
							_pactList.begin(),
							_pactList.end());
				_happyList.erase(
							_happyList.begin(),
							_happyList.end());
				_sadList.erase(
							_sadList.begin(),
							_sadList.end());

				_gameOver = true; // you lose.
			}
			else
			{
				woststr << "\n\n" << tr("STR_COUNCIL_REDUCE_DEBTS");

				_gameSave->setWarned(true);
				resetWarning = false;

				music = OpenXcom::res_MUSIC_GEO_MONTHLYREPORT_BAD;
			}
		}
	}

	if (resetWarning == true
		&& _gameSave->getWarned() == true)
	{
		_gameSave->setWarned(false);
	}

	woststr << countryList(
					_happyList,
					"STR_COUNTRY_IS_PARTICULARLY_PLEASED",
					"STR_COUNTRIES_ARE_PARTICULARLY_HAPPY");
	woststr << countryList(
					_sadList,
					"STR_COUNTRY_IS_UNHAPPY_WITH_YOUR_ABILITY",
					"STR_COUNTRIES_ARE_UNHAPPY_WITH_YOUR_ABILITY");
	woststr << countryList(
					_pactList,
					"STR_COUNTRY_HAS_SIGNED_A_SECRET_PACT",
					"STR_COUNTRIES_HAVE_SIGNED_A_SECRET_PACT");

	_txtDesc->setText(woststr.str());
	_txtDesc->setWordWrap();


	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& MonthlyReportState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MonthlyReportState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MonthlyReportState::btnOkClick,
					Options::keyCancel);


	_txtFailure->setBig();
	_txtFailure->setAlign(ALIGN_CENTER);
	_txtFailure->setVerticalAlign(ALIGN_MIDDLE);
	_txtFailure->setWordWrap();
	_txtFailure->setText(tr("STR_YOU_HAVE_FAILED"));
	_txtFailure->setVisible(false);

	_btnBigOk->setText(tr("STR_OK"));
	_btnBigOk->onMouseClick((ActionHandler)& MonthlyReportState::btnOkClick);
	_btnBigOk->onKeyboardPress(
					(ActionHandler)& MonthlyReportState::btnOkClick,
					Options::keyOk);
	_btnBigOk->onKeyboardPress(
					(ActionHandler)& MonthlyReportState::btnOkClick,
					Options::keyCancel);
	_btnBigOk->setVisible(false);


	_game->getResourcePack()->playMusic(
									music,
									"",1);
}

/**
 * dTor.
 */
MonthlyReportState::~MonthlyReportState()
{}

/**
 * Updates all activity counters, gathers all scores, gets countries to sign
 * pacts, adjusts their fundings, assesses their satisfaction, and finally
 * calculates overall total score, with thanks to Volutar for the formulae.
 */
void MonthlyReportState::calculateChanges() // private.
{
	_ratingLast = 0;

	int
		total = 0,
		aLienTotal = 0;

	const size_t lastMonth = _gameSave->getFundsList().size() - 2;

	for (std::vector<Region*>::const_iterator
			i = _gameSave->getRegions()->begin();
			i != _gameSave->getRegions()->end();
			++i)
	{
		(*i)->newMonth();

		if (lastMonth > 0)
			_ratingLast += (*i)->getActivityXCom().at(lastMonth - 1)
						 - (*i)->getActivityAlien().at(lastMonth - 1);

		total += (*i)->getActivityXCom().at(lastMonth);
		aLienTotal += (*i)->getActivityAlien().at(lastMonth);
	}


	const int diff = static_cast<int>(_gameSave->getDifficulty());

	for (std::vector<Country*>::const_iterator
			i = _gameSave->getCountries()->begin();
			i != _gameSave->getCountries()->end();
			++i)
	{
		std::string st = (*i)->getRules()->getType();

		if ((*i)->getNewPact() == true)
			_pactList.push_back(st);

		(*i)->newMonth(
					total,
					aLienTotal,
					diff);

		_deltaFunds += (*i)->getFunding().back()
					 - (*i)->getFunding().at(lastMonth);

		switch ((*i)->getSatisfaction())
		{
			case 1:
				_sadList.push_back(st);
			break;
			case 3:
				_happyList.push_back(st);
		}
	}

	if (lastMonth > 0)
		_ratingLast += _gameSave->getResearchScores().at(lastMonth - 1);

	total += _gameSave->getResearchScores().at(lastMonth);
	_ratingTotal = total - aLienTotal; // total RATING
}
/*	_ratingLast = 0;

	int
		total = 0,
		subTotal = 0,
		aLienTotal = 0,

		offset = static_cast<int>(_gameSave->getFundsList().size()) - 2,
		offset_pre = offset - 1;

	if (offset_pre < 0)
		offset_pre += 2;

	// update activity meters, calculate a total score based
	// on regional activity and gather last month's score
	for (std::vector<Region*>::const_iterator
			i = _gameSave->getRegions()->begin();
			i != _gameSave->getRegions()->end();
			++i)
	{
		(*i)->newMonth();

		if ((*i)->getActivityXCom().size() > 2)
			_ratingLast += (*i)->getActivityXCom().at(offset_pre)
						 - (*i)->getActivityAlien().at(offset_pre);

		subTotal += (*i)->getActivityXCom().at(offset);
		aLienTotal += (*i)->getActivityAlien().at(offset);
	}

	// apply research bonus AFTER calculating total, because this bonus applies
	// to the council ONLY, and shouldn't influence each country's decision.
	// kL_note: And yet you _do_ add it in to country->newMonth() decisions...!
	// So, hey, I'll take it out for you.. just a sec.
	total = _gameSave->getResearchScores().at(offset) + subTotal;

	// the council is more lenient after the first month
//	if (_gameSave->getMonthsPassed() > 1)
//		_gameSave->getResearchScores().at(offset) += 400;

	if (_gameSave->getResearchScores().size() > 2)
		_ratingLast += _gameSave->getResearchScores().at(offset_pre);


	// now that we have our totals we can send the relevant info to the countries
	// and have them make their decisions weighted on the council's perspective.
	for (std::vector<Country*>::const_iterator
			i = _gameSave->getCountries()->begin();
			i != _gameSave->getCountries()->end();
			++i)
	{
		// add them to the list of new pact members; this is done BEFORE initiating
		// a new month because the _newPact flag will be reset in the process <-
		if ((*i)->getNewPact())
			_pactList.push_back((*i)->getRules()->getType());

		// determine satisfaction level, sign pacts, adjust funding, and update activity meters
		(*i)->newMonth(
					subTotal, // There. done
					aLienTotal,
					static_cast<int>(_gameSave->getDifficulty()));

		// and after they've made their decisions, calculate the difference;
		// and add them to the appropriate lists.
		_deltaFunds += (*i)->getFunding().back()
					 - (*i)->getFunding().at((*i)->getFunding().size() - 2);

		switch ((*i)->getSatisfaction())
		{
			case 1:
				_sadList.push_back((*i)->getRules()->getType());
			break;
			case 3:
				_happyList.push_back((*i)->getRules()->getType());
		}
	}
	_ratingTotal = total - aLienTotal; // total RATING */

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void MonthlyReportState::btnOkClick(Action*)
{
	if (_gameOver == false)
	{
		_game->popState();

		for (std::vector<Base*>::const_iterator // Award medals for service time
				i = _gameSave->getBases()->begin();
				i != _gameSave->getBases()->end();
				++i)
		{
			for (std::vector<Soldier*>::const_iterator
					j = (*i)->getSoldiers()->begin();
					j != (*i)->getSoldiers()->end();
					++j)
			{
				(*j)->getDiary()->addMonthlyService();

				if ((*j)->getDiary()->manageAwards(_game->getRuleset()) == true)
					_soldiersMedalled.push_back(*j);
			}
		}

		if (_soldiersMedalled.empty() == false)
			_game->pushState(new CeremonyState(_soldiersMedalled));

		if (_gameSave->isIronman() == true)
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
	else
	{
		if (_txtFailure->getVisible() == true)
		{
			_game->popState();
			_game->pushState(new DefeatState());
		}
		else
		{
			_window->setColor(static_cast<Uint8>(_game->getRuleset()->getInterface("monthlyReport")->getElement("window")->color2));

			_txtTitle->setVisible(false);
			_txtMonth->setVisible(false);
			_txtRating->setVisible(false);
			_txtChange->setVisible(false);
//			_txtIncome->setVisible(false);
//			_txtMaintenance->setVisible(false);
//			_txtBalance->setVisible(false);
			_txtDesc->setVisible(false);
			_btnOk->setVisible(false);

			_txtFailure->setVisible();
			_btnBigOk->setVisible();


			_game->getResourcePack()->fadeMusic(_game, 1157);
			_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_LOSE);
		}
	}
}

/**
 * Builds a sentence from a list of countries adding the appropriate
 * separators and pluralization.
 * @param countries	- reference a vector of strings that is the list of countries
 * @param singular	- reference a string to append if the returned string is singular
 * @param plural	- reference a string to append if the returned string is plural
 */
std::wstring MonthlyReportState::countryList( // private.
		const std::vector<std::string>& countries,
		const std::string& singular,
		const std::string& plural)
{
	std::wostringstream woststr;

	if (countries.empty() == false)
	{
		woststr << "\n\n";
		if (countries.size() == 1)
			woststr << tr(singular).arg(tr(countries.front()));
		else
		{
			LocalizedText countryList = tr(countries.front());

			std::vector<std::string>::const_iterator i;
			for (
					i = countries.begin() + 1;
					i != countries.end() - 1;
					++i)
			{
				countryList = tr("STR_COUNTRIES_COMMA").arg(countryList).arg(tr(*i));
			}
			countryList = tr("STR_COUNTRIES_AND").arg(countryList).arg(tr(*i));

			woststr << tr(plural).arg(countryList);
		}
	}

	return woststr.str();
}

}
