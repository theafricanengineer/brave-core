// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import Stats from './stats'
import TopSitesGrid from './gridSites'
import FooterInfo from './footerInfo'
import SiteRemovalNotification from './notification'
import {
  ClockWidget as Clock,
  RewardsWidget as Rewards,
  BinanceWidget as Binance
} from '../../components/default'
import * as Page from '../../components/default/page'
import BrandedWallpaperLogo from '../../components/default/brandedWallpaper/logo'

// Helpers
import VisibilityTimer from '../../helpers/visibilityTimer'
import arrayMove from 'array-move'
import { isGridSitePinned } from '../../helpers/newTabUtils'
import { generateQRData } from '../../binance-utils'

// Types
import { SortEnd } from 'react-sortable-hoc'
import * as newTabActions from '../../actions/new_tab_actions'
import * as gridSitesActions from '../../actions/grid_sites_actions'
import { getLocale } from '../../../common/locale'
import currencyData from '../../components/default/binance/data'

interface Props {
  newTabData: NewTab.State
  gridSitesData: NewTab.GridSitesState
  actions: typeof newTabActions & typeof gridSitesActions
  saveShowBackgroundImage: (value: boolean) => void
  saveShowClock: (value: boolean) => void
  saveShowTopSites: (value: boolean) => void
  saveShowStats: (value: boolean) => void
  saveShowRewards: (value: boolean) => void
  saveShowBinance: (value: boolean) => void
  saveBrandedWallpaperOptIn: (value: boolean) => void
}

interface State {
  onlyAnonWallet: boolean
  showSettingsMenu: boolean
  backgroundHasLoaded: boolean
}

function GetBackgroundImageSrc (props: Props) {
  if (!props.newTabData.showBackgroundImage &&
      (!props.newTabData.brandedWallpaperData || props.newTabData.brandedWallpaperData.isSponsored)) {
    return undefined
  }
  if (props.newTabData.brandedWallpaperData) {
    const wallpaperData = props.newTabData.brandedWallpaperData
    if (wallpaperData && wallpaperData.wallpaperImageUrl) {
      return wallpaperData.wallpaperImageUrl
    }
  }
  if (props.newTabData.backgroundImage && props.newTabData.backgroundImage.source) {
    return props.newTabData.backgroundImage.source
  }
  return undefined
}

function GetIsShowingBrandedWallpaper (props: Props) {
  const { newTabData } = props
  return (newTabData.brandedWallpaperData &&
          newTabData.brandedWallpaperData.isSponsored) ? true : false
}

function GetShouldShowBrandedWallpaperNotification (props: Props) {
  return GetIsShowingBrandedWallpaper(props) &&
  !props.newTabData.isBrandedWallpaperNotificationDismissed
}

class NewTabPage extends React.Component<Props, State> {
  state = {
    onlyAnonWallet: false,
    showSettingsMenu: false,
    backgroundHasLoaded: false
  }
  imageSource?: string = undefined
  timerIdForBrandedWallpaperNotification?: number = undefined
  onVisiblityTimerExpired = () => {
    this.dismissBrandedWallpaperNotification(false)
  }
  visibilityTimer = new VisibilityTimer(this.onVisiblityTimerExpired, 4000)

  componentDidMount () {
    // if a notification is open at component mounting time, close it
    this.props.actions.showGridSiteRemovedNotification(false)
    this.imageSource = GetBackgroundImageSrc(this.props)
    this.trackCachedImage()
    if (GetShouldShowBrandedWallpaperNotification(this.props)) {
      this.trackBrandedWallpaperNotificationAutoDismiss()
    }
  }

  componentDidUpdate (prevProps: Props) {
    const oldImageSource = GetBackgroundImageSrc(prevProps)
    const newImageSource = GetBackgroundImageSrc(this.props)
    this.imageSource = newImageSource
    if (newImageSource && oldImageSource !== newImageSource) {
      this.trackCachedImage()
    }
    if (oldImageSource &&
      !newImageSource) {
      // reset loaded state
      this.setState({ backgroundHasLoaded: false })
    }
    if (!GetShouldShowBrandedWallpaperNotification(prevProps) &&
        GetShouldShowBrandedWallpaperNotification(this.props)) {
      this.trackBrandedWallpaperNotificationAutoDismiss()
    }

    if (GetShouldShowBrandedWallpaperNotification(prevProps) &&
        !GetShouldShowBrandedWallpaperNotification(this.props)) {
      this.stopWaitingForBrandedWallpaperNotificationAutoDismiss()
    }

    // Handles updates from brave://settings/newTab
    const oldShowRewards = prevProps.newTabData.showRewards
    const oldShowBinance = prevProps.newTabData.showBinance
    const { showRewards, showBinance } = this.props.newTabData

    if (!oldShowRewards && showRewards) {
      this.props.actions.setForegroundStackWidget('rewards')
    } else if (!oldShowBinance && showBinance) {
      this.props.actions.setForegroundStackWidget('binance')
    } else if (oldShowRewards && !showRewards) {
      this.props.actions.removeStackWidget('rewards')
    } else if (oldShowBinance && !showBinance) {
      this.props.actions.removeStackWidget('binance')
    }
  }

