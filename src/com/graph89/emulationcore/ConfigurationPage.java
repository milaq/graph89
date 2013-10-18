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

import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceManager;
import android.view.KeyEvent;

import com.Bisha.TI89EmuDonation.R;
import com.graph89.common.CalculatorConfiguration;
import com.graph89.common.CalculatorInstance;
import com.graph89.common.CalculatorInstanceHelper;
import com.graph89.common.CalculatorTypes;
import com.graph89.common.SkinDefinition;
import com.graph89.controls.AmbilWarnaPreference;
import com.graph89.controls.SeekBarPreference;

@SuppressWarnings("deprecation")
public class ConfigurationPage extends PreferenceActivity implements OnSharedPreferenceChangeListener
{
	public static final String			CONFIG_NAME				= "Graph89";

	public static int					MaxScreenZoom			= 1;
	public static int					DefaultScreenZoom		= 1;

	private CalculatorInstance			mActiveInstance			= null;
	private CalculatorInstanceHelper	mCalculatorInstances	= null;

	private SeekBarPreference			mHapticFeedback			= null;
	private SeekBarPreference			mCPUSpeed				= null;
	private SeekBarPreference			mTimeout				= null;
	private SeekBarPreference			mScreenScale			= null;

	private ListPreference				mSkinList				= null;
	private ListPreference				mSkinListV200			= null;
	private ListPreference				mSkinListTI84			= null;
	private ListPreference				mOrientationList		= null;
	private AmbilWarnaPreference		mLcdColor				= null;
	private AmbilWarnaPreference		mGridColor				= null;
	private ListPreference				mLcdType				= null;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		GetActiveInstance();
		Init();
		super.onCreate(savedInstanceState);
		this.setRequestedOrientation(EmulatorActivity.Orientation);
		addPreferencesFromResource(R.layout.settings);

		InitMembers();

