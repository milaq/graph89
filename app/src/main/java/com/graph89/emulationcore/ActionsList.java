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

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.util.AttributeSet;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;

import com.graph89.common.CalculatorTypes;
import com.graph89.common.Directories;
import com.graph89.common.EmulatorThread;
import com.graph89.common.TI84Specific;
import com.graph89.common.TI89Specific;
import com.graph89.common.TI92PSpecific;
import com.graph89.common.TI92Specific;
import com.graph89.common.V200Specific;
import com.graph89.controls.AboutScreen;
import com.graph89.controls.FilePickerActivity;
import com.graph89.controls.ListItem;
import com.graph89.controls.ListViewAdapter;
import com.graph89.controls.ScreenshotTaker;
import com.graph89.controls.WhatsNew;

public class ActionsList extends ListView
{
	public static List<ListItem>	ActionEntries			= null;

	public static final int			SHOW_KEYBOARD			= 0;
	public static final int			INSTALL_APPS			= 1;
	public static final int			TAKE_SCREENSHOT			= 2;
	public static final int			SYNCHRONIZE_CLOCK		= 3;
	public static final int			RESET					= 4;
	public static final int			BACKUP_MANAGER			= 5;
	public static final int			ROM_MANAGER				= 6;
	public static final int			CONFIGURATION_SETTINGS	= 7;
	public static final int			WHATSNEW				= 8;
	public static final int			HELP_AND_INFORMATION	= 9;
	public static final int			ABOUT					= 10;

	private Context					mContext				= null;
	private ListViewAdapter			mAdapter				= null;

	static
	{
		ActionEntries = new ArrayList<ListItem>();
		ActionEntries.add(new ListItem(SHOW_KEYBOARD, "Show Keyboard"));
		ActionEntries.add(new ListItem(INSTALL_APPS, "Install Application / Send Files"));
		ActionEntries.add(new ListItem(TAKE_SCREENSHOT, "Take Screenshot"));
		ActionEntries.add(new ListItem(SYNCHRONIZE_CLOCK, "Synchronize Clock"));
		ActionEntries.add(new ListItem(RESET, "Reset"));
		ActionEntries.add(new ListItem(BACKUP_MANAGER, "Backup Manager"));
		ActionEntries.add(new ListItem(ROM_MANAGER, "ROM Manager"));
		ActionEntries.add(new ListItem(CONFIGURATION_SETTINGS, "Configuration Settings"));
		ActionEntries.add(new ListItem(WHATSNEW, "What's New"));
		ActionEntries.add(new ListItem(HELP_AND_INFORMATION, "Help and Information"));
		ActionEntries.add(new ListItem(ABOUT, "About"));
	}

	public ActionsList(Context context)
	{
		super(context);
		Init(context);
	}

	public ActionsList(Context context, AttributeSet attrs)
	{
		super(context, attrs);
		Init(context);
	}

