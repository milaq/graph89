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

import java.lang.ref.WeakReference;

import com.graph89.common.AlertControl;
import com.graph89.common.Util;

import android.app.Activity;
import android.os.Handler;
import android.os.Message;

public abstract class Graph89ActivityBase extends Activity
{
	public static final int		HANDLER_SHOWALERT	= 1;
	public static final int		MAX_HANDLER_ID		= 1;

	protected IncomingHandler	mHandler			= new IncomingHandler(this);

	public static AlertControl	AlertControlObj		= new AlertControl();

	protected void handleMessage(Message msg)
	{
		switch (msg.what)
		{
			case Graph89ActivityBase.HANDLER_SHOWALERT:
				AlertControlObj.DismissAlert();
				AlertControlObj.Alert = Util.ShowAlert(this, AlertControlObj.Title, AlertControlObj.Message);
				break;
		}
	}

	public void HandlerShowAlert()
	{
		mHandler.sendEmptyMessage(Graph89ActivityBase.HANDLER_SHOWALERT);
	}

	public static class IncomingHandler extends Handler
	{
		private final WeakReference<Graph89ActivityBase>	mActivity;

		public IncomingHandler(Graph89ActivityBase activity)
		{
			mActivity = new WeakReference<Graph89ActivityBase>(activity);
		}

		@Override
		public void handleMessage(Message msg)
		{
			Graph89ActivityBase activity = mActivity.get();
			if (activity != null)
			{
				activity.handleMessage(msg);
			}
		}
	}
}