  trackCachedImage () {
    if (this.imageSource) {
      const imgCache = new Image()
      imgCache.src = this.imageSource
      console.timeStamp('image start loading...')
      imgCache.onload = () => {
        console.timeStamp('image loaded')
        this.setState({
          backgroundHasLoaded: true
        })
      }
    }
  }

  trackBrandedWallpaperNotificationAutoDismiss () {
    // Wait until page has been visible for an uninterupted Y seconds and then
    // dismiss the notification.
    this.visibilityTimer.startTracking()
  }

  stopWaitingForBrandedWallpaperNotificationAutoDismiss () {
    this.visibilityTimer.stopTracking()
  }

  onSortEnd = ({ oldIndex, newIndex }: SortEnd) => {
    const { gridSitesData } = this.props
    // Do not update topsites order if the drag
    // destination is a pinned tile
    const gridSite = gridSitesData.gridSites[newIndex]
    if (!gridSite || isGridSitePinned(gridSite)) {
      return
    }
    const items = arrayMove(gridSitesData.gridSites, oldIndex, newIndex)
    this.props.actions.gridSitesDataUpdated(items)
  }

  toggleShowBackgroundImage = () => {
    this.props.saveShowBackgroundImage(
      !this.props.newTabData.showBackgroundImage
    )
  }

  toggleShowClock = () => {
    this.props.saveShowClock(
      !this.props.newTabData.showClock
    )
  }

  toggleShowStats = () => {
    this.props.saveShowStats(
      !this.props.newTabData.showStats
    )
  }

  toggleShowTopSites = () => {
    this.props.saveShowTopSites(
      !this.props.newTabData.showTopSites
    )
  }

  toggleShowRewards = () => {
    const { showRewards } = this.props.newTabData

    if (showRewards) {
      this.removeStackWidget('rewards')
    } else {
      this.setForegroundStackWidget('rewards')
    }

    this.props.saveShowRewards(!showRewards)
  }

  toggleShowBinance = () => {
    const { showBinance } = this.props.newTabData

    if (showBinance) {
      this.removeStackWidget('binance')
    } else {
      this.setForegroundStackWidget('binance')
    }

    this.props.saveShowBinance(!showBinance)

    // If we are about to hide the widget, disconnect
    if (showBinance) {
      chrome.binance.revokeToken(() => {
        this.disconnectBinance()
      })
    }
  }

  onBinanceClientUrl = (clientUrl: string) => {
    this.props.actions.onBinanceClientUrl(clientUrl)
  }

  onValidAuthCode = () => {
    this.props.actions.onValidAuthCode()
  }

  setHideBalance = (hide: boolean) => {
    this.props.actions.setHideBalance(hide)
  }

  disconnectBinance = () => {
    this.props.actions.disconnectBinance()
  }

  setDisconnectInProgress = () => {
    this.props.actions.setDisconnectInProgress(true)
  }

  cancelDisconnect = () => {
    this.props.actions.setDisconnectInProgress(false)
  }

  connectBinance = () => {
    this.props.actions.connectToBinance()
  }

  buyCrypto = (coin: string, amount: string, fiat: string) => {
    const { userTLD } = this.props.newTabData.binanceState
    const refCode = userTLD === 'us' ? '35089877' : '39346846'
    const refParams = `ref=${refCode}&utm_source=brave`

    if (userTLD === 'us') {
      window.open(`https://www.binance.us/en/buy-sell-crypto?crypto=${coin}&amount=${amount}&${refParams}`, '_blank', 'noopener')
    } else {
      window.open(`https://www.binance.com/en/buy-sell-crypto?fiat=${fiat}&crypto=${coin}&amount=${amount}&${refParams}`, '_blank', 'noopener')
    }
  }

  onBinanceUserTLD = (userTLD: NewTab.BinanceTLD) => {
    this.props.actions.onBinanceUserTLD(userTLD)
  }

