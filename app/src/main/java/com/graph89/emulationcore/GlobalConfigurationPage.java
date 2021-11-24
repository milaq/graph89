package com.graph89.emulationcore;

import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceActivity;

import com.Bisha.TI89EmuDonation.R;
import com.graph89.common.ConfigurationHelper;
import com.graph89.controls.SeekBarPreference;

@SuppressWarnings("deprecation")
public class GlobalConfigurationPage extends PreferenceActivity implements OnSharedPreferenceChangeListener {

	private CheckBoxPreference mPrefFullscreen;
	private CheckBoxPreference mPrefKeepScreenOn;
	private SeekBarPreference mPrefAutoOff;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		this.setRequestedOrientation(EmulatorActivity.Orientation);
		addPreferencesFromResource(R.layout.settings_global);
		initSettings();
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
	}

	@Override
	protected void onPause()
	{
		getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
		super.onPause();
	}

	@Override
	protected void onDestroy()
	{
		getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
		super.onDestroy();
	}

	private void initSettings() {
		mPrefFullscreen = (CheckBoxPreference) findPreference(ConfigurationHelper.CONF_KEY_HIDE_STATUSBAR);
		mPrefFullscreen.setChecked(ConfigurationHelper.getBoolean(this,
				ConfigurationHelper.CONF_KEY_HIDE_STATUSBAR, ConfigurationHelper.CONF_DEFAULT_HIDE_STATUSBAR));

		mPrefKeepScreenOn = (CheckBoxPreference) findPreference(ConfigurationHelper.CONF_KEY_KEEP_SCREEN_ON);
		mPrefKeepScreenOn.setChecked(ConfigurationHelper.getBoolean(this,
				ConfigurationHelper.CONF_KEY_KEEP_SCREEN_ON, ConfigurationHelper.CONF_DEFAULT_KEEP_SCREEN_ON));

		mPrefAutoOff = (SeekBarPreference) getPreferenceScreen().findPreference(ConfigurationHelper.CONF_KEY_AUTO_OFF);
		mPrefAutoOff.setCurrentValue(ConfigurationHelper.getInt(this,
				ConfigurationHelper.CONF_KEY_AUTO_OFF, ConfigurationHelper.CONF_DEFAULT_AUTO_OFF));
		mPrefAutoOff.ValuePost = " min";
		mPrefAutoOff.ValueMIN = "Never";
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
		if (key.equals(mPrefFullscreen.getKey())) {
			ConfigurationHelper.writeBoolean(this, ConfigurationHelper.CONF_KEY_HIDE_STATUSBAR,
					mPrefFullscreen.isChecked());
		} else if (key.equals(mPrefKeepScreenOn.getKey())) {
			ConfigurationHelper.writeBoolean(this, ConfigurationHelper.CONF_KEY_KEEP_SCREEN_ON,
					mPrefKeepScreenOn.isChecked());
		} else if (key.equals(mPrefAutoOff.getKey())) {
			ConfigurationHelper.writeInt(this, ConfigurationHelper.CONF_KEY_AUTO_OFF,
					mPrefAutoOff.getCurrentValue());
		}
	}
}