		Configure();
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event)
	{
		switch (keyCode)
		{
			case KeyEvent.KEYCODE_BACK:
			case KeyEvent.KEYCODE_HOME:
				finish();
				break;
		}

		return super.onKeyDown(keyCode, event);
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
	{
		if (key.equals(CalculatorConfiguration.HapticFeedbackKey))
		{
			mActiveInstance.Configuration.HapticFeedback = sharedPreferences.getInt(CalculatorConfiguration.HapticFeedbackKey, 10);
		}
		else if (key.equals(CalculatorConfiguration.AudioFeedBackKey))
		{
			mActiveInstance.Configuration.AudioFeedBack = sharedPreferences.getBoolean(CalculatorConfiguration.AudioFeedBackKey, false);
		}
		else if (key.equals(CalculatorConfiguration.ZoomModeKey))
		{
			mActiveInstance.Configuration.ZoomMode = sharedPreferences.getBoolean(CalculatorConfiguration.ZoomModeKey, false);
		}
		else if (key.equals(CalculatorConfiguration.ScreenScaleKey))
		{
			mActiveInstance.Configuration.ScreenScale = sharedPreferences.getInt(CalculatorConfiguration.ScreenScaleKey, 1);
		}
		else if (key.equals(CalculatorConfiguration.AutoOFFKey))
		{
			mActiveInstance.Configuration.AutoOFF = sharedPreferences.getInt(CalculatorConfiguration.AutoOFFKey, 5);
		}
		else if (key.equals(CalculatorConfiguration.SkinKey))
		{
			String newValue = sharedPreferences.getString(CalculatorConfiguration.SkinKey, "Default");
			mActiveInstance.Configuration.Skin = SkinDefinition.StringToSkinType(newValue, mActiveInstance.CalculatorType);
			mSkinList.setSummary(sharedPreferences.getString(CalculatorConfiguration.SkinKey, newValue));
		}
		else if (key.equals(CalculatorConfiguration.SkinKeyTI84))
		{
			String newValue = sharedPreferences.getString(CalculatorConfiguration.SkinKeyTI84, "Classic 84");
			mActiveInstance.Configuration.Skin = SkinDefinition.StringToSkinType(newValue, mActiveInstance.CalculatorType);
			mSkinListTI84.setSummary(sharedPreferences.getString(CalculatorConfiguration.SkinKeyTI84, newValue));
		}
		else if (key.equals(CalculatorConfiguration.SkinKeyV200))
		{
			String newValue = sharedPreferences.getString(CalculatorConfiguration.SkinKeyV200, "Classic V200");
			mActiveInstance.Configuration.Skin = SkinDefinition.StringToSkinType(newValue, mActiveInstance.CalculatorType);
			mSkinListV200.setSummary(sharedPreferences.getString(CalculatorConfiguration.SkinKeyV200, newValue));
		}
		else if (key.equals(CalculatorConfiguration.SaveStateOnExitKey))
		{
			mActiveInstance.Configuration.SaveStateOnExit = sharedPreferences.getBoolean(CalculatorConfiguration.SaveStateOnExitKey, true);
		}
		else if (key.equals(CalculatorConfiguration.EnableGrayScaleKey))
		{
			mActiveInstance.Configuration.EnableGrayScale = sharedPreferences.getBoolean(CalculatorConfiguration.EnableGrayScaleKey, false);
		}
		else if (key.equals(CalculatorConfiguration.CPUSpeedKey))
		{
			mActiveInstance.Configuration.CPUSpeed = sharedPreferences.getInt(CalculatorConfiguration.CPUSpeedKey, 100);
		}
		else if (key.equals(CalculatorConfiguration.OverclockWhenBusyKey))
		{
			mActiveInstance.Configuration.OverclockWhenBusy = sharedPreferences.getBoolean(CalculatorConfiguration.OverclockWhenBusyKey, true);
		}
		else if (key.equals(CalculatorConfiguration.EnergySaveKey))
		{
			mActiveInstance.Configuration.EnergySave = sharedPreferences.getBoolean(CalculatorConfiguration.EnergySaveKey, true);
		}
		else if (key.equals(CalculatorConfiguration.LCDColorKey))
		{
			mActiveInstance.Configuration.LCDColor = sharedPreferences.getInt(CalculatorConfiguration.LCDColorKey, 0xFFA5BAA0);
		}
		else if (key.equals(CalculatorConfiguration.PixelOffKey))
		{
			mActiveInstance.Configuration.PixelOff = sharedPreferences.getInt(CalculatorConfiguration.PixelOffKey, 0xFFB6C5B7);
		}
		else if (key.equals(CalculatorConfiguration.PixelOnKey))
		{
			mActiveInstance.Configuration.PixelOn = sharedPreferences.getInt(CalculatorConfiguration.PixelOnKey, 0xFF000000);
		}
		else if (key.equals(CalculatorConfiguration.GridColorKey))
		{
			mActiveInstance.Configuration.GridColor = sharedPreferences.getInt(CalculatorConfiguration.GridColorKey, 0xFFA5BAA0);
		}
		else if (key.equals(CalculatorConfiguration.TurnOffOnScreenOffKey))
		{
			mActiveInstance.Configuration.TurnOffOnScreenOff = sharedPreferences.getBoolean(CalculatorConfiguration.TurnOffOnScreenOffKey, true);
		}
		else if (key.equals(CalculatorConfiguration.OrientationKey))
		{
			mActiveInstance.Configuration.Orientation = sharedPreferences.getString(CalculatorConfiguration.OrientationKey, "Portrait");
		}
		else if (key.equals(CalculatorConfiguration.LCDTypeKey))
		{
			mActiveInstance.Configuration.UseLCDGrid = !sharedPreferences.getString(CalculatorConfiguration.LCDTypeKey, "Solid").equals("Solid");

			mLcdType.setSummary(sharedPreferences.getString(CalculatorConfiguration.LCDTypeKey, "Solid"));

			PreferenceCategory d = (PreferenceCategory) findPreference("CONFIG_CAT_DISPLAY_SETTINGS");

			if (mActiveInstance.Configuration.UseLCDGrid)
			{
				d.addPreference(mGridColor);
			}
			else
			{
				d.removePreference(mGridColor);
			}
		}

		mCalculatorInstances.Save();
	}

	@Override
	protected void onResume()
	{
		super.onResume();

		mSkinList.setSummary(getPreferenceScreen().getSharedPreferences().getString(CalculatorConfiguration.SkinKey, "Default"));
		mSkinListV200.setSummary(getPreferenceScreen().getSharedPreferences().getString(CalculatorConfiguration.SkinKeyV200, "Classic V200"));
		mSkinListTI84.setSummary(getPreferenceScreen().getSharedPreferences().getString(CalculatorConfiguration.SkinKeyTI84, "Classic 84"));
		mOrientationList.setSummary(getPreferenceScreen().getSharedPreferences().getString(CalculatorConfiguration.OrientationKey, "Portrait"));
		mLcdType.setSummary(getPreferenceScreen().getSharedPreferences().getString(CalculatorConfiguration.LCDTypeKey, "Solid"));

		getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
	}

	@Override
	protected void onPause()
	{
		super.onPause();
		getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
	}

	@Override
	protected void onDestroy()
	{
		getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
		super.onDestroy();
	}

	private void Init()
	{
		SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(this);
		SharedPreferences.Editor editor = settings.edit();
		editor.putInt(CalculatorConfiguration.HapticFeedbackKey, mActiveInstance.Configuration.HapticFeedback);
		editor.putBoolean(CalculatorConfiguration.AudioFeedBackKey, mActiveInstance.Configuration.AudioFeedBack);
		editor.putBoolean(CalculatorConfiguration.ZoomModeKey, mActiveInstance.Configuration.ZoomMode);
		editor.putInt(CalculatorConfiguration.ScreenScaleKey, mActiveInstance.Configuration.ScreenScale);
		editor.putInt(CalculatorConfiguration.AutoOFFKey, mActiveInstance.Configuration.AutoOFF);

		if (mActiveInstance.CalculatorType == CalculatorTypes.TI89 || mActiveInstance.CalculatorType == CalculatorTypes.TI89T)
		{
			editor.putString(CalculatorConfiguration.SkinKey, SkinDefinition.SkinTypeToString(mActiveInstance.Configuration.Skin, mActiveInstance.CalculatorType));
			editor.putString(CalculatorConfiguration.OrientationKey, mActiveInstance.Configuration.Orientation);
		}
		else if (CalculatorTypes.isTilem(mActiveInstance.CalculatorType))
		{
			editor.putString(CalculatorConfiguration.SkinKeyTI84, SkinDefinition.SkinTypeToString(mActiveInstance.Configuration.Skin, mActiveInstance.CalculatorType));
			editor.putString(CalculatorConfiguration.OrientationKey, mActiveInstance.Configuration.Orientation);
		}
		else
		{
			editor.putString(CalculatorConfiguration.SkinKeyV200, SkinDefinition.SkinTypeToString(mActiveInstance.Configuration.Skin, mActiveInstance.CalculatorType));
		}
		editor.putInt(CalculatorConfiguration.CPUSpeedKey, mActiveInstance.Configuration.CPUSpeed);
		editor.putBoolean(CalculatorConfiguration.OverclockWhenBusyKey, mActiveInstance.Configuration.OverclockWhenBusy);
		editor.putBoolean(CalculatorConfiguration.EnergySaveKey, mActiveInstance.Configuration.EnergySave);
		editor.putBoolean(CalculatorConfiguration.SaveStateOnExitKey, mActiveInstance.Configuration.SaveStateOnExit);
		editor.putBoolean(CalculatorConfiguration.EnableGrayScaleKey, mActiveInstance.Configuration.EnableGrayScale);
		editor.putBoolean(CalculatorConfiguration.TurnOffOnScreenOffKey, mActiveInstance.Configuration.TurnOffOnScreenOff);

		editor.putInt(CalculatorConfiguration.LCDColorKey, mActiveInstance.Configuration.LCDColor);
		editor.putInt(CalculatorConfiguration.PixelOffKey, mActiveInstance.Configuration.PixelOff);
		editor.putInt(CalculatorConfiguration.PixelOnKey, mActiveInstance.Configuration.PixelOn);
		editor.putInt(CalculatorConfiguration.GridColorKey, mActiveInstance.Configuration.GridColor);

		editor.putString(CalculatorConfiguration.LCDTypeKey, mActiveInstance.Configuration.UseLCDGrid ? "Dot Matrix" : "Solid");

		editor.commit();
	}

	private void InitMembers()
	{
		mSkinList = (ListPreference) findPreference(CalculatorConfiguration.SkinKey);
		mSkinListV200 = (ListPreference) findPreference(CalculatorConfiguration.SkinKeyV200);
		mSkinListTI84 = (ListPreference) findPreference(CalculatorConfiguration.SkinKeyTI84);
		mOrientationList = (ListPreference) findPreference(CalculatorConfiguration.OrientationKey);

		mHapticFeedback = (SeekBarPreference) findPreference(CalculatorConfiguration.HapticFeedbackKey);
		mScreenScale = (SeekBarPreference) findPreference(CalculatorConfiguration.ScreenScaleKey);
		mCPUSpeed = (SeekBarPreference) getPreferenceScreen().findPreference(CalculatorConfiguration.CPUSpeedKey);
		mTimeout = (SeekBarPreference) getPreferenceScreen().findPreference(CalculatorConfiguration.AutoOFFKey);
		mLcdColor = (AmbilWarnaPreference) getPreferenceScreen().findPreference(CalculatorConfiguration.LCDColorKey);
		mGridColor = (AmbilWarnaPreference) getPreferenceScreen().findPreference(CalculatorConfiguration.GridColorKey);
		mLcdType = (ListPreference) findPreference(CalculatorConfiguration.LCDTypeKey);

		mHapticFeedback.ValuePost = " ms";
		mHapticFeedback.ValueMIN = "OFF";

		mScreenScale.ValuePost = "x";
		mScreenScale.mMaxValue = MaxScreenZoom;
		mScreenScale.mDefaultValue = DefaultScreenZoom;

		mCPUSpeed.ValuePost = "%";

		mTimeout.ValuePost = " min";
		mTimeout.ValueMAX = "Never";
	}

	private void Configure()
	{
		PreferenceCategory c = (PreferenceCategory) findPreference("CONFIG_TITLE");
		c.setTitle("Configuration - \"" + mActiveInstance.Title + "\"");

		PreferenceCategory d = (PreferenceCategory) findPreference("CONFIG_CAT_DISPLAY_SETTINGS");

		if (mActiveInstance.CalculatorType == CalculatorTypes.TI89 || mActiveInstance.CalculatorType == CalculatorTypes.TI89T)
		{
			d.removePreference(mSkinListV200);
			d.removePreference(mSkinListTI84);
		}
		else if (CalculatorTypes.isTilem(mActiveInstance.CalculatorType))	
		{
			d.removePreference(mSkinListV200);
			d.removePreference(mSkinList);
		}
		else if (mActiveInstance.CalculatorType == CalculatorTypes.TI92 || mActiveInstance.CalculatorType == CalculatorTypes.TI92PLUS || mActiveInstance.CalculatorType == CalculatorTypes.V200)
		{
			d.removePreference(mSkinList);
			d.removePreference(mSkinListTI84);
			d.removePreference(mScreenScale);
			d.removePreference(mLcdColor);
			d.removePreference(mOrientationList);
		}

		if (mActiveInstance.Configuration.UseLCDGrid)
		{
			d.addPreference(mGridColor);
		}
		else
		{
			d.removePreference(mGridColor);
		}
	}

	private void GetActiveInstance()
	{
		mCalculatorInstances = new CalculatorInstanceHelper(this);
		int ID = mCalculatorInstances.GetLastUsedInstanceID();

		mActiveInstance = mCalculatorInstances.GetByIndex(ID);
	}
}