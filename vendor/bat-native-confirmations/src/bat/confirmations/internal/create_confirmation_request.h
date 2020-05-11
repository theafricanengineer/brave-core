/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_CREATE_CONFIRMATION_REQUEST_H_
#define BAT_CONFIRMATIONS_INTERNAL_CREATE_CONFIRMATION_REQUEST_H_

#include <string>
#include <vector>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/confirmation_type.h"

#include "wrapper.hpp"  // NOLINT

namespace confirmations {

class ConfirmationsImpl;
struct ConfirmationInfo;
struct TokenInfo;

using challenge_bypass_ristretto::BlindedToken;

class CreateConfirmationRequest {
 public:
  CreateConfirmationRequest(
      ConfirmationsImpl* confirmations);

  ~CreateConfirmationRequest();

  std::string BuildUrl(
    const std::string& confirmation_id,
    const std::string& credential) const;

  URLRequestMethod GetMethod() const;

  std::string BuildBody(
      const std::string& payload) const;

  std::vector<std::string> BuildHeaders() const;
  std::string GetAcceptHeaderValue() const;

  std::string GetContentType() const;

  std::string CreateConfirmationRequestDTO(
      const ConfirmationInfo& info,
      const std::string& build_channel,
      const std::string& platform) const;

  std::string CreateCredential(
      const TokenInfo& token_info,
      const std::string& payload) const;

 private:
  bool IsLargeAnonymityCountryCode(
      const std::string& country_code) const;

  bool IsOtherCountryCode(
      const std::string& country_code) const;

  ConfirmationsImpl* confirmations_;  // NOT OWNED
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_CREATE_CONFIRMATION_REQUEST_H_
