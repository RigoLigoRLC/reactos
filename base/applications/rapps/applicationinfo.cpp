/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Classes for working with available applications
 * COPYRIGHT:   Copyright 2017 Alexander Shaposhnikov     (sanchaez@reactos.org)
 *              Copyright 2020 He Yang                    (1160386205@qq.com)
 *              Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "applicationinfo.h"

//#include "available.h"
//#include "misc.h"
//#include "dialogs.h"

CApplicationInfo::CApplicationInfo(const CStringW& Identifier, AppsCategories Category)
    : szIdentifier(Identifier)
    , iCategory(Category)
{
}


CApplicationInfo::~CApplicationInfo()
{
}


CAvailableApplicationInfo::CAvailableApplicationInfo(CConfigParser* Parser, const CStringW& PkgName, AppsCategories Category, const CPathW& BasePath)
    : CApplicationInfo(PkgName, Category)
    , m_Parser(Parser)
    , m_ScrnshotRetrieved(false)
{
    m_Parser->GetString(L"Name", szDisplayName);
    m_Parser->GetString(L"Version", szDisplayVersion);
    m_Parser->GetString(L"URLDownload", m_szUrlDownload);
    m_Parser->GetString(L"Description", szComments);


    CPathW IconPath = BasePath;
    IconPath += L"icons";
    //PathAppendW(IconPath.GetBuffer(MAX_PATH), L"icons");

    CStringW IconName;
    if (m_Parser->GetString(L"Icon", IconName))
    {
        IconPath += IconName;
        //BOOL bSuccess = PathAppendNoDirEscapeW(IconPath.GetBuffer(), IconLocation.GetString());
        //IconPath.ReleaseBuffer();

        //if (!bSuccess)
        //{
        //    IconPath.Empty();
        //}
    }
    else
    {
        // inifile.ico
        IconPath += (szIdentifier + L".ico");
        //PathAppendW(IconPath.GetBuffer(), m_szPkgName);
        //IconPath.ReleaseBuffer();
        //IconPath += L".ico";
    }

    //if (!IconPath.IsEmpty())
    //{
        if (PathFileExistsW(IconPath))
        {
            szDisplayIcon = (LPCWSTR)IconPath;
        }
//    }



    //CStringW szDisplayIcon;


    //if (!GetString(L"Name", m_szName)
    //    || !GetString(L"URLDownload", m_szUrlDownload))
    //{
    //    delete m_Parser;
    //    return;
    //}

    //m_Parser->GetString(L"RegName", m_szRegName);
    //m_Parser->GetString(L"License", m_szLicense);
    //m_Parser->GetString(L"Description", m_szDesc);
    //m_Parser->GetString(L"URLSite", m_szUrlSite);
    //m_Parser->GetString(L"SHA1", m_szSHA1);


}

CAvailableApplicationInfo::~CAvailableApplicationInfo()
{
    delete m_Parser;
}


BOOL CAvailableApplicationInfo::Valid() const
{
    return !szDisplayName.IsEmpty() && !m_szUrlDownload.IsEmpty();
}

BOOL CAvailableApplicationInfo::RetrieveIcon(CStringW& Path) const
{
    Path = szDisplayIcon;
    return !Path.IsEmpty();
}

#define MAX_SCRNSHOT_NUM 16
BOOL
CAvailableApplicationInfo::RetrieveScreenshot(CStringW &Path)
{
    //Path = szDisplayIcon1;

    if (!m_ScrnshotRetrieved)
    {
        static_assert(MAX_SCRNSHOT_NUM < 10000, "MAX_SCRNSHOT_NUM is too big");
        for (int i = 0; i < MAX_SCRNSHOT_NUM; i++)
        {
            CStringW ScrnshotField;
            ScrnshotField.Format(L"Screenshot%d", i + 1);
            CStringW ScrnshotLocation;
            if (!m_Parser->GetString(ScrnshotField, ScrnshotLocation))
            {
                // We stop at the first screenshot not found,
                // so screenshots _have_ to be consecutive
                break;
            }

            if (PathIsURLW(ScrnshotLocation.GetString()))
            {
                m_szScrnshotLocation.Add(ScrnshotLocation);
            }
        }
        m_ScrnshotRetrieved = true;
    }

    if (m_szScrnshotLocation.GetSize() > 0)
    {
        Path = m_szScrnshotLocation[0];
    }


    return !Path.IsEmpty();
}


CInstalledApplicationInfo::CInstalledApplicationInfo(HKEY Key, const CStringW& KeyName, AppsCategories Category)
    : CApplicationInfo(KeyName, Category)
    , m_hKey(Key)
{
    if (GetApplicationRegString(L"DisplayName", szDisplayName))
    {
        GetApplicationRegString(L"DisplayIcon", szDisplayIcon);
        GetApplicationRegString(L"DisplayVersion", szDisplayVersion);
        GetApplicationRegString(L"Comments", szComments);

        //bSuccess = TRUE;
    }

}

CInstalledApplicationInfo::~CInstalledApplicationInfo()
{
}

BOOL CInstalledApplicationInfo::Valid() const
{
    return !szDisplayName.IsEmpty();
}

BOOL
CInstalledApplicationInfo::RetrieveIcon(CStringW &Path) const
{
    Path = szDisplayIcon;
    return !Path.IsEmpty();
}

BOOL
CInstalledApplicationInfo::RetrieveScreenshot(CStringW &/*Path*/)
{
    return FALSE;
}

BOOL CInstalledApplicationInfo::GetApplicationRegString(LPCWSTR lpKeyName, ATL::CStringW& String)
{
    //DWORD dwAllocated = 0, dwSize, dwType;

    ULONG nChars = 0;
    // Get the size
    if (m_hKey.QueryStringValue(lpKeyName, NULL, &nChars) != ERROR_SUCCESS)
    {
        String.Empty();
        return FALSE;
    }

    LONG lResult = m_hKey.QueryStringValue(lpKeyName, String.GetBuffer(nChars), &nChars);
    String.ReleaseBuffer(nChars);

    if (lResult != ERROR_SUCCESS)
    {
        String.Empty();
        return FALSE;
    }

    if (String.Find('%') >= 0)
        __debugbreak();


    //if (dwType == REG_EXPAND_SZ)
    //{
    //    CStringW Tmp;

    //    DWORD dwLen = ExpandEnvironmentStringsW(String, NULL, 0);
    //    if (dwLen > 0)
    //    {
    //        BOOL bSuccess = ExpandEnvironmentStringsW(String, Tmp.GetBuffer(dwLen), dwLen) == dwLen;
    //        Tmp.ReleaseBuffer(dwLen - 1);
    //        if (bSuccess)
    //        {
    //            String = Tmp;
    //        }
    //        else
    //        {
    //            String.Empty();
    //            return FALSE;
    //        }
    //    }
    //}

    return TRUE;
}

