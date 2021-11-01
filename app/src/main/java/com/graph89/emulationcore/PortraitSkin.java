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

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Rect;

import com.graph89.common.CalculatorInfoBase;
import com.graph89.common.Dimension2D;
import com.graph89.common.Highlights;
import com.graph89.common.KeyMask;
import com.graph89.common.SkinBase;
import com.graph89.common.SkinDefinition;
import com.graph89.common.Util;

public class PortraitSkin extends SkinBase
{
	private static final int	BorderScreenSkin	= 2;
	private Context				mContext;

	public PortraitSkin(Context context, CalculatorInfoBase calculatorInfo, SkinDefinition skinDefinition) throws IllegalArgumentException
	{
		mContext = context;

		if (calculatorInfo == null)
		{
			throw new IllegalArgumentException("null calculatorInfo PortraitSkin constructor");
		}

		if (skinDefinition == null || Util.StringNullOrEmpty(skinDefinition.ImagePath))
		{
			throw new IllegalArgumentException("skinDefinition is null or empty in PortraitSkin constructor");
		}

		CalculatorInfo = calculatorInfo;
		SkinDefnition = skinDefinition;
	}

	public void Init(int width, int height) throws IOException
	{
		if (EmulatorActivity.ActiveInstance == null) return;
		CanvasDimensions.Height = height;
		CanvasDimensions.Width = width;

		Dimension2D buttonsScaledDimension = new Dimension2D();
		Dimension2D screenDimension = new Dimension2D();

		int maxScreenZoom = CanvasDimensions.Width / CalculatorInfo.ScreenWidth;

		ConfigurationPage.MaxScreenZoom = CanvasDimensions.Width / CalculatorInfo.ScreenWidth;
		ConfigurationPage.DefaultScreenZoom = maxScreenZoom;

		int screenZoom = AdjustScreenZoom(EmulatorActivity.ActiveInstance.Configuration.ScreenScale, maxScreenZoom);

		if (EmulatorActivity.ActiveInstance.Configuration.ScreenScale <= 0 || EmulatorActivity.ActiveInstance.Configuration.ScreenScale > maxScreenZoom)
		{
			EmulatorActivity.ActiveInstance.Configuration.ScreenScale = screenZoom;
		}

		screenDimension.Height = CalculatorInfo.ScreenHeight * screenZoom + 10;
		screenDimension.Width = CalculatorInfo.ScreenWidth * screenZoom;

		SkinBitmap = Bitmap.createBitmap(CanvasDimensions.Width, CanvasDimensions.Height, Bitmap.Config.ARGB_8888);

		Bitmap buttonsImage = null;

		if (SkinDefnition.Source == SkinDefinition.SOURCE_ASSETS)
		{
			try
			{
				buttonsImage = Util.BitmapFromAssets(mContext, SkinDefnition.ImagePath);
				ButtonHighlights = new Highlights(mContext, SkinDefnition.ButtonLocationPath);
			}
			catch (Exception e)
			{
				buttonsImage = null;
				ButtonHighlights = null;
			}
		}
		else if (SkinDefnition.Source == SkinDefinition.SOURCE_FILESYSTEM)
		{

		}

		if (buttonsImage != null)
		{

			ProcessInfoFile(mContext);
			this.LCDPixelOFF = EmulatorActivity.ActiveInstance.Configuration.PixelOff;
			this.LCDPixelON = EmulatorActivity.ActiveInstance.Configuration.PixelOn;
			this.LCDSpaceBackgroundColor = EmulatorActivity.ActiveInstance.Configuration.LCDColor;
			this.LCDGRID = EmulatorActivity.ActiveInstance.Configuration.GridColor;

			float buttonsAsspectRatio = (float) buttonsImage.getWidth() / (float) buttonsImage.getHeight();

			buttonsScaledDimension.Height = CanvasDimensions.Height - screenDimension.Height - BorderScreenSkin;
			buttonsScaledDimension.Width = 0;

			if (EmulatorActivity.ActiveInstance != null && EmulatorActivity.ActiveInstance.Configuration.ZoomMode)
			{
				buttonsScaledDimension.Width = CanvasDimensions.Width;
			}
			else
			{
				buttonsScaledDimension.Width = Math.max(screenDimension.Width, (int) (buttonsScaledDimension.Height * buttonsAsspectRatio));
			}

			if (buttonsScaledDimension.Width > CanvasDimensions.Width) buttonsScaledDimension.Width = CanvasDimensions.Width;

			// Draw the background of the whole view.
			Canvas skinCanvas = new Canvas(SkinBitmap);
			skinCanvas.drawColor(BackgroundColor);

			// Draw the buttons
			Rect source = new Rect();
			source.top = 0;
			source.left = 0;
			source.bottom = buttonsImage.getHeight();
			source.right = buttonsImage.getWidth();

			SkinOriginalDimension = source;

			Rect destination = new Rect();
			destination.top = CanvasDimensions.Height - buttonsScaledDimension.Height;
			destination.left = CanvasDimensions.Width / 2 - buttonsScaledDimension.Width / 2;
			destination.bottom = CanvasDimensions.Height;
			destination.right = destination.left + buttonsScaledDimension.Width;
			this.SkinPositionInCanvas = destination;

			skinCanvas.drawBitmap(buttonsImage, source, destination, Util.FilteredPaint);

			// Draw the screen space///
			Paint screenSpacePaint = new Paint();
			screenSpacePaint.setColor(LCDSpaceBackgroundColor);
			screenSpacePaint.setStyle(Style.FILL);
			Rect screenSpaceRect = new Rect();
			screenSpaceRect.top = 0;
			screenSpaceRect.bottom = screenDimension.Height;
			screenSpaceRect.left = destination.left;
			screenSpaceRect.right = destination.right;
			skinCanvas.drawRect(screenSpaceRect, screenSpacePaint);
			KeyMaskObj = new KeyMask(mContext, SkinDefnition.MaskPath, KeyMaskX, KeyMaskY);
		}
		// Configure Screen
		Rect screenRect = new Rect();
		screenRect.top = 5;
		screenRect.bottom = screenDimension.Height + 5;
		screenRect.left = CanvasDimensions.Width / 2 - screenDimension.Width / 2;
		screenRect.right = screenRect.left + screenDimension.Width;
		Screen = new EmulatorScreen(this, screenRect, false, false);
	}

	private int AdjustScreenZoom(int screenZoom, int maxZoom)
	{
		if (screenZoom <= 0)
		{
			return Math.min(CanvasDimensions.Width / CalculatorInfo.ScreenWidth, (int) (0.5 * CanvasDimensions.Height) / CalculatorInfo.ScreenHeight);
		}
		else if (screenZoom > maxZoom)
		{
			return maxZoom;
		}
		else
		{
			return screenZoom;
		}
	}

	@Override
	public boolean SwapScreen()
	{
		return true;
	}
}
