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

#include "Ufopaedia.h"

#include "ArticleState.h"
#include "ArticleStateArmor.h"
#include "ArticleStateAward.h"
#include "ArticleStateBaseFacility.h"
#include "ArticleStateCraft.h"
#include "ArticleStateCraftWeapon.h"
#include "ArticleStateItem.h"
#include "ArticleStateText.h"
#include "ArticleStateTextImage.h"
#include "ArticleStateTFTD.h"
#include "ArticleStateTFTDArmor.h"
#include "ArticleStateTFTDVehicle.h"
#include "ArticleStateTFTDItem.h"
#include "ArticleStateTFTDFacility.h"
#include "ArticleStateTFTDCraft.h"
#include "ArticleStateTFTDCraftWeapon.h"
#include "ArticleStateTFTDUso.h"
#include "ArticleStateUfo.h"
#include "ArticleStateVehicle.h"
#include "UfopaediaStartState.h"

#include "../Engine/Game.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

//size_t Ufopaedia::_current_index = 0;
int Ufopaedia::_current_index = 0; // kL


/**
 * Checks if an article has already been released.
 * @param save		- pointer to SavedGame
 * @param article	- ArticleDefinition to release
 * @returns, true if the article is available
 */
bool Ufopaedia::isArticleAvailable(
		const SavedGame* const save,
		const ArticleDefinition* const article)
{
	return save->isResearched(article->requires);
}

/**
 * Gets the index of the selected article_id in the visible list.
 * If the id is not found returns -1.
 * @param save			- pointer to SavedGame
 * @param rule			- pointer to Ruleset
 * @param article_id	- reference the article id to find
 * @return, index of the given article id in the internal list, -1 if not found
 */
//size_t Ufopaedia::getArticleIndex(Game *game, std::string &article_id)
int Ufopaedia::getArticleIndex( // kL
		SavedGame* save,
		Ruleset* rule,
		std::string& article_id)
{
	const std::string UC_ID = article_id + "_UC";
	const ArticleDefinitionList articles = getAvailableArticles(save, rule);

	for (size_t
			i = 0;
			i != articles.size();
			++i)
	{
		for (std::vector<std::string>::const_iterator
				j = articles[i]->requires.begin();
				j != articles[i]->requires.end();
				++j)
		{
			if (article_id == *j)
			{
				article_id = articles[i]->id;
				return i;
			}
		}

		if (articles[i]->id == article_id)
			return i;

		if (articles[i]->id == UC_ID)
		{
			article_id = UC_ID;
			return i;
		}
	}

	return -1;
}

/**
 * Creates a new article State dependent on the given ArticleDefinition.
 * @param article - pointer to ArticleDefinition to create from
 * @return, pointer to ArticleState object if created, NULL otherwise
 */
ArticleState* Ufopaedia::createArticleState(ArticleDefinition* article)
{
	//Log(LOG_INFO) << "Ufopaedia::createArticleState() " << (int)article->getType();
	switch (article->getType())
	{
		case UFOPAEDIA_TYPE_CRAFT:
			return new ArticleStateCraft(dynamic_cast<ArticleDefinitionCraft*>(article));

		case UFOPAEDIA_TYPE_CRAFT_WEAPON:
			return new ArticleStateCraftWeapon(dynamic_cast<ArticleDefinitionCraftWeapon*>(article));

		case UFOPAEDIA_TYPE_VEHICLE:
			return new ArticleStateVehicle(dynamic_cast<ArticleDefinitionVehicle*>(article));

		case UFOPAEDIA_TYPE_ITEM:
			return new ArticleStateItem(dynamic_cast<ArticleDefinitionItem*>(article));

		case UFOPAEDIA_TYPE_ARMOR:
			return new ArticleStateArmor(dynamic_cast<ArticleDefinitionArmor*>(article));

		case UFOPAEDIA_TYPE_BASE_FACILITY:
			return new ArticleStateBaseFacility(dynamic_cast<ArticleDefinitionBaseFacility*>(article));

		case UFOPAEDIA_TYPE_TEXT:
			return new ArticleStateText(dynamic_cast<ArticleDefinitionText*>(article));

		case UFOPAEDIA_TYPE_TEXTIMAGE:
			return new ArticleStateTextImage(dynamic_cast<ArticleDefinitionTextImage*>(article));

		case UFOPAEDIA_TYPE_UFO:
			return new ArticleStateUfo(dynamic_cast<ArticleDefinitionUfo*>(article));

		case UFOPAEDIA_TYPE_TFTD:
			return new ArticleStateTFTD(dynamic_cast<ArticleDefinitionTFTD*>(article));

		case UFOPAEDIA_TYPE_TFTD_CRAFT:
			return new ArticleStateTFTDCraft(dynamic_cast<ArticleDefinitionTFTD*>(article));

		case UFOPAEDIA_TYPE_TFTD_CRAFT_WEAPON:
			return new ArticleStateTFTDCraftWeapon(dynamic_cast<ArticleDefinitionTFTD*>(article));

		case UFOPAEDIA_TYPE_TFTD_VEHICLE:
			return new ArticleStateTFTDVehicle(dynamic_cast<ArticleDefinitionTFTD*>(article));

		case UFOPAEDIA_TYPE_TFTD_ITEM:
			return new ArticleStateTFTDItem(dynamic_cast<ArticleDefinitionTFTD*>(article));

		case UFOPAEDIA_TYPE_TFTD_ARMOR:
			return new ArticleStateTFTDArmor(dynamic_cast<ArticleDefinitionTFTD*>(article));

		case UFOPAEDIA_TYPE_TFTD_BASE_FACILITY:
			return new ArticleStateTFTDFacility(dynamic_cast<ArticleDefinitionTFTD*>(article));

		case UFOPAEDIA_TYPE_TFTD_USO:
			return new ArticleStateTFTDUso(dynamic_cast<ArticleDefinitionTFTD*>(article));

		case UFOPAEDIA_TYPE_AWARD:
			return new ArticleStateAward(dynamic_cast<ArticleDefinitionAward*>(article));
	}

	return NULL;
}

