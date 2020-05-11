/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"

#define BRAVE_GET_RENDER_CONTENT_SETTING_RULES                                \
  map->GetSettingsForOneType(ContentSettingsType::AUTOPLAY,                   \
                             ResourceIdentifier(), &(rules->autoplay_rules)); \
  if (base::FeatureList::IsEnabled(                                           \
          brave_shields::features::kFingerprintingProtectionV2)) {            \
    map->GetSettingsForOneType(ContentSettingsType::PLUGINS,                  \
                               brave_shields::kFingerprintingV2,              \
                               &(rules->fingerprinting_rules));               \
  } else {                                                                    \
    map->GetSettingsForOneType(ContentSettingsType::PLUGINS,                  \
                               brave_shields::kFingerprinting,                \
                               &(rules->fingerprinting_rules));               \
  }                                                                           \
  map->GetSettingsForOneType(ContentSettingsType::PLUGINS,                    \
                             brave_shields::kBraveShields,                    \
                             &(rules->brave_shields_rules));

#include "../../../../../components/content_settings/core/browser/content_settings_utils.cc"  // NOLINT

#undef BRAVE_BRAVE_GET_RENDER_CONTENT_SETTING_RULES
