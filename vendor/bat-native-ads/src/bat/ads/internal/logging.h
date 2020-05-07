/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_LOGGING_H_
#define BAT_ADS_INTERNAL_LOGGING_H_

#include <ostream>

namespace ads {

// Undefine base::logging macros so that Brave Ads can use LOG, LOG_IF, VLOG and
// VLOG_IF

#undef LOG
#undef LOG_IF

#undef VLOG
#undef VLOG_IF

// redefine our own logging macros

// Instead of using LOG(INFO, ...) for debugging use VLOG(1, ...). VLOG allows
// for extremely fine-grained logging control
//
// Example usage:
//
//   --log-level=1 --enable-logging=stderr
//
// This runs Brave Ads with the LOG severity set to "print everything with a
// severity of WARNING and above". Severity can be INFO, WARNING or ERROR

#define LOG(severity, stream) ads_client_->Log(__FILE__, __LINE__, \
    LogSeverity::LOG_ ## severity, (std::ostringstream() << stream).str());

// You can also do conditional logging when some extra computation and
// preparation for logs is not needed:
//
//   LOG_IF(WARNING, num_cookies > 10, "Got lots of cookies");

#define LOG_IF(severity, condition, stream) \
    !(condition) ? (void) 0 : LOG(severity, stream)

// |verbose_level| is an arbitrary integer value (higher numbers should be used
// for more verbose logging), so you can make your logging levels as granular as
// you wish and can be adjusted on a per-module basis at runtime
//
// Example usage:
//
//   --enable-logging=stderr --v=1 --vmodule=foo=2,bar=3
//
// This runs Brave Ads with the global VLOG level set to "print everything at
// level 1 and lower", but prints levels up to 2 in foo.cc and levels up to 3 in
// bar.cc

#define VLOG(verbose_level, stream) ads_client_->VerboseLog(__FILE__, \
    __LINE__, verbose_level, (std::ostringstream() << stream).str());

// You can also do conditional verbose logging when some extra computation and
// preparation for logs is not needed:
//
//   VLOG_IF(2, num_cookies > 10, "Got lots of cookies");

#define VLOG_IF(verbose_level, condition, stream) \
    !(condition) ? (void) 0 : LOG(verbose_level, stream)

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_LOGGING_H_
