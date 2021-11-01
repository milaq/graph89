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

import java.io.File;
import java.util.Date;

import com.graph89.common.CalculatorInstance;
import com.graph89.common.CalculatorTypes;
import com.graph89.common.EmulatorThread;
import com.graph89.common.SkinBase;
import com.graph89.common.TiEmuErrorCodes;
import com.graph89.common.Util;

public class TilEmThread extends EmulatorThread implements Runnable
{
	public static volatile int	EngineLoopSleep		= 50;
	public static volatile int	ScreenLoopSleep		= 50;

	public static String		ReceivedFilePath	= null;
	public static String		ReceivedFileName	= null;

	public volatile boolean		IsSleeping			= false;
	private boolean				firstCycleComplete	= false;

	public TilEmThread(EmulatorActivity activity, CalculatorInstance calculatorInstance)
	{
		super(activity, calculatorInstance);

		synchronized (EmulatorThread.EmulatorLock)
		{
			EngineThread = new Thread(this);
			EngineThread.start();
		}
	}

	public class ScreenRunnable implements Runnable
	{
		@Override
		public void run()
		{
			boolean prevScreenOff = true;
			SkinBase skin = EmulatorActivity.CurrentSkin;
			boolean turnOfOnScreenOff = EmulatorActivity.ActiveInstance.Configuration.TurnOffOnScreenOff;
			boolean isScreenOff = false;

			while (KillFlag == false)
			{
				try
				{
					if (!IsSleeping)
					{
						skin.Screen.refresh();

						isScreenOff = skin.Screen.isScreenOff();

						if (firstCycleComplete && KillFlag == false && turnOfOnScreenOff && !prevScreenOff && isScreenOff)
						{
							if (Activity.IsTilem2ndOffPressed())
							{
								Activity.HandlerTerminate();
							}
						}

						prevScreenOff = isScreenOff;
					}

					Thread.sleep(TilEmThread.ScreenLoopSleep);
				}
				catch (InterruptedException e)
				{
					return;
				}
			}
		}
	}

