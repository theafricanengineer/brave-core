/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_client_model_parameters/brave_client_model_parameter_service.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/task/post_task.h"

namespace brave_client_model_parameters {

namespace {

constexpr char kComponentName[] = "BraveClientModelParameters";
constexpr char kComponentId[] = "";
constexpr char kComponentPublicKey[] = "";

}  // namespace

ClientModelParameterService::ClientModelParameterService(Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate) {
  Register(kComponentName, kComponentId, kComponentPublicKey);
}

ClientModelParameterService::~ClientModelParameterService() = default;

void ClientModelParameterService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  // TODO(Moritz Haller): Implement updates
}

}  // namespace brave_client_model_parameters
