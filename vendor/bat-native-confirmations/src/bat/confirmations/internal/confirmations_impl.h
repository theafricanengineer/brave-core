/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_IMPL_H_
#define BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_IMPL_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "bat/confirmations/confirmations.h"
#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/ad_notification_info.h"
#include "bat/confirmations/issuers_info.h"
#include "bat/confirmations/internal/confirmation_info.h"
#include "bat/confirmations/internal/ads_rewards.h"

#include "base/values.h"

namespace confirmations {

class UnblindedTokens;
class RefillTokens;
class RedeemToken;
class PayoutTokens;

class ConfirmationsImpl : public Confirmations {
 public:
  explicit ConfirmationsImpl(ConfirmationsClient* confirmations_client);
  ~ConfirmationsImpl() override;

  ConfirmationsClient* get_client() const;

  void Initialize(OnInitializeCallback callback) override;

  // Wallet
  void SetWalletInfo(std::unique_ptr<WalletInfo> info) override;

  // Catalog issuers
  void SetCatalogIssuers(std::unique_ptr<IssuersInfo> info) override;
  std::map<std::string, std::string> GetCatalogIssuers() const;
  bool IsValidPublicKeyForCatalogIssuers(const std::string& public_key) const;
  double GetEstimatedRedemptionValue(const std::string& public_key) const;

  // Confirmations
  void AppendConfirmationToQueue(const ConfirmationInfo& confirmation_info);
  void StartRetryingFailedConfirmations();

  // Ads rewards
  void UpdateAdsRewards(const bool should_refresh) override;

  void UpdateAdsRewards(
      const double estimated_pending_rewards,
      const uint64_t next_payment_date_in_seconds);

  // Transaction history
  void GetTransactionHistory(
      OnGetTransactionHistoryCallback callback) override;
  void AddUnredeemedTransactionsToPendingRewards();
  void AddTransactionsToPendingRewards(
      const TransactionList& transactions);
  double GetEstimatedPendingRewardsForTransactions(
      const TransactionList& transactions) const;
  uint64_t GetAdNotificationsReceivedThisMonthForTransactions(
      const TransactionList& transactions) const;
  TransactionList GetTransactionHistory(
      const uint64_t from_timestamp_in_seconds,
      const uint64_t to_timestamp_in_seconds);
  TransactionList GetTransactions() const;
  TransactionList GetUnredeemedTransactions();
  void AppendTransactionToHistory(
      const double estimated_redemption_value,
      const ConfirmationType confirmation_type);

  // Refill tokens
  void StartRetryingToGetRefillSignedTokens(const uint64_t start_timer_in);
  void RefillTokensIfNecessary() const;

  // Payout tokens
  uint64_t GetNextTokenRedemptionDateInSeconds() const;

  // Redeem unblinded tokens
  void ConfirmAd(
      const AdInfo& info,
      const ConfirmationType confirmation_type) override;
  void ConfirmAction(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const ConfirmationType confirmation_type) override;

  // State
  virtual void SaveState();

 private:
  bool is_initialized_;
  OnInitializeCallback initialize_callback_;

  void MaybeStart();

  // Wallet
  WalletInfo wallet_info_;
  std::string public_key_;

  // Catalog issuers
  std::map<std::string, std::string> catalog_issuers_;

  // Confirmations
  Timer failed_confirmations_timer_;
  void RemoveConfirmationFromQueue(const ConfirmationInfo& confirmation_info);
  void RetryFailedConfirmations();
  ConfirmationList confirmations_;

  // Transaction history
  TransactionList transaction_history_;

  // Unblinded tokens
  std::unique_ptr<UnblindedTokens> unblinded_tokens_;
  void NotifyAdsIfConfirmationsIsReady();

  std::unique_ptr<UnblindedTokens> unblinded_payment_tokens_;

  // Ads rewards
  double estimated_pending_rewards_;
  uint64_t next_payment_date_in_seconds_;
  std::unique_ptr<AdsRewards> ads_rewards_;

  std::unique_ptr<RefillTokens> refill_tokens_;

  std::unique_ptr<RedeemToken> redeem_token_;

  std::unique_ptr<PayoutTokens> payout_tokens_;

  // State
  void OnStateSaved(const Result result);

  bool state_has_loaded_;
  void LoadState();
  void OnStateLoaded(Result result, const std::string& json);

  void ResetState();
  void OnStateReset(const Result result);

  std::string ToJSON() const;

  base::Value GetCatalogIssuersAsDictionary(
      const std::string& public_key,
      const std::map<std::string, std::string>& issuers) const;

  base::Value GetConfirmationsAsDictionary(
      const ConfirmationList& confirmations) const;

  base::Value GetTransactionHistoryAsDictionary(
      const TransactionList& transaction_history) const;

  bool FromJSON(const std::string& json);

  bool ParseCatalogIssuersFromJSON(
      base::DictionaryValue* dictionary);
  bool GetCatalogIssuersFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* public_key,
      std::map<std::string, std::string>* issuers) const;

  bool ParseNextTokenRedemptionDateInSecondsFromJSON(
      base::DictionaryValue* dictionary);

  bool ParseConfirmationsFromJSON(
      base::DictionaryValue* dictionary);
  bool GetConfirmationsFromDictionary(
      base::DictionaryValue* dictionary,
      ConfirmationList* confirmations);

  bool ParseTransactionHistoryFromJSON(
      base::DictionaryValue* dictionary);
  bool GetTransactionHistoryFromDictionary(
      base::DictionaryValue* dictionary,
      TransactionList* transaction_history);

  bool ParseUnblindedTokensFromJSON(
      base::DictionaryValue* dictionary);

  bool ParseUnblindedPaymentTokensFromJSON(
      base::DictionaryValue* dictionary);

  // Confirmations::Client
  ConfirmationsClient* confirmations_client_;  // NOT OWNED

  // Not copyable, not assignable
  ConfirmationsImpl(const ConfirmationsImpl&) = delete;
  ConfirmationsImpl& operator=(const ConfirmationsImpl&) = delete;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_CONFIRMATIONS_IMPL_H_
