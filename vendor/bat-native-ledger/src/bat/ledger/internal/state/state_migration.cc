/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_migration.h"
#include "bat/ledger/internal/state/state_util.h"
#include "bat/ledger/internal/legacy/publisher_state.h"

using std::placeholders::_1;

namespace {

const int kCurrentVersionNumber = 1;

}  // namespace

namespace braveledger_state {

StateMigration::StateMigration(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

StateMigration::~StateMigration() = default;

void StateMigration::Migrate() {
  const int current_version = GetVersion(ledger_);
  const int version = current_version + 1;

  if (current_version == kCurrentVersionNumber) {
    return;
  }

  auto migrate_callback = std::bind(&StateMigration::OnMigration,
      this,
      _1,
      version);

  switch (version) {
    case 1: {
      MigrateToVersion1(migrate_callback);
    }
  }

  NOTREACHED();
}

void StateMigration::OnMigration(ledger::Result result, int version) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
        "State: Error with migration from " <<
        (version - 1) <<
        " to " << version;
    return;
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO) <<
      "State: Migrated to version " << version;

  SetVersion(ledger_, version);
  Migrate();
}

void StateMigration::MigrateToVersion1(ledger::ResultCallback callback) {
  auto legacy_publisher_state =
      std::make_unique<braveledger_publisher::LegacyPublisherState>(ledger_);

  auto load_callback = std::bind(&StateMigration::OnLoadState,
      this,
      _1);

  legacy_publisher_state->Load(load_callback);
}

void StateMigration::OnLoadState(const ledger::Result result) {

}

}  // namespace braveledger_state
