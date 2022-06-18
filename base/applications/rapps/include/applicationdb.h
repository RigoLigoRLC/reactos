#pragma once

//#include <windef.h>
#include <atlcoll.h>
#include <atlpath.h>

//#include "misc.h"
#include "applicationinfo.h"


//#define MAX_SCRNSHOT_NUM 16

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

class CApplicationDB
{
private:
    CPathW m_BasePath;
    CAtlList<CApplicationInfo*> m_Available;
    CAtlList<CApplicationInfo*> m_Installed;

    BOOL EnumerateFiles();

    CApplicationInfo* FindByPackageName(const CStringW& name);

public:

    CApplicationDB(const CStringW& path);

    void GetApps(CAtlList<CApplicationInfo*>& List, AppsCategories Type) const;


    VOID UpdateAvailable();
    VOID UpdateInstalled();
    VOID ForceUpdateAvailable();
    VOID Delete();

};
