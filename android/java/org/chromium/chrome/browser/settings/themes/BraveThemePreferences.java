/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings.themes;

import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.UI_THEME_DARKEN_WEBSITES_ENABLED;
import static org.chromium.chrome.browser.preferences.ChromePreferenceKeys.UI_THEME_SETTING;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.preference.Preference;

import org.chromium.chrome.R;
import org.chromium.base.BuildInfo;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.preferences.SharedPreferencesManager;
import org.chromium.chrome.browser.settings.SettingsUtils;
import org.chromium.chrome.browser.settings.themes.ThemeType;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;

public class BraveThemePreferences extends ThemeSettingsFragment {

    private static final String SUPER_REFERRAL = "super_referral";

    @Override
    public void onCreatePreferences(@Nullable Bundle savedInstanceState, String rootKey) {
        SettingsUtils.addPreferencesFromResource(this, R.xml.brave_theme_preferences);
        getActivity().setTitle(getResources().getString(R.string.theme_settings));

        Profile mProfile = Profile.getLastUsedProfile();
        NTPBackgroundImagesBridge mNTPBackgroundImagesBridge = NTPBackgroundImagesBridge.getInstance(mProfile);
        if (!NTPBackgroundImagesBridge.enableSponsoredImages()
            || (mNTPBackgroundImagesBridge != null
            && !mNTPBackgroundImagesBridge.isSuperReferral())) {
            Preference superReferralPreference = getPreferenceScreen().findPreference(SUPER_REFERRAL);
            if (superReferralPreference != null) {
                getPreferenceScreen().removePreference(superReferralPreference);
            }
        }

        SharedPreferencesManager sharedPreferencesManager = SharedPreferencesManager.getInstance();
        BraveRadioButtonGroupThemePreference radioButtonGroupThemePreference =
                (BraveRadioButtonGroupThemePreference) findPreference(PREF_UI_THEME_PREF);

        int defaultThemePref = ThemeType.SYSTEM_DEFAULT;
        if (!BuildInfo.isAtLeastQ()) {
            defaultThemePref = GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                    ? ThemeType.DARK
                    : ThemeType.LIGHT;
        }
        radioButtonGroupThemePreference.initialize(
                sharedPreferencesManager.readInt(UI_THEME_SETTING, defaultThemePref),
                sharedPreferencesManager.readBoolean(UI_THEME_DARKEN_WEBSITES_ENABLED, false));

        radioButtonGroupThemePreference.setOnPreferenceChangeListener((preference, newValue) -> {
            if (ChromeFeatureList.isEnabled(
                        ChromeFeatureList.DARKEN_WEBSITES_CHECKBOX_IN_THEMES_SETTING)) {
                sharedPreferencesManager.writeBoolean(UI_THEME_DARKEN_WEBSITES_ENABLED,
                        radioButtonGroupThemePreference.isDarkenWebsitesEnabled());
            }
            int theme = (int) newValue;
            sharedPreferencesManager.writeInt(UI_THEME_SETTING, theme);
            return true;
        });
    }
}
