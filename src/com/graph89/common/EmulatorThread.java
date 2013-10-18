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

import com.graph89.emulationcore.EmulatorActivity;

public abstract class EmulatorThread
{
	public static final Object		EmulatorLock				= new Object();

	public static final int			STATE_NOT_INIT				= 0;
	public static final int			STATE_INIT					= 1;
	public static final int			STATE_ENGINE_LOOP			= 2;
	public static final int			STATE_EMULATORSTATE_SAVED	= 3;
	public static final int			STATE_EXIT_COMPLETE			= 4;

	public Thread					EngineThread				= null;
	public Thread					ScreenThread				= null;

	public CalculatorInstance		CalculatorInstance			= null;
	public static EmulatorActivity	Activity					= null;

	public static volatile boolean	ResetCalc					= false;

	protected int					mState						= STATE_NOT_INIT;

	protected volatile boolean		KillFlag					= false;

	public void Kill()
	{
		KillFlag = true;

		if (EngineThread != null)
		{
			try
			{
				synchronized (EmulatorThread.EmulatorLock)
				{
					EngineThread.join();
					if (ScreenThread != null) ScreenThread.join();
				}
			}
			catch (InterruptedException e)
			{
			}
		}
	}

	public EmulatorThread(EmulatorActivity activity, CalculatorInstance calculatorInstance)
	{
		synchronized (EmulatorThread.EmulatorLock)
		{
			Activity = activity;
			this.CalculatorInstance = calculatorInstance;
		}
	}
}
