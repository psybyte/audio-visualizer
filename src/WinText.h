#pragma once

#include <string>
#include <string_view>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

inline std::wstring wideFromUtf8(std::string_view text) {
  if (text.empty()) {
    return {};
  }
  const int size = static_cast<int>(text.size());
  int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), size, nullptr, 0);
  const DWORD flags = count > 0 ? MB_ERR_INVALID_CHARS : 0;
  if (count <= 0) {
    count = MultiByteToWideChar(CP_UTF8, 0, text.data(), size, nullptr, 0);
  }
  if (count <= 0) {
    return {};
  }
  std::wstring out(static_cast<size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, flags == 0 ? 0 : MB_ERR_INVALID_CHARS, text.data(), size, out.data(), count);
  return out;
}

inline std::string utf8FromWide(std::wstring_view text) {
  if (text.empty()) {
    return {};
  }
  const int size = static_cast<int>(text.size());
  const int count = WideCharToMultiByte(CP_UTF8, 0, text.data(), size, nullptr, 0, nullptr, nullptr);
  if (count <= 0) {
    return {};
  }
  std::string out(static_cast<size_t>(count), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text.data(), size, out.data(), count, nullptr, nullptr);
  return out;
}
