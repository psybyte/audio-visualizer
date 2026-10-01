#include "ui/FileDialog.h"

#include <shobjidl.h>
#include <shlobj.h>

std::optional<std::wstring> openAudioFileDialog(HWND owner) {
  IFileOpenDialog* dialog = nullptr;
  HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
  if (FAILED(result) || dialog == nullptr) {
    return std::nullopt;
  }

  DWORD options = 0;
  dialog->GetOptions(&options);
  dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
  dialog->SetTitle(L"Open audio");
  const COMDLG_FILTERSPEC filters[] = {
      {L"Audio", L"*.wav;*.mp3;*.flac"},
      {L"All files", L"*.*"},
  };
  dialog->SetFileTypes(2, filters);

  PWSTR music = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Music, 0, nullptr, &music))) {
    IShellItem* folder = nullptr;
    if (SUCCEEDED(SHCreateItemFromParsingName(music, nullptr, IID_PPV_ARGS(&folder)))) {
      dialog->SetFolder(folder);
      folder->Release();
    }
    CoTaskMemFree(music);
  }

  result = dialog->Show(owner);
  if (FAILED(result)) {
    dialog->Release();
    return std::nullopt;
  }

  IShellItem* item = nullptr;
  result = dialog->GetResult(&item);
  if (FAILED(result) || item == nullptr) {
    dialog->Release();
    return std::nullopt;
  }
  PWSTR path = nullptr;
  result = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
  std::optional<std::wstring> chosen;
  if (SUCCEEDED(result) && path != nullptr) {
    chosen = path;
    CoTaskMemFree(path);
  }
  item->Release();
  dialog->Release();
  return chosen;
}
