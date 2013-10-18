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

import java.util.ArrayList;

import android.view.KeyEvent;

public class TI92Specific
{
	public static void AddAppExtensions(ArrayList<String> extensions)
	{
		extensions.add(".92k"); // Apps
		extensions.add(".92z"); // Assembly Program
		extensions.add(".92f"); // Y=/Function/Equation
		extensions.add(".92p"); // Program
		extensions.add(".92l"); // List
		extensions.add(".92g"); // Group
		extensions.add(".92q"); // Certificate
		extensions.add(".92m"); // Matrix
		extensions.add(".92i"); // Picture
		extensions.add(".92c"); // Data Variable
		extensions.add(".92t"); // Text Object
		extensions.add(".92y"); // AppVars
		extensions.add(".92x"); // Geometry Macro
		extensions.add(".92a"); // Geometry Figure
		extensions.add(".92s"); // String
		extensions.add(".92e"); // Expression
		extensions.add(".92d"); // Graph Database
		extensions.add(".tig"); // tig
	}

	public static boolean ProcessKeyPress(int keyCode, KeyEvent event)
	{
		return V200Specific.ProcessKeyPress(keyCode, event);
	}
}