  setBalanceInfo = (info: Record<string, Record<string, string>>) => {
    this.props.actions.onAssetsBalanceInfo(info)
  }

  setAssetDepositInfo = (symbol: string, address: string, url: string) => {
    this.props.actions.onAssetDepositInfo(symbol, address, url)
  }

  disableBrandedWallpaper = () => {
    this.props.saveBrandedWallpaperOptIn(false)
  }

  toggleShowBrandedWallpaper = () => {
    this.props.saveBrandedWallpaperOptIn(
      !this.props.newTabData.brandedWallpaperOptIn
    )
  }

  enableAds = () => {
    chrome.braveRewards.saveAdsSetting('adsEnabled', 'true')
  }

  enableRewards = () => {
    chrome.braveRewards.saveSetting('enabledMain', '1')
  }

  createWallet = () => {
    this.props.actions.createWallet()
  }

  dismissBrandedWallpaperNotification = (isUserAction: boolean) => {
    this.props.actions.dismissBrandedWallpaperNotification(isUserAction)
  }

  dismissNotification = (id: string) => {
    this.props.actions.dismissNotification(id)
  }

  closeSettings = () => {
    this.setState({ showSettingsMenu: false })
  }

  toggleSettings = () => {
    this.setState({ showSettingsMenu: !this.state.showSettingsMenu })
  }

  setForegroundStackWidget = (widget: NewTab.StackWidget) => {
    this.props.actions.setForegroundStackWidget(widget)
  }

  removeStackWidget = (widget: NewTab.StackWidget) => {
    this.props.actions.removeStackWidget(widget)
  }

  setInitialAmount = (amount: string) => {
    this.props.actions.setInitialAmount(amount)
  }

  setInitialFiat = (fiat: string) => {
    this.props.actions.setInitialFiat(fiat)
  }

  setInitialAsset = (asset: string) => {
    this.props.actions.setInitialAsset(asset)
  }

  setUserTLDAutoSet = () => {
    this.props.actions.setUserTLDAutoSet()
  }

  learnMoreRewards = () => {
    window.open('https://brave.com/brave-rewards/', '_blank', 'noopener')
  }

  learnMoreBinance = () => [
    window.open('https://brave.com/binance/', '_blank', 'noopener')
  ]

  setAssetDepositQRCodeSrc = (asset: string, src: string) => {
    this.props.actions.onDepositQRForAsset(asset, src)
  }

  setConvertableAssets = (asset: string, assets: string[]) => {
    this.props.actions.onConvertableAssets(asset, assets)
  }

  setSelectedView = (view: string) => {
    this.props.actions.setSelectedView(view)
  }

  updateActions = () => {
    this.fetchBalance()
    this.getConvertAssets()
  }

  getCurrencyList = () => {
    const { accountBalances, userTLD } = this.props.newTabData.binanceState
    const { usCurrencies, comCurrencies } = currencyData
    const baseList = userTLD === 'us' ? usCurrencies : comCurrencies

    if (!accountBalances) {
      return baseList
    }

    const accounts = Object.keys(accountBalances)
    const nonHoldingList = baseList.filter((symbol: string) => {
      return !accounts.includes(symbol)
    })

    return accounts.concat(nonHoldingList)
  }

  getConvertAssets = () => {
    chrome.binance.getConvertAssets((assets: any) => {
      for (let asset in assets) {
        if (assets[asset]) {
          this.setConvertableAssets(asset, assets[asset])
        }
      }
    })
  }

  fetchBalance = () => {
    chrome.binance.getAccountBalances((balances: Record<string, Record<string, string>>, success: boolean) => {
      const hasBalances = Object.keys(balances).length

      if (!hasBalances) {
        return
      } else if (!success) {
        this.setAuthInvalid()
        return
      }

      this.setBalanceInfo(balances)
      this.setDepositInfo()
    })
  }

  setDepositInfo = () => {
    chrome.binance.getCoinNetworks((networks: Record<string, string>) => {
      const currencies = this.getCurrencyList()
      for (let ticker in networks) {
        if (currencies.includes(ticker)) {
          chrome.binance.getDepositInfo(ticker, networks[ticker], (address: string, tag: string) => {
            this.setAssetDepositInfo(ticker, address, tag)
            generateQRData((tag || address), ticker, this.setAssetDepositQRCodeSrc)
          })
        }
      }
    })
  }

  setAuthInvalid = () => {
    this.props.actions.setAuthInvalid(true)
    this.props.actions.disconnectBinance()
  }

