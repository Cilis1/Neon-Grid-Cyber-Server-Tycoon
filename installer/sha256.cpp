#include "sha256.hpp"

#include <array>
#include <cstdio>
#include <cstring>

namespace neon::installer {

namespace {

constexpr std::array<std::uint32_t, 64> K = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

inline std::uint32_t rotr(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

struct ShaState {
  std::array<std::uint32_t, 8> h = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  std::uint64_t total_len = 0;
  std::array<std::uint8_t, 64> buffer{};
  std::size_t buffer_len = 0;
  bool finalized = false;
  std::array<std::uint8_t, 32> digest{};
};

void compress(ShaState& st, const std::uint8_t* block) {
  std::array<std::uint32_t, 64> w{};
  for (int i = 0; i < 16; ++i)
    w[i] = (std::uint32_t(block[i * 4]) << 24) | (std::uint32_t(block[i * 4 + 1]) << 16) |
           (std::uint32_t(block[i * 4 + 2]) << 8) | std::uint32_t(block[i * 4 + 3]);
  for (int i = 16; i < 64; ++i) {
    std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  auto& h = st.h;
  std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
  for (int i = 0; i < 64; ++i) {
    std::uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
    std::uint32_t ch = (e & f) ^ (~e & g);
    std::uint32_t t1 = hh + S1 + ch + K[i] + w[i];
    std::uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
    std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    std::uint32_t t2 = S0 + maj;
    hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
  }
  h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

} // namespace

struct Sha256::Pimpl {
  ShaState s;
};

Sha256::Sha256() : pimpl_(new Pimpl) {}
Sha256::~Sha256() { delete pimpl_; }
Sha256::Sha256(Sha256&& o) noexcept : pimpl_(o.pimpl_) { o.pimpl_ = nullptr; }
Sha256& Sha256::operator=(Sha256&& o) noexcept {
  if (this != &o) {
    delete pimpl_;
    pimpl_ = o.pimpl_;
    o.pimpl_ = nullptr;
  }
  return *this;
}

void Sha256::update(const void* data, std::size_t len) {
  auto& s = pimpl_->s;
  s.total_len += len;
  const auto* p = static_cast<const std::uint8_t*>(data);
  while (len > 0) {
    std::size_t take = 64 - s.buffer_len;
    if (take > len) take = len;
    std::memcpy(s.buffer.data() + s.buffer_len, p, take);
    s.buffer_len += take;
    p += take;
    len -= take;
    if (s.buffer_len == 64) {
      compress(s, s.buffer.data());
      s.buffer_len = 0;
    }
  }
}

std::vector<std::uint8_t> Sha256::digest_bytes() {
  auto& s = pimpl_->s;
  if (!s.finalized) {
    std::uint64_t bit_len = s.total_len * 8;
    std::uint8_t pad = 0x80;
    update(&pad, 1);
    std::uint8_t zero = 0;
    while (s.buffer_len != 56) update(&zero, 1);
    std::uint8_t len_bytes[8];
    for (int i = 0; i < 8; ++i) len_bytes[i] = static_cast<std::uint8_t>(bit_len >> (56 - 8 * i));
    update(len_bytes, 8);
    s.finalized = true;
    for (int i = 0; i < 8; ++i) {
      s.digest[i * 4 + 0] = static_cast<std::uint8_t>(s.h[i] >> 24);
      s.digest[i * 4 + 1] = static_cast<std::uint8_t>(s.h[i] >> 16);
      s.digest[i * 4 + 2] = static_cast<std::uint8_t>(s.h[i] >> 8);
      s.digest[i * 4 + 3] = static_cast<std::uint8_t>(s.h[i]);
    }
  }
  return std::vector<std::uint8_t>(s.digest.begin(), s.digest.end());
}

std::string Sha256::hex_digest() {
  auto d = digest_bytes();
  static const char* hexd = "0123456789abcdef";
  std::string out;
  out.reserve(64);
  for (auto b : d) {
    out.push_back(hexd[b >> 4]);
    out.push_back(hexd[b & 0xF]);
  }
  return out;
}

std::string Sha256::hex(const void* data, std::size_t len) {
  Sha256 s;
  s.update(data, len);
  return s.hex_digest();
}

std::string Sha256::file_hex(const std::string& path) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) return "";
  Sha256 s;
  std::array<std::uint8_t, 8192> buf{};
  std::size_t n;
  while ((n = std::fread(buf.data(), 1, buf.size(), f)) > 0) s.update(buf.data(), n);
  std::fclose(f);
  return s.hex_digest();
}

} // namespace neon::installer
