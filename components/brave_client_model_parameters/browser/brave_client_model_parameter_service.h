/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_CLIENT_MODEL_PARAMETERS_BRAVE_CLIENT_MODEL_PARAMETER_SERVICE_H_  // NOLINT
#define BRAVE_COMPONENTS_BRAVE_CLIENT_MODEL_PARAMETERS_BRAVE_CLIENT_MODEL_PARAMETER_SERVICE_H_  // NOLINT

#include <memory>
#include <string>
#include <map>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"

namespace brave_client_model_parameters {

using ClientModelParameters = std::string;
using ClientModelParametersList = std::map<std::string, ClientModelParameters>;

class ClientModelParameterService : public brave_component_updater::BraveComponent {  // NOLINT
 public:
  explicit ClientModelParameterService(Delegate* delegate);
  ~ClientModelParameterService() override;

  ClientModelParameterService(const ClientModelParameterService&) = delete;
  ClientModelParameterService& operator=(const ClientModelParameterService&) =
      delete;

  ClientModelParameters GetParameters(
      const std::string& model_name);

 private:
  // brave_component_updater::BraveComponent:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

  ClientModelParametersList client_model_parameter_list_;  // TODO(Moritz Haller): extract into dedicated class? NOLINT
};

}  // namespace brave_client_model_parameters

#endif  // BRAVE_COMPONENTS_BRAVE_CLIENT_MODEL_PARAMETERS_BRAVE_CLIENT_MODEL_PARAMETER_SERVICE_H_ NOLINT
