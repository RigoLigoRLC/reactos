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

void CApplicationDB::GetApps(CAtlList<CApplicationInfo*>& List, AppsCategories Type) const
{
    POSITION CurrentListPosition = m_List.GetHeadPosition();
    while (CurrentListPosition)
    {
        CApplicationInfo* Info = m_List.GetNext(CurrentListPosition);

        if (Type == ENUM_ALL_AVAILABLE ||
            Type == Info->iCategory)
        {
            List.AddTail(Info);
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
        CApplicationInfo* Info = new CAvailableApplicationInfo(FindFileData.cFileName);
        if (Info->Valid())
        {
            m_List.AddTail(Info);
        }
        else
        {
            delete Info;
        }
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


BOOL CApplicationDB::Update()
{

    if (!CreateDirectoryW(m_BasePath/*m_Strings.szPath*/, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return FALSE;
    }

    if (EnumerateFiles())
        return TRUE;

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

    DownloadApplicationsDB(SettingsInfo.bUseSource ? SettingsInfo.szSourceURL : APPLICATION_DATABASE_URL,
        !SettingsInfo.bUseSource);

    CPathW AppsPath = m_BasePath;
    AppsPath += L"rapps";
    if (!ExtractFilesFromCab(APPLICATION_DATABASE_NAME, m_BasePath, AppsPath))
    {
        return FALSE;
    }

    CPathW CabFile = m_BasePath;
    CabFile += APPLICATION_DATABASE_NAME;
    DeleteFileW(CabFile);

    return EnumerateFiles();
}

BOOL CApplicationDB::ForceUpdate()
{
    Delete();
    return Update();
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

