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

import java.util.ArrayList;
import java.util.List;

import android.content.Context;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

public class CalculatorInstanceHelper
{
	private final static Object				LockObj		= new Object();
	private static List<CalculatorInstance>	sInstances	= null;
	private Context							mContext	= null;

	public CalculatorInstanceHelper(Context context)
	{
		mContext = context;
		ReadFromConfiguration();
	}

	public void Add(CalculatorInstance newInstance)
	{
		synchronized (LockObj)
		{
			int maxId = Integer.MIN_VALUE;

			for (int i = 0; i < sInstances.size(); ++i)
			{
				CalculatorInstance instance = sInstances.get(i);
				instance.WasLastUsed = false;
				if (instance.ID > maxId) maxId = instance.ID;
			}

			if (maxId < 0) maxId = 0;

			newInstance.WasLastUsed = true;
			newInstance.ID = maxId + 1;

			sInstances.add(newInstance);
			Save();
		}
	}

	public CalculatorInstance GetByIndex(int index)
	{
		synchronized (LockObj)
		{
			if (index >= sInstances.size()) return null;

			return sInstances.get(index);
		}
	}

	public int size()
	{
		synchronized (LockObj)
		{
			if (sInstances == null) return 0;

			return sInstances.size();
		}
	}

	public void Remove(int indexNumber)
	{
		synchronized (LockObj)
		{
			CalculatorInstance instance = GetByIndex(indexNumber);

			if (instance != null)
			{
				sInstances.remove(instance);
				Save();
			}
		}
	}

	public void Remove(CalculatorInstance instance)
	{
		synchronized (LockObj)
		{
			if (sInstances.remove(instance))
			{
				Save();
			}
		}
	}

	public int GetLastUsedInstanceID()
	{
		synchronized (LockObj)
		{
			for (int i = 0; i < sInstances.size(); ++i)
			{
				CalculatorInstance instance = sInstances.get(i);
				if (instance.WasLastUsed) return i;
			}

			if (sInstances.size() > 0)
			{
				SetLastUsed(0);
				return 0;
			}

			return -1;
		}
	}

	public int GetIndexFromInstance(CalculatorInstance instance)
	{
		synchronized (LockObj)
		{
			for (int i = 0; i < sInstances.size(); ++i)
			{
				CalculatorInstance inst = sInstances.get(i);

				if (inst == instance) return i;
			}

			return -1;
		}
	}

	public void SetLastUsed(int index)
	{
		synchronized (LockObj)
		{
			CalculatorInstance instance = GetByIndex(index);

			if (instance == null) return;

			for (int i = 0; i < sInstances.size(); ++i)
			{
				sInstances.get(i).WasLastUsed = false;
			}

			instance.WasLastUsed = true;

			Save();
		}
	}

	public List<CalculatorInstance> GetInstances()
	{
		return sInstances;
	}

	public String toJson()
	{
		return new Gson().toJson(sInstances);
	}

	public void Save()
	{
		synchronized (LockObj)
		{
			ConfigurationHelper.WriteString(mContext, ConfigurationHelper.CONF_KEY_CALCULATOR_INSTANCES, toJson());
		}
	}

	private void ReadFromConfiguration()
	{
		synchronized (LockObj)
		{
			if (sInstances == null)
			{
				sInstances = new ArrayList<CalculatorInstance>();

				String instancesJson = ConfigurationHelper.GetString(mContext, ConfigurationHelper.CONF_KEY_CALCULATOR_INSTANCES, null);

				if (!Util.StringNullOrEmpty(instancesJson))
				{
					Gson gsonHelper = new Gson();
					sInstances = gsonHelper.fromJson(instancesJson, new TypeToken<List<CalculatorInstance>>() {
					}.getType());
				}
			}
		}
	}
}
