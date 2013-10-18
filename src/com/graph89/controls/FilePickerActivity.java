/*
 * Copyright 2011 Anders Kalør
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.graph89.controls;

import java.io.File;
import java.io.FilenameFilter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.Bisha.TI89EmuDonation.R;
import com.graph89.emulationcore.EmulatorActivity;

public class FilePickerActivity extends Activity
{
	public final static String			EXTRA_FILE_PATH					= "file_path";
	public final static String			EXTRA_SHOW_HIDDEN_FILES			= "show_hidden_files";
	public final static String			EXTRA_ACCEPTED_FILE_EXTENSIONS	= "accepted_file_extensions";
	public final static String			EXTRA_FILE_TYPE					= "file_type";
	public final static String			EXTRA_MULTISELECT				= "multiselect";
	private final static String			DEFAULT_INITIAL_DIRECTORY		= "/";

	public static String				mFileType;
	protected File						mDirectory;
	protected ArrayList<SelectedFile>	mFiles;
	protected FilePickerListAdapter		mAdapter;
	protected boolean					mShowHiddenFiles				= false;
	protected String[]					acceptedFileExtensions;
	private ListView					mListView						= null;
	private Button						mInstallFilesButton				= null;
	private boolean						mMultiSelect					= false;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		this.setRequestedOrientation(EmulatorActivity.Orientation);

		setContentView(R.layout.file_picker_main);

		mListView = (ListView) this.findViewById(R.id.file_picker_listview);
		mInstallFilesButton = (Button) this.findViewById(R.id.file_picker_load_button);

		mInstallFilesButton.setVisibility(View.GONE);

		LayoutInflater inflator = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		View emptyView = inflator.inflate(R.layout.file_picker_empty_view, null);
		((ViewGroup) mListView.getParent()).addView(emptyView);
		mListView.setEmptyView(emptyView);

		mFileType = null;
		mDirectory = new File(DEFAULT_INITIAL_DIRECTORY);
		mFiles = new ArrayList<SelectedFile>();

		mAdapter = new FilePickerListAdapter(this, mFiles);
		mListView.setAdapter(mAdapter);

		acceptedFileExtensions = new String[] {};

		// Get intent extras
		if (getIntent().hasExtra(EXTRA_FILE_PATH))
		{
			mDirectory = new File(getIntent().getStringExtra(EXTRA_FILE_PATH));
		}
		if (getIntent().hasExtra(EXTRA_SHOW_HIDDEN_FILES))
		{
			mShowHiddenFiles = getIntent().getBooleanExtra(EXTRA_SHOW_HIDDEN_FILES, false);
		}
		if (getIntent().hasExtra(EXTRA_FILE_TYPE))
		{
			mFileType = getIntent().getStringExtra(EXTRA_FILE_TYPE);
		}
		if (getIntent().hasExtra(EXTRA_ACCEPTED_FILE_EXTENSIONS))
		{
			ArrayList<String> collection = getIntent().getStringArrayListExtra(EXTRA_ACCEPTED_FILE_EXTENSIONS);
			acceptedFileExtensions = (String[]) collection.toArray(new String[collection.size()]);
		}
		if (getIntent().hasExtra(EXTRA_MULTISELECT))
		{
			mMultiSelect = getIntent().getBooleanExtra(EXTRA_MULTISELECT, false);
		}

		AddOnClickListener();
	}

	@Override
	protected void onResume()
	{
		refreshFilesList();
		super.onResume();
	}

	/**
	 * Updates the list view to the current directory
	 */
	protected void refreshFilesList()
	{
		// Clear the files ArrayList
		mFiles.clear();

		// Set the extension file filter
		ExtensionFilenameFilter filter = new ExtensionFilenameFilter(acceptedFileExtensions);

		// Get the files in the directory
		File[] files = mDirectory.listFiles(filter);

		boolean haveResults = false;

		if (files != null && files.length > 0)
		{
			for (File f : files)
			{
				if (f.isHidden() && !mShowHiddenFiles)
				{
					// Don't add the file
					continue;
				}

				if (f.isFile()) haveResults = true;
				SelectedFile selectedFile = new SelectedFile();
				selectedFile.FileObj = f;
				mFiles.add(selectedFile);
			}

			Collections.sort(mFiles, new FileComparator());
		}

		if (mMultiSelect)
		{
			if (haveResults)
			{
				mInstallFilesButton.setVisibility(View.VISIBLE);
				mInstallFilesButton.setEnabled(false);
			}
			else
			{
				mInstallFilesButton.setVisibility(View.GONE);
			}
		}
		else
		{
			mInstallFilesButton.setVisibility(View.GONE);
		}

		mAdapter.notifyDataSetChanged();
	}

	@Override
	public void onBackPressed()
	{
		if (mDirectory.getParentFile() != null)
		{
			// Go to parent directory
			mDirectory = mDirectory.getParentFile();
			refreshFilesList();
			return;
		}

		super.onBackPressed();
	}

	private void AddOnClickListener()
	{
		mListView.setOnItemClickListener(new AdapterView.OnItemClickListener()
		{
			public void onItemClick(AdapterView<?> parent, View v, int position, long id)
			{
				SelectedFile selectedFile = (SelectedFile) mListView.getItemAtPosition(position);

				if (selectedFile.FileObj.isFile())
				{
					if (!mMultiSelect)
					{
						ReturnResults(selectedFile.FileObj);
					}
				}
				else
				{
					mDirectory = selectedFile.FileObj;
					// Update the files list
					refreshFilesList();
				}
			}
		});

		mInstallFilesButton.setOnClickListener(new OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				ReturnResults(null);
			}
		});
	}

	private void ReturnResults(File f)
	{
		ArrayList<String> result = new ArrayList<String>();
		if (mMultiSelect)
		{
			for (int i = 0; i < mFiles.size(); ++i)
			{
				if (mFiles.get(i).IsSelected)
				{
					result.add(mFiles.get(i).FileObj.getAbsolutePath());
				}
			}
		}
		else
		{
			result.add(f.getAbsolutePath());
		}

		Intent extra = new Intent();
		extra.putStringArrayListExtra(EXTRA_FILE_PATH, result);
		setResult(RESULT_OK, extra);
		finish();
	}

	private class FilePickerListAdapter extends ArrayAdapter<SelectedFile>
	{
		private List<SelectedFile>	mObjects;

		public FilePickerListAdapter(Context context, List<SelectedFile> objects)
		{
			super(context, R.layout.file_picker_list_item, android.R.id.text1, objects);
			mObjects = objects;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent)
		{
			View row = null;

			if (convertView == null)
			{
				LayoutInflater inflater = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
				row = inflater.inflate(R.layout.file_picker_list_item, parent, false);
			}
			else
			{
				row = convertView;
			}

			CheckBox select = (CheckBox) row.findViewById(R.id.file_picker_checkbox);
			SelectedFile object = mObjects.get(position);
			select.setTag(position);

			if (mMultiSelect)
			{
				select.setChecked(object.IsSelected);

				select.setOnCheckedChangeListener(new OnCheckedChangeListener()
				{
					public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
					{
						int pos = (Integer) buttonView.getTag();
						mFiles.get(pos).IsSelected = isChecked;

						boolean foundSelected = false;
						for (int i = 0; i < mFiles.size(); ++i)
						{
							if (mFiles.get(i).IsSelected)
							{
								foundSelected = true;
								break;
							}
						}

						mInstallFilesButton.setEnabled(foundSelected);
					}
				});
			}

			ImageView imageView = (ImageView) row.findViewById(R.id.file_picker_image);
			TextView textView = (TextView) row.findViewById(R.id.file_picker_text);
			// Set single line
			textView.setSingleLine(true);

			textView.setText(object.FileObj.getName());
			if (object.FileObj.isFile())
			{
				if (FilePickerActivity.mFileType != null && FilePickerActivity.mFileType.equals("APP"))
				{
					imageView.setImageResource(R.drawable.appfile);
				}
				else
				{
					imageView.setImageResource(R.drawable.romfile);
				}

				if (mMultiSelect) select.setVisibility(View.VISIBLE);
			}
			else
			{
				// Show the folder icon
				imageView.setImageResource(R.drawable.folder);
				select.setVisibility(View.GONE);
			}

			return row;
		}
	}

	private class FileComparator implements Comparator<SelectedFile>
	{
		public int compare(SelectedFile f1, SelectedFile f2)
		{
			if (f1 == f2)
			{
				return 0;
			}
			if (f1.FileObj.isDirectory() && f2.FileObj.isFile())
			{
				// Show directories above files
				return -1;
			}
			if (f1.FileObj.isFile() && f2.FileObj.isDirectory())
			{
				// Show files below directories
				return 1;
			}
			// Sort the directories alphabetically
			return f1.FileObj.getName().compareToIgnoreCase(f2.FileObj.getName());
		}
	}

	private class ExtensionFilenameFilter implements FilenameFilter
	{
		private String[]	mExtensions;

		public ExtensionFilenameFilter(String[] extensions)
		{
			super();
			mExtensions = extensions;
		}

		public boolean accept(File dir, String filename)
		{
			if (new File(dir, filename).isDirectory())
			{
				// Accept all directory names
				return true;
			}
			if (mExtensions != null && mExtensions.length > 0)
			{
				for (int i = 0; i < mExtensions.length; i++)
				{
					if (filename.toLowerCase().endsWith(mExtensions[i].toLowerCase()))
					{
						// The filename ends with the extension
						return true;
					}
				}
				// The filename did not match any of the extensions
				return false;
			}
			// No extensions has been set. Accept all file extensions.
			return true;
		}
	}
}

class SelectedFile
{
	public File		FileObj		= null;
	public boolean	IsSelected	= false;
}
