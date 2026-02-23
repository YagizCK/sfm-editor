/*
 * Copyright 2026 Yağız Cem Kocabıyık
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FileDialog.h"

#include "Core/Application.h"

#include <windows.h>
#include <ShObjIdl_core.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace sfmeditor {
    std::string FileDialog::openFile(const char* filter) {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window(g_nativeWindow);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        return (GetOpenFileNameA(&ofn) == TRUE) ? std::string(ofn.lpstrFile) : std::string();
    }

    std::string FileDialog::saveFile(const char* filter, int* outFilterIndex) {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = glfwGetWin32Window(g_nativeWindow);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetSaveFileNameA(&ofn) == TRUE) {
            if (outFilterIndex) {
                *outFilterIndex = ofn.nFilterIndex;
            }
            return std::string(ofn.lpstrFile);
        }
        return std::string();
    }

    std::string FileDialog::pickFolder() {
        std::string result = "";
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        if (SUCCEEDED(hr)) {
            IFileDialog* pfd = nullptr;
            hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
            if (SUCCEEDED(hr)) {
                DWORD dwOptions;
                if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
                    pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
                }

                if (SUCCEEDED(pfd->Show(glfwGetWin32Window(g_nativeWindow)))) {
                    IShellItem* psi = nullptr;
                    if (SUCCEEDED(pfd->GetResult(&psi))) {
                        PWSTR pszPath = nullptr;
                        if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                            const int size_needed =
                                WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, nullptr, 0, nullptr, nullptr);
                            result.resize(size_needed - 1);
                            WideCharToMultiByte(CP_UTF8, 0, pszPath, -1, &result[0], size_needed, nullptr, nullptr);
                            CoTaskMemFree(pszPath);
                        }
                        psi->Release();
                    }
                }
                pfd->Release();
            }
            CoUninitialize();
        }
        return result;
    }
}