  dismissAuthInvalid = () => {
    this.props.actions.setAuthInvalid(false)
  }

  getCryptoContent () {
    const { widgetStackOrder } = this.props.newTabData
    const renderLookup = {
      'rewards': this.renderRewardsWidget.bind(this),
      'binance': this.renderBinanceWidget.bind(this)
    }

    return (
      <>
        {widgetStackOrder.map((widget: NewTab.StackWidget, i: number) => {
          const isForeground = i === widgetStackOrder.length - 1
          return (
            <div key={`widget-${widget}`}>
              {renderLookup[widget](isForeground)}
            </div>
          )
        })}
      </>
    )
  }

  renderCryptoContent () {
    const { newTabData } = this.props
    const { widgetStackOrder } = newTabData

    if (!widgetStackOrder.length) {
      return null
    }

    return (
      <Page.GridItemWidgetStack>
        {this.getCryptoContent()}
      </Page.GridItemWidgetStack>
    )
  }

  renderRewardsWidget (showContent: boolean) {
    const { newTabData } = this.props
    const {
      rewardsState,
      showRewards: rewardsWidgetOn,
      textDirection
    } = newTabData
    const isShowingBrandedWallpaper = GetIsShowingBrandedWallpaper(this.props)
    const shouldShowBrandedWallpaperNotification = GetShouldShowBrandedWallpaperNotification(this.props)
    const shouldShowRewardsWidget = rewardsWidgetOn || shouldShowBrandedWallpaperNotification

    if (!shouldShowRewardsWidget) {
      return null
    }

    return (
      <Rewards
        {...rewardsState}
        widgetTitle={getLocale('rewardsWidgetBraveRewards')}
        onLearnMore={this.learnMoreRewards}
        menuPosition={'left'}
        isCrypto={true}
        isCryptoTab={!showContent}
        textDirection={textDirection}
        preventFocus={false}
        hideWidget={this.toggleShowRewards}
        showContent={showContent}
        onShowContent={this.setForegroundStackWidget.bind(this, 'rewards')}
        onCreateWallet={this.createWallet}
        onEnableAds={this.enableAds}
        onEnableRewards={this.enableRewards}
        isShowingBrandedWallpaper={isShowingBrandedWallpaper}
        showBrandedWallpaperNotification={shouldShowBrandedWallpaperNotification}
        onDisableBrandedWallpaper={this.disableBrandedWallpaper}
        brandedWallpaperData={newTabData.brandedWallpaperData}
        isNotification={!rewardsWidgetOn}
        onDismissNotification={this.dismissNotification}
        onDismissBrandedWallpaperNotification={this.dismissBrandedWallpaperNotification}
      />
    )
  }

  renderBinanceWidget (showContent: boolean) {
    const { newTabData } = this.props
    const { binanceState, showBinance, textDirection } = newTabData
    const menuActions = { onLearnMore: this.learnMoreBinance }

    if (!showBinance || !binanceState.binanceSupported) {
      return null
    }

    if (binanceState.userAuthed) {
      menuActions['onDisconnect'] = this.setDisconnectInProgress
      menuActions['onRefreshData'] = this.updateActions
    }

    return (
      <Binance
        {...menuActions}
        {...binanceState}
        isCrypto={true}
        isCryptoTab={!showContent}
        menuPosition={'left'}
        widgetTitle={'Binance'}
        textDirection={textDirection}
        preventFocus={false}
        hideWidget={this.toggleShowBinance}
        showContent={showContent}
        onSetHideBalance={this.setHideBalance}
        onBinanceClientUrl={this.onBinanceClientUrl}
        onConnectBinance={this.connectBinance}
        onDisconnectBinance={this.disconnectBinance}
        onCancelDisconnect={this.cancelDisconnect}
        onValidAuthCode={this.onValidAuthCode}
        onBuyCrypto={this.buyCrypto}
        onBinanceUserTLD={this.onBinanceUserTLD}
        onShowContent={this.setForegroundStackWidget.bind(this, 'binance')}
        onSetInitialAmount={this.setInitialAmount}
        onSetInitialAsset={this.setInitialAsset}
        onSetInitialFiat={this.setInitialFiat}
        onSetUserTLDAutoSet={this.setUserTLDAutoSet}
        onUpdateActions={this.updateActions}
        onDismissAuthInvalid={this.dismissAuthInvalid}
        onSetSelectedView={this.setSelectedView}
        getCurrencyList={this.getCurrencyList}
      />
    )
  }

