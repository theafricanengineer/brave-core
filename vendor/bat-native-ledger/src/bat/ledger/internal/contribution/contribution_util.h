/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UTIL_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UTIL_H_

#include <map>
#include <string>

#include "bat/ledger/mojom_structs.h"

namespace braveledger_contribution {

ledger::ReportType GetReportTypeFromRewardsType(const ledger::RewardsType type);

ledger::ContributionProcessor GetProcessor(const std::string& wallet_type);

std::string GetNextProcessor(const std::string& current_processor);

bool HaveEnoughFundsToContribute(
    double* amount,
    const bool partial,
    const double balance);

void AdjustPublisherListAmounts(
    ledger::ContributionQueuePublisherList publishers,
    ledger::ContributionQueuePublisherList* publishers_new,
    ledger::ContributionQueuePublisherList* publishers_left,
    double reduce_fee_for);

int32_t GetVotesFromAmount(const double amount);

}  // namespace braveledger_contribution

#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_UTIL_H_
