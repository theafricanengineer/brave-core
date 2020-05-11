// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Utils
import { debounce } from '../../common/debounce'

export const keyName = 'new-tab-data'

export const defaultState: NewTab.State = {
  initialDataLoaded: false,
  textDirection: window.loadTimeData.getString('textdirection'),
  featureFlagBraveNTPSponsoredImagesWallpaper: window.loadTimeData.getBoolean('featureFlagBraveNTPSponsoredImagesWallpaper'),
  showBackgroundImage: false,
  showStats: false,
  showClock: false,
  showTopSites: false,
  showRewards: false,
  showBinance: false,
  brandedWallpaperOptIn: false,
  isBrandedWallpaperNotificationDismissed: true,
  showEmptyPage: false,
  isIncognito: chrome.extension.inIncognitoContext,
  useAlternativePrivateSearchEngine: false,
  isTor: false,
  isQwant: false,
  stats: {
    adsBlockedStat: 0,
    javascriptBlockedStat: 0,
    httpsUpgradesStat: 0,
    fingerprintingBlockedStat: 0
  },
  rewardsState: {
    adsEstimatedEarnings: 0,
    balance: {
      total: 0,
      rates: {},
      wallets: {}
    },
    dismissedNotifications: [],
    enabledAds: false,
    adsSupported: false,
    enabledMain: false,
    promotions: [],
    onlyAnonWallet: false,
    totalContribution: 0.0,
    walletCreated: false,
    walletCreating: false,
    walletCreateFailed: false,
    walletCorrupted: false
  },
  currentStackWidget: '',
  removedStackWidgets: [],
  // Order is ascending, with last entry being in the foreground
  widgetStackOrder: ['binance', 'rewards'],
  binanceState: {
    userTLD: 'com',
    initialFiat: 'USD',
    initialAmount: '',
    initialAsset: 'BTC',
    userTLDAutoSet: false,
    binanceSupported: false,
    hideBalance: true,
    binanceClientUrl: '',
    userAuthed: false,
    authInProgress: false,
    btcBalanceValue: '0.00',
    accountBalances: {},
    assetBTCValues: {},
    assetBTCVolumes: {},
    assetUSDValues: {},
    btcPrice: '0.00',
    btcVolume: '0',
    assetDepositInfo: {},
    assetDepoitQRCodeSrcs: {},
    convertAssets: {},
    accountBTCValue: '0.00',
    accountBTCUSDValue: '0.00',
    disconnectInProgress: false,
    authInvalid: false,
    selectedView: 'summary'
  }
}

if (chrome.extension.inIncognitoContext) {
  defaultState.isTor = window.loadTimeData.getBoolean('isTor')
  defaultState.isQwant = window.loadTimeData.getBoolean('isQwant')
}

// For users upgrading to the new list based widget stack state,
// a list in the current format will need to be generated based on their
// previous configuration.
const getMigratedWidgetOrder = (state: NewTab.State) => {
  const {
    showRewards,
    showBinance,
    currentStackWidget
  } = state

  if (!showRewards && !showBinance) {
    return {
      widgetStackOrder: [],
      removedStackWidgets: ['rewards', 'binance']
    }
  }

  if (showRewards && !showBinance) {
    return {
      widgetStackOrder: ['rewards'],
      removedStackWidgets: ['binance']
    }
  }

  if (!showRewards && showBinance) {
    return {
      widgetStackOrder: ['binance'],
      removedStackWidgets: ['rewards']
    }
  }

  const widgetStackOrder = []
  const nonCurrentWidget = currentStackWidget === 'rewards'
    ? 'binance'
    : 'rewards'

  widgetStackOrder.push(currentStackWidget)
  widgetStackOrder.unshift(nonCurrentWidget)

  return {
    widgetStackOrder,
    removedStackWidgets: []
  }
}

export const migrateStackWidgetSettings = (state: NewTab.State) => {
  // Migrating to the new stack widget data format
  const { widgetStackOrder, removedStackWidgets } = getMigratedWidgetOrder(state)
  state.widgetStackOrder = widgetStackOrder as NewTab.StackWidget[]
  state.removedStackWidgets = removedStackWidgets as NewTab.StackWidget[]
  state.currentStackWidget = ''

  // Ensure any new stack widgets introduced are put behind
  // the others, and not re-added unecessarily if removed
  // at one point.
  const defaultWidgets = defaultState.widgetStackOrder
  defaultWidgets.map((widget: NewTab.StackWidget) => {
    if (!state.widgetStackOrder.includes(widget) &&
        !state.removedStackWidgets.includes(widget)) {
      state.widgetStackOrder.unshift(widget)
    }
  })

  return state
}

const cleanData = (state: NewTab.State) => {
  // We need to disable linter as we defined in d.ts that this values are number,
  // but we need this check to covert from old version to a new one
  /* tslint:disable */
  if (typeof state.rewardsState.adsEstimatedEarnings === 'string') {
    state.rewardsState.adsEstimatedEarnings = 0.0
  }

  if (typeof state.rewardsState.totalContribution === 'string') {
    state.rewardsState.totalContribution = 0.0
  }
  /* tslint:enable */

  return state
}

export const load = (): NewTab.State => {
  const data: string | null = window.localStorage.getItem(keyName)
  let state = defaultState
  let storedState

  if (data) {
    try {
      storedState = JSON.parse(data)
      // add defaults for non-peristant data
      state = {
        ...state,
        ...storedState
      }
    } catch (e) {
      console.error('[NewTabData] Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce<NewTab.State>((data: NewTab.State) => {
  if (data) {
    const dataToSave = {
      showEmptyPage: data.showEmptyPage,
      rewardsState: data.rewardsState,
      binanceState: data.binanceState,
      removedStackWidgets: data.removedStackWidgets,
      widgetStackOrder: data.widgetStackOrder
    }
    window.localStorage.setItem(keyName, JSON.stringify(dataToSave))
  }
}, 50)
