#ifndef INSTALLED_PROGRAMS
#define INSTALLED_PROGRAMS
#include <vector>
#include <Windows.h>
#include "RegistryKey.h"
using namespace std;

class Software
{
public:
	wstring DisplayName;
	wstring InstallLocation;
	wstring Version;
	wstring Icon;
	wstring Publisher;
	wstring UninstallString;
	Arch Architecture; // 32 or 64
	Software(wstring&&,
		wstring&&, 
		wstring&&, 
		wstring&&, 
		wstring&&, 
		wstring&&, 
		Arch);
};


class InstalledPrograms
{
public:
	InstalledPrograms(void) = default;
	~InstalledPrograms(void) = default;
	static vector<Software> GetInstalledPrograms();
private:
	static vector<Software> GetInstalledProgramsImp();
	static void GetUninstallKeyPrograms(
		pRegistryKey& UninstallKey, 
		pRegistryKey& ClassesKey, 
		vector<Software>&);
	static void GetUserInstallerKeyPrograms(
		pRegistryKey& uInstallerKey, 
		vector<Software>& ExistingProgramList);
};

#endif