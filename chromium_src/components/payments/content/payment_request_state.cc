/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/payments/content/payment_request.h"

#define BRAVE_BAT_PAYMENT_UI_ON_PAYMENT_APP_CREATED \
    if (base::StartsWith( \
            available_apps_.back().get()->GetLabel(), \
            base::ASCIIToUTF16("bat"), \
            base::CompareCase::SENSITIVE)) { \
      SetSelectedApp(available_apps_.back().get(), \
          SectionSelectionStatus::kAddedSelected); \
    } \

#include "../../../../../components/payments/content/payment_request_state.cc"
#undef BRAVE_BAT_PAYMENT_UI_ON_PAYMENT_APP_CREATED
