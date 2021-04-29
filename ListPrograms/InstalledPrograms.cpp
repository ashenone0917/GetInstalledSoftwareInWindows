#include "InstalledPrograms.h"
#include <algorithm>
#include <regex>

wstring GetInstallerKeyNameFromGuid(wstring GuidName);
void AddToList(vector<Software>& TheList, 
	wstring& DisplayName,
	wstring& InstallLocation,
	wstring& Version,
	wstring& Icon,
	wstring& Publisher,
	wstring& UninstallString,
	Arch Architecture);

#define SYSTEM_COMPONENT L"SystemComponent"
#define WINDOWS_INSTALLER L"WindowsInstaller"
#define RELEASE_TYPE L"ReleaseType"
#define DISPLAY_VERSION L"DisplayVersion"
#define DISPLAY_NAME L"DisplayName"
#define INSTALL_LOCATION L"InstallLocation"
#define DISPLAY_ICON L"DisplayIcon"
#define UNISTALL_STRING L"UninstallString"
#define ESTIMATE_SIZE L"EstimatedSize"
#define PUBLISHER L"Publisher"

Software::Software(
	wstring&& name, 
	wstring&& location, 
	wstring&& version, 
	wstring&& icon, 
	wstring&& publisher,
	wstring&& uninstallString,
	Arch arch): 
	DisplayName(forward<wstring>(name)), 
	InstallLocation(forward<wstring>(location)),
	Version(forward<wstring>(version)),
	Icon(forward<wstring>(icon)),
	Publisher(forward<wstring>(publisher)),
	UninstallString(forward<wstring>(uninstallString)),
	Architecture(arch) {

}

BOOL Is64System()
{
	BOOL bRet = FALSE;

	SYSTEM_INFO	si;
	typedef void (WINAPI* GetNativeSystemInfo_T)(LPSYSTEM_INFO lpSystemInfo);
	static GetNativeSystemInfo_T _GetNativeSystemInfo =
		(GetNativeSystemInfo_T)::GetProcAddress(
			::GetModuleHandleW(L"kernel32"), "GetNativeSystemInfo");
	if (NULL == _GetNativeSystemInfo) {
		::GetSystemInfo(&si);
	}
	else {
		_GetNativeSystemInfo(&si);
	}

	if (PROCESSOR_ARCHITECTURE_IA64 == si.wProcessorArchitecture ||
		PROCESSOR_ARCHITECTURE_AMD64 == si.wProcessorArchitecture ||
		PROCESSOR_ARCHITECTURE_ALPHA64 == si.wProcessorArchitecture) {
		bRet = TRUE;
	}

	return bRet;
}

vector<Software> InstalledPrograms::GetInstalledPrograms() {
	return GetInstalledProgramsImp();
}

