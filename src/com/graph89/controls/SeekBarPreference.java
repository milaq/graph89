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

package com.graph89.controls;

import com.Bisha.TI89EmuDonation.R;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.preference.DialogPreference;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.SeekBar.OnSeekBarChangeListener;

public final class SeekBarPreference extends DialogPreference implements OnSeekBarChangeListener
{
	// Namespaces to read attributes
	private static final String	PREFERENCE_NS			= "com.graph89.controls.SeekBarPreference";
	private static final String	ANDROID_NS				= "http://schemas.android.com/apk/res/android";

	// Attribute names
	private static final String	ATTR_DEFAULT_VALUE		= "defaultValue";
	private static final String	ATTR_MIN_VALUE			= "minValue";
	private static final String	ATTR_MAX_VALUE			= "maxValue";

	// Default values for defaults
	private static final int	DEFAULT_CURRENT_VALUE	= 50;
	private static final int	DEFAULT_MIN_VALUE		= 0;
	private static final int	DEFAULT_MAX_VALUE		= 100;

	// Real defaults
	public int					mDefaultValue;
	public int					mMaxValue;
	public int					mMinValue;

	public String				ValuePost				= "";
	public String				ValueMAX				= "";
	public String				ValueMIN				= "";

	// Current value
	private int					mCurrentValue;

	// View elements
	private SeekBar				mSeekBar;
	private TextView			mValueText;

	public SeekBarPreference(Context context, AttributeSet attrs)
	{
		super(context, attrs);

		// Read parameters from attributes
		mMinValue = attrs.getAttributeIntValue(PREFERENCE_NS, ATTR_MIN_VALUE, DEFAULT_MIN_VALUE);
		mMaxValue = attrs.getAttributeIntValue(PREFERENCE_NS, ATTR_MAX_VALUE, DEFAULT_MAX_VALUE);
		mDefaultValue = attrs.getAttributeIntValue(ANDROID_NS, ATTR_DEFAULT_VALUE, DEFAULT_CURRENT_VALUE);
	}

	@Override
	protected View onCreateDialogView()
	{
		// Get current value from preferences
		mCurrentValue = getPersistedInt(mDefaultValue);

		// Inflate layout
		LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		View view = inflater.inflate(R.layout.dialog_slider, null);

		// Setup minimum and maximum text labels

		if (ValueMIN.length() > 0)
		{
			((TextView) view.findViewById(R.id.dialog_slider_min_value)).setText(ValueMIN);
		}
		else
		{
			((TextView) view.findViewById(R.id.dialog_slider_min_value)).setText(Integer.toString(mMinValue) + ValuePost);
		}

		if (ValueMAX.length() > 0)
		{
			((TextView) view.findViewById(R.id.dialog_slider_max_value)).setText(ValueMAX);
		}
		else
		{
			((TextView) view.findViewById(R.id.dialog_slider_max_value)).setText(Integer.toString(mMaxValue) + ValuePost);
		}

		// Setup SeekBar
		mSeekBar = (SeekBar) view.findViewById(R.id.dialog_slider_seek_bar);
		mSeekBar.setMax(mMaxValue - mMinValue);
		mSeekBar.setProgress(mCurrentValue - mMinValue);
		mSeekBar.setOnSeekBarChangeListener(this);

		// Setup text label for current value
		mValueText = (TextView) view.findViewById(R.id.dialog_slider_current_value);
		SetValueText();

		return view;
	}

	@Override
	protected void onDialogClosed(boolean positiveResult)
	{
		super.onDialogClosed(positiveResult);

		// Return if change was cancelled
		if (!positiveResult)
		{
			return;
		}

		// Persist current value if needed
		if (shouldPersist())
		{
			persistInt(mCurrentValue);
		}

		// Notify activity about changes (to update preference summary line)
		notifyChanged();
	}

	@Override
	public CharSequence getSummary()
	{
		// Format summary string with current value
		String summary = super.getSummary().toString();
		int value = getPersistedInt(mDefaultValue);
		return String.format(summary, value);
	}

	public void onProgressChanged(SeekBar seek, int value, boolean fromTouch)
	{
		// Update current value
		mCurrentValue = value + mMinValue;
		// Update label with current value
		SetValueText();

	}

	private void SetValueText()
	{
		if (ValueMIN.length() > 0 && mCurrentValue == mMinValue)
		{
			mValueText.setText(ValueMIN);
		}
		else if (ValueMAX.length() > 0 && mCurrentValue == mMaxValue)
		{
			mValueText.setText(ValueMAX);
		}
		else
		{
			mValueText.setText(Integer.toString(mCurrentValue) + ValuePost);
		}
	}

	public void onStartTrackingTouch(SeekBar seek)
	{
		// Not used
	}

	public void onStopTrackingTouch(SeekBar seek)
	{
		// Not used
	}
}