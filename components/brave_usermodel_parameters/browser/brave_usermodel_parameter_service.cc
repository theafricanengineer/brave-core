/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/task/post_task.h"
#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/brave_usermodel_parameters/browser/brave_usermodel_parameter_service.h"
#include "brave/components/brave_usermodel_parameters/browser/regions.h"

namespace brave_usermodel_parameters {

constexpr char kComponentName[] = "BraveUsermodelParameters";
constexpr char kSchemaVersionPath[] = "schemaVersion";
constexpr char kModelsPath[] = "models";
constexpr char kModelsIdPath[] = "id";
constexpr char kModelsFilenamePath[] = "filename";
constexpr char kModelsVersionPath[] = "version";

constexpr base::FilePath::CharType kManifestFile[] =
    FILE_PATH_LITERAL("models.json");

UsermodelParameterService::UsermodelParameterService(
    Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate) {
  // TODO(Moritz Haller): Return when testing vs. mock/fake object?
  if (!delegate)
    return;

  const std::string locale =
      brave_ads::LocaleHelper::GetInstance()->GetLocale();
  const std::string country_code =
      brave_ads::LocaleHelper::GetCountryCode(locale);
  const auto& info = Regions::GetRegionInfo(country_code);

  if (!info) {
    // TODO(Moritz Haller): Right log channel/level?
    DVLOG(2) << "No client parameter component for " << country_code;
    return;
  }

  Register(kComponentName, info->component_id,
      info->component_base64_public_key);
}

UsermodelParameterService::~UsermodelParameterService() = default;

std::string GetManifest(
    const base::FilePath& manifest_path) {
  std::string contents;
  bool success = base::ReadFileToString(manifest_path, &contents);
  if (!success || contents.empty()) {
    DVLOG(2) << __func__ << ": cannot read manifest file " << manifest_path;
    return contents;
  }

  return contents;
}

void UsermodelParameterService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&GetManifest, install_dir.Append(kManifestFile)),
      base::BindOnce(&UsermodelParameterService::OnGetManifest,
          weak_factory_.GetWeakPtr(), install_dir));
}

void UsermodelParameterService::OnGetManifest(
    const base::FilePath& install_dir,
    const std::string& manifest_json) {
  parameters_.reset(new Parameters());

  // TODO(Moritz Haller): Run outside main thread?
  base::Optional<base::Value> manifest = base::JSONReader::Read(manifest_json);
  if (!manifest) {
    DVLOG(2) << "Reading manifest json failed";
    return;
  }

  // TODO(Moritz Haller): Add schema version check
  if (base::Optional<int> version = manifest->FindIntPath(kSchemaVersionPath)) {
    parameters_->SetSchemaVersion(*version);
  }

  if (auto* models = manifest->FindListPath(kModelsPath)) {
    for (const auto& model : models->GetList()) {
      ParametersInfo info;
      if (auto* maybe_id = model.FindStringPath(kModelsIdPath)) {
        info.model_id = *maybe_id;
      }

      if (auto* maybe_version = model.FindStringPath(kModelsVersionPath)) {
        info.version = *maybe_version;
      }

      if (auto* maybe_path = model.FindStringPath(kModelsFilenamePath)) {
        info.parameter_file = install_dir.AppendASCII(*maybe_path);
      }

      // TODO(Moritz Haller): check if logic is sound, no matter if has values
      // assigned add info and |AddParametersInfo| will validate if it should
      // be added
      parameters_->AddParametersInfo(info);
    }
  }
}

ParametersInfo UsermodelParameterService::GetParametersForModel(
    const std::string& model_id) {
  ParametersInfo parameters;

  if (model_id.empty()) {
    return parameters;
  }

  return parameters_->GetParametersInfo(model_id);
}

}  // namespace brave_usermodel_parameters
