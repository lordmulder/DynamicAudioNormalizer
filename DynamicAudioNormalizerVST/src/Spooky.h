// Spooky Hash
// A 128-bit noncryptographic hash, for checksums and table lookup
// By Bob Jenkins.  Public domain.

#include <cstdlib>
#include <cstdint>

void hash128(const void *const message, size_t length, uint64_t *hash1, uint64_t *hash2);
void hash128(const wchar_t *const message, uint64_t *hash1, uint64_t *hash2);
