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
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BitmapFactory.Options;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Environment;

import com.graph89.emulationcore.EmulatorActivity;
import com.graph89.emulationcore.Graph89ActivityBase;

public class Util
{
	public static Paint	FilteredPaint;
	public static Paint	SimplePaint;

	static
	{
		FilteredPaint = new Paint();
		FilteredPaint.setFilterBitmap(true);

		SimplePaint = new Paint();
	}

	public static boolean StringNullOrEmpty(String value)
	{
		return value == null || value.trim().length() == 0;
	}

	public static boolean Inbounds(int x, int y, Rect rect)
	{
		return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
	}

	public static boolean Inbounds(int x, int y, RectF rect)
	{
		return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
	}
	
	public static int Bool2Int(Boolean value)
	{
		return value ? 1 : 0;
	}

	public static AlertDialog ShowAlert(Context context, String title, String text)
	{
		if (title == null && text == null) return null;
		
		AlertDialog alert = new AlertDialog.Builder(context).setTitle(title).setMessage(text).setPositiveButton(android.R.string.ok, null).create();
		alert.show();
		return alert;
	}

	public static void ShowAlert(final EmulatorActivity context, String location, Exception e)
	{
		final AlertDialog alert = new AlertDialog.Builder(context).setTitle("Error").setMessage("Location: " + location + "\n" + e.getMessage() + "\n" + e.getStackTrace()).setPositiveButton(android.R.string.ok, new Dialog.OnClickListener() {
			@Override
			public void onClick(DialogInterface d, int which)
			{
				context.Terminate();
			}
		}).create();

		alert.show();
	}

	public static boolean FileExists(String path)
	{
		if (StringNullOrEmpty(path)) return false;
		File f = new File(path);
		return f.exists();
	}

	public static void DeleteFile(String filePath)
	{
		if (StringNullOrEmpty(filePath)) return;

		File file = new File(filePath);
		if (file.exists())
		{
			file.delete();
		}
	}

	public static void deleteDirectory(File f)
	{
		if (f.isDirectory())
		{
			for (File c : f.listFiles())
				deleteDirectory(c);
		}

		f.delete();
	}

	public static boolean CreateDirectory(String path)
	{
		return new File(path).mkdirs();
	}

	public static boolean IsStorageAvailable()
	{
		String state = Environment.getExternalStorageState();
		return Environment.MEDIA_MOUNTED.equals(state);
	}

	public static boolean IsStorageAvailable(Graph89ActivityBase activity)
	{
		boolean ret = IsStorageAvailable();

		if (ret) return true;

		Graph89ActivityBase.AlertControlObj.SetTitleMessage("Error", "\nInternal storage is not available!!!\nIs your device connected to a PC with a USB cable? If so, disconnect it and restart the Graph89 application.");
		activity.HandlerShowAlert();

		return false;
	}

	public static String GetMediaRootFolder(Graph89ActivityBase activity)
	{
		File externalPath = Environment.getExternalStorageDirectory();
		if (externalPath == null)
		{
			Graph89ActivityBase.AlertControlObj.SetTitleMessage("Error", "\nInternal storage is not available!!!\nIs your device connected to a PC with a USB cable? If so, disconnect it and restart the Graph89 application.");
			activity.HandlerShowAlert();
			return "No External Storage";
		}

		return externalPath.getAbsolutePath() + "/";
	}

	public static String GetInternalAppStorage(Graph89ActivityBase activity)
	{
		File f = activity.getExternalFilesDir(null);
		if (f == null)
		{
			Graph89ActivityBase.AlertControlObj.SetTitleMessage("Error", "\nInternal storage is not available!!!\nIs your device connected to a PC with a USB cable? If so, disconnect it and restart the Graph89 application.");
			activity.HandlerShowAlert();
			return null;
		}

		return f.getAbsolutePath() + "/";
	}

	public static Bitmap BitmapFromAssets(Context context, String assetsPath) throws IOException
	{
		InputStream inputStream = null;

		try
		{
			Options options = new BitmapFactory.Options();
			options.inScaled = false;
			options.inDither = false;

			inputStream = ((EmulatorActivity) context).getAssets().open(assetsPath);
			return BitmapFactory.decodeStream(inputStream, null, options);
		}
		finally
		{
			if (inputStream != null) inputStream.close();
		}
	}

	public static void WriteAllText(String path, String text) throws IOException
	{
		BufferedWriter writer = null;
		try
		{
			FileWriter output = new FileWriter(path);
			writer = new BufferedWriter(output);
			writer.write(text);
		}
		finally
		{
			if (writer != null) writer.close();
		}
	}

	public static String ReadAllText(String path) throws IOException
	{
		String returnValue = "";

		BufferedReader reader = null;

		try
		{
			FileReader file = new FileReader(path);
			reader = new BufferedReader(file);
			String line = "";
			while ((line = reader.readLine()) != null)
			{
				returnValue += line + "\n";
			}
		}
		finally
		{
			if (reader != null) reader.close();
		}

		return returnValue;
	}

	public static String getTimestamp()
	{
		SimpleDateFormat formatter = new SimpleDateFormat("yyyyMMddHHmmss", Locale.getDefault());
		return formatter.format((new Date()).getTime());
	}

	public static String getMD5(String value)
	{
		if (value == null || value.length() == 0) return null;

		MessageDigest m = null;

		try
		{
			m = MessageDigest.getInstance("MD5");
		}
		catch (NoSuchAlgorithmException e)
		{
			return null;
		}

		m.update(value.getBytes(), 0, value.length());
		return new BigInteger(1, m.digest()).toString(16).toUpperCase(Locale.getDefault());
	}

	public static void CopyFile(String source, String destination) throws IOException
	{
		FileInputStream fin = new FileInputStream(source);
		FileOutputStream fout = new FileOutputStream(destination);

		byte[] b = new byte[1024 * 1024];
		int noOfBytes = 0;

		while ((noOfBytes = fin.read(b)) != -1)
		{
			fout.write(b, 0, noOfBytes);
		}

		fin.close();
		fout.close();
	}
}
