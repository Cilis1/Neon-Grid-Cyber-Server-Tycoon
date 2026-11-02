#pragma once

#include <deque>
#include <mutex>
#include <string>

namespace neon::utils {

// Thread-safe ring buffer that mirrors log lines into the in-game log window.
class LogBuffer {
public:
  static LogBuffer& instance();

  void push(const std::string& line);
  // Snapshot of the most recent `max_lines` lines.
  std::deque<std::string> snapshot(std::size_t max_lines = 500) const;
  void clear();

private:
  mutable std::mutex mutex_;
  std::deque<std::string> lines_;
  static constexpr std::size_t kCapacity = 2000;
};

// Initialize spdlog: file sink at <log_dir>/game.log plus the LogBuffer sink.
void init_logging();

} // namespace neon::utils
