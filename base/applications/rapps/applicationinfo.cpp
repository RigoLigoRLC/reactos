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

CApplicationInfo::CApplicationInfo()
    :iCategory(ENUM_INVALID)
{

}


CApplicationInfo::~CApplicationInfo()
{
}


CAvailableApplicationInfo::CAvailableApplicationInfo(const CStringW& Filename)
{
    m_Parser = new CConfigParser(Filename);

    m_szPkgName = Filename;
    PathRemoveExtensionW(m_szPkgName.GetBuffer(MAX_PATH));
    m_szPkgName.ReleaseBuffer();

    m_Parser->GetString(L"Name", szDisplayName);
    m_Parser->GetString(L"Version", szDisplayVersion);
    m_Parser->GetString(L"URLDownload", m_szUrlDownload);
    m_Parser->GetString(L"Description", szComments);
    int Cat;
    if (!m_Parser->GetInt(L"Category", Cat))
        Cat = ENUM_INVALID;
    iCategory = static_cast<AppsCategories>(Cat);

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


