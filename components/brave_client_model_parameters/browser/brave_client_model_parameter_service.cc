/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_client_model_parameters/browser/brave_client_model_parameter_service.h"
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/task/post_task.h"

namespace brave_client_model_parameters {

namespace {

constexpr char kComponentName[] = "BraveClientModelParameters";
constexpr char kComponentId[] = "";  // TODO(Moritz Haller): region specific, look up from map NOLINT
constexpr char kComponentPublicKey[] = "";  // TODO(Moritz Haller): region specific, look up from map NOLINT
std::vector<std::string> kModels = {
    "purchase_intent_classifier",
    "contextual_page_classifier"};  // TODO(Moritz Haller): extract into dedicated model registry NOLINT

}  // namespace

ClientModelParameterService::ClientModelParameterService(Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate) {
  // TODO(Moritz Haller): Rather solve with mock/fake object
  if (!delegate)
    return;

  Register(kComponentName, kComponentId, kComponentPublicKey);
  std::cout << "***DEBUG 1 register component\n";
}

ClientModelParameterService::~ClientModelParameterService() = default;

void ClientModelParameterService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  // TODO(Moritz Haller): Implement updates
  std::cout << "***DEBUG 2 component ready\n";
}

ClientModelParameters ClientModelParameterService::GetParameters(
    const std::string& model_name) {
  ClientModelParameters parameters;
  if (model_name.empty()) {
    return parameters;
  }

  if (std::find(kModels.begin(), kModels.end(), model_name) != kModels.end()) {
    parameters = R"({"foo": "bar})";
    return parameters;
  }

  return parameters;
}

}  // namespace brave_client_model_parameters
