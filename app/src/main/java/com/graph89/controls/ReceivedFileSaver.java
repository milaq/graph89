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

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
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
import com.graph89.emulationcore.TIEmuThread;

public class ReceivedFileSaver
{
	private Context	mContext		= null;
	private String	mReceivedFolder	= null;

	public ReceivedFileSaver(Context context, String receivedFolder)
	{
		mContext = context;
		mReceivedFolder = receivedFolder;

		Util.CreateDirectory(mReceivedFolder);
	}

	public void ShowDialog()
	{
		final EmulatorActivity activity = (EmulatorActivity) mContext;

		if (!Util.IsStorageAvailable(activity))
		{
			return;
		}

		final View view = LayoutInflater.from(mContext).inflate(R.layout.save_received_file, (ViewGroup) activity.findViewById(R.id.save_received_file_layout));
		final TextView constantpath = (TextView) view.findViewById(R.id.save_received_file_readonly_path);
		final EditText filenameEdit = (EditText) view.findViewById(R.id.save_received_file_path);

		constantpath.setText(mReceivedFolder);

		if (TIEmuThread.ReceivedFileName.startsWith("."))
		{
			TIEmuThread.ReceivedFileName = "noname" + TIEmuThread.ReceivedFileName;
		}
		filenameEdit.setText(TIEmuThread.ReceivedFileName);
		filenameEdit.setSelection(TIEmuThread.ReceivedFileName.length());

		final AlertDialog d = new AlertDialog.Builder(mContext).setView(view).setTitle("Save Received File").setPositiveButton(android.R.string.ok, new Dialog.OnClickListener()
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
							File src = new File(TIEmuThread.ReceivedFilePath);

							File dest = new File(new File(mReceivedFolder), filename);
							src.renameTo(dest);

							MediaScannerConnection.scanFile(mContext, new String[] { dest.getAbsolutePath() }, null, new MediaScannerConnection.OnScanCompletedListener()
							{
								public void onScanCompleted(String path, Uri uri)
								{
								}
							});
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
