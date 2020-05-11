/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_L10N_COMMON_LOCALE_UTIL_H_
#define BRAVE_COMPONENTS_L10N_COMMON_LOCALE_UTIL_H_

#include <string>

namespace brave_l10n {

const char kDefaultLanguage[] = "en";
const char kDefaultRegion[] = "US";

std::string GetLanguageCode(
    const std::string& locale);

std::string GetRegionCode(
    const std::string& locale);

}  // namespace brave_l10n

#endif  // BRAVE_COMPONENTS_L10N_COMMON_LOCALE_UTIL_H_
