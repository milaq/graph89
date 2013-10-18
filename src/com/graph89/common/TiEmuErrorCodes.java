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

public class TiEmuErrorCodes
{
	public static String GetErrorCode(int code)
	{
		switch (code)
		{
			case 0:
				return "ERR_NONE";
			case 768:
				return "ERR_CANT_OPEN";
			case 770:
				return "ERR_INVALID_IMAGE";
			case 771:
				return "ERR_INVALID_UPGRADE";
			case 772:
				return "ERR_NO_IMAGE";
			case 774:
				return "ERR_INVALID_ROM_SIZE";
			case 775:
				return "ERR_NOT_TI_FILE";
			case 776:
				return "ERR_MALLOC";
			case 777:
				return "ERR_CANT_OPEN_DIR";
			case 778:
				return "ERR_CANT_UPGRADE";
			case 779:
				return "ERR_INVALID_ROM";
			case 800:
				return "Not .89u or .rom";

			default:
				return Integer.toString(code) + " - Unknown...";
		}
	}
}
