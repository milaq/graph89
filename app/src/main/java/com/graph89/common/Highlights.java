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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;

import com.graph89.emulationcore.EmulatorActivity;

public class Highlights
{
	public List<HighlightInfo>			HighlightInfos			= null;
	public List<HighlightButtonType>	HighlightButtonTypes	= null;

	public Highlights(Context context, String assetPath)
	{
		HighlightInfos = new ArrayList<HighlightInfo>();
		HighlightButtonTypes = new ArrayList<HighlightButtonType>();

		BufferedReader br = null;

		try
		{
			br = new BufferedReader(new InputStreamReader(context.getAssets().open(assetPath)));

			String line;
			while ((line = br.readLine()) != null)
			{
				String[] parts = line.split("\\s+");

				if (HighlightButtonType.IsHighlightButtonType(parts))
				{
					HighlightButtonTypes.add(new HighlightButtonType(parts));
				}
				else if (HighlightInfo.IsHighlightInfo(parts))
				{
					HighlightInfos.add(new HighlightInfo(parts, this));
				}
			}
		}
		catch (IOException e)
		{
			Util.ShowAlert((EmulatorActivity) context, "Highlights constructor", e);
		}
		finally
		{
			if (br != null)
			{
				try
				{
					br.close();
				}
				catch (IOException e)
				{
					Util.ShowAlert((EmulatorActivity) context, "Highlights constructor - 2", e);
				}
			}
		}
	}

	public HighlightButtonType FindButtonTypeByName(String name)
	{
		for (int i = 0; i < HighlightButtonTypes.size(); ++i)
		{
			if (HighlightButtonTypes.get(i).Name.equals(name))
			{
				return HighlightButtonTypes.get(i);
			}
		}

		return null;
	}

	public List<HighlightInfo> FindHighlightInfoByKeyCode(int code)
	{
		List<HighlightInfo> ret = new ArrayList<HighlightInfo>();
		for (int i = 0; i < HighlightInfos.size(); ++i)
		{
			HighlightInfo button = HighlightInfos.get(i);
			if (button.Code == code) ret.add(button);
		}

		return ret;
	}
}
