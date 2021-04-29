#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>

using namespace std;

enum Arch
{
	UnKnown = 0,
	X86 = 1,
	X64 = 2
};

class RegistryKey;
using u_RegistryKey = std::unique_ptr<RegistryKey>;
using pRegistryKey = std::shared_ptr<RegistryKey>;

class RegistryKey
{
public:
	~RegistryKey();
	pRegistryKey OpenSubKey64(LPCWSTR subkey);
	pRegistryKey OpenSubKey32(LPCWSTR subkey);
	pRegistryKey OpenSubKey(LPCWSTR subkey);
	pRegistryKey OpenSubKey(LPCWSTR subkey, Arch a);

	vector<wstring> GetSubKeyNames();
	wstring GetValue(LPCWSTR query);
	Arch arch();
	static RegistryKey& HKLM();
	static RegistryKey& HKU();
public:
private:
	RegistryKey(HKEY hKey, Arch arch);
	RegistryKey() = delete;
	RegistryKey(const RegistryKey&) = delete;
	RegistryKey& operator=(const RegistryKey&) = delete;
	RegistryKey(RegistryKey&&) = delete;
	RegistryKey& operator=(RegistryKey&&) = delete;
private:
	Arch m_keyArch = Arch::UnKnown;
	HKEY m_hKey = NULL;
};
