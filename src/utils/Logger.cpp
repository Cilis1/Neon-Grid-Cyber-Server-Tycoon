#include "utils/Logger.hpp"

#include "utils/FileSystem.hpp"

#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <ctime>

namespace neon::utils {

LogBuffer& LogBuffer::instance() {
  static LogBuffer buf;
  return buf;
}

void LogBuffer::push(const std::string& line) {
  std::lock_guard<std::mutex> lock(mutex_);
  lines_.push_back(line);
  while (lines_.size() > kCapacity) lines_.pop_front();
}

std::deque<std::string> LogBuffer::snapshot(std::size_t max_lines) const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::size_t start = lines_.size() > max_lines ? lines_.size() - max_lines : 0;
  return std::deque<std::string>(lines_.begin() + static_cast<std::ptrdiff_t>(start), lines_.end());
}

void LogBuffer::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  lines_.clear();
}

namespace {

template <typename Mutex>
class log_buffer_sink : public spdlog::sinks::base_sink<Mutex> {
protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    spdlog::memory_buf_t buf;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, buf);
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char timebuf[16];
    std::strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tm);
    LogBuffer::instance().push(std::string(timebuf) + " [" + spdlog::level::to_string_view(msg.level).data() + "] " +
                               std::string(buf.data(), buf.size()));
  }
  void flush_() override {}
};

using log_buffer_sink_mt = log_buffer_sink<std::mutex>;

} // namespace

void init_logging() {
  try {
    fs::ensure_dir(fs::log_dir());
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        (fs::log_dir() / "game.log").string(), 1024 * 1024 * 5, 3);
    auto buffer_sink = std::make_shared<log_buffer_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("neongrid", spdlog::sinks_init_list{file_sink, buffer_sink});
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::warn);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%H:%M:%S] [%l] %v");
  } catch (const std::exception& e) {
    spdlog::default_logger()->error("failed to init file logging: {}", e.what());
  }
}

} // namespace neon::utils
