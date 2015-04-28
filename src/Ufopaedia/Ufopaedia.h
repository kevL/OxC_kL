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

#ifndef OPENXCOM_UFOPAEDIA_H
#define OPENXCOM_UFOPAEDIA_H

//#include <string>
//#include <vector>


namespace OpenXcom
{


class ArticleDefinition;
class ArticleState;
class Game;
class Ruleset;
class SavedGame;


/// definition of an article list
typedef std::vector<ArticleDefinition*> ArticleDefinitionList;


/// define Ufopaedia sections, which must be consistent
static const std::string
	UFOPAEDIA_XCOM_CRAFT_ARMAMENT		= "STR_XCOM_CRAFT_ARMAMENT",
	UFOPAEDIA_HEAVY_WEAPONS_PLATFORMS	= "STR_HEAVY_WEAPONS_PLATFORMS",
	UFOPAEDIA_WEAPONS_AND_EQUIPMENT		= "STR_WEAPONS_AND_EQUIPMENT",
	UFOPAEDIA_ALIEN_ARTIFACTS			= "STR_ALIEN_ARTIFACTS",
	UFOPAEDIA_BASE_FACILITIES			= "STR_BASE_FACILITIES",
	UFOPAEDIA_ALIEN_LIFE_FORMS			= "STR_ALIEN_LIFE_FORMS",
	UFOPAEDIA_ALIEN_RESEARCH			= "STR_ALIEN_RESEARCH_UC",
	UFOPAEDIA_UFO_COMPONENTS			= "STR_UFO_COMPONENTS",
	UFOPAEDIA_UFOS						= "STR_UFOS",
	UFOPAEDIA_AWARDS					= "STR_AWARDS",
	UFOPAEDIA_NOT_AVAILABLE				= "STR_NOT_AVAILABLE";

// This last section is meant for articles that have to be activated
// but have no own entry in a list. E.g. Ammunition items.
// Maybe others as well that should just not be selectable.


/**
 * This static class encapsulates all functions related to Ufopaedia
 * for the game. It manages the relationship between the UfopaediaSaved
 * instance in SavedGame and the UfopaediaFactory in Ruleset.
 * Main purpose is to open Ufopaedia from Geoscape, navigate between articles
 * and release new articles after successful research.
 */
class Ufopaedia
{

protected:
	/// current selected article index (for prev/next navigation).
//	static size_t _current_index;
	static int _current_index; // kL

	/// get index of the given article id in the visible list.
//	static size_t getArticleIndex(Game* game, std::string& article_id);
	static int getArticleIndex( // kL
			SavedGame* save,
			Ruleset* rule,
			std::string& article_id);
	/// get list of researched articles
	static ArticleDefinitionList getAvailableArticles(
			SavedGame* save,
			Ruleset* rule);
	/// create a new state object from article definition.
	static ArticleState* createArticleState(ArticleDefinition* article);


	public:
		/// check if a specific article is currently available.
		static bool isArticleAvailable(
				const SavedGame* const save,
				const ArticleDefinition* const article);
		/// open Ufopaedia on a certain entry.
		static void openArticle(
				Game* game,
				std::string& article_id);
		/// open Ufopaedia article from a given article definition.
		static void openArticle(
				Game* game,
				ArticleDefinition* article);

		/// open Ufopaedia with selection dialog.
		static void open(
				Game* game,
				bool battle = false);

		/// article navigation to next article.
		static void next(Game* game);
		/// article navigation to previous article.
		static void prev(Game* game);

		/// load a vector with article ids that are currently visible of a given section.
		static void list(
				SavedGame* save,
				Ruleset* rule,
				const std::string& section,
				ArticleDefinitionList& data);
};

}

#endif
