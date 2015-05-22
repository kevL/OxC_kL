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

#ifndef OPENXCOM_FMATH_H
#define OPENXCOM_FMATH_H

//#include <cmath>
//#include <limits>


/**
 * Returns true if two floating point values are within epsilon.
 */
template <class _Tx>
inline bool AreSame(
		const _Tx& l,
		const _Tx& r)
{
	return std::fabs(l - r) <= std::numeric_limits<_Tx>::epsilon();
}


/**
 * Rounds a floating point value up or down to its nearest whole value.
 * @note The standard library functions ceil() and floor() expect doubles.
 */
template <class _Tx>
inline _Tx Round(const _Tx& x)
{
	return x < static_cast<_Tx>(0) ? std::ceil(x - static_cast<_Tx>(0.5)) : std::floor(x + static_cast<_Tx>(0.5));
}


/**
 * Returns the square of a value.
 */
template <class _Tx>
inline _Tx Sqr(const _Tx& x)
{
	return x * x;
}

#endif
