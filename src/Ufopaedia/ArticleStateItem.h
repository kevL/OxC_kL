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

#ifndef OPENXCOM_ARTICLESTATEITEM_H
#define OPENXCOM_ARTICLESTATEITEM_H

#include "ArticleState.h"


namespace OpenXcom
{

class ArticleDefinitionItem;
class Surface;
class Text;
class TextList;


/**
 * ArticleStateItem has a caption, text, preview image and a stats block.
 * The facility image is found using the RuleBasefacility class.
 */
class ArticleStateItem
	:
		public ArticleState
{

protected:
	Surface
		*_image,
		*_imageAmmo[3];
	Text
		* _txtTitle,
		* _txtInfo,
		* _txtShotType,
		* _txtAccuracy,
		* _txtTuCost,
		* _txtDamage,
		* _txtAmmo,
		* _txtAmmoType[3],
		* _txtAmmoDamage[3];
	TextList* _lstInfo;


	public:
		/// cTor.
		ArticleStateItem(ArticleDefinitionItem* article_defs);
		/// dTor.
		virtual ~ArticleStateItem();
};

}

#endif