	private void Init(Context context)
	{
		mContext = context;

		mAdapter = new ListViewAdapter(context, android.R.layout.simple_list_item_1, android.R.id.text1, ActionEntries);

		this.setAdapter(mAdapter);

		this.setOnItemClickListener(new OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view, int position, long id)
			{
				EmulatorActivity activity = (EmulatorActivity) mContext;

				switch (position)
				{
					case SHOW_KEYBOARD:
						activity.ShowKeyboard();
						activity.HideActions();
						break;
					case INSTALL_APPS:
						ChooseUploadFiles();
						break;
					case TAKE_SCREENSHOT:
						ScreenshotTaker screenshot = new ScreenshotTaker(activity, Directories.getScreenShotDirectory(activity));
						activity.HideActions();
						screenshot.ShowDialog();
						break;
					case SYNCHRONIZE_CLOCK:
						EmulatorActivity.SyncClock = true;
						activity.HideActions();
						break;
					case RESET:
						if (EmulatorActivity.IsEmulating)
						{
							String s68k = "This will clear the entire RAM. Unarchived data will be erased. It is equivalent of removing the batteries from your calculator.\nContinue?";
							String sz80 = "This will clear the whole memory, RAM and Archive. All the data and applications will be erased. \nContinue?"; 
							
							String msg = CalculatorTypes.isTilem(EmulatorActivity.ActiveInstance.CalculatorType) ? sz80 : s68k;
							final AlertDialog alert = new AlertDialog.Builder(mContext).setTitle("Warning").setMessage(msg).setNegativeButton(android.R.string.no, null).setPositiveButton(android.R.string.ok, new Dialog.OnClickListener() {
								@Override
								public void onClick(DialogInterface d, int which)
								{
									EmulatorThread.ResetCalc = true;
								}
							}).create();

							alert.show();
							activity.HideActions();
						}
						break;
					case ROM_MANAGER:
					{
						Intent intent = new Intent(activity, RomManagerActivity.class);
						intent.putExtra("Orientation", Integer.toString(EmulatorActivity.Orientation));
						activity.startActivity(intent);
					}
						break;
					case BACKUP_MANAGER:
					{
						Intent intent = new Intent(activity, BackupManager.class);
						intent.putExtra("Orientation", Integer.toString(EmulatorActivity.Orientation));
						activity.startActivity(intent);
					}
						break;
					case CONFIGURATION_SETTINGS:
					{
						Intent intent = new Intent(activity, ConfigurationPage.class);
						activity.startActivity(intent);
					}
						break;
					case WHATSNEW:
						WhatsNew wn = new WhatsNew(activity);
						wn.Show();
						break;
					case HELP_AND_INFORMATION:
					{
						Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("http://www.graph89.com"));
						activity.startActivity(intent);
					}
						break;
					case ABOUT:
						AboutScreen a = new AboutScreen(mContext);
						a.Show();
						break;

				}
			}
		});
	}

	public void AdjustVisibility()
	{
		if (EmulatorActivity.IsEmulating)
		{
			EmulatorActivity.UIStateManagerObj.ActionsListIntstance.setBackgroundColor(0xDA000000);

			ResetVisibility(true);
		}
		else
		{
			EmulatorActivity.UIStateManagerObj.ActionsListIntstance.setBackgroundColor(0xFF000000);

			ResetVisibility(false);
			ActionEntries.get(BACKUP_MANAGER).IsActive = true;
			ActionEntries.get(ROM_MANAGER).IsActive = true;
			ActionEntries.get(WHATSNEW).IsActive = true;
			ActionEntries.get(HELP_AND_INFORMATION).IsActive = true;
			ActionEntries.get(ABOUT).IsActive = true;
		}

		mAdapter.notifyDataSetChanged();
	}

	private void ResetVisibility(boolean visibilityOn)
	{
		for (int i = 0; i < ActionEntries.size(); ++i)
		{
			ActionEntries.get(i).IsActive = visibilityOn;
		}
	}

	private void ChooseUploadFiles()
	{
		Intent myIntent = new Intent(mContext, FilePickerActivity.class);
		myIntent.putExtra(FilePickerActivity.EXTRA_FILE_PATH, Environment.getExternalStorageDirectory().getAbsolutePath());
		ArrayList<String> extensions = new ArrayList<String>();
		AddAppExtensions(extensions);
		myIntent.putExtra(FilePickerActivity.EXTRA_ACCEPTED_FILE_EXTENSIONS, extensions);
		myIntent.putExtra(FilePickerActivity.EXTRA_FILE_TYPE, "APP");
		myIntent.putExtra(FilePickerActivity.EXTRA_MULTISELECT, true);
		((EmulatorActivity) mContext).startActivityForResult(myIntent, EmulatorActivity.INSTALL_APP);
	}

	private void AddAppExtensions(ArrayList<String> extensions)
	{
		switch (EmulatorActivity.ActiveInstance.CalculatorType)
		{
			case CalculatorTypes.TI89:
			case CalculatorTypes.TI89T:
				TI89Specific.AddAppExtensions(extensions);
				break;
			case CalculatorTypes.V200:
				V200Specific.AddAppExtensions(extensions);
				TI92PSpecific.AddAppExtensions(extensions);
				TI92Specific.AddAppExtensions(extensions);
				break;
			case CalculatorTypes.TI92PLUS:
				TI92PSpecific.AddAppExtensions(extensions);
				TI92Specific.AddAppExtensions(extensions);
				break;
			case CalculatorTypes.TI92:
				TI92Specific.AddAppExtensions(extensions);
				break;
			case CalculatorTypes.TI83:
			case CalculatorTypes.TI83PLUS:
			case CalculatorTypes.TI83PLUS_SE:
			case CalculatorTypes.TI84PLUS:
			case CalculatorTypes.TI84PLUS_SE:
				TI84Specific.AddAppExtensions(extensions);
				break;
		}
	}
}
