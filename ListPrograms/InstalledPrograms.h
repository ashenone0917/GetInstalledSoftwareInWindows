#ifndef INSTALLED_PROGRAMS
#define INSTALLED_PROGRAMS
#include <vector>
#include <string>
#include <memory>
using namespace std;

enum Arch;

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

class RegistryKey;
class InstalledPrograms
{
public:
	InstalledPrograms(void) = default;
	~InstalledPrograms(void) = default;
	static vector<Software> GetInstalledPrograms();
private:
	static vector<Software> GetInstalledProgramsImp();
	static void GetUninstallKeyPrograms(
		std::shared_ptr<RegistryKey>& UninstallKey,
		std::shared_ptr<RegistryKey>& ClassesKey,
		vector<Software>&);
	static void GetUserInstallerKeyPrograms(
		std::shared_ptr<RegistryKey>& uInstallerKey,
		vector<Software>& ExistingProgramList);
};

#endif