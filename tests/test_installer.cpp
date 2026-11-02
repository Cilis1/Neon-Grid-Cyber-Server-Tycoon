#include <catch2/catch_test_macros.hpp>

#include "sha256.hpp"
#include "unzipper.hpp"
#include "utils/StringUtils.hpp"

#include <miniz.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace neon;

TEST_CASE("sha256 known vectors", "[installer]") {
  REQUIRE(installer::Sha256::hex("") ==
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  REQUIRE(installer::Sha256::hex("abc") ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  REQUIRE(installer::Sha256::hex("The quick brown fox jumps over the lazy dog") ==
          "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
  // long input crossing block boundaries
  std::string long_input(1000000, 'a');
  REQUIRE(installer::Sha256::hex(long_input) ==
          "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST_CASE("sha256 streaming equals one-shot", "[installer]") {
  std::string data = "Neon Grid bootstrap payload 0123456789";
  installer::Sha256 s;
  for (char c : data) s.update(&c, 1);
  REQUIRE(s.hex_digest() == installer::Sha256::hex(data));
}

TEST_CASE("version compare", "[installer]") {
  REQUIRE(utils::compare_version("1.0.0", "1.0.1") == -1);
  REQUIRE(utils::compare_version("1.10.0", "1.9.9") == 1);
  REQUIRE(utils::compare_version("2.0", "2.0.0") == 0);
  REQUIRE(utils::compare_version("1.2.3", "1.2.3") == 0);
}

TEST_CASE("zip round trip with miniz + unzipper", "[installer]") {
  auto dir = std::filesystem::temp_directory_path() / "neongrid_zip_test";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  // create source tree
  auto src = dir / "src";
  std::filesystem::create_directories(src / "assets" / "icon");
  {
    std::ofstream(src / "NeonGrid.exe", std::ios::binary) << "MZ fake exe bytes";
    std::ofstream(src / "assets" / "icon" / "icon.ico", std::ios::binary) << "ICON fake";
    std::ofstream(src / "data.json") << "{\"ok\":true}";
  }
  // zip it with miniz writer
  auto zip_path = (dir / "game.zip").string();
  mz_zip_archive zip;
  std::memset(&zip, 0, sizeof(zip));
  REQUIRE(mz_zip_writer_init_file(&zip, zip_path.c_str(), 0));
  REQUIRE(mz_zip_writer_add_file(&zip, "NeonGrid.exe", (src / "NeonGrid.exe").string().c_str(), nullptr, 0, MZ_BEST_COMPRESSION));
  REQUIRE(mz_zip_writer_add_file(&zip, "assets/icon/icon.ico", (src / "assets" / "icon" / "icon.ico").string().c_str(), nullptr, 0, MZ_BEST_COMPRESSION));
  REQUIRE(mz_zip_writer_add_file(&zip, "data.json", (src / "data.json").string().c_str(), nullptr, 0, MZ_BEST_COMPRESSION));
  REQUIRE(mz_zip_writer_finalize_archive(&zip));
  mz_zip_writer_end(&zip);

  // hash it, then extract and verify
  std::string sha = installer::Sha256::file_hex(zip_path);
  REQUIRE(sha.size() == 64);

  auto dest = dir / "dest";
  std::string err;
  auto files = installer::unzip(zip_path, dest.string(), &err);
  REQUIRE(err.empty());
  REQUIRE(files.size() == 3);
  REQUIRE(std::filesystem::exists(dest / "NeonGrid.exe"));
  REQUIRE(std::filesystem::exists(dest / "assets" / "icon" / "icon.ico"));
  {
    std::ifstream in(dest / "data.json");
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    REQUIRE(content == "{\"ok\":true}");
  }
  // extracted exe hash matches source
  REQUIRE(installer::Sha256::file_hex((dest / "NeonGrid.exe").string()) ==
          installer::Sha256::file_hex((src / "NeonGrid.exe").string()));
  std::filesystem::remove_all(dir);
}
