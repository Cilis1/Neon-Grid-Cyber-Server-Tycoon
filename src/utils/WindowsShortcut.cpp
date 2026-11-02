#include "utils/WindowsShortcut.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h>
#include <shlobj.h>

#include <iostream>
#endif

namespace neon::utils {

std::filesystem::path desktop_dir() {
#ifdef _WIN32
  wchar_t* path = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &path))) {
    std::filesystem::path p(path);
    CoTaskMemFree(path);
    return p;
  }
  const char* user = std::getenv("USERPROFILE");
  if (user) return std::filesystem::path(user) / "Desktop";
  return std::filesystem::path("Desktop");
#else
  const char* home = std::getenv("HOME");
  return std::filesystem::path(home ? home : ".") / "Desktop";
#endif
}

bool create_shortcut(const ShortcutSpec& spec) {
#ifdef _WIN32
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  const bool was_initialized = SUCCEEDED(hr);
  if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;

  bool ok = false;
  IShellLinkW* psl = nullptr;
  hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                        reinterpret_cast<void**>(&psl));
  if (SUCCEEDED(hr)) {
    psl->SetPath(spec.target.wstring().c_str());
    if (!spec.working_dir.empty()) psl->SetWorkingDirectory(spec.working_dir.wstring().c_str());
    if (!spec.description.empty()) psl->SetDescription(spec.description.c_str());
    if (!spec.arguments.empty()) psl->SetArguments(spec.arguments.c_str());
    if (!spec.icon.empty()) {
      // Icon location: file path with optional index
      psl->SetIconLocation(spec.icon.wstring().c_str(), 0);
    }
    IPersistFile* ppf = nullptr;
    hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
    if (SUCCEEDED(hr)) {
      hr = ppf->Save(spec.link_path.wstring().c_str(), TRUE);
      ok = SUCCEEDED(hr);
      ppf->Release();
    }
    psl->Release();
  }
  if (was_initialized) CoUninitialize();
  return ok;
#else
  (void)spec;
  return false;
#endif
}

} // namespace neon::utils