vector<Software> InstalledPrograms::GetInstalledProgramsImp()
{
    vector<Software> SoftwareList;
	SoftwareList.reserve(100);

	auto ClassesKey = RegistryKey::HKLM().OpenSubKey(
		L"Software\\Classes\\Installer\\Products");

	// The Classes\\Installer\\Products key is shared 
	// Documentation Here http://msdn.microsoft.com/en-us/library/windows/desktop/aa384253(v=vs.85).aspx

	auto Wow64UninstallKey = RegistryKey::HKLM().OpenSubKey32(
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
	GetUninstallKeyPrograms(Wow64UninstallKey, ClassesKey, SoftwareList);

	if (Is64System()) {
		auto UninstallKey = RegistryKey::HKLM().OpenSubKey64(
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
		GetUninstallKeyPrograms(UninstallKey, ClassesKey, SoftwareList);
	}
	

	vector<wstring> subkeys = RegistryKey::HKU().GetSubKeyNames();
	
	for(auto & it : subkeys)
	{
		// These Two Keys are Both Shared
		// Documentation Here http://msdn.microsoft.com/en-us/library/windows/desktop/aa384253(v=vs.85).aspx
		wstring uninstallsubs = it + L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
		auto UninstallKey = RegistryKey::HKU().OpenSubKey(uninstallsubs.c_str());
		GetUninstallKeyPrograms(UninstallKey, ClassesKey, SoftwareList);

		wstring installersubs = it + L"\\Software\\Microsoft\\Installer\\Products";
		auto InstallerKey = RegistryKey::HKU().OpenSubKey(installersubs.c_str());
		GetUserInstallerKeyPrograms(InstallerKey, SoftwareList);
	}

	std::sort(SoftwareList.begin(), SoftwareList.end(), 
		[](Software s1, Software s2){
		std::transform(s1.DisplayName.begin(), s1.DisplayName.end(), s1.DisplayName.begin(), ::tolower);
		std::transform(s2.DisplayName.begin(), s2.DisplayName.end(), s2.DisplayName.begin(), ::tolower);
		return s1.DisplayName < s2.DisplayName;
		} );
	return std::move(SoftwareList);

}

void InstalledPrograms::GetUserInstallerKeyPrograms(
	pRegistryKey& uInstallerKey, 
	vector<Software>& ExistingProgramList)
{
	if(uInstallerKey == nullptr) return;

	auto UserData32 = RegistryKey::HKLM().OpenSubKey32(
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData");
	pRegistryKey UserData64 = nullptr;
	if (Is64System()) {
		UserData64.swap(RegistryKey::HKLM().OpenSubKey64(
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData"));
	}
	/*auto UserData64 = RegistryKey::HKLM().OpenSubKey64(
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData");*/

	if(UserData32== nullptr && UserData64== nullptr) return;

	pRegistryKey UserDatas[2] = {UserData32, UserData64};
	for(int two = 0; two < 2; two++)
	{
		pRegistryKey UserData = UserDatas[two];
		if(UserData== nullptr) continue;

		vector<wstring> subkeys = uInstallerKey->GetSubKeyNames();
		for(auto & it : subkeys)
		{
			vector<wstring> userDataSubKeys = UserData->GetSubKeyNames();
			bool ProductFound = false;
			for(auto & userdatait : userDataSubKeys)
			{
				//Ignore the LocalSystem account
				if (userdatait == L"S-1-5-18") {
					continue;
				}
			
				auto ProductsKey = RegistryKey::HKLM().OpenSubKey(
					wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\").
					append(userdatait).append(L"\\Products").c_str());
                
				if (ProductsKey != nullptr)
				{
					vector<wstring> LmProductGuids = ProductsKey->GetSubKeyNames();
					for(auto &  productit : LmProductGuids) 
					{
						if (productit == it) 
						{	
							auto UserDataProgramKey = RegistryKey::HKLM().OpenSubKey(
								wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\").
								append(userdatait).append(L"\\Products\\").append(productit).append(L"\\InstallProperties").c_str());
							if(UserDataProgramKey!= nullptr)
							{
								if ( UserDataProgramKey->GetValue(L"SystemComponent").empty() || 
									_wtoi(UserDataProgramKey->GetValue(L"SystemComponent").c_str())!=1 ) 
								{
									auto temp = uInstallerKey->OpenSubKey(it.c_str(), uInstallerKey->arch());

									// Name 
									wstring Name = temp->GetValue(L"ProductName");
									if(Name.empty()) Name = UserDataProgramKey->GetValue(DISPLAY_NAME);

									// Icon
									wstring Icon = temp->GetValue(L"ProductIcon");
									if (Icon.empty()) Icon = UserDataProgramKey->GetValue(DISPLAY_ICON);
									

									if ( Name.empty()) 
									{
										auto arch = UserData->arch();
										AddToList(ExistingProgramList,
											Name,
											UserDataProgramKey->GetValue(INSTALL_LOCATION),
											UserDataProgramKey->GetValue(DISPLAY_VERSION),
											Icon,
											UserDataProgramKey->GetValue(PUBLISHER),
											UserDataProgramKey->GetValue(UNISTALL_STRING),
											arch);
										ProductFound = true;
									}
								}
							}
							break;
						}
					}
					if(ProductFound)  {
						break; // TODO: might not be correct. Was : Exit For
					}
				}
			}
		}
	}
}

void InstalledPrograms::GetUninstallKeyPrograms(
	pRegistryKey& UninstallKey, 
	pRegistryKey& ClassesKey, 
	vector<Software>& ExistingProgramList)
{
    //Make sure the key exists
    if (UninstallKey != nullptr)
	{
        //Loop through all subkeys (each one represents an installed program)
		vector<wstring> UninstallSubKeys = UninstallKey->GetSubKeyNames();
		for(auto & SubKeyName : UninstallSubKeys) 
		{
			auto CurrentSubKey = UninstallKey->OpenSubKey(SubKeyName.c_str(), UninstallKey->arch());
			if (CurrentSubKey == nullptr) continue;

            int IsSystemComponent = 1;
            bool ErrorCheckingSystemComponent = false;
            
			wstring issystemcomponent = CurrentSubKey->GetValue(SYSTEM_COMPONENT);
			if( issystemcomponent.empty() || _wtoi(issystemcomponent.c_str())!= 1 )
				IsSystemComponent = 0;
            
            if (IsSystemComponent != 1) 
			{
                //If the WindowsInstaller flag is set then add the key name to our list of Windows Installer GUIDs
				auto windows_install = CurrentSubKey->GetValue(WINDOWS_INSTALLER);
				if (windows_install.empty() ||
					_wtoi(windows_install.c_str())!= 1)
				{
                    static wregex WindowsUpdateRegEx(L"KB[0-9]{6}$");
					// Release Type
					wstring ProgramReleaseType = CurrentSubKey->GetValue(RELEASE_TYPE);

					//Check to see if this program is classed as an update
					if (!(regex_match(SubKeyName, WindowsUpdateRegEx) == true || 
						!CurrentSubKey->GetValue(L"ParentKeyName").empty() ||
						ProgramReleaseType == L"Security Update" ||
						ProgramReleaseType == L"Update Rollup" ||
						ProgramReleaseType == L"Hotfix")) {
						wstring uninstallvalue = CurrentSubKey->GetValue(UNISTALL_STRING);
						if (!uninstallvalue.empty()) {
							// Name 
							wstring Name = CurrentSubKey->GetValue(DISPLAY_NAME);

							if (!Name.empty()) {
								auto arch = UninstallKey->arch();
								AddToList(ExistingProgramList,
									Name,
									CurrentSubKey->GetValue(INSTALL_LOCATION),
									CurrentSubKey->GetValue(DISPLAY_VERSION),
									CurrentSubKey->GetValue(DISPLAY_ICON),
									CurrentSubKey->GetValue(PUBLISHER),
									CurrentSubKey->GetValue(UNISTALL_STRING),
									arch);
							}
						}
					}
                } 
				//If WindowsInstaller
				else 
				{
                    // Name
					wstring ProductName{};
					wstring ProductIcon{};
                    wstring MsiKeyName = GetInstallerKeyNameFromGuid(SubKeyName);
					pRegistryKey CrGuidKey = ClassesKey->OpenSubKey(
						MsiKeyName.c_str(),ClassesKey->arch());
                    if (CrGuidKey != nullptr) {
						ProductName = CrGuidKey->GetValue(L"ProductName");
						ProductIcon = CrGuidKey->GetValue(L"ProductIcon");
                    }
					wstring Name = ProductName;
					if(ProductName.empty()) Name = CurrentSubKey->GetValue(DISPLAY_NAME);

					// Icon
					wstring Icon = ProductIcon;
					if(ProductIcon.empty()) Icon = CurrentSubKey->GetValue(DISPLAY_ICON);

					if (!Name.empty()) {
						auto arch = UninstallKey->arch();
                        AddToList(ExistingProgramList,
							Name,
							CurrentSubKey->GetValue(INSTALL_LOCATION),
							CurrentSubKey->GetValue(DISPLAY_VERSION),
							Icon,
							CurrentSubKey->GetValue(PUBLISHER),
							CurrentSubKey->GetValue(UNISTALL_STRING),
							arch);
                    }
                }
            }
        }
    }
}

// Util Functions
void AddToList(vector<Software>& TheList,
	wstring& DisplayName,
	wstring& InstallLocation,
	wstring& Version,
	wstring& Icon,
	wstring& Publisher,
	wstring& UninstallString,
	Arch Architecture)
{
	do
	{
		int index = -1;
		for (int i = 0; i < TheList.size(); i++) {
			if (TheList[i].DisplayName == DisplayName) {
				index = i;
				break;
			}
		}
		if (index == -1) break;		
		else {
			Software& duplicate = TheList[index];

			// Merge Architecture
			if (Architecture != Arch::UnKnown &&
				duplicate.Architecture != Arch::UnKnown &&
				duplicate.Architecture != Architecture) {
				break;
			}
			else {
				if (Architecture == Arch::UnKnown)
					Architecture = duplicate.Architecture;
			}

			// Merge Icon
			if (!Icon.empty() &&
				!duplicate.Icon.empty() &&
				Icon != duplicate.Icon) {
				break;
			}
			else {
				if (Icon.empty())
					Icon = duplicate.Icon;
			}

			// Merge Location
			if (!InstallLocation.empty() &&
				!duplicate.InstallLocation.empty() &&
				InstallLocation != duplicate.InstallLocation) {
				break;
			}
			else {
				if (InstallLocation.empty())
					InstallLocation = duplicate.InstallLocation;
			}

			// Merge Version
			if (!Version.empty() &&
				!duplicate.Version.empty() &&
				Version != duplicate.Version) {
				break;
			}
			else {
				if (Version.empty())
					Version = duplicate.Version;
			}
			TheList.erase(TheList.begin() + index);
		}
	} while (0);

	TheList.emplace_back(move(DisplayName),
		move(InstallLocation),
		move(Version),
		move(Icon),
		move(Publisher),
		move(UninstallString),
		Architecture);
}

wstring ReverseString(wstring input);
void replaceAll(wstring& str, const wstring& from, const wstring& to);
vector<wstring> split( const wstring& text, wchar_t delimiter );

wstring GetInstallerKeyNameFromGuid(wstring GuidName)
{
	replaceAll(GuidName, L"{", L"");
	replaceAll(GuidName, L"}", L"");
	vector<wstring> MsiNameParts = split(GuidName, L'-');
    wstring MsiName;
    //Just reverse the first 3 parts
    for (int i = 0; i <= 2; i++) {
        MsiName.append(ReverseString(MsiNameParts[i]));
    }
    //For the last 2 parts, reverse each character pair
    for (int j = 3; j <= 4; j++) {
        for (int i = 0; i <= MsiNameParts[j].length() - 1; i++) {
            MsiName.append(wstring(1,MsiNameParts[j].c_str()[i + 1]));
            MsiName.append(wstring(1,MsiNameParts[j].c_str()[i]));
            i += 1;
        }
    }
    return std::move(MsiName);
}

wstring ReverseString(wstring input) {
	std::reverse(input.begin(), input.end());
	return std::move(input);
}

void replaceAll(wstring& str, const wstring& from, const wstring& to) 
{
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != wstring::npos)  {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

vector<wstring> split( const wstring& text, wchar_t delimiter )
{
	if (text.empty()) return std::vector<wstring>();

	wstring::size_type start = 0;
	wstring::size_type end = 0;
	wstring::size_type size = text.size();
	std::vector<wstring> retVal{};
	do
	{
		end = text.find(delimiter, start);
		if (end == wstring::npos) end = size;
		if (start != end) retVal.emplace_back(text.substr(start, end - start));
		start = end + 1;
	} while (start < size);

	return std::move(retVal);
}