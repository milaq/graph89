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

public class CalculatorConfiguration
{
	public static final String	HapticFeedbackKey		= "CONF_HAPTIC_FEEDBACK";
	public static final String	AudioFeedBackKey		= "CONF_AUDIO_FEEDBACK";
	public static final String	ZoomModeKey				= "CONF_ZOOM_MODE";
	public static final String	ScreenScaleKey			= "CONF_SCREENSCALE";
	public static final String	AutoOFFKey				= "CONF_AUTO_OFF";
	public static final String	SkinKey					= "CONF_SKIN";
	public static final String	SkinKeyV200				= "CONF_SKIN_V200";
	public static final String	SkinKeyTI84				= "CONF_SKIN_TI84";
	public static final String	CPUSpeedKey				= "CONF_CPU_SPEED";
	public static final String	EnergySaveKey			= "CONF_ENERGY_SAVE";
	public static final String	OverclockWhenBusyKey	= "CONF_OVERCLOCK";
	public static final String	SaveStateOnExitKey		= "CONF_SAVE_STATE";
	public static final String	EnableGrayScaleKey		= "CONF_ENABLE_GRAYSCALE";
	public static final String	PixelOffKey				= "CONF_PIXEL_OFF";
	public static final String	PixelOnKey				= "CONF_PIXEL_ON";
	public static final String	LCDColorKey				= "CONF_LCD_COLOR";
	public static final String	GridColorKey			= "CONF_GRID_COLOR";
	public static final String	TurnOffOnScreenOffKey	= "CONF_OFF_ON_SCREENOFF";
	public static final String	OrientationKey			= "CONF_ORIENTATION";
	public static final String	LCDTypeKey				= "CONF_LCD_TYPE";

	public int					HapticFeedback			= 10;
	public boolean				AudioFeedBack			= false;
	public boolean				ZoomMode				= true;
	public int					ScreenScale				= -1;
	public int					AutoOFF					= 5;
	public int					Skin					= SkinDefinition.BUILD_IN_UNKNOWN;
	public boolean				EnableGrayScale			= false;
	public boolean				SaveStateOnExit			= true;
	public int					CPUSpeed				= 100;
	public boolean				EnergySave				= true;
	public boolean				OverclockWhenBusy		= true;
	public boolean				TurnOffOnScreenOff		= true;
	public String				Orientation				= "Portrait";
	public boolean				UseLCDGrid				= false;

	public int					LCDColor				= 0xFFA5BAA0;
	public int					PixelOn					= 0xFF000000;
	public int					PixelOff				= 0xFFB6C5B7;
	public int					GridColor				= 0xFFA5BAA0;
}
