#pragma once

//#include <windef.h>
#include <atlstr.h>
#include <atlpath.h>
//#include <atlsimpcoll.h>
//#include <atlcoll.h>

//#include "misc.h"
//#include "configparser.h"



//enum LicenseType
//{
//    LICENSE_NONE,
//    LICENSE_OPENSOURCE,
//    LICENSE_FREEWARE,
//    LICENSE_TRIAL,
//    LICENSE_MIN = LICENSE_NONE,
//    LICENSE_MAX = LICENSE_TRIAL
//};
//
//inline BOOL IsLicenseType(INT x)
//{
//    return (x >= LICENSE_MIN && x <= LICENSE_MAX);
//}
//
//struct AvailableStrings
//{
//    ATL::CStringW szPath;
//    ATL::CStringW szCabPath;
//    ATL::CStringW szAppsPath;
//    ATL::CStringW szSearchPath;
//    ATL::CStringW szCabName;
//    ATL::CStringW szCabDir;
//
//    AvailableStrings();
//};

class CApplicationInfo
{
public:

    CApplicationInfo(const CStringW& Identifier, AppsCategories Category);
    virtual ~CApplicationInfo();

    const CStringW szIdentifier;  // PkgName or KeyName
    const AppsCategories iCategory;

    CStringW szDisplayIcon;
    CStringW szDisplayName;
    CStringW szDisplayVersion;
    CStringW szComments;

    virtual BOOL Valid() const = 0;
    virtual BOOL RetrieveIcon(CStringW& Path) const = 0;
    virtual BOOL RetrieveScreenshot(CStringW& Path) = 0;

    //INT m_Category;
    //BOOL m_IsSelected;
//    LicenseType m_LicenseType;
//    ATL::CStringW m_szName; // software's display name.
//    ATL::CStringW m_szRegName;
//    ATL::CStringW m_szVersion;
//    ATL::CStringW m_szLicense;
//    ATL::CStringW m_szDesc;
//    ATL::CStringW m_szSize;
//    ATL::CStringW m_szUrlSite;
//    ATL::CStringW m_szUrlDownload;
//    ATL::CSimpleArray<LCID> m_LanguageLCIDs;
//    ATL::CSimpleArray<ATL::CStringW> m_szScrnshotLocation;
//    ATL::CStringW m_szIconLocation;
//    ATL::CStringW m_szPkgName; // software's package name.
//
//    ULONG m_SizeBytes;
//
//    // Caching mechanism related entries
//    ATL::CStringW m_sFileName;
//    FILETIME m_ftCacheStamp;
//
//    // Optional integrity checks (SHA-1 digests are 160 bit = 40 characters in hex string form)
//    ATL::CStringW m_szSHA1;
//    ATL::CStringW m_szInstalledVersion;
//
//
//    // Create an object from file
//    CAvailableApplicationInfo(const ATL::CStringW& sFileNameParam, AvailableStrings& m_Strings);
//
//    VOID EnsureDetailsLoaded();
//
//    // Load all info from the file
//    VOID RefreshAppInfo(AvailableStrings& m_Strings);
//    BOOL HasLanguageInfo() const;
//    BOOL HasNativeLanguage() const;
//    BOOL HasEnglishLanguage() const;
//    BOOL IsInstalled() const;
//    BOOL HasInstalledVersion() const;
//    BOOL HasUpdate() const;
//    BOOL RetrieveScrnshot(UINT Index, ATL::CStringW& ScrnshotLocation) const;
//    BOOL RetrieveIcon(ATL::CStringW& IconLocation) const;
//    // Set a timestamp
//    VOID SetLastWriteTime(FILETIME* ftTime);
//
//private:
//    BOOL m_IsInstalled;
//    BOOL m_HasLanguageInfo;
//    BOOL m_HasInstalledVersion;
//    CConfigParser* m_Parser;
//
//    inline BOOL GetString(LPCWSTR lpKeyName, ATL::CStringW& ReturnedString);
//
//    // Lazily load general info from the file
//    VOID RetrieveGeneralInfo(AvailableStrings& m_Strings);
//    VOID RetrieveInstalledStatus();
//    VOID RetrieveInstalledVersion();
//    VOID RetrieveLanguages();
//    VOID RetrieveLicenseType();
//    VOID RetrieveSize();
//    inline BOOL FindInLanguages(LCID what) const;
};

class CAvailableApplicationInfo
    : public CApplicationInfo
{
    class CConfigParser* m_Parser;
    CSimpleArray<CStringW> m_szScrnshotLocation;
    bool m_ScrnshotRetrieved;
  public:
    CStringW m_szUrlDownload;

    CAvailableApplicationInfo(CConfigParser* Parser, const CStringW& PkgName, AppsCategories Category, const CPathW& BasePath);
    ~CAvailableApplicationInfo();

    virtual BOOL Valid() const override;
    virtual BOOL RetrieveIcon(CStringW& Path) const override;
    virtual BOOL RetrieveScreenshot(CStringW& Path) override;
};

class CInstalledApplicationInfo
    : public CApplicationInfo
{
    CRegKey m_hKey;

    BOOL GetApplicationRegString(LPCWSTR lpKeyName, ATL::CStringW& String);

public:
    CInstalledApplicationInfo(HKEY Key, const CStringW& KeyName, AppsCategories Category);
    ~CInstalledApplicationInfo();


    virtual BOOL Valid() const override;
    virtual BOOL RetrieveIcon(CStringW& Path) const override;
    virtual BOOL RetrieveScreenshot(CStringW& Path) override;
};

//typedef BOOL(CALLBACK *AVAILENUMPROC)(CAvailableApplicationInfo *Info, BOOL bInitialCheckState, PVOID param);
//
//class CAvailableApps
//{
//    ATL::CAtlList<CAvailableApplicationInfo*> m_InfoList;
//    ATL::CAtlList< CAvailableApplicationInfo*> m_SelectedList;
//
//public:
//    static AvailableStrings m_Strings;
//
//    CAvailableApps();
//
//    static BOOL UpdateAppsDB();
//    static BOOL ForceUpdateAppsDB();
//    static VOID DeleteCurrentAppsDB();
//
//    VOID FreeCachedEntries();
//    BOOL Enum(INT EnumType, AVAILENUMPROC lpEnumProc, PVOID param);
//
//    BOOL AddSelected(CAvailableApplicationInfo *AvlbInfo);
//    BOOL RemoveSelected(CAvailableApplicationInfo *AvlbInfo);
//
//    VOID RemoveAllSelected();
//    int GetSelectedCount();
//
//    CAvailableApplicationInfo* FindAppByPkgName(const ATL::CStringW& szPkgName) const;
//    ATL::CSimpleArray<CAvailableApplicationInfo> FindAppsByPkgNameList(const ATL::CSimpleArray<ATL::CStringW> &arrAppsNames) const;
//    //ATL::CSimpleArray<CAvailableApplicationInfo> GetSelected() const;
//
//    const ATL::CStringW& GetFolderPath() const;
//    const ATL::CStringW& GetAppPath() const;
//    const ATL::CStringW& GetCabPath() const;
//};

//class CApplicationInfoList
//{
//private:
//    ATL::CAtlList<CApplicationInfo*> m_List;
//
//public:
//
//};
