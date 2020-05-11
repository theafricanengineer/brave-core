/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_SKU_COMMON_H_
#define BRAVELEDGER_SKU_COMMON_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/sku/sku_order.h"
#include "bat/ledger/internal/sku/sku_transaction.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_sku {

class SKUCommon {
 public:
  explicit SKUCommon(bat_ledger::LedgerImpl* ledger);
  ~SKUCommon();

  void CreateOrder(
      const std::vector<ledger::SKUOrderItem>& items,
      ledger::SKUOrderCallback callback);

  void CreateTransaction(
      ledger::SKUOrderPtr order,
      const std::string& destination,
      const ledger::ExternalWallet& wallet,
      ledger::SKUOrderCallback callback);

  void SendExternalTransaction(
      const std::string& order_id,
      ledger::SKUOrderCallback callback);

 private:
  void OnTransactionCompleted(
      const ledger::Result result,
      const std::string& order_id,
      ledger::SKUOrderCallback callback);

  void GetSKUTransactionByOrderId(
      ledger::SKUTransactionPtr transaction,
      ledger::SKUOrderCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<SKUOrder> order_;
  std::unique_ptr<SKUTransaction> transaction_;
};

}  // namespace braveledger_sku

#endif  // BRAVELEDGER_SKU_COMMON_H_
