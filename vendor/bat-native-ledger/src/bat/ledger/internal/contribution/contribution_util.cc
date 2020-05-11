/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/contribution/contribution_util.h"
#include "bat/ledger/internal/static_values.h"

namespace braveledger_contribution {

ledger::ReportType GetReportTypeFromRewardsType(
    const ledger::RewardsType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(ledger::RewardsType::AUTO_CONTRIBUTE): {
      return ledger::ReportType::AUTO_CONTRIBUTION;
    }
    case static_cast<int>(ledger::RewardsType::ONE_TIME_TIP): {
      return ledger::ReportType::TIP;
    }
    case static_cast<int>(ledger::RewardsType::RECURRING_TIP): {
      return ledger::ReportType::TIP_RECURRING;
    }
    default: {
      // missing conversion, returning dummy value.
      NOTREACHED();
      return ledger::ReportType::TIP;
    }
  }
}

ledger::ContributionProcessor GetProcessor(const std::string& wallet_type) {
  if (wallet_type == ledger::kWalletUnBlinded) {
    return ledger::ContributionProcessor::BRAVE_TOKENS;
  }

  if (wallet_type == ledger::kWalletAnonymous) {
    return ledger::ContributionProcessor::BRAVE_USER_FUNDS;
  }

  if (wallet_type == ledger::kWalletUphold) {
    return ledger::ContributionProcessor::UPHOLD;
  }

  return ledger::ContributionProcessor::NONE;
}

std::string GetNextProcessor(const std::string& current_processor) {
  if (current_processor == ledger::kWalletUnBlinded) {
    return ledger::kWalletAnonymous;
  }

  if (current_processor == ledger::kWalletAnonymous) {
    return ledger::kWalletUphold;
  }

  if (current_processor == ledger::kWalletUphold) {
    return "";
  }

  return ledger::kWalletUnBlinded;
}

bool HaveEnoughFundsToContribute(
    double* amount,
    const bool partial,
    const double balance) {
  DCHECK(amount);

  if (partial) {
    if (balance == 0) {
      return false;
    }

    if (*amount > balance) {
      *amount = balance;
    }

    return true;
  }

  if (*amount > balance) {
    return false;
  }

  return true;
}

void AdjustPublisherListAmounts(
    ledger::ContributionQueuePublisherList publishers,
    ledger::ContributionQueuePublisherList* publishers_new,
    ledger::ContributionQueuePublisherList* publishers_left,
    double reduce_fee_for) {
  DCHECK(publishers_new && publishers_left);

  for (auto& item : publishers) {
    if (!item) {
      continue;
    }

    if (reduce_fee_for == 0) {
      publishers_left->push_back(std::move(item));
      continue;
    }

    if (item->amount_percent <= reduce_fee_for) {
      publishers_new->push_back(item->Clone());
      reduce_fee_for -= item->amount_percent;
      continue;
    }

    if (item->amount_percent > reduce_fee_for) {
      // current list
      const auto original_weight = item->amount_percent;
      item->amount_percent = reduce_fee_for;
      publishers_new->push_back(item->Clone());

      // next list
      item->amount_percent = original_weight - reduce_fee_for;
      publishers_left->push_back(item->Clone());

      reduce_fee_for = 0;
    }
  }
}

int32_t GetVotesFromAmount(const double amount) {
  DCHECK_GT(braveledger_ledger::_vote_price, 0);
  return std::floor(amount / braveledger_ledger::_vote_price);
}

}  // namespace braveledger_contribution
