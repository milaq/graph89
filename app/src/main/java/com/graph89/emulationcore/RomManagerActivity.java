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

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TableRow;
import android.widget.TextView;

import com.Bisha.TI89EmuDonation.R;
import com.graph89.common.CalculatorInstance;
import com.graph89.common.CalculatorInstanceHelper;
import com.graph89.common.CalculatorTypes;
import com.graph89.common.Directories;
import com.graph89.common.TiEmuErrorCodes;
import com.graph89.common.Util;
import com.graph89.controls.ControlBar;
import com.graph89.controls.FilePickerActivity;

public class RomManagerActivity extends Graph89ActivityBase
{
	private final int					ROM_BROWSE					= 1;

	public static final int				HANDLER_UPDATE_BROWSEBUTTON	= MAX_HANDLER_ID + 1;
	public static final int				HANDLER_REFRESHUI			= MAX_HANDLER_ID + 2;
	// ////////////////////////////////////////////////////////////////////////

	private ControlBar					mControlBar					= null;
	private ListView					mRomList					= null;
	private TextView					mNoRomsInstalledTextView	= null;
	private Button						mAddRomButton				= null;
	private String						mBrowseText					= null;

	private CalculatorInstanceHelper	mCalculatorInstances		= null;
	private AlertDialog					mAddEditdialog				= null;
	private RomManagerActivity			mThis						= null;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.rom_manager_main);
		mThis = this;

		this.setRequestedOrientation(EmulatorActivity.Orientation);

		mCalculatorInstances = new CalculatorInstanceHelper(this);

		mControlBar = new ControlBar(this);
		mControlBar.HideCalculatorTypeSpinner();

		mRomList = (ListView) this.findViewById(R.id.rom_manager_main_rom_list);
		mRomList.setClickable(true);

		mRomList.setOnItemClickListener(new AdapterView.OnItemClickListener() {
			public void onItemClick(AdapterView<?> parent, View v, int position, long id)
			{
				ShowAddRom(position);
			}
		});

		mNoRomsInstalledTextView = (TextView) this.findViewById(R.id.rom_manager_main_norommessage);

		mAddRomButton = (Button) this.findViewById(R.id.rom_manager_main_add_rom);

		mAddRomButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v)
			{
				ShowAddRom(-1);
			}
		});
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		RefreshUI();

		if (mBrowseText != null)
		{
			ShowAddRom(-1);
		}
	}

	@Override
	protected void onPause()
	{
		DismissAddEditDialog();
		super.onPause();
	}

	private void RefreshUI()
	{
		if (AreRomsInstalled())
		{
			mRomList.setVisibility(View.VISIBLE);
			mNoRomsInstalledTextView.setVisibility(View.GONE);

			PopulateRomsList();
		}
		else
		{
			mRomList.setVisibility(View.GONE);
			mNoRomsInstalledTextView.setVisibility(View.VISIBLE);
		}
	}

	private void ShowAddRom(final int ID)
	{
		final View view = LayoutInflater.from(this).inflate(R.layout.rom_manager_add_rom, (ViewGroup) this.findViewById(R.id.rom_manager_add_rom_layout));
		Button browseButton = (Button) view.findViewById(R.id.rom_manager_add_rom_browse_button);
		TextView browseTextView = (TextView) view.findViewById(R.id.rom_manager_add_rom_browse_text);
		final EditText romTitle = (EditText) view.findViewById(R.id.rom_manager_add_rom_title);
		final ImageButton deleteIcon = (ImageButton) view.findViewById(R.id.rom_manager_add_rom_deleterom);
		final Spinner calcTypeSpinner = (Spinner) view.findViewById(R.id.add_rom_calctype_spinner);
		final TableRow calcTypeRow = (TableRow) view.findViewById(R.id.add_rom_calctype_tablerow);

		String windowTitle = null;

		final boolean isEdit = ID >= 0;

		if (isEdit)
		{
			calcTypeRow.setVisibility(View.GONE);
			browseButton.setVisibility(View.GONE);
			browseTextView.setVisibility(View.VISIBLE);
			deleteIcon.setVisibility(View.VISIBLE);
			browseTextView.setText("");
			windowTitle = "Edit ROM";

			deleteIcon.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View v)
				{
					DeleteRom(ID);
				}
			});
		}
		else
		{
			deleteIcon.setVisibility(View.GONE);
			windowTitle = "Add ROM";

			if (mBrowseText != null)
			{
				browseButton.setVisibility(View.GONE);
				browseTextView.setVisibility(View.VISIBLE);
				browseTextView.setText(StringGetFileName(mBrowseText));
			}
			else
			{
				browseButton.setVisibility(View.VISIBLE);
				browseTextView.setVisibility(View.GONE);
			}

			browseButton.setOnClickListener(new OnClickListener() {
				@Override
				public void onClick(View v)
				{
					ChooseRomFile();
				}
			});

			ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this, R.array.calctypeArray, android.R.layout.simple_spinner_item);
			adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
			calcTypeSpinner.setAdapter(adapter);

			calcTypeSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
				@Override
				public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id)
				{
					String value = calcTypeSpinner.getSelectedItem().toString();
					if (!value.startsWith("-"))
					{
						romTitle.setText(value);
						romTitle.setSelection(value.length());
					}
				}

				@Override
				public void onNothingSelected(AdapterView<?> parentView)
				{
				}
			});
		}

		final AlertDialog addEditdialog = new AlertDialog.Builder(this).setView(view).setTitle(windowTitle).setPositiveButton(android.R.string.ok, null).setNegativeButton(android.R.string.cancel, new Dialog.OnClickListener() {
			@Override
			public void onClick(DialogInterface d, int which)
			{
				mBrowseText = null;
				mAddEditdialog = null;
				d.dismiss();
			}
		}).create();

		addEditdialog.setOnShowListener(new DialogInterface.OnShowListener() {
			@Override
			public void onShow(DialogInterface dialog)
			{
				Button b = addEditdialog.getButton(AlertDialog.BUTTON_POSITIVE);
				b.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View view)
					{
						String description = romTitle.getText().toString().trim();
						String calcType = null;

						if (!isEdit) calcType = calcTypeSpinner.getSelectedItem().toString();

						if (description.length() > 0 && (isEdit || (!Util.StringNullOrEmpty(mBrowseText) && !calcType.startsWith("-"))))
						{
							if (!isEdit)
							{
								if (!Util.IsStorageAvailable())
								{
									Util.ShowAlert(mThis, "Error", "Cannot access the internal storage. Ensure that your device is disconnected from the PC.");
									DismissAddEditDialog();
									return;
								}

								CalculatorInstance newInstance = new CalculatorInstance();
								newInstance.Title = description;

								newInstance.InitialROMFile = StringGetFileName(mBrowseText);
								mCalculatorInstances.Add(newInstance);

								String folder = Directories.GetInternalAppStorage(mThis);

								if (folder == null)
								{
									DismissAddEditDialog();
									return;
								}

								String root = folder + Integer.toString(newInstance.ID);

								newInstance.ImageFilePath = root + "/image.img";
								newInstance.StateFilePath = root + "/image.img.state";

								Util.CreateDirectory(root);

								int calculatorType = CalculatorTypes.GetType(calcType);

								boolean isRom = mBrowseText.toLowerCase(Locale.US).endsWith(".rom");
								boolean isTilemUpdate = mBrowseText.toLowerCase(Locale.US).endsWith(".8xu");
								
								int error = 0;
								String rom_mismatch = null;
								if (isTilemUpdate && calculatorType != CalculatorTypes.TI83PLUS && calculatorType != CalculatorTypes.TI83PLUS_SE && calculatorType != CalculatorTypes.TI84PLUS && calculatorType != CalculatorTypes.TI84PLUS_SE)
								{
									rom_mismatch = "You can only use a 8Xu with a TI84+, TI84+SE, TI83+, TI83+SE";
								}
								else
								{
									error = EmulatorActivity.nativeInstallROM(mBrowseText, newInstance.ImageFilePath, calculatorType, Util.Bool2Int(isRom));
								}

								if (error != 0 || rom_mismatch!=null)
								{
									String msg = null;
									if (rom_mismatch != null)
									{
										msg = rom_mismatch;
									}
									else
									{
										msg = "Loading ROM failed. ErrorCode: " + TiEmuErrorCodes.GetErrorCode(error);
									}

									Util.ShowAlert(mThis, "Error", msg);

									mCalculatorInstances.Remove(newInstance);

									DismissAddEditDialog();
									return;
								}
								else
								{
									newInstance.CalculatorType = calculatorType;
									mCalculatorInstances.Save();
								}
							}
							else
							{
								CalculatorInstance instance = mCalculatorInstances.GetByIndex(ID);
								instance.Title = description;
								mCalculatorInstances.Save();
							}

							mBrowseText = null;
							HandlerRefreshUI();
							DismissAddEditDialog();
						}
						else
						{
							if (!isEdit)
							{
								if (Util.StringNullOrEmpty(mBrowseText))
								{
									ShowAlert("Alert", "Please browse to the ROM location by cliking the 'Browse' button");
								}
								else if (calcType.startsWith("-"))
								{
									ShowAlert("Alert", "Please provide the Calcualtor Type");
								}
							}

							if (Util.StringNullOrEmpty(description))
							{
								ShowAlert("Alert", "Please provide a short description for this instance. i.e 'Voyage 200 - Calculus'. You can edit this later.");
							}
						}
					}
				});
			}
		});
		addEditdialog.setCanceledOnTouchOutside(false);
		mAddEditdialog = addEditdialog;

		if (isEdit)
		{
			CalculatorInstance instance = mCalculatorInstances.GetByIndex(ID);
			browseTextView.setText(instance.InitialROMFile);
			romTitle.setText(instance.Title);
			romTitle.setSelection(instance.Title.length());
		}

		addEditdialog.show();
	}

	private void DismissAddEditDialog()
	{
		if (mAddEditdialog != null)
		{
			mAddEditdialog.dismiss();
			mAddEditdialog = null;
		}
	}

	private String StringGetFileName(String fullFileName)
	{
		return new File(fullFileName).getName();
	}

	private void ShowAlert(String title, String message)
	{
		Util.ShowAlert(this, title, message);
	}

	private boolean AreRomsInstalled()
	{
		return mCalculatorInstances.GetInstances().size() > 0;
	}

	private void DeleteRom(final int index)
	{
		AlertDialog alert = new AlertDialog.Builder(this).setTitle("Warning").setMessage("Are you sure you want to remove this instance?").setPositiveButton(android.R.string.ok, new Dialog.OnClickListener() {
			@Override
			public void onClick(DialogInterface d, int which)
			{
				CalculatorInstance instance = mCalculatorInstances.GetByIndex(index);
				Util.DeleteFile(instance.ImageFilePath);
				Util.DeleteFile(instance.StateFilePath);
				mCalculatorInstances.Remove(instance);

				HandlerRefreshUI();
				DismissAddEditDialog();
			}
		}).setNegativeButton(android.R.string.cancel, null).create();
		alert.show();
	}

	private void PopulateRomsList()
	{
		List<CalculatorInstance> calcInstances = mCalculatorInstances.GetInstances();

		ArrayList<String> romList = new ArrayList<String>();

		for (int i = 0; i < calcInstances.size(); ++i)
		{
			romList.add(calcInstances.get(i).Title);
		}

		ArrayAdapter<String> listAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, romList);

		mRomList.setAdapter(listAdapter);
	}

	private void ChooseRomFile()
	{
		Intent myIntent = new Intent(this, FilePickerActivity.class);
		myIntent.putExtra(FilePickerActivity.EXTRA_FILE_PATH, Environment.getExternalStorageDirectory().getAbsolutePath());
		ArrayList<String> extensions = new ArrayList<String>();
		extensions.add(".rom");
		extensions.add(".8Xu");
		extensions.add(".89u");
		extensions.add(".v2u");
		extensions.add(".9xu");
		extensions.add(".tib");
		myIntent.putExtra(FilePickerActivity.EXTRA_ACCEPTED_FILE_EXTENSIONS, extensions);
		myIntent.putExtra(FilePickerActivity.EXTRA_MULTISELECT, false);
		startActivityForResult(myIntent, ROM_BROWSE);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if (resultCode == RESULT_OK)
		{
			switch (requestCode)
			{
				case ROM_BROWSE:
					if (data.hasExtra(FilePickerActivity.EXTRA_FILE_PATH))
					{
						ArrayList<String> results = data.getStringArrayListExtra(FilePickerActivity.EXTRA_FILE_PATH);
						if (results != null)
						{
							mBrowseText = results.get(0);

							if (mBrowseText != null) mBrowseText = mBrowseText.trim();
						}
					}
					break;
			}
		}
	}

	private void HandlerRefreshUI()
	{
		mHandler.sendEmptyMessage(HANDLER_REFRESHUI);
	}

	protected void handleMessage(Message msg)
	{
		super.handleMessage(msg);

		switch (msg.what)
		{
			case HANDLER_REFRESHUI:
				RefreshUI();
				break;
		}
	}
}
