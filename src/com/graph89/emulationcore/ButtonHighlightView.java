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

import java.util.List;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;

import com.graph89.common.HighlightButtonType;
import com.graph89.common.HighlightInfo;
import com.graph89.common.KeyPress;
import com.graph89.common.SkinBase;
import com.graph89.common.Util;

public class ButtonHighlightView extends View
{
	private RectF	mCachedRect	= new RectF();

	public ButtonHighlightView(Context context)
	{
		super(context);
	}

	public ButtonHighlightView(Context context, AttributeSet attrs)
	{
		super(context, attrs);
	}

	public ButtonHighlightView(Context context, AttributeSet attrs, int defStyle)
	{
		super(context, attrs, defStyle);
	}

	@Override
	public void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);
		
		if (!EmulatorActivity.IsEmulating) return;

		canvas.drawColor(0x00);

		SkinBase skin = EmulatorActivity.CurrentSkin;

		if (skin != null)
		{
			KeyPress[] kp = ButtonState.GetPressedKeys();

			for (int i = 0; i < kp.length; ++i)
			{
				List<HighlightInfo> hInfos = skin.ButtonHighlights.FindHighlightInfoByKeyCode(kp[i].KeyCode);

				for (int k = 0; k < hInfos.size(); ++k)
				{
					HighlightInfo hInfo = hInfos.get(k);

					int shapeType = hInfo.ButtonType.ShapeType;
					int height = hInfo.ButtonType.Height;
					int width = hInfo.ButtonType.Width;
					int centerx = hInfo.CenterX;
					int centery = hInfo.CenterY;

					//adjust for 40% more coverage area, before we ignore the highlight.
					mCachedRect.left = centerx - (width / 2 * 1.4f);
					mCachedRect.right = centerx + (width / 2 * 1.4f);
					mCachedRect.top = centery - (height / 2 * 1.4f);
					mCachedRect.bottom = centery + (height / 2 * 1.4f);

					if (!Util.Inbounds(kp[i].X, kp[i].Y, mCachedRect))
					{
						continue;
					}

					if (shapeType == HighlightButtonType.SHAPETYPE_SQUARE)
					{
						canvas.drawRect(skin.TranslateXSkinToScreen(centerx - width / 2), skin.TranslateYSkinToScreen(centery - height / 2), skin.TranslateXSkinToScreen(centerx + width / 2), skin.TranslateYSkinToScreen(centery + height / 2), hInfo.ButtonType.Paint);
					}
					else if (shapeType == HighlightButtonType.SHAPETYPE_CIRCLE)
					{
						mCachedRect.left = skin.TranslateXSkinToScreen(centerx - width / 2);
						mCachedRect.top = skin.TranslateYSkinToScreen(centery - height / 2);
						mCachedRect.right = skin.TranslateXSkinToScreen(centerx + width / 2);
						mCachedRect.bottom = skin.TranslateYSkinToScreen(centery + height / 2);

						canvas.drawArc(mCachedRect, 0, 360, true, hInfo.ButtonType.Paint);
					}
				}
			}
		}
	}
}
