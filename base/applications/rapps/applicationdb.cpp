/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Classes for working with available applications
 * COPYRIGHT:   Copyright 2017 Alexander Shaposhnikov     (sanchaez@reactos.org)
 *              Copyright 2020 He Yang                    (1160386205@qq.com)
 *              Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "applicationdb.h"

//#include "available.h"
//#include "misc.h"
//#include "dialogs.h"


//CApplicationDB::~CApplicationDB()
//{
//}
//
//static CPathW GetAppsPath(const CStringW& BasePath)
//{
//    CPathW Path = BasePath;
//    Path += L"rapps";
//}
//
//static CPathW GetSearchPath(const CStringW& BasePath)
//{
//    return GetAppsPath(BasePath) + L"*.txt";
//}
//
//static CPathW GetDBPath(const CStringW& BasePath)
//{
//    CPathW Path = BasePath;
//    Path += APPLICATION_DATABASE_NAME;
//    return Path;
//}

CApplicationDB::CApplicationDB(const CStringW& path)
    :m_BasePath(path)
{
    m_BasePath.Canonicalize();
    //if (GetStorageDirectory(szPath))
    //{
    //    szAppsPath = szPath;
    //    PathAppendW(szAppsPath.GetBuffer(MAX_PATH), L"rapps");
    //    szAppsPath.ReleaseBuffer();

    //    szCabName = APPLICATION_DATABASE_NAME;
    //    szCabDir = szPath;
    //    szCabPath = szCabDir;
    //    PathAppendW(szCabPath.GetBuffer(MAX_PATH), szCabName);
    //    szCabPath.ReleaseBuffer();

    //    szSearchPath = szAppsPath;
    //    PathAppendW(szSearchPath.GetBuffer(MAX_PATH), L"*.txt");
    //    szSearchPath.ReleaseBuffer();
    //}


}

CApplicationInfo* CApplicationDB::FindByPackageName(const CStringW& name)
{
    POSITION CurrentListPosition = m_Available.GetHeadPosition();
    while (CurrentListPosition)
    {
        CApplicationInfo* Info = m_Available.GetNext(CurrentListPosition);
        if (Info->szIdentifier == name)
        {
            return Info;
        }
    }
    return NULL;
}


void CApplicationDB::GetApps(CAtlList<CApplicationInfo*>& List, AppsCategories Type) const
{
    if (Type == ENUM_INSTALLED_APPLICATIONS ||
        Type == ENUM_UPDATES)
    {
        POSITION CurrentListPosition = m_Installed.GetHeadPosition();
        while (CurrentListPosition)
        {
            CApplicationInfo* Info = m_Installed.GetNext(CurrentListPosition);

            if (Type == ENUM_ALL_INSTALLED ||
                Type == Info->iCategory)
            {
                List.AddTail(Info);
            }
        }
    }
    else
    {
        POSITION CurrentListPosition = m_Available.GetHeadPosition();
        while (CurrentListPosition)
        {
            CApplicationInfo* Info = m_Available.GetNext(CurrentListPosition);

            if (Type == ENUM_ALL_AVAILABLE ||
                Type == Info->iCategory)
            {
                List.AddTail(Info);
            }
        }
    }
}



