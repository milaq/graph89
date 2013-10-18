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

import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.SpinnerAdapter;

import com.Bisha.TI89EmuDonation.R;

public class ControlBar
{
	public Spinner			CalculatorTypeSpinnerInstance	= null;
	private LinearLayout	mControlBarLayout				= null;
	private Context			mContext						= null;

	public ControlBar(Context context)
	{
		mContext = context;
		Activity activity = (Activity) mContext;

		mControlBarLayout = (LinearLayout) activity.findViewById(R.id.emulator_main_controlbar);
		CalculatorTypeSpinnerInstance = (Spinner) activity.findViewById(R.id.controlbar_calctype);

		SetListOfCalculatorTypes(null);
	}

	public void SetListOfCalculatorTypes(List<String> list)
	{
		if (list == null || list.size() == 0)
		{
			HideCalculatorTypeSpinner();
		}
		else
		{
			ArrayAdapter<String> dataAdapter = new ArrayAdapter<String>(mContext, android.R.layout.simple_spinner_item, list);
			dataAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
			CalculatorTypeSpinnerInstance.setAdapter(dataAdapter);

			ShowCalculatorTypeSpinner();
		}
	}

	public void ShowCalculatorTypeSpinner()
	{
		SpinnerAdapter adapter = CalculatorTypeSpinnerInstance.getAdapter();
		if (adapter == null)
			return;

		if (adapter.getCount() > 0)
		{
			CalculatorTypeSpinnerInstance.setVisibility(View.VISIBLE);
		}
	}

	public void HideCalculatorTypeSpinner()
	{
		CalculatorTypeSpinnerInstance.setVisibility(View.INVISIBLE);
	}

	public void ShowControlBar()
	{
		mControlBarLayout.setVisibility(View.VISIBLE);
	}

	public void HideControlBar()
	{
		mControlBarLayout.setVisibility(View.GONE);
	}

	public int ToggleControlBar()
	{
		int newVisibility = mControlBarLayout.getVisibility() == View.VISIBLE ? View.GONE : View.VISIBLE;

		mControlBarLayout.setVisibility(newVisibility);

		return newVisibility;
	}
}