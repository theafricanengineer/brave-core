/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/credentials/credentials_sku.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_sku.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/static_values.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

bool IsPublicKeyValid(const std::string& public_key) {
  if (public_key.empty()) {
    return false;
  }

  std::vector<std::string> keys;
  if (ledger::_environment == ledger::Environment::PRODUCTION) {
    keys = {
        "yr4w9Y0XZQISBOToATNEl5ADspDUgm7cBSOhfYgPWx4=",  // AC
        "PGLvfpIn8QXuQJEtv2ViQSWw2PppkhexKr1mlvwCpnM="  // User funds
    };
  }

  if (ledger::_environment == ledger::Environment::STAGING) {
    keys = {
        "mMMWZrWPlO5b9IB8vF5kUJW4f7ULH1wuEop3NOYqNW0=",  // AC
        "CMezK92X5wmYHVYpr22QhNsTTq6trA/N9Alw+4cKyUY="  // User funds
    };
  }

  if (ledger::_environment == ledger::Environment::DEVELOPMENT) {
    keys = {
        "RhfxGp4pT0Kqe2zx4+q+L6lwC3G9v3fIj1L+PbINNzw=",  // AC
        "nsSoWgGMJpIiCGVdYrne03ldQ4zqZOMERVD5eSPhhxc="  // User funds
    };
  }

  auto it = std::find(keys.begin(), keys.end(), public_key);

  return it != keys.end();
}

std::string ConvertItemTypeToString(const std::string& type) {
  int type_int;
  base::StringToInt(type, &type_int);
  switch (static_cast<ledger::SKUOrderItemType>(type_int)) {
    case ledger::SKUOrderItemType::SINGLE_USE: {
      return "single-use";
    }
    case ledger::SKUOrderItemType::NONE: {
      return "";
    }
  }
}

}  // namespace

namespace braveledger_credentials {

CredentialsSKU::CredentialsSKU(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    common_(std::make_unique<CredentialsCommon>(ledger))  {
  DCHECK(ledger_ && common_);
}

CredentialsSKU::~CredentialsSKU() = default;

void CredentialsSKU::Start(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  DCHECK_EQ(trigger.data.size(), 2ul);
  if (trigger.data.empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Trigger data is missing";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto get_callback = std::bind(&CredentialsSKU::OnStart,
      this,
      _1,
      trigger,
      callback);

  ledger_->GetCredsBatchByTrigger(trigger.id, trigger.type, get_callback);
}

void CredentialsSKU::OnStart(
    ledger::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  ledger::CredsBatchStatus status = ledger::CredsBatchStatus::NONE;
  if (creds) {
    status = creds->status;
  }

  switch (status) {
    case ledger::CredsBatchStatus::NONE:
    case ledger::CredsBatchStatus::BLINDED: {
      Blind(trigger, callback);
      break;
    }
    case ledger::CredsBatchStatus::CLAIMED: {
      FetchSignedCreds(trigger, callback);
      break;
    }
    case ledger::CredsBatchStatus::SIGNED: {
      auto get_callback = std::bind(&CredentialsSKU::Unblind,
          this,
          _1,
          trigger,
          callback);
      ledger_->GetCredsBatchByTrigger(trigger.id, trigger.type, get_callback);
      break;
    }
    case ledger::CredsBatchStatus::FINISHED: {
      callback(ledger::Result::LEDGER_OK);
      break;
    }
  }
}

void CredentialsSKU::Blind(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  auto blinded_callback = std::bind(&CredentialsSKU::Claim,
      this,
      _1,
      _2,
      trigger,
      callback);
  common_->GetBlindedCreds(trigger, blinded_callback);
}

void CredentialsSKU::Claim(
    const ledger::Result result,
    const std::string& blinded_creds_json,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Claim failed";
    callback(result);
    return;
  }

  auto blinded_creds = ParseStringToBaseList(blinded_creds_json);

  if (!blinded_creds || blinded_creds->empty()) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR)
        << "Blinded creds are corrupted";
    callback(ledger::Result::RETRY);
    return;
  }

  DCHECK_EQ(trigger.data.size(), 2ul);
  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("itemId", trigger.data[0]);
  body.SetStringKey("type", ConvertItemTypeToString(trigger.data[1]));
  body.SetKey("blindedCreds", base::Value(std::move(*blinded_creds)));

  std::string json;
  base::JSONWriter::Write(body, &json);

  const std::string sign_url = base::StringPrintf(
      "post /v1/orders/%s/credentials",
      trigger.id.c_str());

  const std::string url =
      braveledger_request_util::GetOrderCredentialsURL(trigger.id);
  auto url_callback = std::bind(&CredentialsSKU::OnClaim,
      this,
      _1,
      _2,
      _3,
      trigger,
      callback);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      json,
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void CredentialsSKU::OnClaim(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::RETRY);
    return;
  }

  auto save_callback = std::bind(&CredentialsSKU::ClaimStatusSaved,
      this,
      _1,
      trigger,
      callback);

  ledger_->UpdateCredsBatchStatus(
      trigger.id,
      trigger.type,
      ledger::CredsBatchStatus::CLAIMED,
      save_callback);
}

