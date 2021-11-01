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

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.Context;

public class GoogleAccount
{
	public static String getEmail(Context context)
	{
		AccountManager accountManager = AccountManager.get(context);
		Account account = getAccount(accountManager);

		if (account == null)
		{
			return null;
		}
		else
		{
			return account.name.trim().toLowerCase() + "+";
		}
	}

	private static Account getAccount(AccountManager accountManager)
	{
		Account[] accounts = accountManager.getAccountsByType("com.google");
		Account account;
		if (accounts.length > 0)
		{
			account = accounts[0];
		}
		else
		{
			account = null;
		}
		return account;
	}
}
