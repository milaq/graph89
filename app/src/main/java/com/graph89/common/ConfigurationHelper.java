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

import android.content.Context;
import android.content.SharedPreferences;

public class ConfigurationHelper
{
	private static final String	ConfigurationName				= "TI_EMU_DH";
	
	public static final String	CONF_KEY_CALCULATOR_INSTANCES	= "CalculatorInstances";

	public static void WriteString(Context context, String key, String value)
	{
		SharedPreferences settings = context.getSharedPreferences(ConfigurationName, Context.MODE_PRIVATE);
		SharedPreferences.Editor editor = settings.edit();

		editor.putString(key, value);
		editor.commit();
	}

	public static void WriteInt(Context context, String key, int value)
	{
		SharedPreferences settings = context.getSharedPreferences(ConfigurationName, Context.MODE_PRIVATE);
		SharedPreferences.Editor editor = settings.edit();

		editor.putInt(key, value);
		editor.commit();
	}

	public static String GetString(Context context, String key, String defaultValue)
	{
		SharedPreferences settings = context.getSharedPreferences(ConfigurationName, Context.MODE_PRIVATE);

		return settings.getString(key, defaultValue);
	}

	public static int GetInt(Context context, String key, int defaultValue)
	{
		SharedPreferences settings = context.getSharedPreferences(ConfigurationName, Context.MODE_PRIVATE);

		return settings.getInt(key, defaultValue);
	}
}
