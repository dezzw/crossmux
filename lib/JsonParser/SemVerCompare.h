#pragma once

#include <cstddef>

// Compares dotted semver strings (major.minor.patch) with optional "-rc" pre-release
// suffix on the current side. Returns +1 if latest is newer, 0 if equal/not newer,
// -1 if latest is older (downgrade).
int semverCompareNewer(const char* currentVersion, const char* latestVersion);
