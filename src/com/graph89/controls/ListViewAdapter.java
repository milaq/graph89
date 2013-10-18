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
import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

public class ListViewAdapter extends ArrayAdapter<ListItem>
{
	private Context			mContext			= null;
	private int				mLayoutResourceId	= 0;
	private int				mTextViewResourceID	= 0;
	private List<ListItem>	mItems				= null;

	public ListViewAdapter(Context context, int resource, int textViewResourceId, List<ListItem> objects)
	{
		super(context, resource, textViewResourceId, objects);

		mContext = context;
		mLayoutResourceId = resource;
		mTextViewResourceID = textViewResourceId;
		mItems = objects;
	}

	@Override
	public boolean areAllItemsEnabled ()
	{
		return false;
	}
	
	@Override
	public boolean isEnabled (int position)
	{
		return mItems.get(position).IsActive;
	}
	
	@Override
	public View getView(int position, View convertView, ViewGroup parent)
	{
		View row = convertView;

		TextView holder = null;

		if (row == null)
		{
			LayoutInflater inflater = ((Activity) mContext).getLayoutInflater();
			row = inflater.inflate(mLayoutResourceId, parent, false);

			holder = (TextView) row.findViewById(mTextViewResourceID);

			row.setTag(holder);
		}
		else
		{
			holder = (TextView) row.getTag();
		}

		ListItem item = mItems.get(position);
		holder.setText(item.ItemName);
		if (item.IsBold)
		{
			holder.setTypeface(Typeface.DEFAULT_BOLD);
		}
		else
		{
			holder.setTypeface(Typeface.DEFAULT);
		}
		
		if (item.IsItalic)
		{
			holder.setTypeface(Typeface.defaultFromStyle(Typeface.ITALIC));
		}
		else
		{
			holder.setTypeface(Typeface.DEFAULT);
		}
		
		holder.setEnabled(item.IsActive);

		return row;
	}
}