  render () {
    const { newTabData, gridSitesData, actions } = this.props
    const { showSettingsMenu } = this.state
    const { binanceState } = newTabData

    if (!newTabData) {
      return null
    }

    const hasImage = this.imageSource !== undefined
    const isShowingBrandedWallpaper = newTabData.brandedWallpaperData ? true : false
    const showTopSites = !!this.props.gridSitesData.gridSites.length && newTabData.showTopSites
    const cryptoContent = this.renderCryptoContent()

    return (
      <Page.App dataIsReady={newTabData.initialDataLoaded}>
        <Page.PosterBackground
          hasImage={hasImage}
          imageHasLoaded={this.state.backgroundHasLoaded}
        >
          {hasImage &&
            <img src={this.imageSource} />
          }
        </Page.PosterBackground>
        {hasImage &&
          <Page.Gradient
            imageHasLoaded={this.state.backgroundHasLoaded}
          />
        }
        <Page.Page
            showClock={newTabData.showClock}
            showStats={newTabData.showStats}
            showRewards={!!cryptoContent}
            showBinance={newTabData.showBinance}
            showTopSites={showTopSites}
            showBrandedWallpaper={isShowingBrandedWallpaper}
        >
          {newTabData.showStats &&
          <Page.GridItemStats>
            <Stats
              widgetTitle={getLocale('statsTitle')}
              textDirection={newTabData.textDirection}
              stats={newTabData.stats}
              hideWidget={this.toggleShowStats}
              menuPosition={'right'}
            />
          </Page.GridItemStats>
          }
          {newTabData.showClock &&
          <Page.GridItemClock>
            <Clock
              widgetTitle={getLocale('clockTitle')}
              textDirection={newTabData.textDirection}
              hideWidget={this.toggleShowClock}
              menuPosition={'left'}
            />
          </Page.GridItemClock>
          }
          {
            showTopSites
              ? (
              <Page.GridItemTopSites>
                <TopSitesGrid
                  actions={actions}
                  widgetTitle={getLocale('topSitesTitle')}
                  gridSites={gridSitesData.gridSites}
                  menuPosition={'right'}
                  hideWidget={this.toggleShowTopSites}
                  textDirection={newTabData.textDirection}
                />
              </Page.GridItemTopSites>
              ) : null
          }
          {
            gridSitesData.shouldShowSiteRemovedNotification
            ? (
            <Page.GridItemNotification>
              <SiteRemovalNotification actions={actions} />
            </Page.GridItemNotification>
            ) : null
          }
            {cryptoContent}
          <Page.Footer>
            <Page.FooterContent>
            {isShowingBrandedWallpaper && newTabData.brandedWallpaperData &&
            newTabData.brandedWallpaperData.logo &&
            <Page.GridItemBrandedLogo>
              <BrandedWallpaperLogo
                menuPosition={'right'}
                textDirection={newTabData.textDirection}
                data={newTabData.brandedWallpaperData.logo}
              />
            </Page.GridItemBrandedLogo>}
            <FooterInfo
              textDirection={newTabData.textDirection}
              onClickOutside={this.closeSettings}
              backgroundImageInfo={newTabData.backgroundImage}
              onClickSettings={this.toggleSettings}
              showSettingsMenu={showSettingsMenu}
              showPhotoInfo={!isShowingBrandedWallpaper && newTabData.showBackgroundImage}
              toggleShowBackgroundImage={this.toggleShowBackgroundImage}
              toggleShowClock={this.toggleShowClock}
              toggleShowStats={this.toggleShowStats}
              toggleShowTopSites={this.toggleShowTopSites}
              toggleBrandedWallpaperOptIn={this.toggleShowBrandedWallpaper}
              showBackgroundImage={newTabData.showBackgroundImage}
              showClock={newTabData.showClock}
              showStats={newTabData.showStats}
              showTopSites={newTabData.showTopSites}
              showRewards={newTabData.showRewards}
              showBinance={newTabData.showBinance}
              brandedWallpaperOptIn={newTabData.brandedWallpaperOptIn}
              allowSponsoredWallpaperUI={newTabData.featureFlagBraveNTPSponsoredImagesWallpaper}
              toggleShowRewards={this.toggleShowRewards}
              toggleShowBinance={this.toggleShowBinance}
              binanceSupported={binanceState.binanceSupported}
            />
            </Page.FooterContent>
          </Page.Footer>
        </Page.Page>
      </Page.App>
    )
  }
}

export default NewTabPage
