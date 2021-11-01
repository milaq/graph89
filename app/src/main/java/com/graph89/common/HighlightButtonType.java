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

import android.graphics.Paint;

public class HighlightButtonType
{
	public static final int	SHAPETYPE_SQUARE	= 0;
	public static final int	SHAPETYPE_CIRCLE	= 1;

	public int				ShapeType;

	public int				Width;
	public int				Height;
	public String			Name;
	public Paint			Paint;

	public HighlightButtonType(String[] parts)
	{
		if (parts[0].startsWith("B"))
		{
			ShapeType = HighlightButtonType.SHAPETYPE_SQUARE;
			Name = parts[0];
			Width = Integer.parseInt(parts[1]);
			Height = Integer.parseInt(parts[2]);
			Paint = new Paint();
			Paint.setARGB(Integer.parseInt(parts[3]), Integer.parseInt(parts[4]), Integer.parseInt(parts[5]), Integer.parseInt(parts[6]));
		}
		else if (parts[0].startsWith("C"))
		{
			ShapeType = HighlightButtonType.SHAPETYPE_CIRCLE;
			Name = parts[0];
			Width = Integer.parseInt(parts[1]);
			Height = Integer.parseInt(parts[2]);
			Paint = new Paint();
			Paint.setARGB(Integer.parseInt(parts[3]), Integer.parseInt(parts[4]), Integer.parseInt(parts[5]), Integer.parseInt(parts[6]));
		}
	}

	public static boolean IsHighlightButtonType(String[] parts)
	{
		return parts.length >= 7 && (parts[0].startsWith("B") || parts[0].startsWith("C"));
	}
}
