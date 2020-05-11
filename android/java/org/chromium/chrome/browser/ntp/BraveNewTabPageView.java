/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.content.Context;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import android.view.ContextMenu;
import android.widget.LinearLayout;
import android.view.LayoutInflater;
import android.view.Gravity;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.view.View;
import android.view.ViewTreeObserver;
import android.content.res.Configuration;
import android.os.Build;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.support.design.widget.FloatingActionButton;
import java.util.List;
import android.view.MenuItem;

import org.chromium.base.TraceEvent;
import org.chromium.chrome.R;
import org.chromium.base.task.AsyncTask;
import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.ntp.NewTabPageView;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.BravePrefServiceBridge;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.tile.TileGroup;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabImpl;
import org.chromium.chrome.browser.ntp_background_images.model.NTPImage;
import org.chromium.chrome.browser.ntp_background_images.model.BackgroundImage;
import org.chromium.chrome.browser.ntp_background_images.model.Wallpaper;
import org.chromium.chrome.browser.ntp_background_images.model.TopSite;
import org.chromium.chrome.browser.ntp_background_images.model.SponsoredTab;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;
import org.chromium.chrome.browser.ntp_background_images.util.NewTabPageListener;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;
import org.chromium.chrome.browser.ntp_background_images.util.FetchWallpaperWorkerTask;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.SuperReferralShareDialogFragment;
import org.chromium.chrome.browser.tab.EmptyTabObserver;
import org.chromium.chrome.browser.tab.TabObserver;
import org.chromium.chrome.browser.tab.TabAttributes;
import org.chromium.chrome.browser.night_mode.GlobalNightModeStateProviderHolder;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.local_database.TopSiteTable;
import org.chromium.chrome.browser.offlinepages.OfflinePageBridge;
import org.chromium.chrome.browser.offlinepages.RequestCoordinatorBridge;

public class BraveNewTabPageView extends NewTabPageView {
    private static final String TAG = "BraveNewTabPageView";

    private static final String PREF_TRACKERS_BLOCKED_COUNT = "trackers_blocked_count";
    private static final String PREF_ADS_BLOCKED_COUNT = "ads_blocked_count";
    private static final String PREF_HTTPS_UPGRADES_COUNT = "https_upgrades_count";
    private static final short MILLISECONDS_PER_ITEM = 50;
    private static final int BOTTOM_TOOLBAR_HEIGHT = 56;

    private TextView mAdsBlockedCountTextView;
    private TextView mDataSavedValueTextView;
    private TextView mEstTimeSavedCountTextView;
    private TextView mAdsBlockedTextView;
    private TextView mDataSavedTextView;
    private TextView mEstTimeSavedTextView;
    private ImageView bgImageView;
    private Profile mProfile;

    private TabImpl mTabImpl;
    private Tab mTab;
    private SponsoredTab sponsoredTab;

    private NewTabPageLayout mNewTabPageLayout;
    private BitmapDrawable imageDrawable;

    private FetchWallpaperWorkerTask mWorkerTask;
    private boolean isFromBottomSheet;
    private NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;
    private DatabaseHelper mDatabaseHelper;

