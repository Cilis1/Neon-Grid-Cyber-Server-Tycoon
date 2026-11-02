#include "unzipper.hpp"

#include <miniz.h>

#include <cstdio>
#include <cstring>
#include <fstream>

namespace neon::installer {

std::vector<std::string> unzip(const std::string& zip_path, const std::string& dest_dir, std::string* error_out) {
  std::vector<std::string> extracted;
  mz_zip_archive zip;
  std::memset(&zip, 0, sizeof(zip));
  if (!mz_zip_reader_init_file(&zip, zip_path.c_str(), 0)) {
    if (error_out) *error_out = "failed to open zip: " + zip_path;
    return extracted;
  }
  const mz_uint count = mz_zip_reader_get_num_files(&zip);
  std::filesystem::create_directories(dest_dir);
  for (mz_uint i = 0; i < count; ++i) {
    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(&zip, i, &stat)) continue;
    if (mz_zip_reader_is_file_a_directory(&zip, i)) {
      std::filesystem::create_directories(std::filesystem::path(dest_dir) / stat.m_filename);
      continue;
    }
    std::filesystem::path out_path = std::filesystem::path(dest_dir) / stat.m_filename;
    // guard against zip-slip
    auto canonical_dest = std::filesystem::weakly_canonical(dest_dir);
    auto canonical_out = std::filesystem::weakly_canonical(out_path.parent_path());
    if (canonical_out.string().rfind(canonical_dest.string(), 0) != 0) {
      if (error_out) *error_out = "zip-slip detected: " + std::string(stat.m_filename);
      mz_zip_reader_end(&zip);
      return extracted;
    }
    std::filesystem::create_directories(out_path.parent_path());
    if (!mz_zip_reader_extract_to_file(&zip, i, out_path.string().c_str(), 0)) {
      if (error_out) *error_out = "failed to extract: " + std::string(stat.m_filename);
      mz_zip_reader_end(&zip);
      return extracted;
    }
    extracted.push_back(out_path.string());
  }
  mz_zip_reader_end(&zip);
  return extracted;
}

} // namespace neon::installer
