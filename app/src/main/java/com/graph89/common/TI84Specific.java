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

import android.view.KeyCharacterMap;
import android.view.KeyEvent;

import com.graph89.emulationcore.EmulatorActivity;

public class TI84Specific
{
	private static KeyCharacterMap	sKeyCharacterMap	= null;
	private static int				sKeyboardDeviceID	= -1;

	public static void AddAppExtensions(ArrayList<String> extensions)
	{
		extensions.add(".8xu"); //OS upgrade
		extensions.add(".8xk"); //Application
		extensions.add(".8xp"); //Program
		extensions.add(".8xn"); //Real
		extensions.add(".8xl"); //List
		extensions.add(".8xm"); //Matrix
		extensions.add(".8xe"); //Equation
		extensions.add(".8xs"); //String
		extensions.add(".8xi"); //Picture
		extensions.add(".8xw"); //Window
		extensions.add(".8xc"); //Complex
		extensions.add(".8xz"); //Zoom
		extensions.add(".8xt"); //Table
		extensions.add(".8xb"); //Backup
		extensions.add(".8xv"); //Var
		extensions.add(".8xo"); //group
		extensions.add(".8xg"); //group
		
		extensions.add(".83l");  //list
		extensions.add(".83m");  //matrix
		extensions.add(".83p");  //program
		extensions.add(".83y");  // y-var
		extensions.add(".83s");  // String
		extensions.add(".83i");  // picture
		extensions.add(".83c");  // complex
		extensions.add(".83w");  // window
		extensions.add(".83z");  // zoom
		extensions.add(".83t");  // table
		extensions.add(".83b");  // backup
	}

	public static boolean ProcessKeyPress(int keyCode, KeyEvent event)
	{
		int deviceID = event.getDeviceId();

		if (sKeyCharacterMap == null || sKeyboardDeviceID != deviceID)
		{
			sKeyCharacterMap = KeyCharacterMap.load(deviceID);
			sKeyboardDeviceID = deviceID;
		}

		char c = (char) sKeyCharacterMap.get(event.getKeyCode(), event.getMetaState());

		int alphakey = 48;
		int secondkey = 54;
		boolean isAlpha = Character.isLetter(c);

		if (isAlpha)
		{
			char letter = Character.toLowerCase(c);

			switch (letter)
			{
				case 'a':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 47 });
					return true;
				case 'b':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 39 });
					return true;
				case 'c':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 31 });
					return true;
				case 'd':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 46 });
					return true;
				case 'e':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 38 });
					return true;
				case 'f':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 30 });
					return true;
				case 'g':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 22 });
					return true;
				case 'h':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 14 });
					return true;
				case 'i':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 45 });
					return true;
				case 'j':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 37 });
					return true;
				case 'k':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 29 });
					return true;
				case 'l':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 21 });
					return true;
				case 'm':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 13 });
					return true;
				case 'n':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 44 });
					return true;
				case 'o':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 36 });
					return true;
				case 'p':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 28 });
					return true;
				case 'q':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 20 });
					return true;
				case 'r':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 12 });
					return true;
				case 's':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 43 });
					return true;
				case 't':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 35 });
					return true;
				case 'u':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 27 });
					return true;
				case 'v':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 19 });
					return true;
				case 'w':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 11 });
					return true;
				case 'x':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 42 });
					return true;
				case 'y':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 34 });
					return true;
				case 'z':
					EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 26 });
					return true;
			}
		}

		switch (c)
		{
			case '1':
				EmulatorActivity.SendKeysToCalc(new int[] { 34 });
				return true;
			case '2':
				EmulatorActivity.SendKeysToCalc(new int[] { 26 });
				return true;
			case '3':
				EmulatorActivity.SendKeysToCalc(new int[] { 18 });
				return true;
			case '4':
				EmulatorActivity.SendKeysToCalc(new int[] { 35 });
				return true;
			case '5':
				EmulatorActivity.SendKeysToCalc(new int[] { 27 });
				return true;
			case '6':
				EmulatorActivity.SendKeysToCalc(new int[] { 19 });
				return true;
			case '7':
				EmulatorActivity.SendKeysToCalc(new int[] { 36 });
				return true;
			case '8':
				EmulatorActivity.SendKeysToCalc(new int[] { 28 });
				return true;
			case '9':
				EmulatorActivity.SendKeysToCalc(new int[] { 20 });
				return true;
			case '0':
				EmulatorActivity.SendKeysToCalc(new int[] { 33 });
				return true;
			case '*':
				EmulatorActivity.SendKeysToCalc(new int[] { 12 });
				return true;
			case '-':
				EmulatorActivity.SendKeysToCalc(new int[] { 11 });
				return true;
			case '+':
				EmulatorActivity.SendKeysToCalc(new int[] { 10 });
				return true;
			case '/':
				EmulatorActivity.SendKeysToCalc(new int[] { 13 });
				return true;
			case '_':
				EmulatorActivity.SendKeysToCalc(new int[] { 17 });
				return true;
			case '?':
				EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 17 });
				return true;
			case '@':
				EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 18 });
				return true;
			case '(':
				EmulatorActivity.SendKeysToCalc(new int[] { 29 });
				return true;
			case ')':
				EmulatorActivity.SendKeysToCalc(new int[] { 21 });
				return true;
			case '{':
				EmulatorActivity.SendKeysToCalc(new int[] { secondkey, 29 });
				return true;
			case '}':
				EmulatorActivity.SendKeysToCalc(new int[] { secondkey, 21 });
				return true;
			case ' ':
				EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 33 });
				return true;
			case ':':
				EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 25 });
				return true;
			case '"':
				EmulatorActivity.SendKeysToCalc(new int[] { alphakey, 10 });
				return true;
			case '.':
				EmulatorActivity.SendKeysToCalc(new int[] { 25 });
				return true;
			case '^':
				EmulatorActivity.SendKeysToCalc(new int[] { 14 });
				return true;
		}

		switch (keyCode)
		{
			case KeyEvent.KEYCODE_ENTER:
				EmulatorActivity.SendKeysToCalc(new int[] { 9 });
				return true;
			case KeyEvent.KEYCODE_DEL:
				EmulatorActivity.SendKeysToCalc(new int[] { 56 });
				return true;
			case KeyEvent.KEYCODE_DPAD_DOWN:
				EmulatorActivity.SendKeysToCalc(new int[] { 1 });
				return true;
			case KeyEvent.KEYCODE_DPAD_RIGHT:
				EmulatorActivity.SendKeysToCalc(new int[] { 3 });
				return true;
			case KeyEvent.KEYCODE_DPAD_UP:
				EmulatorActivity.SendKeysToCalc(new int[] { 4 });
				return true;
			case KeyEvent.KEYCODE_DPAD_LEFT:
				EmulatorActivity.SendKeysToCalc(new int[] { 2 });
				return true;
			case KeyEvent.KEYCODE_CLEAR:
				EmulatorActivity.SendKeysToCalc(new int[] { 15 });
				return true;
		}

		return false;
	}
}
