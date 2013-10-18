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

import java.io.File;

import android.content.SharedPreferences;

import com.graph89.emulationcore.EmulatorActivity;

public class BackwardCompatibility
{
	public static void RunPatches(EmulatorActivity activity)
	{
		SharedPreferences settings = activity.getSharedPreferences("TI_EMU_DH", 0);
		String imgPath = settings.getString("IMG", null);
		
		//older to 1.1.0
		if (imgPath != null)
		{
			File f = new File(imgPath);
			if (!f.isFile()) imgPath = null;

			if (imgPath != null)
			{
				String imageFile = f.getAbsolutePath();
				String state = imageFile + ".state";

				EmulatorActivity.nativeTiEmuStep1LoadDefaultConfig();
				int err = EmulatorActivity.nativeTiEmuStep2LoadImage(imageFile);
				if (err == 0) err = EmulatorActivity.nativeTiEmuStep3Init();
				if (err == 0)
				{
					CalculatorInstance ci = new CalculatorInstance();
					ci.ImageFilePath = imageFile;
					ci.StateFilePath = state;
					ci.Title = "TI-89";
					ci.CalculatorType = CalculatorTypes.TI89;
					CalculatorInstanceHelper CalculatorInstances = new CalculatorInstanceHelper(activity);
					CalculatorInstances.Add(ci);
				}
			}

			SharedPreferences.Editor editor = settings.edit();
			editor.remove("IMG");
			editor.commit();
		}	
	}
}
