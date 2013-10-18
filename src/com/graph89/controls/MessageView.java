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

import android.content.Context;
import android.text.Spannable;
import android.text.util.Linkify;
import android.util.AttributeSet;
import android.widget.TextView;

public class MessageView extends TextView
{
	private String	MessageText		= "";
	private int		MessagePriority	= 0;

	public MessageView(Context context)
	{
		super(context);
	}

	public MessageView(Context context, AttributeSet attrs)
	{
		super(context, attrs);
	}

	public MessageView(Context context, AttributeSet attrs, int defStyle)
	{
		super(context, attrs, defStyle);
	}

	public void SetText(int priority, String text)
	{
		if (priority >= MessagePriority)
		{
			MessageText = text;
			MessagePriority = priority;
		}

		this.setText(MessageText);

		Linkify.addLinks(this, Linkify.ALL);
	}

	public void SetSpannable(int priority, Spannable text)
	{
		if (priority >= MessagePriority)
		{
			MessagePriority = priority;
			
			this.setText(text, BufferType.SPANNABLE);
			Linkify.addLinks(this, Linkify.ALL);
		}
	}
}
