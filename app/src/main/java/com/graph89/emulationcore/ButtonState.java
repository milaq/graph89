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

package com.graph89.emulationcore;

import java.util.ArrayList;
import java.util.List;

import com.graph89.common.KeyPress;

public class ButtonState
{
	public static int				ActivePointerID	= -1;
	private static List<KeyPress>	sPressedButtons	= new ArrayList<KeyPress>();

	public static void Reset()
	{
		sPressedButtons.clear();
		ActivePointerID = -1;
	}

	public static void ButtonPress(KeyPress key)
	{
		if (IsKeyCodeInvalid(key.KeyCode)) return;
		boolean found = false;
		for (int i = 0; i < sPressedButtons.size(); ++i)
		{
			if (sPressedButtons.get(i).KeyCode == key.KeyCode || sPressedButtons.get(i).TouchID == key.TouchID)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			sPressedButtons.add(key);
			EmulatorActivity.SendKeyToCalc(key.KeyCode, 1, true);
		}

		RefreshButtonHighlightView();
	}

	public static void ButtonUnpress(int touchID)
	{
		for (int i = 0; i < sPressedButtons.size(); ++i)
		{
			KeyPress button = sPressedButtons.get(i);

			if (button.TouchID == touchID)
			{
				EmulatorActivity.SendKeyToCalc(button.KeyCode, 0, false);
				RefreshButtonHighlightView();
				sPressedButtons.remove(i);
				return;
			}
		}
	}

	public static void UnpressAll()
	{
		for (int i = 0; i < sPressedButtons.size(); ++i)
		{
			KeyPress button = sPressedButtons.get(i);
			EmulatorActivity.SendKeyToCalc(button.KeyCode, 0, false);
		}
		sPressedButtons.clear();
		RefreshButtonHighlightView();
	}

	public static KeyPress[] GetPressedKeys()
	{
		return (KeyPress[]) sPressedButtons.toArray(new KeyPress[sPressedButtons.size()]);
	}

	private static void RefreshButtonHighlightView()
	{
		if (EmulatorActivity.UIStateManagerObj != null && EmulatorActivity.UIStateManagerObj.ButtonHighlightViewInstance != null)
		{
			EmulatorActivity.UIStateManagerObj.ButtonHighlightViewInstance.invalidate();
		}
	}

	public static boolean IsKeyCodeInvalid(int keyCode)
	{
		return keyCode < 0 || keyCode >= 255;
	}
}
