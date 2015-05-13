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

#ifndef OPENXCOM_ARTICLESTATEVEHICLE_H
#define OPENXCOM_ARTICLESTATEVEHICLE_H

#include "ArticleState.h"


namespace OpenXcom
{

class ArticleDefinitionVehicle;
class Text;
class TextList;


/**
 * ArticleStateVehicle has a caption, text, and a stats block.
 */
class ArticleStateVehicle
	:
		public ArticleState
{

protected:
	Text
		* _txtInfo,
		* _txtTitle;
	TextList* _lstStats;


	public:
		/// cTor.
		explicit ArticleStateVehicle(ArticleDefinitionVehicle* article_defs);
		/// dTor.
		virtual ~ArticleStateVehicle();
};

}

#endif