BOOL CApplicationDB::EnumerateFiles()
{
    CPathW AppsPath = m_BasePath;
    AppsPath += L"rapps";// GetAppsPath(m_BasePath);
    CPathW WildcardPath = AppsPath;
    WildcardPath += L"*.txt";

    WIN32_FIND_DATAW FindFileData;
    HANDLE hFind = FindFirstFileW(WildcardPath, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    do
    {
        CStringW szPkgName = FindFileData.cFileName;
        PathRemoveExtensionW(szPkgName.GetBuffer(MAX_PATH));
        szPkgName.ReleaseBuffer();

        CApplicationInfo* Info = FindByPackageName(szPkgName);
        if (Info)
        {

        }
        else
        {
            CConfigParser* Parser = new CConfigParser(FindFileData.cFileName);
            int Cat;
            if (!Parser->GetInt(L"Category", Cat))
                Cat = ENUM_INVALID;

            Info = new CAvailableApplicationInfo(Parser, szPkgName, static_cast<AppsCategories>(Cat), AppsPath);
            if (Info->Valid())
            {
                m_Available.AddTail(Info);
            }
            else
            {
                delete Info;
            }
        }


        // loop for all the cached entries
        //POSITION CurrentListPosition = m_InfoList.GetHeadPosition();
        //CAvailableApplicationInfo* Info = NULL;

        //while (CurrentListPosition != NULL)
        //{
        //    POSITION LastListPosition = CurrentListPosition;
        //    Info = m_InfoList.GetNext(CurrentListPosition);

        //    // do we already have this entry in cache?
        //    if (Info->m_sFileName == FindFileData.cFileName)
        //    {
        //        // is it current enough, or the file has been modified since our last time here?
        //        if (CompareFileTime(&FindFileData.ftLastWriteTime, &Info->m_ftCacheStamp) == 1)
        //        {
        //            // recreate our cache, this is the slow path
        //            m_InfoList.RemoveAt(LastListPosition);

        //            // also remove this in selected list (if exist)
        //            RemoveSelected(Info);

        //            delete Info;
        //            Info = NULL;
        //            break;
        //        }
        //        else
        //        {
        //            // speedy path, compare directly, we already have the data
        //            goto skip_if_cached;
        //        }
        //    }
        //}

        //// create a new entry
        //// set a timestamp for the next time
        //Info->SetLastWriteTime(&FindFileData.ftLastWriteTime);

        ///* Check if we have the download URL */
        //if (Info->m_szUrlDownload.IsEmpty())
        //{
        //    /* Can't use it, delete it */
        //    delete Info;
        //    continue;
        //}

        //

    //skip_if_cached:
    //    if (EnumType == Info->m_Category
    //        || EnumType == ENUM_ALL_AVAILABLE)
    //    {
    //        Info->RefreshAppInfo(m_Strings);

    //        if (lpEnumProc)
    //        {
    //            if (m_SelectedList.Find(Info))
    //            {
    //                lpEnumProc(Info, TRUE, param);
    //            }
    //            else
    //            {
    //                lpEnumProc(Info, FALSE, param);
    //            }
    //        }
    //    }
    } while (FindNextFileW(hFind, &FindFileData));


    FindClose(hFind);
    return TRUE;
}


VOID CApplicationDB::UpdateAvailable()
{
    if (!CreateDirectoryW(m_BasePath/*m_Strings.szPath*/, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return;

    if (EnumerateFiles())
        return;

    ////if there are some files in the db folder - we're good
    //CPathW AppsPath = m_BasePath;
    //AppsPath += L"rapps";// GetAppsPath(m_BasePath);
    //CPathW WildcardPath = AppsPath;
    //WildcardPath += L"*.txt";
    //hFind = FindFirstFileW(WildcardPath, &FindFileData);
    //if (hFind != INVALID_HANDLE_VALUE)
    //{
    //    FindClose(hFind);
    //    return TRUE;
    //}

    DownloadApplicationsDB(
        SettingsInfo.bUseSource ? SettingsInfo.szSourceURL : APPLICATION_DATABASE_URL, !SettingsInfo.bUseSource);

    CPathW AppsPath = m_BasePath;
    AppsPath += L"rapps";
    if (!ExtractFilesFromCab(APPLICATION_DATABASE_NAME, m_BasePath, AppsPath))
        return;

    CPathW CabFile = m_BasePath;
    CabFile += APPLICATION_DATABASE_NAME;
    DeleteFileW(CabFile);

    EnumerateFiles();
}

VOID CApplicationDB::ForceUpdateAvailable()
{
    Delete();
    return UpdateAvailable();
}

VOID CApplicationDB::UpdateInstalled()
{
    HKEY RootKeyEnum[3] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_LOCAL_MACHINE };
    REGSAM RegSamEnum[3] = { KEY_WOW64_32KEY,   KEY_WOW64_32KEY,    KEY_WOW64_64KEY };

    int LoopKeys = 2;

    if (IsSystem64Bit())
    {
        // loop for all 3 combination.
        // note that HKEY_CURRENT_USER\Software don't have a redirect
        // https://docs.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys#redirected-shared-and-reflected-keys-under-wow64
        LoopKeys = 3;
    }

    for (int i = 0; i < LoopKeys; i++)
    {
        DWORD dwSize = MAX_PATH;
        HKEY hKey, hSubKey;
        LONG ItemIndex = 0;
        WCHAR szKeyName[MAX_PATH];

        if (RegOpenKeyExW(RootKeyEnum[i],
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            NULL,
            KEY_READ | RegSamEnum[i],
            &hKey) != ERROR_SUCCESS)
        {
            continue;
        }

        while (1)
        {
            dwSize = MAX_PATH;
            if (RegEnumKeyExW(hKey, ItemIndex, szKeyName, &dwSize, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            {
                break;
            }

            ItemIndex++;

            if (RegOpenKeyW(hKey, szKeyName, &hSubKey) == ERROR_SUCCESS)
            {
                DWORD dwValue = 0;

                dwSize = sizeof(DWORD);
                if (RegQueryValueExW(hSubKey, L"SystemComponent", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS &&
                    dwValue == 1)
                {
                    // Ignore system components
                    RegCloseKey(hSubKey);
                    continue;
                }

                // RootKeyEnum[i] == HKEY_CURRENT_USER, RegSamEnum[i],

                BOOL bIsUpdate = (RegQueryValueExW(hSubKey, L"ParentKeyName", NULL, NULL, NULL, &dwSize) == ERROR_SUCCESS);

                CInstalledApplicationInfo* Info = new CInstalledApplicationInfo(hSubKey, szKeyName, bIsUpdate ? ENUM_UPDATES : ENUM_INSTALLED_APPLICATIONS);

                // items without display name are ignored
                //if (Info->GetApplicationRegString(L"DisplayName", Info->szDisplayName))
                //{
                //    Info->GetApplicationRegString(L"DisplayIcon", Info->szDisplayIcon);
                //    Info->GetApplicationRegString(L"DisplayVersion", Info->szDisplayVersion);
                //    Info->GetApplicationRegString(L"Comments", Info->szComments);

                //    bSuccess = TRUE;
                //}

                if (Info->Valid())
                {
                    // add to InfoList.
                    m_Installed.AddTail(Info);

                    //// invoke callback
                    //if (lpEnumProc)
                    //{
                    //    if ((EnumType == ENUM_ALL_INSTALLED) || /* All components */
                    //        ((EnumType == ENUM_INSTALLED_APPLICATIONS) && (!Info->bIsUpdate)) || /* Applications only */
                    //        ((EnumType == ENUM_UPDATES) && (Info->bIsUpdate))) /* Updates only */
                    //    {
                    //        lpEnumProc(Info, param);
                    //    }
                    //}
                }
                else
                {
                    delete Info;
                }
            }
        }

        RegCloseKey(hKey);
    }
}


static void DeleteWithWildcard(const CPathW& Dir, const CStringW& Filter)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindFileData;

    CPathW DirWithFilter = Dir;
    DirWithFilter += Filter;

    hFind = FindFirstFileW(DirWithFilter, &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    //CPathW Dir = DirWithFilter;
    //Dir.RemoveFileSpec();
    //PathRemoveFileSpecW(Dir.GetBuffer(MAX_PATH));
    //Dir.ReleaseBuffer();

    do
    {
        CPathW szTmp = Dir;
        szTmp += FindFileData.cFileName;

        if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            DeleteFileW(szTmp);
        }
    } while (FindNextFileW(hFind, &FindFileData) != 0);
    FindClose(hFind);
}


VOID CApplicationDB::Delete()
{
    // Delete icons
    CPathW AppsPath = m_BasePath;
    AppsPath += L"rapps";
    CPathW IconPath = AppsPath;
    IconPath += L"icons";
    //ATL::CStringW IconPath = m_Strings.szAppsPath;
    //PathAppendW(IconPath.GetBuffer(MAX_PATH), L"icons");
    //IconPath.ReleaseBuffer();
    DeleteWithWildcard(IconPath, L"*.ico");

    // Delete leftover screenshots
    CPathW ScrnshotFolder = AppsPath;
    ScrnshotFolder += L"screenshots";
    DeleteWithWildcard(ScrnshotFolder, L"*.tmp");

    // Delete data base files (*.txt)
    DeleteWithWildcard(AppsPath, L"*.txt");

    RemoveDirectoryW(IconPath);
    RemoveDirectoryW(ScrnshotFolder);
    RemoveDirectoryW(AppsPath);
    RemoveDirectoryW(m_BasePath);
}

