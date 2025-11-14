// SPDX-FileCopyrightText: 2006-2025 Knut Reinert & Freie Universität Berlin
// SPDX-FileCopyrightText: 2016-2025 Knut Reinert & MPI für molekulare Genetik
// SPDX-License-Identifier: BSD-3-Clause

/*!\file
 * \brief Provides KSETS version macros and global variables.
 * \author René Kmiecinski stolen from Enrico Seiler <enrico.seiler AT fu-berlin.de>
 */

#pragma once

#include <cstdint>

//!\brief The major version as MACRO.
#define KSETS_VERSION_MAJOR 0
//!\brief The minor version as MACRO.
#define KSETS_VERSION_MINOR 1
//!\brief The patch version as MACRO.
#define KSETS_VERSION_PATCH 0
//!\brief The release candidate number. 0 means stable release, >= 1 means release candidate.
#define KSETS_RELEASE_CANDIDATE 0

//!\brief The full version as MACRO (number).
#define KSETS_VERSION (KSETS_VERSION_MAJOR * 10000 + KSETS_VERSION_MINOR * 100 + KSETS_VERSION_PATCH)

/*!\brief Converts a number to a string. Preprocessor needs this indirection to
 * properly expand the values to strings.
 */
#define KSETS_VERSION_CSTRING_HELPER_STR(str) #str

//!\brief Converts version numbers to string.
#define KSETS_VERSION_CSTRING_HELPER_FUNC(MAJOR, MINOR, PATCH)                                                        \
    KSETS_VERSION_CSTRING_HELPER_STR(MAJOR)                                                                           \
    "." KSETS_VERSION_CSTRING_HELPER_STR(MINOR) "." KSETS_VERSION_CSTRING_HELPER_STR(PATCH)

#if (KSETS_RELEASE_CANDIDATE > 0)
//!\brief A helper function that expands to a suitable release candidate suffix.
#    define KSETS_RELEASE_CANDIDATE_HELPER(RC) "-rc." KSETS_VERSION_CSTRING_HELPER_STR(RC)
#else
//!\brief A helper function that expands to a suitable release candidate suffix.
#    define KSETS_RELEASE_CANDIDATE_HELPER(RC) ""
#endif

//!\brief The full version as null terminated string.
#define KSETS_VERSION_CSTRING                                                                                         \
    KSETS_VERSION_CSTRING_HELPER_FUNC(KSETS_VERSION_MAJOR, KSETS_VERSION_MINOR, KSETS_VERSION_PATCH)               \
    KSETS_RELEASE_CANDIDATE_HELPER(KSETS_RELEASE_CANDIDATE)

namespace ksets
{

//!\brief The major version.
constexpr uint8_t ksets_version_major = KSETS_VERSION_MAJOR;
//!\brief The minor version.
constexpr uint8_t ksets_version_minor = KSETS_VERSION_MINOR;
//!\brief The patch version.
constexpr uint8_t ksets_version_patch = KSETS_VERSION_PATCH;

//!\brief The full version as `std::size_t`.
constexpr std::size_t ksets_version = KSETS_VERSION;

//!\brief The full version as null terminated string.
constexpr char const * ksets_version_cstring = KSETS_VERSION_CSTRING;

} // namespace ksets

#undef KSETS_VERSION_CSTRING_HELPER_STR
#undef KSETS_VERSION_CSTRING_HELPER_FUNC
#undef KSETS_RELEASE_CANDIDATE_HELPER
