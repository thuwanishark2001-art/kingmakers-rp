#pragma once

#include <string>

namespace kingmakers::assets {

// Opens an existing client-owned asset beneath the app-private runtime root.
// Returns a read-only file descriptor, or -1 on invalid input / open failure.
// The descriptor is owned by the caller and must be closed with close().
//
// Only SAMP/, gui/, fonts/, kingmakers/, and audio/streams/ paths are eligible.
// The resolver rejects absolute paths, traversal, and symlinks at every path
// component. The root directory itself must be a trusted app-private directory.
int OpenOwnedAssetReadOnly(const std::string& runtimeRoot, const std::string& virtualPath);

}  // namespace kingmakers::assets
