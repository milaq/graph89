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

package com.graph89.controls;

import android.app.AlertDialog;
import android.content.Context;
import android.text.method.ScrollingMovementMethod;
import android.text.util.Linkify;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.Bisha.TI89EmuDonation.R;
import com.graph89.emulationcore.EmulatorActivity;

public class WhatsNew
{
	private Context		mContext;
	private TextView	tv	= null;

	public WhatsNew(Context context)
	{
		mContext = context;
	}

	public void Show()
	{
		final View view = LayoutInflater.from(mContext).inflate(R.layout.whats_new, (ViewGroup) ((EmulatorActivity) mContext).findViewById(R.id.whats_new_layout));

		tv = (TextView) view.findViewById(R.id.whats_new_text);
		tv.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		final AlertDialog addEditdialog = new AlertDialog.Builder(mContext).setView(view).setTitle("What's New").setPositiveButton(android.R.string.ok, null).create();
		AddText();
		tv.setMovementMethod(new ScrollingMovementMethod());
		addEditdialog.show();
	}

	private void AddText()
	{
		// @formatter:off
		String text =  
				"****Graph 89 - v1.1.3c - October 2013****\n\n" +
				"Open the source code to https://bitbucket.org/dhashoandroid/graph89-paid.\n\n"
				+ "Update license to GPLv3\n\n" + 
						
		"****Graph 89 - v1.1.3b - September 2013****\n\n" +
		"An 8Xu (firmware update) file can now be used as a ROM for TI84+, TI84+SE, TI83+ and TI83+SE.\n\n" +
		"Group files (8xg) are now supported.\n\n" +
		"Some users had issues with installing apps on TI83/TI84 series. These issues are partially resolved.\n\n" + 
		"Some users were experiencing Graph89 shutdowns while running apps on TI83/TI84, especially with 'Overclock when Busy' enabled. This should now be fixed.\n\n" + 
				
		"****Graph 89 - v1.1.3 - July 2013****\n\n" + 
		"Added support for: TI-83, TI-83 Plus, TI-83 Plus SE, TI-84 Plus and TI-84 Plus SE using the TilEm 2.00 engine.\n\n" +
		"Only '*.rom' files are supported for the calculators above. Update files such as '*.8Xu' cannot be used as ROM files due to them being incomplete. However, you can use the '*.8Xu' files to upgrade the OS in Graph89 by installing it as a App once the initial '*.rom' file is loaded.\n\n" +
					
		"****Graph 89 - v1.1.2b - Feb 2013****\n\n" +
		"Bug fixes:\n" +
		"Issue with 'State Save' on devices with low memory\n\n"+
		"Possible 'Out of Memory' errors while switching instances\n\n"+
			
		"****Graph 89 - v1.1.2 - Feb 2013****\n\n" +
		"Backup Manager\n\n" + 
		"Dot Matrix screen effect\n\n" + 
		"Click screen to Zoom for landscape modes\n\n" + 
		"Reset RAM\n\n" + 
		"Bug fixes:\n" + 
		"Increased stability for older phones, i.e Motorola RAZR\n\n" + 
		"V200 and TI92 roms mistakenly recognized as TI89\n\n" + 
				
		"****Graph 89 - v1.1.1 - Oct 2012****\n\n" +
		"TI92Plus Skin\n\n" +
		"Landscape mode for TI89 and TI89 Titanium\n\n" +
		"New Orientation setting for TI89 and TI89 Titanium: Auto, Portrait (Default), Landscape.\n\n" +
		"Bug fixes:\n" +
		"Fixed Blurry screen in Portrait mode. Landscape mode can be slightly blurry\n\n" +
		"Voyage 200 should load *.9x? files\n\n" +
		"Fixed some crash issues\n\n" +
		"****Graph 89 - v1.1.0 - Oct 2012****\n\n" +
		"Added support for Voyage 200, TI-92 and TI-92 Plus\n\n" +
		"Multiple ROMs can be installed.\n\n" +
		"Easily switch from one ROM to the other with a dropdown in the upper right corner\n\n" +
		"Back Button now replaces the Menu Button. Pressing it will display the new Action List\n\n" + 
		"Send group files (*.tig, *.89g)\n\n" +
		"Send *.89l, and *.tib files\n\n"+
		"Receive files (var-link/F4/F3/send)\n\n" +
		"Multitouch\n\n" +
		"Take Screenshots\n\n" +
		"Generate a Calculator ID under F1/About. Permission to read accounts is needed to accomplish this.\n\n" +
		"Synchronize the Clock with the Android device\n\n"+
		"Audio feedback on keypress. Touch sounds should be enabled on your Android device (Settings/Sound)\n\n" +
		"Smoother graphics\n\n" +
		"Automatic overclock when the calculator is 'Busy'\n\n" +
		"Grayscale support\n\n" +
		"Ability to send multiple App files at once\n\n" +
		"Customize LCD colors\n\n" +
		"Further performance improvements\n\n" +
		"Keyboard bug fixes\n\n" +
		"Optimized for Android 4.0 (ICS) or later. Older versions are supported as well.\n\n" +
		"Due to major changes, all the previous configuration settings might need to be reentered \n\n\n" +
		
		"****Graph 89 - v1.0.1 - Aug 2012****\n\n" +
		"Fixed a crash issue with Samsung S3 or other fast devices\n\n" +
		"Configure 'Screen Zoom' for TI89 and TI89T\n\n\n" +
		
		"****Graph 89 - v1.0.0 - May 2012****\n\n" +
		"Added classic skins for TI89 and TI89T\n\n" +
		"Energy Save (slow down if no activity)\n\n" +
		"Configurable Auto-OFF\n\n" +
		"Upload *.89p files\n\n" +
		"Keyboard support";
		// @formatter:on

		tv.setText(text);
		Linkify.addLinks(tv, Linkify.WEB_URLS);
	}
}