    // TODO - call NTPBackgroundImagesBridge.getCurrentWallpaper
    // if null then display regular background image
    // on every NTP load call NTPBackgroundImagesBridge.registerPageView()
    public BraveNewTabPageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mProfile = Profile.getLastUsedProfile();
        mNewTabPageLayout = getNewTabPageLayout();
        mNTPBackgroundImagesBridge = NTPBackgroundImagesBridge.getInstance(mProfile);
        mNTPBackgroundImagesBridge.setNewTabPageListener(newTabPageListener);
        mDatabaseHelper = DatabaseHelper.getInstance();
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (sponsoredTab == null)
            initilizeSponsoredTab();
        checkAndShowNTPImage();
    }

    @Override
    protected void onDetachedFromWindow() {
        if(mWorkerTask != null && mWorkerTask.getStatus() == AsyncTask.Status.RUNNING) {
            mWorkerTask.cancel(true);
            mWorkerTask = null;
        }

        if(!isFromBottomSheet){
            mNewTabPageLayout.setBackgroundResource(0);
            if (imageDrawable != null && imageDrawable.getBitmap() != null && !imageDrawable.getBitmap().isRecycled()) {
                imageDrawable.getBitmap().recycle();
            }
        }
        super.onDetachedFromWindow();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (sponsoredTab != null && NTPUtil.shouldEnableNTPFeature(sponsoredTab.isMoreTabs())) {
            NTPImage ntpImage = sponsoredTab.getTabNTPImage();
            if(ntpImage == null) {
                sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            } else if (ntpImage instanceof Wallpaper) {
                Wallpaper mWallpaper = (Wallpaper) ntpImage;
                if(mWallpaper == null) {
                    sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
                }
            }
            checkForNonDistruptiveBanner(ntpImage);
            super.onConfigurationChanged(newConfig);
            showNTPImage(ntpImage);
        } else {
            super.onConfigurationChanged(newConfig);
        }
    }

    @Override
    public void initialize(NewTabPageManager manager, Tab tab, TileGroup.Delegate tileGroupDelegate,
            boolean searchProviderHasLogo, boolean searchProviderIsGoogle, int scrollPosition,
            long constructedTimeNs, ActivityLifecycleDispatcher activityLifecycleDispatcher) {
        super.initialize(manager, tab, tileGroupDelegate,
            searchProviderHasLogo, searchProviderIsGoogle, scrollPosition,
            constructedTimeNs, activityLifecycleDispatcher);

        mTabImpl = (TabImpl) tab;
        mTab = tab;
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        ViewGroup braveStatsView = (ViewGroup) getNewTabPageLayout().findViewById(R.id.brave_stats);
        mAdsBlockedCountTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_ads_count);
        mDataSavedValueTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_data_saved_value);
        mEstTimeSavedCountTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_time_count);

        mAdsBlockedTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_ads);
        mDataSavedTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_data_saved_text);
        mEstTimeSavedTextView = (TextView) braveStatsView.findViewById(R.id.brave_stats_text_time);
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility) {
        super.onWindowVisibilityChanged(visibility);
        if (visibility == VISIBLE) {
            updateBraveStats();
        }
    }

    /**
     * Sets up Brave stats.
     */
    private void updateBraveStats() {
        TraceEvent.begin(TAG + ".updateBraveStats()");
        long trackersBlockedCount = BravePrefServiceBridge.getInstance().getTrackersBlockedCount(mProfile);
        long adsBlockedCount = BravePrefServiceBridge.getInstance().getAdsBlockedCount(mProfile);
        long dataSaved = BravePrefServiceBridge.getInstance().getDataSaved(mProfile);
        long estimatedMillisecondsSaved = (trackersBlockedCount + adsBlockedCount) * MILLISECONDS_PER_ITEM;

        mAdsBlockedCountTextView.setText(getBraveStatsStringFormNumber(adsBlockedCount, false));
        mDataSavedValueTextView.setText(getBraveStatsStringFormNumber(dataSaved, true));
        mEstTimeSavedCountTextView.setText(getBraveStatsStringFromTime(estimatedMillisecondsSaved / 1000));

        if((BravePrefServiceBridge.getInstance().getBoolean(BravePref.NTP_SHOW_BACKGROUND_IMAGE)
            || NTPUtil.isReferralEnabled())
            && sponsoredTab != null && NTPUtil.shouldEnableNTPFeature(sponsoredTab.isMoreTabs())) {
            mAdsBlockedTextView.setTextColor(mNewTabPageLayout.getResources().getColor(android.R.color.white));
            mDataSavedTextView.setTextColor(mNewTabPageLayout.getResources().getColor(android.R.color.white));
            mEstTimeSavedTextView.setTextColor(mNewTabPageLayout.getResources().getColor(android.R.color.white));
            mEstTimeSavedCountTextView.setTextColor(mNewTabPageLayout.getResources().getColor(android.R.color.white));
        }

        TraceEvent.end(TAG + ".updateBraveStats()");
    }

    /*
    * Gets string view of specific number for Brave stats
    */
    private String getBraveStatsStringFormNumber(long number, boolean isBytes) {
        String result = "";
        String suffix = "";
        long base = isBytes ? 1024L : 1000L;
        if (number >= base * base * base) {
            result = result + (number / (base * base * base));
            number = number % (base * base * base);
            result = result + "." + (number / (10L * base * base));
            suffix = isBytes ? "Gb" : "B";
        } else if (number >= (10L * base * base) && number < (base * base * base)) {
            result = result + (number / (base * base));
            suffix = isBytes ? "Mb" : "M";
        } else if (number >= (base * base) && number < (10L * base * base)) {
            result = result + (number / (base * base));
            number = number % (base * base);
            result = result + "." + (number / (100L * base));
            suffix = isBytes ? "Mb" : "M";
        } else if (number >= (10L * base) && number < (base * base)) {
            result = result + (number / base);
            suffix = isBytes ? "Kb" : "K";
        } else if (number >= base && number < (10L * base)) {
            result = result + (number / base);
            number = number % base;
            result = result + "." + (number / 100L);
            suffix = isBytes ? "Kb" : "K";
        } else {
            result = result + number;
        }
        result = result + suffix;
        return result;
    }

    /*
    * Gets string view of specific time in seconds for Brave stats
    */
    private String getBraveStatsStringFromTime(long seconds) {
        String result = "";
        if (seconds > 24 * 60 * 60) {
            result = result + (seconds / (24 * 60 * 60)) + "d";
        }
        else if (seconds > 60 * 60) {
            result = result + (seconds / (60 * 60)) + "h";
        }
        else if (seconds > 60) {
            result = result + (seconds / 60) + "m";
        }
        else {
            result = result + seconds + "s";
        }
        return result;
    }

    private void showNTPImage(NTPImage ntpImage) {
        NTPUtil.updateOrientedUI(mTabImpl.getActivity(), mNewTabPageLayout);
        ImageView mSponsoredLogo = (ImageView)mNewTabPageLayout.findViewById(R.id.sponsored_logo);
        FloatingActionButton mSuperReferralLogo = (FloatingActionButton) mNewTabPageLayout.findViewById(R.id.super_referral_logo);
        if (ntpImage instanceof Wallpaper 
                && NTPUtil.isReferralEnabled()
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            setBackgroundImage(ntpImage);
            mSuperReferralLogo.setVisibility(View.VISIBLE);
            int floatingButtonIcon = GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                    ? R.drawable.qrcode_dark
                    : R.drawable.qrcode_light;
            mSuperReferralLogo.setImageResource(floatingButtonIcon);
            mSuperReferralLogo.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    SuperReferralShareDialogFragment mSuperReferralShareDialogFragment = new SuperReferralShareDialogFragment();
                    mSuperReferralShareDialogFragment.show(mTabImpl.getActivity().getSupportFragmentManager(), "SuperReferralShareDialogFragment");
                }
            });
        } else if(BravePrefServiceBridge.getInstance().getBoolean(BravePref.NTP_SHOW_BACKGROUND_IMAGE)
            && sponsoredTab != null && NTPUtil.shouldEnableNTPFeature(sponsoredTab.isMoreTabs())) {
            setBackgroundImage(ntpImage);
            if (ntpImage instanceof BackgroundImage){
                BackgroundImage backgroundImage = (BackgroundImage) ntpImage;
                mSponsoredLogo.setVisibility(View.GONE);
                mSuperReferralLogo.setVisibility(View.GONE);
                if (backgroundImage.getImageCredit() != null) {
                    String imageCreditStr = String.format(mNewTabPageLayout.getResources().getString(R.string.photo_by, backgroundImage.getImageCredit().getName()));

                    SpannableStringBuilder spannableString = new SpannableStringBuilder(imageCreditStr);
                    spannableString.setSpan(new android.text.style.StyleSpan(android.graphics.Typeface.BOLD), ((imageCreditStr.length()-1) - (backgroundImage.getImageCredit().getName().length()-1)), imageCreditStr.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

                    TextView creditText = (TextView)mNewTabPageLayout.findViewById(R.id.credit_text);
                    creditText.setText(spannableString);
                    creditText.setVisibility(View.VISIBLE);
                    creditText.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            if (backgroundImage.getImageCredit() != null) {
                                NTPUtil.openUrlInSameTab(backgroundImage.getImageCredit().getUrl());
                            }
                        }
                    });
                }
            }
        }
    }

    private void setBackgroundImage(NTPImage ntpImage) {
        bgImageView = (ImageView)mNewTabPageLayout.findViewById(R.id.bg_image_view);
        bgImageView.setScaleType(ImageView.ScaleType.MATRIX);

        ViewTreeObserver observer = bgImageView.getViewTreeObserver();
        observer.addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                mWorkerTask = new FetchWallpaperWorkerTask(ntpImage, bgImageView.getMeasuredWidth(), bgImageView.getMeasuredHeight(), wallpaperRetrievedCallback);
                mWorkerTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);

                bgImageView.getViewTreeObserver().removeOnGlobalLayoutListener(this);
            }
        });
    }

    private void checkForNonDistruptiveBanner(NTPImage ntpImage) {
        int brOption = NTPUtil.checkForNonDistruptiveBanner(ntpImage, sponsoredTab);
        if (SponsoredImageUtil.BR_INVALID_OPTION != brOption 
            && !NTPUtil.isReferralEnabled()) {
            NTPUtil.showNonDistruptiveBanner(mTabImpl.getActivity(), mNewTabPageLayout, brOption, sponsoredTab, newTabPageListener);
        }
    }

    private void checkAndShowNTPImage() {
        NTPImage ntpImage = sponsoredTab.getTabNTPImage();
        if(ntpImage == null) {
            sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
        } else if (ntpImage instanceof Wallpaper) {
            Wallpaper mWallpaper = (Wallpaper) ntpImage;
            if(mWallpaper == null) {
                sponsoredTab.setNTPImage(SponsoredImageUtil.getBackgroundImage());
            }
        }
        checkForNonDistruptiveBanner(ntpImage);
        showNTPImage(ntpImage);
    }

    private void initilizeSponsoredTab() {
        if (TabAttributes.from(mTab).get(String.valueOf((mTabImpl).getId())) == null) {
            SponsoredTab mSponsoredTab = new SponsoredTab(mNTPBackgroundImagesBridge);
            TabAttributes.from(mTab).set(String.valueOf((mTabImpl).getId()), mSponsoredTab);
        }
        sponsoredTab = TabAttributes.from(mTab).get(String.valueOf((mTabImpl).getId()));
        if (mNTPBackgroundImagesBridge.isSuperReferral()
            && NTPBackgroundImagesBridge.enableSponsoredImages()
            && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
            mNTPBackgroundImagesBridge.getTopSites();
    }

    private TabObserver mTabObserver = new EmptyTabObserver() {
        @Override
        public void onInteractabilityChanged(Tab tab, boolean interactable) {
            // Force a layout update if the tab is now in the foreground.
            if (interactable) {
                if (sponsoredTab == null)
                    initilizeSponsoredTab();
                if(!sponsoredTab.isMoreTabs()) {
                    checkAndShowNTPImage();
                }
            } else {
                if(!isFromBottomSheet){
                    mNewTabPageLayout.setBackgroundResource(0);
                    if (imageDrawable != null && imageDrawable.getBitmap() != null && !imageDrawable.getBitmap().isRecycled()) {
                        imageDrawable.getBitmap().recycle();
                    }
                }
            }
        }
    };

    private NewTabPageListener newTabPageListener = new NewTabPageListener() {
        @Override
        public void updateInteractableFlag(boolean isBottomSheet) {
            isFromBottomSheet = isBottomSheet;
        }

        @Override
        public void updateNTPImage() {
            if (sponsoredTab == null)
                initilizeSponsoredTab();
            checkAndShowNTPImage();
        }

        @Override
        public void updateTopSites(List<TopSite> topSites) {
            new AsyncTask<Void>() {
                @Override
                protected Void doInBackground() {
                    for(TopSite topSite : topSites) {
                        mDatabaseHelper.insertTopSite(topSite);
                    }
                    return null;
                }

                @Override
                protected void onPostExecute(Void result) {
                    assert ThreadUtils.runningOnUiThread();
                    if (isCancelled()) return;

                    List<TopSiteTable> topSites = mDatabaseHelper.getAllTopSites();
                    loadTopSites(topSites);
                }
            }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    };

    private FetchWallpaperWorkerTask.WallpaperRetrievedCallback wallpaperRetrievedCallback= new FetchWallpaperWorkerTask.WallpaperRetrievedCallback() {
        @Override
        public void bgWallpaperRetrieved(Bitmap bgWallpaper) {
            bgImageView.setImageBitmap(bgWallpaper);
        }

        @Override
        public void logoRetrieved(Wallpaper mWallpaper, Bitmap logoWallpaper) {
            if(!NTPUtil.isReferralEnabled()) {
                FloatingActionButton mSuperReferralLogo = (FloatingActionButton) mNewTabPageLayout.findViewById(R.id.super_referral_logo);
                mSuperReferralLogo.setVisibility(View.GONE);

                ImageView sponsoredLogo = (ImageView)mNewTabPageLayout.findViewById(R.id.sponsored_logo);
                sponsoredLogo.setVisibility(View.VISIBLE);
                sponsoredLogo.setImageBitmap(logoWallpaper);
                sponsoredLogo.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        if (mWallpaper.getLogoDestinationUrl() != null) {
                            NTPUtil.openUrlInSameTab(mWallpaper.getLogoDestinationUrl());
                        }
                    }
                });
            }
        }
    };

    private void loadTopSites(List<TopSiteTable> topSites) {
        LinearLayout superReferralSitesLayout = (LinearLayout) mNewTabPageLayout.findViewById(R.id.ntp_super_referral_sites_layout);
        LayoutInflater inflater = (LayoutInflater) mTabImpl.getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);

        for(TopSiteTable topSite : topSites) {
            final View view = inflater.inflate(R.layout.suggestions_tile_view, null);

            TextView tileViewTitleTv = view.findViewById(R.id.tile_view_title);
            tileViewTitleTv.setText(topSite.getName());

            if (!GlobalNightModeStateProviderHolder.getInstance().isInNightMode()
                && !BravePrefServiceBridge.getInstance().getBoolean(BravePref.NTP_SHOW_BACKGROUND_IMAGE)
                && !NTPUtil.isReferralEnabled()) {
                tileViewTitleTv.setTextColor(getResources().getColor(android.R.color.black));
            } else {
                tileViewTitleTv.setTextColor(getResources().getColor(android.R.color.white));
            }

            ImageView iconIv = view.findViewById(R.id.tile_view_icon);
            if (NTPUtil.imageCache.get(topSite.getDestinationUrl()) == null) {
                NTPUtil.imageCache.put(topSite.getDestinationUrl(), new java.lang.ref.SoftReference(NTPUtil.getTopSiteBitmap(topSite.getImagePath())));
            }
            iconIv.setImageBitmap(NTPUtil.imageCache.get(topSite.getDestinationUrl()).get());
            iconIv.setClickable(false);

            view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    NTPUtil.openUrlInSameTab(topSite.getDestinationUrl());
                }
            });

            int paddingTop = getResources().getDimensionPixelSize(R.dimen.tile_grid_layout_no_logo_padding_top);
            view.setPadding(0, paddingTop, 0, view.getPaddingBottom());

            LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.MATCH_PARENT);
            layoutParams.weight = 0.25f;
            layoutParams.gravity = Gravity.CENTER;
            view.setLayoutParams(layoutParams);
            view.setOnCreateContextMenuListener(new View.OnCreateContextMenuListener() {
                @Override
                public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
                    menu.add(R.string.contextmenu_open_in_new_tab).setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            NTPUtil.openNewTab(false, topSite.getDestinationUrl());
                            return true;
                        }
                    });
                    menu.add(R.string.contextmenu_open_in_incognito_tab).setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            NTPUtil.openNewTab(true, topSite.getDestinationUrl());
                            return true;
                        }
                    });
                    menu.add(R.string.contextmenu_save_link).setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            RequestCoordinatorBridge.getForProfile(mProfile).savePageLater(
                                topSite.getDestinationUrl(), OfflinePageBridge.NTP_SUGGESTIONS_NAMESPACE, true /* userRequested */);
                            return true;
                        }
                    });
                    menu.add(R.string.remove).setOnMenuItemClickListener(new MenuItem.OnMenuItemClickListener() {
                        @Override
                        public boolean onMenuItemClick(MenuItem item) {
                            NTPUtil.imageCache.remove(topSite.getDestinationUrl());
                            mDatabaseHelper.deleteTopSite(topSite.getDestinationUrl());
                            NTPUtil.addToRemovedTopSite(topSite.getDestinationUrl());
                            superReferralSitesLayout.removeView(view);
                            return true;
                        }
                    });
                }
            });
            superReferralSitesLayout.addView(view);
        }
    }
}
