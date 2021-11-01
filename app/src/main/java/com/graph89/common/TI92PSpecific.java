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

public class TI92PSpecific
{
	public static void AddAppExtensions(ArrayList<String> extensions)
	{
		extensions.add(".9xk"); // Apps
		extensions.add(".9xz"); // Assembly Program
		extensions.add(".9xf"); // Y=/Function/Equation
		extensions.add(".9xp"); // Program
		extensions.add(".9xl"); // List
		extensions.add(".9xg"); // Group
		extensions.add(".9xq"); // Certificate
		extensions.add(".9xm"); // Matrix
		extensions.add(".9xi"); // Picture
		extensions.add(".9xc"); // Data Variable
		extensions.add(".9xt"); // Text Object
		extensions.add(".9xy"); // AppVars
		extensions.add(".9xx"); // Geometry Macro
		extensions.add(".9xa"); // Geometry Figure
		extensions.add(".9xs"); // String
		extensions.add(".9xe"); // Expression
		extensions.add(".9xd"); // Graph Database
		extensions.add(".tig"); // tig
	}

	public static boolean ProcessKeyPress(int keyCode, KeyEvent event)
	{
		return V200Specific.ProcessKeyPress(keyCode, event);
	}
}
