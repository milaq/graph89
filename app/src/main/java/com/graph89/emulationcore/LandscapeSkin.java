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

import java.io.IOException;
import java.util.Date;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;

import com.graph89.common.CalculatorInfoBase;
import com.graph89.common.Dimension2D;
import com.graph89.common.Highlights;
import com.graph89.common.KeyMask;
import com.graph89.common.SkinBase;
import com.graph89.common.SkinDefinition;
import com.graph89.common.Util;

public class LandscapeSkin extends SkinBase
{
	private Context			mContext;

	private EmulatorScreen	otherScreen		= null;

	public LandscapeSkin(Context context, CalculatorInfoBase calculatorInfo, SkinDefinition skinDefinition) throws IllegalArgumentException
	{
		mContext = context;

		if (calculatorInfo == null)
		{
			throw new IllegalArgumentException("null calculatorInfo in LandscapeSkin constructor");
		}

		if (skinDefinition == null || Util.StringNullOrEmpty(skinDefinition.ImagePath))
		{
			throw new IllegalArgumentException("skinDefinition is null or empty in LandscapeSkin constructor");
		}

		CalculatorInfo = calculatorInfo;
		SkinDefnition = skinDefinition;
	}

	@Override
	public void Init(int width, int height) throws IOException
	{
		if (EmulatorActivity.ActiveInstance == null) return;

		CanvasDimensions.Height = height;
		CanvasDimensions.Width = width;

		Dimension2D buttonsScaledDimension = new Dimension2D();

		SkinBitmap = Bitmap.createBitmap(CanvasDimensions.Width, CanvasDimensions.Height, Bitmap.Config.ARGB_8888);

		Bitmap buttonsImage = null;

		if (SkinDefnition.Source == SkinDefinition.SOURCE_ASSETS)
		{
			buttonsImage = Util.BitmapFromAssets(mContext, SkinDefnition.ImagePath);
			ButtonHighlights = new Highlights(mContext, SkinDefnition.ButtonLocationPath);
		}
		else if (SkinDefnition.Source == SkinDefinition.SOURCE_FILESYSTEM)
		{

		}

		ProcessInfoFile(mContext);
		this.LCDPixelOFF = EmulatorActivity.ActiveInstance.Configuration.PixelOff;
		this.LCDPixelON = EmulatorActivity.ActiveInstance.Configuration.PixelOn;
		this.LCDSpaceBackgroundColor = EmulatorActivity.ActiveInstance.Configuration.LCDColor;
		this.LCDGRID = EmulatorActivity.ActiveInstance.Configuration.GridColor;

		float buttonsAsspectRatio = (float) buttonsImage.getWidth() / (float) buttonsImage.getHeight();
		float screenAspectRatio = (float) CanvasDimensions.Width / (float) CanvasDimensions.Height;

		if (EmulatorActivity.ActiveInstance.Configuration.ZoomMode)
		{
			buttonsScaledDimension.Height = CanvasDimensions.Height;
			buttonsScaledDimension.Width = CanvasDimensions.Width;
		}
		else
		{
			if (screenAspectRatio > buttonsAsspectRatio) // wide enough.
			{
				buttonsScaledDimension.Height = CanvasDimensions.Height;
				buttonsScaledDimension.Width = (int) (CanvasDimensions.Height * buttonsAsspectRatio);
			}
			else
			// high enough
			{
				buttonsScaledDimension.Width = CanvasDimensions.Width;
				buttonsScaledDimension.Height = (int) (CanvasDimensions.Width / buttonsAsspectRatio);
			}
		}

		// Draw the buttons
		Rect source = new Rect();
		source.top = 0;
		source.left = 0;
		source.bottom = buttonsImage.getHeight();
		source.right = buttonsImage.getWidth();

		SkinOriginalDimension = source;

		Rect destination = new Rect();
		destination.top = CanvasDimensions.Height / 2 - buttonsScaledDimension.Height / 2;
		destination.left = CanvasDimensions.Width / 2 - buttonsScaledDimension.Width / 2;
		destination.bottom = CanvasDimensions.Height / 2 + buttonsScaledDimension.Height / 2;
		destination.right = CanvasDimensions.Width / 2 + buttonsScaledDimension.Width / 2;
		this.SkinPositionInCanvas = destination;

		Canvas skinCanvas = new Canvas(SkinBitmap);
		skinCanvas.drawColor(BackgroundColor);

		skinCanvas.drawBitmap(buttonsImage, source, destination, Util.FilteredPaint);

		// Configure Screen
		Rect screenRect = new Rect();
		screenRect.top = TranslateYSkinToScreen(ScreenPositionInSkin.top);
		screenRect.bottom = TranslateYSkinToScreen(ScreenPositionInSkin.bottom);
		screenRect.left = TranslateXSkinToScreen(ScreenPositionInSkin.left);
		screenRect.right = TranslateXSkinToScreen(ScreenPositionInSkin.right);
		Screen = new EmulatorScreen(this, screenRect, false, false);

		otherScreen = new EmulatorScreen(this, new Rect(0, 0, EmulatorActivity.AndroidDeviceScreenDimension.Width, EmulatorActivity.AndroidDeviceScreenDimension.Height), !EmulatorActivity.ActiveInstance.Configuration.ZoomMode, true);

		KeyMaskObj = new KeyMask(mContext, SkinDefnition.MaskPath, KeyMaskX, KeyMaskY);
	}

	@Override
	public boolean SwapScreen()
	{
		synchronized (EmulatorScreen.ScreenChangeLock)
		{
			EmulatorScreen curr = Screen;
			Screen = otherScreen;
			otherScreen = curr;
	
			otherScreen.CRC = 0;
			Screen.CRC = 0;
					
			EmulatorActivity.LastTouched = new Date();
			IsFull = !IsFull;
		}
		
		return true;
	}
}
