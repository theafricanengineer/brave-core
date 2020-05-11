/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>
#include <fstream>
#include <sstream>

#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"

using std::placeholders::_1;

using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;

namespace ads {

class AdsTabsTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockAdsClient> mock_ads_client_;
  std::unique_ptr<AdsImpl> ads_;

  AdsTabsTest() :
      mock_ads_client_(std::make_unique<MockAdsClient>()),
      ads_(std::make_unique<AdsImpl>(mock_ads_client_.get())) {
  }

  ~AdsTabsTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    EXPECT_CALL(*mock_ads_client_, IsEnabled())
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_ads_client_, Load(_, _))
        .WillRepeatedly(
            Invoke([this](
                const std::string& name,
                LoadCallback callback) {
              auto path = GetTestDataPath();
              path = path.AppendASCII(name);

              std::string value;
              if (!Load(path, &value)) {
                callback(FAILED, value);
                return;
              }

              callback(SUCCESS, value);
            }));

    ON_CALL(*mock_ads_client_, Save(_, _, _))
        .WillByDefault(
            Invoke([](
                const std::string& name,
                const std::string& value,
                ResultCallback callback) {
              callback(SUCCESS);
            }));

    const std::vector<std::string> user_model_languages = {"en", "de", "fr"};
    EXPECT_CALL(*mock_ads_client_, GetUserModelLanguages())
        .WillRepeatedly(Return(user_model_languages));

    EXPECT_CALL(*mock_ads_client_, LoadUserModelForLanguage(_, _))
        .WillRepeatedly(
            Invoke([this](
                const std::string& language,
                LoadCallback callback) {
              auto path = GetResourcesPath();
              path = path.AppendASCII("user_models");
              path = path.AppendASCII("languages");
              path = path.AppendASCII(language);
              path = path.AppendASCII("user_model.json");

              std::string value;
              if (!Load(path, &value)) {
                callback(FAILED, value);
                return;
              }

              callback(SUCCESS, value);
            }));

    EXPECT_CALL(*mock_ads_client_, LoadJsonSchema(_))
        .WillRepeatedly(
            Invoke([this](
                const std::string& name) -> std::string {
              auto path = GetTestDataPath();
              path = path.AppendASCII(name);

              std::string value;
              Load(path, &value);

              return value;
            }));

    auto callback = std::bind(&AdsTabsTest::OnInitialize, this, _1);
    ads_->Initialize(callback);
  }

  void OnInitialize(const Result result) {
    EXPECT_EQ(Result::SUCCESS, result);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  base::FilePath GetTestDataPath() {
    base::FilePath path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("brave/vendor/bat-native-ads/test/data");
    return path;
  }

  base::FilePath GetResourcesPath() {
    base::FilePath path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("brave/vendor/bat-native-ads/resources");
    return path;
  }

  bool Load(const base::FilePath path, std::string* value) {
    if (!value) {
      return false;
    }

    std::ifstream ifs{path.value().c_str()};
    if (ifs.fail()) {
      *value = "";
      return false;
    }

    std::stringstream stream;
    stream << ifs.rdbuf();
    *value = stream.str();
    return true;
  }
};

TEST_F(AdsTabsTest, Media_IsPlaying) {
  // Arrange
  ads_->OnTabUpdated(1, "https://brave.com", true, false);
  ads_->OnMediaPlaying(1);

  // Act
  auto is_playing = ads_->IsMediaPlaying();

  // Assert
  EXPECT_TRUE(is_playing);
}

TEST_F(AdsTabsTest, Media_NotPlaying) {
  // Arrange
  ads_->OnTabUpdated(1, "https://brave.com", true, false);

  ads_->OnMediaPlaying(1);
  ads_->OnMediaPlaying(2);

  ads_->OnMediaStopped(1);
  ads_->OnMediaStopped(2);

  // Act
  auto is_playing = ads_->IsMediaPlaying();

  // Assert
  EXPECT_FALSE(is_playing);
}

TEST_F(AdsTabsTest, TabUpdated_Incognito) {
  // Arrange
  EXPECT_CALL(*mock_ads_client_, VerboseLog(_, _, _, _))
      .Times(0);

  // Act
  ads_->OnTabUpdated(1, "https://brave.com", true, true);

  // Assert
}

TEST_F(AdsTabsTest, TabUpdated_InactiveIncognito) {
  // Arrange
  EXPECT_CALL(*mock_ads_client_, VerboseLog(_, _, _, _))
      .Times(0);

  // Act
  ads_->OnTabUpdated(1, "https://brave.com", false, true);

  // Assert
}

TEST_F(AdsTabsTest, TabUpdated_Active) {
  // Arrange
  EXPECT_CALL(*mock_ads_client_, VerboseLog(_, _, _, _))
      .Times(2);

  // Act
  ads_->OnTabUpdated(1, "https://brave.com", true, false);

  // Assert
}

TEST_F(AdsTabsTest, TabUpdated_Inactive) {
  // Arrange
  EXPECT_CALL(*mock_ads_client_, VerboseLog(_, _, _, _))
      .Times(2);

  // Act
  ads_->OnTabUpdated(1, "https://brave.com", false, false);

  // Assert
}

TEST_F(AdsTabsTest, TabClosed_WhileMediaIsPlaying) {
  // Arrange
  ads_->OnMediaPlaying(1);

  // Act
  ads_->OnTabClosed(1);

  // Assert
  EXPECT_FALSE(ads_->IsMediaPlaying());
}

}  // namespace ads
