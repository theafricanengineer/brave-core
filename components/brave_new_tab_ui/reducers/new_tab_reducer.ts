// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { Reducer } from 'redux'

// Constants
import { types, DismissBrandedWallpaperNotificationPayload } from '../constants/new_tab_types'
import { Stats } from '../api/stats'
import { PrivateTabData } from '../api/privateTabData'

// API
import * as backgroundAPI from '../api/background'
import { InitialData, InitialRewardsData, PreInitialRewardsData } from '../api/initialData'
import { registerViewCount } from '../api/brandedWallpaper'
import * as preferencesAPI from '../api/preferences'
import * as storage from '../storage/new_tab_storage'
import { getTotalContributions } from '../rewards-utils'
import { isValidClientURL } from '../binance-utils'

const initialState = storage.load()

let sideEffectState: NewTab.State = initialState

type SideEffectFunction = (currentState: NewTab.State) => void

function performSideEffect (fn: SideEffectFunction): void {
  window.setTimeout(() => fn(sideEffectState), 0)
}

export const newTabReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State | undefined, action) => {
  console.timeStamp('reducer ' + action.type)
  if (state === undefined) {
    console.timeStamp('reducer init')
    state = initialState
  }

  const startingState = state
  const payload = action.payload

  switch (action.type) {
    case types.NEW_TAB_SET_INITIAL_DATA:
      const initialDataPayload = payload as InitialData
      state = {
        ...state,
        initialDataLoaded: true,
        ...initialDataPayload.preferences,
        stats: initialDataPayload.stats,
        brandedWallpaperData: initialDataPayload.brandedWallpaperData,
        ...initialDataPayload.privateTabData
      }
      if (state.brandedWallpaperData && !state.brandedWallpaperData.isSponsored) {
        // Update feature flag if this is super referral wallpaper.
        state = {
          ...state,
          featureFlagBraveNTPSponsoredImagesWallpaper: false
        }
      }
      // TODO(petemill): only get backgroundImage if no sponsored background this time.
      // ...We would also have to set the value at the action
      // the branded wallpaper is turned off. Since this is a cheap string API
      // (no image will be downloaded), we can afford to leave this here for now.
      if (initialDataPayload.preferences.showBackgroundImage) {
        state.backgroundImage = backgroundAPI.randomBackgroundImage()
      }
      console.timeStamp('reducer initial data received')

      performSideEffect(async function (state) {
        if (!state.isIncognito) {
          try {
            await registerViewCount()
          } catch (e) {
            console.error('Error calling registerViewCount', e)
          }
        }
      })

      if (state.currentStackWidget) {
        state = storage.migrateStackWidgetSettings(state)
      }

      break

    case types.NEW_TAB_STATS_UPDATED:
      const stats: Stats = payload.stats
      state = {
        ...state,
        stats
      }
      break

    case types.NEW_TAB_PRIVATE_TAB_DATA_UPDATED:
      const privateTabData = payload as PrivateTabData
      state = {
        ...state,
        useAlternativePrivateSearchEngine: privateTabData.useAlternativePrivateSearchEngine
      }
      break

    case types.NEW_TAB_DISMISS_BRANDED_WALLPAPER_NOTIFICATION:
      const { isUserAction } = payload as DismissBrandedWallpaperNotificationPayload
      // Save persisted data.
      preferencesAPI.saveIsBrandedWallpaperNotificationDismissed(true)
      // Only change current data if user explicitly took an action (e.g. clicked
      // on the "Close notification Button" - x).
      if (isUserAction) {
        state = {
          ...state,
          isBrandedWallpaperNotificationDismissed: true
        }
      }
      break

    case types.NEW_TAB_PREFERENCES_UPDATED:
      const preferences = payload as preferencesAPI.Preferences
      const newState = {
        ...state,
        ...preferences
      }
      // We don't want to update dismissed status of branded wallpaper notification
      // since this can happen automatically when the notification is counted as
      // 'viewed', but we want to keep showing it until the page is navigated away from
      // or refreshed.
      newState.isBrandedWallpaperNotificationDismissed = state.isBrandedWallpaperNotificationDismissed
      // Remove branded wallpaper when opting out or turning wallpapers off
      const hasTurnedBrandedWallpaperOff = !preferences.brandedWallpaperOptIn && state.brandedWallpaperData
      const hasTurnedWallpaperOff = !preferences.showBackgroundImage && state.showBackgroundImage
      // We always show SR images regardless of background options state.
      const isSuperReferral = state.brandedWallpaperData && !state.brandedWallpaperData.isSponsored
      if (!isSuperReferral &&
          (hasTurnedBrandedWallpaperOff || (state.brandedWallpaperData && hasTurnedWallpaperOff))) {
        newState.brandedWallpaperData = undefined
      }
      // Get a new wallpaper image if turning that feature on
      const shouldChangeBackgroundImage =
        !state.showBackgroundImage && preferences.showBackgroundImage
      if (shouldChangeBackgroundImage) {
        newState.backgroundImage = backgroundAPI.randomBackgroundImage()
      }
      state = newState
      break

    case types.CREATE_WALLET:
      chrome.braveRewards.createWallet()
      state = { ...state }
      state.rewardsState.walletCreating = true
      break

    case types.ON_ENABLED_MAIN:
      state = { ...state }
      state.rewardsState.enabledMain = payload.enabledMain
      if (payload.enabledAds !== undefined) {
        state.rewardsState.enabledAds = payload.enabledAds
      }
      break

    case types.ON_WALLET_INITIALIZED: {
      const result: NewTab.RewardsResult = payload.result
      state = { ...state }

      switch (result) {
        case NewTab.RewardsResult.WALLET_CORRUPT:
          state.rewardsState.walletCorrupted = true
          break
        case NewTab.RewardsResult.WALLET_CREATED:
          state.rewardsState.walletCreated = true
          state.rewardsState.walletCreateFailed = false
          state.rewardsState.walletCreating = false
          state.rewardsState.walletCorrupted = false
          chrome.braveRewards.saveAdsSetting('adsEnabled', 'true')
          break
        case NewTab.RewardsResult.LEDGER_OK:
          state.rewardsState.walletCreateFailed = true
          state.rewardsState.walletCreating = false
          state.rewardsState.walletCreated = false
          state.rewardsState.walletCorrupted = false
          break
      }
      break
    }

    case types.ON_ADS_ENABLED:
      state = { ...state }
      state.rewardsState.enabledAds = payload.enabled
      break

    case types.ON_ADS_ESTIMATED_EARNINGS:
      state = { ...state }
      state.rewardsState.adsEstimatedEarnings = payload.amount
      break

    case types.ON_BALANCE_REPORT:
      state = { ...state }
      const report = payload.report || {}
      state.rewardsState.totalContribution = getTotalContributions(report)
      break

    case types.DISMISS_NOTIFICATION:
      state = { ...state }
      const dismissedNotifications = state.rewardsState.dismissedNotifications
      dismissedNotifications.push(payload.id)
      state.rewardsState.dismissedNotifications = dismissedNotifications

      state.rewardsState.promotions = state.rewardsState.promotions.filter((promotion) => {
        return promotion.promotionId !== payload.id
      })
      break

    case types.ON_PROMOTIONS: {
      if (action.payload.result === 1) {
        break
      }

      const promotions = payload.promotions

      state = { ...state }

      if (!state.rewardsState.promotions) {
        state.rewardsState.promotions = []
      }

      promotions.forEach((promotion: NewTab.Promotion) => {
        if (!state || !state.rewardsState) {
          return
        }

        if (!state.rewardsState.dismissedNotifications) {
          state.rewardsState.dismissedNotifications = []
        }

        if (state.rewardsState.dismissedNotifications.indexOf(promotion.promotionId) > -1) {
          return
        }

        const hasPromotion = state.rewardsState.promotions.find((promotion: NewTab.Promotion) => {
          return promotion.promotionId === promotion.promotionId
        })
        if (hasPromotion) {
          return
        }

        const updatedPromotions = state.rewardsState.promotions
        updatedPromotions.push({
          promotionId: promotion.promotionId,
          type: promotion.type
        })

        state.rewardsState.promotions = updatedPromotions
      })

      break
    }

    case types.ON_PROMOTION_FINISH:
      if (payload.result !== 0) {
        break
      }

      if (!state.rewardsState.promotions) {
        state.rewardsState.promotions = []
      }

      state = { ...state }
      const oldNotifications = state.rewardsState.dismissedNotifications

      oldNotifications.push(payload.promotion.promotionId)
      state.rewardsState.dismissedNotifications = oldNotifications

      state.rewardsState.promotions = state.rewardsState.promotions.filter((promotion: NewTab.Promotion) => {
        return promotion.promotionId !== payload.promotion.promotionId
      })
      break

    case types.ON_BALANCE:
      state = { ...state }
      state.rewardsState.balance = payload.balance
      break

    case types.ON_WALLET_EXISTS:
      if (!payload.exists || state.rewardsState.walletCreated) {
        break
      }
      state.rewardsState.walletCreated = true
      break

    case types.SET_PRE_INITIAL_REWARDS_DATA:
      const preInitialRewardsDataPayload = payload as PreInitialRewardsData
      state = {
        ...state,
        rewardsState: {
          ...state.rewardsState,
          enabledAds: preInitialRewardsDataPayload.enabledAds,
          adsSupported: preInitialRewardsDataPayload.adsSupported,
          enabledMain: preInitialRewardsDataPayload.enabledMain
        }
      }
      break

    case types.SET_INITIAL_REWARDS_DATA:
      const initialRewardsDataPayload = payload as InitialRewardsData
      const newRewardsState = {
        balance: initialRewardsDataPayload.balance,
        totalContribution: getTotalContributions(initialRewardsDataPayload.report),
        adsEstimatedEarnings: initialRewardsDataPayload.adsEstimatedEarnings
      }

      state = {
        ...state,
        rewardsState: {
          ...state.rewardsState,
          ...newRewardsState
        }
      }
      break

    case types.REMOVE_STACK_WIDGET:
      const widget: NewTab.StackWidget = payload.widget
      let { removedStackWidgets, widgetStackOrder } = state

      if (!widgetStackOrder.length) {
        break
      }

      widgetStackOrder = widgetStackOrder.filter((curWidget: NewTab.StackWidget) => {
        return curWidget !== widget
      })

      if (!removedStackWidgets.includes(widget)) {
        removedStackWidgets.push(widget)
      }

      state = {
        ...state,
        removedStackWidgets,
        widgetStackOrder
      }
      break

    case types.SET_FOREGROUND_STACK_WIDGET:
      const frontWidget: NewTab.StackWidget = payload.widget
      let newWidgetStackOrder = state.widgetStackOrder

      newWidgetStackOrder = newWidgetStackOrder.filter((widget: NewTab.StackWidget) => {
        return widget !== frontWidget
      })

      newWidgetStackOrder.push(frontWidget)

      state = {
        ...state,
        widgetStackOrder: newWidgetStackOrder
      }
      break

    case types.ON_BINANCE_USER_TLD:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          userTLD: payload.userTLD
        }
      }
      break

    case types.SET_INITIAL_AMOUNT:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          initialAmount: payload.initialAmount
        }
      }
      break

    case types.SET_INITIAL_FIAT:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          initialFiat: payload.initialFiat
        }
      }
      break

    case types.SET_INITIAL_ASSET:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          initialAsset: payload.initialAsset
        }
      }
      break

    case types.SET_USER_TLD_AUTO_SET:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          userTLDAutoSet: true
        }
      }
      break

    case types.SET_ONLY_ANON_WALLET:
      state = { ...state }
      state = {
        ...state,
        rewardsState: {
          ...state.rewardsState,
          onlyAnonWallet: payload.onlyAnonWallet
        }
      }
      break

    case types.SET_BINANCE_SUPPORTED:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          binanceSupported: payload.supported
        }
      }
      break

    case types.SET_HIDE_BALANCE:
      state = { ...state }
      state.binanceState.hideBalance = payload.hide
      break

    case types.ON_VALID_AUTH_CODE:
      state = { ...state }
      state.binanceState.userAuthed = true
      state.binanceState.authInProgress = false
      break

    case types.DISCONNECT_BINANCE:
      state = { ...state }
      state.binanceState = {
        ...storage.defaultState.binanceState,
        binanceSupported: true
      }
      break

    case types.CONNECT_TO_BINANCE:
      state = { ...state }
      state.binanceState.authInProgress = true
      break

    case types.ON_BINANCE_CLIENT_URL:
      const { clientUrl } = payload

      if (!isValidClientURL(clientUrl)) {
        break
      }

      state = { ...state }
      state.binanceState.binanceClientUrl = clientUrl
      break

    case types.ON_ASSETS_BALANCE_INFO:
      state = { ...state }
      const balances = payload.info

      if (!state.binanceState.accountBalances) {
        state.binanceState.accountBalances = {}
      }

      if (!state.binanceState.assetUSDValues) {
        state.binanceState.assetUSDValues = {}
      }

      let totalBtcValue = 0.00
      let totalUSDValue = 0.00

      for (let ticker in balances) {
        const balance = balances[ticker].balance
        const usdValue = balances[ticker].fiatValue
        const btcValue = balances[ticker].btcValue
        const assetUSDValue = parseFloat(usdValue).toFixed(2)

        state.binanceState.accountBalances[ticker] = balance
        state.binanceState.assetUSDValues[ticker] = assetUSDValue.toString()

        totalUSDValue += parseFloat(usdValue)
        totalBtcValue += parseFloat(btcValue)
      }

      const usdValue = totalUSDValue.toFixed(2).toString()
      state.binanceState.accountBTCUSDValue = usdValue
      state.binanceState.accountBTCValue = totalBtcValue.toString()
      break

    case types.ON_ASSET_DEPOSIT_INFO:
      const { symbol, address, tag } = payload
      if (!symbol || (!address && !tag)) {
        break
      }

      state = { ...state }
      if (!state.binanceState.assetDepositInfo) {
        state.binanceState.assetDepositInfo = {}
      }
      state.binanceState.assetDepositInfo[symbol] = {
        address,
        tag
      }
      break

    case types.ON_DEPOSIT_QR_FOR_ASSET:
      const { asset, imageSrc } = payload
      if (!asset || !imageSrc) {
        break
      }

      state = { ...state }
      if (!state.binanceState.assetDepoitQRCodeSrcs) {
        state.binanceState.assetDepoitQRCodeSrcs = {}
      }
      state.binanceState.assetDepoitQRCodeSrcs[asset] = imageSrc
      break

    case types.ON_CONVERTABLE_ASSETS:
      const { convertAsset, assets } = payload
      if (!convertAsset || !assets) {
        break
      }

      state = { ...state }
      if (!state.binanceState.convertAssets) {
        state.binanceState.convertAssets = {}
      }
      state.binanceState.convertAssets[convertAsset] = assets
      break

    case types.SET_DISCONNECT_IN_PROGRESS:
      const { inProgress } = payload
      state = { ...state }
      state.binanceState = {
        ...state.binanceState,
        disconnectInProgress: inProgress
      }
      break

    case types.SET_AUTH_INVALID:
      const { authInvalid } = payload
      state = { ...state }
      state.binanceState = {
        ...state.binanceState,
        authInvalid
      }
      break

    case types.SET_SELECTED_VIEW:
      const { view } = payload
      state = { ...state }
      state.binanceState = {
        ...state.binanceState,
        selectedView: view
      }
      break

    default:
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  sideEffectState = state
  return state
}

export default newTabReducer
