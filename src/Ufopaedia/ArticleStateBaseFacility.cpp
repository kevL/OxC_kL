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

#include "ArticleStateBaseFacility.h"

//#include <sstream>

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Palette.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionBaseFacility (ArticleDefinition.h)
 */
ArticleStateBaseFacility::ArticleStateBaseFacility(const ArticleDefinitionBaseFacility* const defs)
	:
		ArticleState(defs->id)
{
	RuleBaseFacility* facRule = _game->getRuleset()->getBaseFacility(defs->id);

	_txtTitle = new Text(200, 17, 10, 24);

	setPalette("PAL_BASESCAPE");

	ArticleState::initLayout();

	add(_txtTitle);

	_game->getResourcePack()->getSurface("BACK09.SCR")->blit(_bg);

	_btnOk->setColor(BASESCAPE_VIOLET);
	_btnPrev->setColor(BASESCAPE_VIOLET);
	_btnNext->setColor(BASESCAPE_VIOLET);

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(BASESCAPE_BLUE);
	_txtTitle->setBig();


	const int tile_size = 32;
	_image = new Surface( // build preview image
						tile_size * 2,
						tile_size * 2,
						232,16);
	add(_image);

	SurfaceSet* const baseBits = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
	Surface* srfFac;
	int
		x_offset,
		y_offset,
		x_pos,
		y_pos,
		i = 0;
	const size_t facSize = facRule->getSize();

	if (facSize == 1)
		x_offset =
		y_offset = tile_size / 2;
	else
		x_offset =
		y_offset = 0;

	y_pos = y_offset;
	for (size_t
			y = 0;
			y != facSize;
			++y)
	{
		x_pos = x_offset;
		for (size_t
				x = 0;
				x != facSize;
				++x)
		{
			srfFac = baseBits->getFrame(facRule->getSpriteShape() + i);
			srfFac->setX(x_pos);
			srfFac->setY(y_pos);
			srfFac->blit(_image);

			if (facSize == 1)
			{
				srfFac = baseBits->getFrame(facRule->getSpriteFacility() + i);
				srfFac->setX(x_pos);
				srfFac->setY(y_pos);
				srfFac->blit(_image);
			}

			x_pos += tile_size;

			++i;
		}

		y_pos += tile_size;
	}

	_txtInfo = new Text(300, 90, 10, 104);
	add(_txtInfo);

	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(BASESCAPE_BLUE);
	_txtInfo->setWordWrap();

	_lstInfo = new TextList(200, 41, 10, 42);
	add(_lstInfo);

	_lstInfo->setColumns(2, 140, 60);
	_lstInfo->setColor(BASESCAPE_BLUE);
	_lstInfo->setDot();

	_lstInfo->addRow(
				2,
				tr("STR_CONSTRUCTION_TIME").c_str(),
				tr("STR_DAY", facRule->getBuildTime()).c_str());
	_lstInfo->setCellColor(0,1, BASESCAPE_WHITE);

	std::wostringstream woststr;
	woststr << Text::formatFunding(facRule->getBuildCost());
	_lstInfo->addRow(
				2,
				tr("STR_CONSTRUCTION_COST").c_str(),
				woststr.str().c_str());
	_lstInfo->setCellColor(1,1, BASESCAPE_WHITE);

	woststr.str(L"");
	woststr.clear();
	woststr << Text::formatFunding(facRule->getMonthlyCost());
	_lstInfo->addRow(
				2,
				tr("STR_MAINTENANCE_COST").c_str(),
				woststr.str().c_str());
	_lstInfo->setCellColor(2,1, BASESCAPE_WHITE);

	if (facRule->getDefenseValue() != 0)
	{
		woststr.str(L"");
		woststr.clear();
		woststr << facRule->getDefenseValue();
		_lstInfo->addRow(
					2,
					tr("STR_DEFENSE_VALUE").c_str(),
					woststr.str().c_str());
		_lstInfo->setCellColor(3,1, BASESCAPE_WHITE);

		woststr.str(L"");
		woststr.clear();
		woststr << Text::formatPercentage(facRule->getHitRatio());
		_lstInfo->addRow(
					2,
					tr("STR_HIT_RATIO").c_str(),
					woststr.str().c_str());
		_lstInfo->setCellColor(4,1, BASESCAPE_WHITE);
	}

	centerAllSurfaces();
}

/**
 * dTor.
 */
ArticleStateBaseFacility::~ArticleStateBaseFacility() // virtual.
{}

}
