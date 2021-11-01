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

import java.io.File;
import java.io.FileOutputStream;
import java.util.Timer;
import java.util.TimerTask;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.Bisha.TI89EmuDonation.R;
import com.graph89.common.Util;
import com.graph89.emulationcore.EmulatorActivity;

public class ScreenshotTaker
{
	private Context		mContext			= null;
	private String		mScreenshotFolder	= null;

	public static File	LastFile			= null;

	public ScreenshotTaker(Context context, String screenshotFolder)
	{
		mContext = context;
		mScreenshotFolder = screenshotFolder;
	}

	public void ShowDialog()
	{
		final EmulatorActivity activity = (EmulatorActivity) mContext;

		if (!Util.IsStorageAvailable(activity))
		{
			return;
		}

		final View view = LayoutInflater.from(mContext).inflate(R.layout.take_screenshot, (ViewGroup) activity.findViewById(R.id.take_screenshot_layout));
		final TextView constantpath = (TextView) view.findViewById(R.id.take_screenshot_readonly_path);
		final EditText filenameEdit = (EditText) view.findViewById(R.id.take_screenshot_path);

		constantpath.setText(mScreenshotFolder);

		String dateNow = Util.getTimestamp();
		filenameEdit.setText(dateNow + ".png");
		filenameEdit.setSelection(dateNow.length());

		final AlertDialog d = new AlertDialog.Builder(mContext).setView(view).setTitle("Take Screenshot").setPositiveButton(android.R.string.ok, new Dialog.OnClickListener()
		{
			@Override
			public void onClick(DialogInterface d, int which)
			{
			}
		}).setNegativeButton(android.R.string.cancel, new Dialog.OnClickListener()
		{
			@Override
			public void onClick(DialogInterface d, int which)
			{
				activity.HideKeyboard();
			}
		}).create();

		d.setOnShowListener(new DialogInterface.OnShowListener()
		{
			@Override
			public void onShow(DialogInterface dialog)
			{
				Button b = d.getButton(AlertDialog.BUTTON_POSITIVE);
				b.setOnClickListener(new View.OnClickListener()
				{
					@Override
					public void onClick(View view)
					{
						String filename = filenameEdit.getText().toString().trim();
						filename = filename.replace("/", "");

						if (filename.length() > 0)
						{
							if (!filename.endsWith(".png")) filename += ".png";

							filename = mScreenshotFolder + filename;

							final File f = new File(filename);
							f.getParentFile().mkdirs();

							Bitmap image = EmulatorActivity.CurrentSkin.Screen.getScreenShot();
							if (image != null)
							{
								try
								{
									FileOutputStream out = new FileOutputStream(filename);
									image.compress(Bitmap.CompressFormat.PNG, 90, out);
									out.close();

									MediaScannerConnection.scanFile(mContext, new String[] { f.getAbsolutePath() }, null, new MediaScannerConnection.OnScanCompletedListener()
									{
										public void onScanCompleted(String path, Uri uri)
										{
										}
									});

									LastFile = f;

									activity.HideKeyboard();

									// keyboard doesn't hide. Delay the new
									// intent to give it time to hide.
									Timer timer = new Timer();
									timer.schedule(new TimerTask()
									{
										@Override
										public void run()
										{
											activity.HandlerStartGallery();
										}
									}, 400);
								}
								catch (Exception e)
								{
									Util.ShowAlert((EmulatorActivity) mContext, "ScreenshotTaker ShowDialog", e);
								}
							}

							activity.HideKeyboard();
							d.dismiss();
						}
					}
				});
			}
		});
		d.setCanceledOnTouchOutside(false);
		activity.ShowKeyboard();
		d.show();
	}
}
