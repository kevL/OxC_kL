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

#ifndef OPENXCOM_ARTICLESTATETEXTIMAGE_H
#define OPENXCOM_ARTICLESTATETEXTIMAGE_H

#include "ArticleState.h"


namespace OpenXcom
{

class ArticleDefinitionTextImage;
class Text;
class TextButton;


/**
 * ArticleStateTextImage has a title, text block and a background image.
 */
class ArticleStateTextImage
	:
		public ArticleState
{

protected:
	const ArticleDefinitionTextImage* _defs;

	Text
		* _txtInfo,
		* _txtTitle;
	TextButton* _btnExtraInfo;

	/// Shows ExtraInfo on aliens researched w/ autopsy.
	void btnInfo(Action* action);
	/// Checks if necessary research has been done before showing the ExtraInfo button.
	bool showInfoBtn();


	public:
		/// cTor.
		explicit ArticleStateTextImage(const ArticleDefinitionTextImage* const defs);
		/// dTor.
		virtual ~ArticleStateTextImage();
};

}

#endif
