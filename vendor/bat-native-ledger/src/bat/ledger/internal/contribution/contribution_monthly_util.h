/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_MONTHLY_UTIL_H_
#define BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_MONTHLY_UTIL_H_

#include "base/values.h"
#include "bat/ledger/mojom_structs.h"

namespace braveledger_contribution {

double GetTotalFromVerifiedTips(
    const ledger::PublisherInfoList& publisher_list);

}  // namespace braveledger_contribution

#endif  // BRAVELEDGER_CONTRIBUTION_CONTRIBUTION_MONTHLY_UTIL_H_
