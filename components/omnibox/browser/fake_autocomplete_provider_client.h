/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_FAKE_AUTOCOMPLETE_PROVIDER_CLIENT_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_FAKE_AUTOCOMPLETE_PROVIDER_CLIENT_H_

#include "brave/components/omnibox/browser/suggested_sites_provider.h"

#include "components/omnibox/browser/mock_autocomplete_provider_client.h"
#include "components/prefs/testing_pref_service.h"

class FakeAutocompleteProviderClient : public MockAutocompleteProviderClient {
 public:
  FakeAutocompleteProviderClient();
  PrefService* GetPrefs() override;

 private:
  TestingPrefServiceSimple pref_service_;
  DISALLOW_COPY_AND_ASSIGN(FakeAutocompleteProviderClient);
};

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_FAKE_AUTOCOMPLETE_PROVIDER_CLIENT_H_