	@Override
	public void run()
	{
		synchronized (EmulatorThread.EmulatorLock)
		{
			try
			{
				int err = 0;
				err = EmulatorActivity.nativeTilemLoadImage(this.CalculatorInstance.ImageFilePath);
				if (err != 0)
				{
					EmulatorActivity.AlertControlObj.SetTitleMessage("Error", "There was an error loading the IMG file. Error code: " + TiEmuErrorCodes.GetErrorCode(err));
					Activity.HandlerShowAlert();
					return;
				}

				err = LoadState();
				if (err != 0)
				{
					EmulatorActivity.AlertControlObj.SetTitleMessage("Error", "There was an error reading the State file. Make sure your internal storage is accessible.");
					Activity.HandlerShowAlert();
					return;
				}

				EmulatorActivity.nativeTilemTurnScreenOn();

				float speedCoefficient = EmulatorActivity.ActiveInstance.Configuration.CPUSpeed / 100.0f;

				ScreenThread = new Thread(new ScreenRunnable());
				ScreenThread.start();

				EmulatorActivity.IsEmulating = true;
				EmulatorActivity.UIStateManagerObj.EmulatorViewIntstance.postInvalidate();

				boolean turbo = false;
				int runCntr = 0;

				int sleepInterval = (int) ((float) TilEmThread.EngineLoopSleep / speedCoefficient);
				boolean disableOverclock = false;
				int turbocount = 0;

				while (true)
				{
					++runCntr;

					long diff = new Date().getTime() - EmulatorActivity.LastTouched.getTime();

					if (KillFlag == true)
					{
						if (runCntr > 20) WriteState(); //don't save if the engine hasn't run for a second or so. Android tends to kill the app on orientation change. Might corrupt the state.
						break;
					}

					if (ResetCalc)
					{
						EmulatorActivity.nativeTilemReset();
						ResetCalc = false;
					}

					SkinBase skin = EmulatorActivity.CurrentSkin;

					if (EmulatorActivity.UploadFilesPath != null)
					{
						EmulatorActivity.ProgressDialogObj.Message = "";
						Activity.HandlerShowProgressDialog();
						for (int i = 0; i < EmulatorActivity.UploadFilesPath.size(); ++i)
						{
							String file = EmulatorActivity.UploadFilesPath.get(i);

							EmulatorActivity.ProgressDialogObj.Message = "Sending - " + file;
							Activity.HandlerUpdateProgressDialog();
							int ret = EmulatorActivity.nativeTilemUploadFile(file);

							if (ret != 0)
							{
								boolean is_unsupported = EmulatorActivity.ActiveInstance.CalculatorType == CalculatorTypes.TI83 || EmulatorActivity.ActiveInstance.CalculatorType == CalculatorTypes.TI83PLUS;
								
								String msg = "There was an error sending the application.\nIf these errors persist, consider doing a 'Backup' and then a 'Reset'. Both of these actions can be accessed by pressing the 'Back' button of your android device.";
								
								if (is_unsupported)
								{
									msg += "\nIf you are still unable to upload applications, consider providing a new ROM, or use a PLUS SE version.\nError code: ";
								}
								else
								{
									msg += "\nError code: ";
								}
								
								EmulatorActivity.AlertControlObj.SetTitleMessage("Error", msg + ret);
								Activity.HandlerShowAlert();
							}
						}

						EmulatorActivity.UploadFilesPath = null;
						Activity.HandlerHideProgressDialog();
					}

					if (EmulatorActivity.SyncClock)
					{
						EmulatorActivity.nativeTilemSyncClock();
						EmulatorActivity.SyncClock = false;
					}

					if (EmulatorActivity.ActiveInstance.Configuration.EnergySave)
					{
						IsSleeping = diff > 30 * 1000 && runCntr % 50 != 0 && !skin.Screen.isBusy();
					}

					if (runCntr % 40 == 0)
					{
						if (EmulatorActivity.ActiveInstance.Configuration.AutoOFF != 31 && diff > EmulatorActivity.ActiveInstance.Configuration.AutoOFF * 60 * 1000)
						{
							Activity.HandlerTerminate();
						}
					}

					if (!IsSleeping)
					{
						EmulatorActivity.nativeTilemRunEngine();
						firstCycleComplete = true;
					}

					turbo = EmulatorActivity.ActiveInstance.Configuration.OverclockWhenBusy && skin.Screen.isBusy();
					
					if (turbo)
					{
						++turbocount;
					}
					else
					{
						turbocount = 0;
					}
									
					if (turbocount > 15)  //app works so fast it shuts down in matter of seconds.
					{
						disableOverclock = true;
					}
					
					//if (maxx<turbocount)maxx = turbocount;
					
					if (!disableOverclock && turbo && !IsSleeping)
					{
						// run it in a loop.
						//one iteration takes 4ms
						for (int i = 0; i < 30 && KillFlag == false && skin.Screen.isBusy(); ++i)
						{
							EmulatorActivity.nativeTilemRunEngine();
						}

						Thread.sleep(1);
					}
					else
					{
						Thread.sleep(sleepInterval);
					}
				}
			}
			catch (InterruptedException e)
			{
			}
		}
	}

	public static void ReceiveFile(String source, String dest)
	{
		if (Activity != null)
		{
			ReceivedFilePath = source;
			ReceivedFileName = dest;
			Activity.HandlerReceiveFile();
		}
	}

	private int LoadState()
	{
		boolean isStorageAvailable = Util.IsStorageAvailable();
		boolean isROMFileVisible = new File(CalculatorInstance.ImageFilePath).isFile();
		boolean isStateFileVisible = new File(CalculatorInstance.StateFilePath).isFile();

		if (!isStorageAvailable || !isROMFileVisible) return -1;

		if (isStateFileVisible && !CalculatorInstance.WasStateFileCreated)
		{
			CalculatorInstance.WasStateFileCreated = true;
			EmulatorActivity.CalculatorInstances.Save();
		}

		if (CalculatorInstance.WasStateFileCreated)
		{
			return EmulatorActivity.nativeTilemLoadState(CalculatorInstance.StateFilePath);
		}

		return 0;
	}

	private void WriteState()
	{
		if (CalculatorInstance.Configuration.SaveStateOnExit && Util.IsStorageAvailable() && !Activity.isFinishing())
		{
			//int ret = 
			EmulatorActivity.nativeTilemSaveState(CalculatorInstance.ImageFilePath, CalculatorInstance.StateFilePath);

			if (CalculatorInstance.WasStateFileCreated == false)
			{
				CalculatorInstance.WasStateFileCreated = true;
				EmulatorActivity.CalculatorInstances.Save();
			}
		}
	}
}