/**
 * Set UPSaved index and open the new state.
 * @param game		- pointer to actual Game
 * @param article	- pointer to ArticleCefinition of the article to open
 */
void Ufopaedia::openArticle(
		Game* game,
		ArticleDefinition* article)
{
	_current_index = getArticleIndex(
								game->getSavedGame(),
								game->getRuleset(),
								article->id);
//	if (_current_index != (size_t)-1)
	if (_current_index != -1) // kL
		game->pushState(createArticleState(article));
}

/**
 * Checks if selected article_id is available -> if yes, open it.
 * @param game			- pointer to actual Game
 * @param article_id	- reference the article id to find
 */
void Ufopaedia::openArticle(
		Game* game,
		std::string& article_id)
{
	_current_index = getArticleIndex(
								game->getSavedGame(),
								game->getRuleset(),
								article_id);
//	if (_current_index != (size_t) -1)
	if (_current_index != -1) // kL
	{
		ArticleDefinition* article = game->getRuleset()->getUfopaediaArticle(article_id);
		game->pushState(createArticleState(article));
	}
}

/**
 * Open Ufopaedia start state, presenting the section selection buttons.
 * @param game		- pointer to the Game
 * @param battle	- true if opening Ufopaedia from battlescape
 */
void Ufopaedia::open(
		Game* game,
		bool battle)
{
	game->pushState(new UfopaediaStartState(battle));
}

/**
 * Open the next article in the list. Loops to the first.
 * @param game - pointer to actual Game
 */
void Ufopaedia::next(Game* game)
{
	ArticleDefinitionList articles = getAvailableArticles(
														game->getSavedGame(),
														game->getRuleset());
//	if (_current_index >= articles.size() - 1)
	if (_current_index >= static_cast<int>(articles.size()) - 1) // kL
		_current_index = 0; // goto first
	else
		++_current_index;

	game->popState();
	game->pushState(createArticleState(articles[_current_index]));
}

/**
 * Open the previous article in the list. Loops to the last.
 * @param game - pointer to actual Game
 */
void Ufopaedia::prev(Game* game)
{
	ArticleDefinitionList articles = getAvailableArticles(
														game->getSavedGame(),
														game->getRuleset());
	if (_current_index == 0)
//		_current_index = articles.size() - 1; // goto last
		_current_index = static_cast<int>(articles.size()) - 1; // kL
	else
		--_current_index;

	game->popState();
	game->pushState(createArticleState(articles[_current_index]));
}

/**
 * Fill an ArticleList with the currently visible ArticleIds of the given section.
 * @param save		- pointer to SavedGame
 * @param rule		- pointer to Ruleset
 * @param section	- reference the article section to find, e.g. "XCOM Crafts & Armaments", "Alien Lifeforms", etc.
 * @param data		- reference the article definition list object to fill data in
 */
void Ufopaedia::list(
		SavedGame* save,
		Ruleset* rule,
		const std::string& section,
		ArticleDefinitionList& data)
{
	ArticleDefinitionList articles = getAvailableArticles(save, rule);
	for (ArticleDefinitionList::const_iterator
			i = articles.begin();
			i != articles.end();
			++i)
	{
		if ((*i)->section == section)
			data.push_back(*i);
	}
}

/**
 * Return an ArticleList with all the currently visible ArticleIds.
 * @param save - pointer to SavedGame
 * @param rule - pointer to Ruleset
 * @return, ArticleDefinitionList of visible articles
 */
ArticleDefinitionList Ufopaedia::getAvailableArticles(
		SavedGame* save,
		Ruleset* rule)
{
	const std::vector<std::string>& pedList = rule->getUfopaediaList();
	ArticleDefinitionList articles;

	for (std::vector<std::string>::const_iterator
			i = pedList.begin();
			i != pedList.end();
			++i)
	{
		ArticleDefinition* const article = rule->getUfopaediaArticle(*i);
		if (isArticleAvailable(save, article) == true
			&& article->section != UFOPAEDIA_NOT_AVAILABLE)
		{
			articles.push_back(article);
		}
	}

	return articles;
}

}
