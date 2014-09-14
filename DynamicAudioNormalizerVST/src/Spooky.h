// Spooky Hash
// A 128-bit noncryptographic hash, for checksums and table lookup
// By Bob Jenkins.  Public domain.
//
// This is a stripped-down version derived from the original source code. Find the original code at:
// http://burtleburtle.net/bob/hash/spooky.html

#include <cstdlib>
#include <cstdint>

void hash128(const void *const message, size_t length, uint64_t *hash1, uint64_t *hash2);
void hash128(const wchar_t *const message, uint64_t *hash1, uint64_t *hash2);
