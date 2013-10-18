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

import com.graph89.common.KeyPress;
import com.graph89.common.Util;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

public class EmulatorView extends View implements OnTouchListener
{
	private EmulatorActivity	mContext	= null;

	public EmulatorView(Context context)
	{
		super(context);
		mContext = (EmulatorActivity) context;
	}

	public EmulatorView(Context context, AttributeSet attrs)
	{
		super(context, attrs);
		mContext = (EmulatorActivity) context;
	}

	public EmulatorView(Context context, AttributeSet attrs, int defStyle)
	{
		super(context, attrs, defStyle);
		mContext = (EmulatorActivity) context;
	}

	@Override
	public void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);

		if (!EmulatorActivity.IsEmulating) return;

		if (EmulatorActivity.CurrentSkin.CanvasDimensions.Height != canvas.getHeight() || EmulatorActivity.CurrentSkin.CanvasDimensions.Width != canvas.getWidth())
		{
			try
			{
				EmulatorActivity.CurrentSkin.Init(canvas.getWidth(), canvas.getHeight());
			}
			catch (IOException e)
			{
				Util.ShowAlert(mContext, "EmulatorView - onDraw", e);
			}
		}

		if (EmulatorActivity.CurrentSkin.Screen.IsFullScreen)
		{
			canvas.drawColor(EmulatorActivity.CurrentSkin.LCDSpaceBackgroundColor);
			EmulatorActivity.CurrentSkin.Screen.drawScreen(canvas);
		}
		else
		{
			if (EmulatorActivity.CurrentSkin.SkinBitmap != null)
			{
				canvas.drawBitmap(EmulatorActivity.CurrentSkin.SkinBitmap, 0, 0, null);
			}

			if (EmulatorActivity.CurrentSkin.Screen != null)
			{
				EmulatorActivity.CurrentSkin.Screen.drawScreen(canvas);
			}
		}
	}

	@Override
	public boolean onTouch(View v, MotionEvent event)
	{
		if (!EmulatorActivity.IsEmulating) return false;

		final int eventaction = event.getActionMasked();
		final int actionIndex = event.getActionIndex();
		final int pointerID = event.getPointerId(actionIndex);

		int x = (int) event.getX(actionIndex);
		int y = (int) event.getY(actionIndex);

		switch (eventaction)
		{

			case MotionEvent.ACTION_DOWN:
			case MotionEvent.ACTION_POINTER_DOWN:

				if (EmulatorActivity.CurrentSkin instanceof LandscapeSkin)
				{
					if (EmulatorActivity.CurrentSkin.IsKeypressInScreen(x, y) || EmulatorActivity.CurrentSkin.IsFull)
					{
						EmulatorActivity.CurrentSkin.SwapScreen();
						return true;
					}
				}
				KeyPress key = EmulatorActivity.CurrentSkin.GetKeypress(x, y);
				if (key != null)
				{
					key.TouchID = pointerID;
					ButtonState.ButtonPress(key);
				}
				break;
			case MotionEvent.ACTION_UP:
			case MotionEvent.ACTION_POINTER_UP:
				ButtonState.ButtonUnpress(pointerID);
				break;

			case MotionEvent.ACTION_CANCEL:
				ButtonState.UnpressAll();
				break;
		}

		return true;
	}
}
