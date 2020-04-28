/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <iostream>

#include "brave/components/brave_usermodel_parameters/browser/parameters.h"
#include "brave/components/brave_usermodel_parameters/browser/models.h"

namespace brave_usermodel_parameters {

Parameters::Parameters() = default;
Parameters::~Parameters() = default;

ParametersInfo Parameters::GetParametersInfo(
    const std::string& model_id) {
  ParametersInfo info;
  if (auto maybe_model = Models::GetModelInfo(model_id)) {
    info = parameters_.at(model_id);
    return info;
  }

  return info;
}

uint16_t Parameters::GetSchemaVersion() {
  return schema_version_;
}

void Parameters::SetSchemaVersion(uint16_t version) {
  schema_version_ = version;
}

void Parameters::AddParametersInfo(
    const ParametersInfo& info) {
  if (auto maybe_model = Models::GetModelInfo(info.model_id)) {
    parameters_.insert({info.model_id, info});
  }
}

}  // namespace brave_usermodel_parameters
