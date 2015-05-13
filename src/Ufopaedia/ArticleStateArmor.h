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

#ifndef OPENXCOM_ARTICLESTATEARMOR_H
#define OPENXCOM_ARTICLESTATEARMOR_H

#include "ArticleState.h"


namespace OpenXcom
{

class ArticleDefinitionArmor;
class Surface;
class Text;
class TextList;


/**
 * ArticleStateArmor has a caption, preview image and a stats block.
 * The image is found using the Armor class.
 */
class ArticleStateArmor
	:
		public ArticleState
{

protected:
	int _row;

	Surface* _image;
	Text
		* _txtInfo,
		* _txtTitle;
	TextList* _lstInfo;

	///
	void addStat(
			const std::string& label,
			int stat,
			bool addPlus = false);
	///
	void addStat(
			const std::string& label,
			const std::wstring& stat);


	public:
		/// cTor.
		explicit ArticleStateArmor(ArticleDefinitionArmor* article_defs);
		/// dTor.
		virtual ~ArticleStateArmor();
};

}

#endif
