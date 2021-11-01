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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URI;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

public class ZipHelper
{
	public static void Unzip(String dir, byte[] data) throws Exception
	{
		ByteArrayInputStream input = new ByteArrayInputStream(data);
		ZipInputStream zipIn = new ZipInputStream(input);
		ZipEntry entry;
		while ((entry = zipIn.getNextEntry()) != null)
		{
			String outpath = dir + entry.getName();
			FileOutputStream output = null;
			try
			{
				File f = new File (outpath);
				f.getParentFile().mkdirs();
				
				output = new FileOutputStream(f);
				int len = 0;
				byte[] buffer = new byte[4096];
				while ((len = zipIn.read(buffer)) > 0)
				{
					output.write(buffer, 0, len);
				}
			}
			finally
			{
				if (output != null) output.close();
			}
		}

		zipIn.close();
	}

	public static void zipDir(String zipFileName, String dir) throws Exception
	{
		ZipOutputStream zipOut = new ZipOutputStream(new FileOutputStream(zipFileName));

		zipDir(dir, zipOut);

		zipOut.close();
	}

	public static byte[] zipDir(String dir) throws Exception
	{
		ByteArrayOutputStream out = new ByteArrayOutputStream();
		ZipOutputStream zipOut = new ZipOutputStream(out);

		zipDir(dir, zipOut);

		byte[] zipped = out.toByteArray();
		zipOut.close();

		return zipped;
	}

	private static void GetFiles(File directory, ArrayList<File> files)
	{
		File[] f = directory.listFiles();

		for (int i = 0; i < f.length; i++)
		{
			if (f[i].isDirectory())
			{
				GetFiles(f[i], files);
			}
			else
			{
				files.add(f[i]);
			}
		}
	}

	private static void zipDir(String dir, ZipOutputStream out) throws IOException
	{
		File directory = new File(dir);

		URI base = directory.toURI();

		ArrayList<File> filesToZip = new ArrayList<File>();

		GetFiles(directory, filesToZip);

		for (int i = 0; i < filesToZip.size(); ++i)
		{
			FileInputStream in = new FileInputStream(filesToZip.get(i));

			String name = base.relativize(filesToZip.get(i).toURI()).getPath();

			out.putNextEntry(new ZipEntry(name));

			byte[] buf = new byte[4096];
			int bytes = 0;

			while ((bytes = in.read(buf)) != -1)
			{
				out.write(buf, 0, bytes);
			}

			out.closeEntry();

			in.close();
		}

		out.finish();
		out.flush();
	}
}