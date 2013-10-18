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

import com.graph89.emulationcore.Graph89ActivityBase;

public class Directories
{
	public static String getTempDirectory(Graph89ActivityBase activity)
	{
		String tmp = null;
		String folder = Util.GetInternalAppStorage(activity);

		if (folder != null)
		{
			tmp = folder + "tmp";
			Util.CreateDirectory(tmp);
		}

		return tmp;
	}
	
	public static String GetInternalAppStorage(Graph89ActivityBase context)
	{
		return Util.GetInternalAppStorage(context);
	}

	public static String getScreenShotDirectory(Graph89ActivityBase activity)
	{
		return Util.GetMediaRootFolder(activity) + "graph89/screenshots/";
	}

	public static String getLicenceFile(Graph89ActivityBase activity)
	{
		return Util.GetMediaRootFolder(activity) + "graph89/licence.lic";
	}

	public static String getReceivedDirectory(Graph89ActivityBase activity)
	{
		return Util.GetMediaRootFolder(activity) + "graph89/received/";
	}

	public static String getBackupDirectory(Graph89ActivityBase activity)
	{
		return Util.GetMediaRootFolder(activity) + "graph89/backup/";
	}
}