void CredentialsSKU::ClaimStatusSaved(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Claim status not saved";
    callback(ledger::Result::RETRY);
    return;
  }

  FetchSignedCreds(trigger, callback);
}

void CredentialsSKU::FetchSignedCreds(
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  const std::string url = braveledger_request_util::GetOrderCredentialsURL(
      trigger.id,
      trigger.data[0]);
  auto url_callback = std::bind(&CredentialsSKU::OnFetchSignedCreds,
      this,
      _1,
      _2,
      _3,
      trigger,
      callback);

  ledger_->LoadURL(
      url,
      std::vector<std::string>(),
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void CredentialsSKU::OnFetchSignedCreds(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code == net::HTTP_ACCEPTED) {
    callback(ledger::Result::RETRY_SHORT);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::RETRY);
    return;
  }

  auto get_callback = std::bind(&CredentialsSKU::SignedCredsSaved,
      this,
      _1,
      trigger,
      callback);
  common_->GetSignedCredsFromResponse(trigger, response, get_callback);
}

void CredentialsSKU::SignedCredsSaved(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Signed creds were not saved";
    callback(ledger::Result::RETRY);
    return;
  }

  auto get_callback = std::bind(&CredentialsSKU::Unblind,
      this,
      _1,
      trigger,
      callback);
  ledger_->GetCredsBatchByTrigger(trigger.id, trigger.type, get_callback);
}

void CredentialsSKU::Unblind(
    ledger::CredsBatchPtr creds,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (!creds) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Corrupted data";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  if (!IsPublicKeyValid(creds->public_key)) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Public key is not valid";
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> unblinded_encoded_creds;
  std::string error;
  bool result;
  if (ledger::is_testing) {
    result = UnBlindCredsMock(*creds, &unblinded_encoded_creds);
  } else {
    result = UnBlindCreds(*creds, &unblinded_encoded_creds, &error);
  }

  if (!result) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "UnBlindTokens: " << error;
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto save_callback = std::bind(&CredentialsSKU::Completed,
      this,
      _1,
      trigger,
      callback);

  const uint64_t expires_at = 0ul;

  common_->SaveUnblindedCreds(
      expires_at,
      braveledger_ledger::_vote_price,
      *creds,
      unblinded_encoded_creds,
      trigger,
      save_callback);
}

void CredentialsSKU::Completed(
    const ledger::Result result,
    const CredentialsTrigger& trigger,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) << "Unblinded token save failed";
    callback(result);
    return;
  }

  ledger_->UnblindedTokensReady();
  callback(result);
}

void CredentialsSKU::RedeemTokens(
    const CredentialsRedeem& redeem,
    ledger::ResultCallback callback) {
  common_->RedeemTokens(redeem, callback);
}

}  // namespace braveledger_credentials
