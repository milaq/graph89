/*
 *   Graph89 - Emulator for Android
 *  
 *	 Copyright (C) 2012-2013  Dritan Hashorva
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

package com.graph89.common;

public class CalculatorTypes
{
	public static final int	UNKNOWN		= 0;

	public static final int	TI89		= 1;
	public static final int	TI89T		= 2;
	public static final int	V200		= 3;
	public static final int	TI92		= 4;
	public static final int	TI92PLUS	= 5;

	public static final int	TI84PLUS_SE	= 6;
	public static final int	TI84PLUS	= 7;
	public static final int	TI83PLUS_SE	= 8;
	public static final int	TI83PLUS	= 9;
	public static final int	TI83	    = 10;

	public static int GetType(String hwtype)
	{
		if (hwtype.equals("TI89"))
		{
			return CalculatorTypes.TI89;
		}
		else if (hwtype.equals("TI89 Titanium"))
		{
			return CalculatorTypes.TI89T;
		}
		else if (hwtype.equals("TI92"))
		{
			return CalculatorTypes.TI92;
		}
		else if (hwtype.equals("TI92 Plus"))
		{
			return CalculatorTypes.TI92PLUS;
		}
		else if (hwtype.equals("Voyage 200"))
		{
			return CalculatorTypes.V200;
		}
		else if (hwtype.equals("TI84 Plus SE"))
		{
			return CalculatorTypes.TI84PLUS_SE;
		}
		else if (hwtype.equals("TI84 Plus"))
		{
			return CalculatorTypes.TI84PLUS;
		}
		else if (hwtype.equals("TI83 Plus SE"))
		{
			return CalculatorTypes.TI83PLUS_SE;
		}
		else if (hwtype.equals("TI83 Plus"))
		{
			return CalculatorTypes.TI83PLUS;
		}
		else if (hwtype.equals("TI83"))
		{
			return CalculatorTypes.TI83;
		}
		else
		{
			return CalculatorTypes.UNKNOWN;
		}
	}

	public static boolean isTIEmu(int calculatorType)
	{
		return calculatorType == CalculatorTypes.TI89 || calculatorType == CalculatorTypes.TI89T || calculatorType == CalculatorTypes.V200 || 
				calculatorType == CalculatorTypes.TI92PLUS || calculatorType == CalculatorTypes.TI92;
	}
	
	public static boolean isTilem(int calculatorType)
	{
		return calculatorType == CalculatorTypes.TI83 || calculatorType == CalculatorTypes.TI83PLUS || calculatorType == CalculatorTypes.TI83PLUS_SE || calculatorType == CalculatorTypes.TI84PLUS || calculatorType == CalculatorTypes.TI84PLUS_SE;
	}
}
