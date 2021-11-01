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

import android.content.Context;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;

import com.Bisha.TI89EmuDonation.R;
import com.graph89.common.CalculatorInstanceHelper;
import com.graph89.controls.ControlBar;
import com.graph89.controls.MessageView;

public class UIStateManager
{
	public static final int		UI_STATE_UNINITIALIZED		= 0;
	public static final int		UI_STATE_TEXTMODE			= 1;
	public static final int		UI_STATE_EMULATOR			= 2;

	public ControlBar			ControlBarIntstance			= null;
	public MessageView			MessageViewIntstance		= null;
	public EmulatorView			EmulatorViewIntstance		= null;
	public ActionsList			ActionsListIntstance		= null;
	public ButtonHighlightView	ButtonHighlightViewInstance	= null;

	private int					mCurrentUIState				= UIStateManager.UI_STATE_UNINITIALIZED;
	private Context				mContext					= null;

	public UIStateManager(final Context context)
	{
		mContext = context;
		EmulatorActivity activity = (EmulatorActivity) mContext;

		ControlBarIntstance = new ControlBar(context);

		MessageViewIntstance = (MessageView) activity.findViewById(R.id.emulator_main_messageview);
		ActionsListIntstance = (ActionsList) activity.findViewById(R.id.actionslist);
		EmulatorViewIntstance = (EmulatorView) activity.findViewById(R.id.emulator_main_emulatorview);
		ButtonHighlightViewInstance = (ButtonHighlightView) activity.findViewById(R.id.emulator_main_buttonhighlightview);

		ControlBarIntstance.CalculatorTypeSpinnerInstance.setOnItemSelectedListener(new OnItemSelectedListenerWrapper(new OnItemSelectedListener() {
			@Override
			public void onItemSelected(AdapterView<?> parent, View view, int position, long id)
			{
				if (EmulatorActivity.ActiveInstance == null)
					return;
				EmulatorActivity.ActiveInstanceIndex = position;

				EmulatorActivity emulatorActivity = (EmulatorActivity) mContext;
				CalculatorInstanceHelper instances = new CalculatorInstanceHelper(context);
				instances.SetLastUsed(position);
				emulatorActivity.HideActions();
				emulatorActivity.RestartEmulator();
			}

			@Override
			public void onNothingSelected(AdapterView<?> arg0)
			{
			}
		}));
	}

	public void ShowTextViewer()
	{
		EmulatorViewIntstance.setVisibility(View.GONE);
		HideActions();
		MessageViewIntstance.setVisibility(View.VISIBLE);
		ControlBarIntstance.ShowControlBar();
		ControlBarIntstance.HideCalculatorTypeSpinner();

		mCurrentUIState = UIStateManager.UI_STATE_TEXTMODE;
	}

	public void ShowCalc()
	{
		ControlBarIntstance.ShowCalculatorTypeSpinner();
		ControlBarIntstance.HideControlBar();
		MessageViewIntstance.setVisibility(View.GONE);
		HideActions();
		EmulatorViewIntstance.setVisibility(View.VISIBLE);

		mCurrentUIState = UIStateManager.UI_STATE_EMULATOR;
	}

	public void ShowActions()
	{
		ActionsListIntstance.AdjustVisibility();
		ActionsListIntstance.setVisibility(View.VISIBLE);
	}

	public void HideActions()
	{
		ActionsListIntstance.setVisibility(View.GONE);
		if (mCurrentUIState == UIStateManager.UI_STATE_EMULATOR)
			ControlBarIntstance.HideControlBar();
	}

	public boolean AreActionsVisible()
	{
		return ActionsListIntstance.getVisibility() == View.VISIBLE;
	}

	public void BackKeyPressed()
	{
		ActionsListIntstance.AdjustVisibility();

		if (mCurrentUIState == UIStateManager.UI_STATE_EMULATOR)
		{
			int newVisibility = ControlBarIntstance.ToggleControlBar();
			ActionsListIntstance.setVisibility(newVisibility);
		}
		else if (mCurrentUIState == UIStateManager.UI_STATE_TEXTMODE)
		{
			ToggleActionListVisibility();
		}
	}

	private void ToggleActionListVisibility()
	{
		int newVisibility = ActionsListIntstance.getVisibility() == View.VISIBLE ? View.GONE : View.VISIBLE;
		ActionsListIntstance.setVisibility(newVisibility);
	}
}

class OnItemSelectedListenerWrapper implements OnItemSelectedListener
{
	private OnItemSelectedListener	listener;

	public OnItemSelectedListenerWrapper(OnItemSelectedListener aListener)
	{
		listener = aListener;
	}

	@Override
	public void onItemSelected(AdapterView<?> aParentView, View aView, int aPosition, long anId)
	{
		if (EmulatorActivity.ActiveInstanceIndex != aPosition)
		{
			listener.onItemSelected(aParentView, aView, aPosition, anId);
		}
	}

	@Override
	public void onNothingSelected(AdapterView<?> aParentView)
	{
		listener.onNothingSelected(aParentView);
	}
}
