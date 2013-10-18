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
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Locale;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Rect;

import com.graph89.emulationcore.EmulatorScreen;

public abstract class SkinBase
{
	public static final int			ORIENTATION_UNKNOWN		= 0;
	public static final int			ORIENTATION_PORTRAIT	= 1;
	public static final int			ORIENTATION_LANDSCAPE	= 2;

	public Dimension2D				CanvasDimensions		= new Dimension2D();

	public Bitmap					SkinBitmap				= null;
	public volatile EmulatorScreen	Screen					= null;

	public CalculatorInfoBase		CalculatorInfo			= null;
	public int						Orientation				= ORIENTATION_UNKNOWN;

	public SkinDefinition			SkinDefnition			= null;

	public int						BackgroundColor			= 0xFF000000;
	public int						LCDSpaceBackgroundColor	= 0xFFA5BAA0;
	public int						LCDPixelOFF				= 0xFFB6C5B7;
	public int						LCDPixelON				= 0xFF000000;
	public int						LCDGRID					= 0xFFA5BAA0;

	public boolean					IsFull					= false;

	public boolean					IsFullScreen			= false;

	protected KeyMask				KeyMaskObj				= null;
	public Highlights				ButtonHighlights		= null;

	protected Rect					SkinOriginalDimension	= null;
	protected Rect					SkinPositionInCanvas	= null;
	protected Rect					ScreenPositionInSkin	= null;

	protected int					KeyMaskX				= 0;
	protected int					KeyMaskY				= 0;

	// if Height or Width are different from Canvas call this method to resize.
	public abstract void Init(int width, int height) throws IOException;

	public abstract boolean SwapScreen();

	public void ReleaseBitmaps()
	{
		if (SkinBitmap != null)
		{
			SkinBitmap.recycle();
			SkinBitmap = null;
		}

		if (Screen != null)
		{
			Screen.ReleaseBitmaps();
			Screen = null;
		}
	}

	public boolean IsKeypressInScreen(int x, int y)
	{
		x = TranslateXScreenToSkin(x);
		y = TranslateYScreenToSkin(y);

		return Util.Inbounds(x, y, ScreenPositionInSkin);
	}

	public KeyPress GetKeypress(int x, int y)
	{
		if (KeyMaskObj == null || !Util.Inbounds(x, y, SkinPositionInCanvas))
		{
			return null;
		}

		int dx = SkinPositionInCanvas.right - SkinPositionInCanvas.left;
		int dy = SkinPositionInCanvas.bottom - SkinPositionInCanvas.top;

		int maskX = (int) (((float) KeyMaskObj.XDim / dx) * (x - SkinPositionInCanvas.left));
		int maskY = (int) (((float) KeyMaskObj.YDim / dy) * (y - SkinPositionInCanvas.top));

		if (maskX < 0) maskX = 0;
		if (maskY < 0) maskY = 0;
		if (maskX > KeyMaskObj.XDim - 1) maskX = KeyMaskObj.XDim - 1;
		if (maskY > KeyMaskObj.YDim - 1) maskY = KeyMaskObj.YDim - 1;

		KeyPress key = new KeyPress();
		key.KeyCode = KeyMaskObj.Keys[maskX + maskY * KeyMaskObj.XDim];

		key.X = TranslateXScreenToSkin(x);
		key.Y = TranslateYScreenToSkin(y);

		return key;
	}

	public int TranslateXSkinToScreen(int x)
	{
		float xRatio = ((float) (SkinPositionInCanvas.right - SkinPositionInCanvas.left)) / SkinOriginalDimension.right;

		return (int) (xRatio * x) + SkinPositionInCanvas.left;
	}

	public int TranslateXScreenToSkin(int x)
	{
		x = x - SkinPositionInCanvas.left;

		float xRatio = ((float) (SkinPositionInCanvas.right - SkinPositionInCanvas.left)) / SkinOriginalDimension.right;

		return (int) (x / xRatio);
	}

	public int TranslateYSkinToScreen(int y)
	{
		float yRatio = ((float) (SkinPositionInCanvas.bottom - SkinPositionInCanvas.top)) / SkinOriginalDimension.bottom;

		return (int) (yRatio * y) + SkinPositionInCanvas.top;
	}

	public int TranslateYScreenToSkin(int y)
	{
		y = y - SkinPositionInCanvas.top;

		float yRatio = ((float) (SkinPositionInCanvas.bottom - SkinPositionInCanvas.top)) / SkinOriginalDimension.bottom;

		return (int) (y / yRatio);
	}

	protected void ProcessInfoFile(Context context) throws IOException
	{
		if (SkinDefnition.InfoPath == null) return;

		InputStream fstream = context.getAssets().open(SkinDefnition.InfoPath);

		DataInputStream in = new DataInputStream(fstream);
		BufferedReader br = new BufferedReader(new InputStreamReader(in));

		String strLine;

		while ((strLine = br.readLine()) != null)
		{
			String parts[] = strLine.split(":");

			if (parts.length != 2) continue;

			parts[0] = parts[0].trim().toLowerCase(Locale.getDefault());

			if (parts[0].equals("backgroundcolor"))
			{
				BackgroundColor = (int) Long.parseLong(parts[1].trim(), 16);
			}
			else if (parts[0].equals("lcdpixeloff"))
			{
				LCDPixelOFF = (int) Long.parseLong(parts[1].trim(), 16);
			}
			else if (parts[0].equals("lcdpixelon"))
			{
				LCDPixelON = (int) Long.parseLong(parts[1].trim(), 16);
			}
			else if (parts[0].equals("screen"))
			{
				String[] subParts = parts[1].trim().split("\\s");

				ScreenPositionInSkin = new Rect();
				ScreenPositionInSkin.left = Integer.parseInt(subParts[0].trim());
				ScreenPositionInSkin.top = Integer.parseInt(subParts[1].trim());
				ScreenPositionInSkin.right = Integer.parseInt(subParts[2].trim());
				ScreenPositionInSkin.bottom = Integer.parseInt(subParts[3].trim());
			}
			else if (parts[0].equals("mask"))
			{
				String[] subParts = parts[1].trim().split("\\s");

				KeyMaskX = Integer.parseInt(subParts[0].trim());
				KeyMaskY = Integer.parseInt(subParts[1].trim());
			}
		}

		br.close();
	}
}
