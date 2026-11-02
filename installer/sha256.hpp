#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace neon::installer {

// Self-contained SHA-256 (no external dependency).
class Sha256 {
public:
  Sha256();
  ~Sha256();
  Sha256(Sha256&&) noexcept;
  Sha256& operator=(Sha256&&) noexcept;
  Sha256(const Sha256&) = delete;
  Sha256& operator=(const Sha256&) = delete;

  void update(const void* data, std::size_t len);
  void update(const std::string& s) { update(s.data(), s.size()); }
  // Finalize and return the 32-byte digest.
  std::vector<std::uint8_t> digest_bytes();
  // Finalize and return the lowercase hex digest.
  std::string hex_digest();

  // Convenience: hash a buffer / string.
  static std::string hex(const void* data, std::size_t len);
  static std::string hex(const std::string& s) { return hex(s.data(), s.size()); }
  // Convenience: hash a whole file; returns empty string on error.
  static std::string file_hex(const std::string& path);

private:
  struct Pimpl;
  Pimpl* pimpl_;
};

} // namespace neon::installer
