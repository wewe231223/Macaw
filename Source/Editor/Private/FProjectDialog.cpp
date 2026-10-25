/**
 * @file FProjectDialog.cpp
 * @brief 프로젝트 파일 생성 및 열기용 Windows 대화상자에 대한 구현입니다.
 * @ingroup Editor
 */

#include "Editor/FProjectDialog.h"

#include <Windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#include <wrl/client.h>
#include <array>

std::filesystem::path FProjectDialog::Choose(void* Owner, bool Create) {
    std::array<wchar_t, 32768> Buffer{};
    OPENFILENAMEW Dialog{};
    Dialog.lStructSize = sizeof(Dialog);
    Dialog.hwndOwner = static_cast<HWND>(Owner);
    Dialog.lpstrTitle = Create ? L"Create Project - choose a name and parent folder" : L"Open Project";
    Dialog.lpstrFilter = L"Macaw Project (*.mproject)\0*.mproject\0";
    Dialog.lpstrFile = Buffer.data();
    Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
    Dialog.lpstrDefExt = L"mproject";
    Dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (Create ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!(Create ? GetSaveFileNameW(&Dialog) : GetOpenFileNameW(&Dialog))) {
        return {};
    }
    const std::filesystem::path Selected{Buffer.data()};
    return Create ? Selected.parent_path() / Selected.stem() / Selected.filename() : Selected;
}

std::filesystem::path FProjectDialog::ChooseDirectory(void* Owner) {
    const HRESULT Initialized{CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)};
    std::filesystem::path Selected{};
    Microsoft::WRL::ComPtr<IFileOpenDialog> Dialog{};
    if ((SUCCEEDED(Initialized) || Initialized == RPC_E_CHANGED_MODE) && SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(Dialog.GetAddressOf())))) {
        FILEOPENDIALOGOPTIONS Options{};
        Dialog->GetOptions(&Options);
        Dialog->SetOptions(Options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);
        Dialog->SetTitle(L"Choose Project Location");
        if (SUCCEEDED(Dialog->Show(static_cast<HWND>(Owner)))) {
            Microsoft::WRL::ComPtr<IShellItem> Item{};
            PWSTR Path{};
            if (SUCCEEDED(Dialog->GetResult(Item.GetAddressOf())) && SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &Path))) {
                Selected = Path;
                CoTaskMemFree(Path);
            }
        }
    }
    Dialog.Reset();
    if (SUCCEEDED(Initialized)) {
        CoUninitialize();
    }
    return Selected;
}
