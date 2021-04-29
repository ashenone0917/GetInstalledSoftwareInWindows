#include "RegistryKey.h"

#define SubKeySizeDefault 64
RegistryKey& RegistryKey::HKLM() {
	static RegistryKey Key(HKEY_LOCAL_MACHINE, Arch::UnKnown);
	return Key;
}

RegistryKey& RegistryKey::HKU() {
	static RegistryKey Key(HKEY_USERS, Arch::UnKnown);
	return Key;
}

RegistryKey::RegistryKey(HKEY hKey, Arch arch)
	:m_hKey(hKey), m_keyArch(arch) {
}

RegistryKey::~RegistryKey() {
	if (m_hKey == HKEY_LOCAL_MACHINE || m_hKey == HKEY_USERS) return;
	if (m_hKey) {
		RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
}

pRegistryKey RegistryKey::OpenSubKey64(LPCWSTR subkey) {
	return OpenSubKey(subkey, Arch::X64);
}

pRegistryKey RegistryKey::OpenSubKey32(LPCWSTR subkey) {
	return OpenSubKey(subkey, Arch::X86);
}

pRegistryKey RegistryKey::OpenSubKey(LPCWSTR subkey) {
	HKEY hKey;
	if (RegOpenKeyEx(m_hKey, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
		return nullptr;
	}
	else {
		return std::shared_ptr<RegistryKey>(new RegistryKey(hKey, Arch::UnKnown));
	}
}

pRegistryKey RegistryKey::OpenSubKey(LPCWSTR subkey, Arch a) {
	HKEY hKey = NULL;
	DWORD FLAG = 0;
	if (a == Arch::X64) FLAG = KEY_WOW64_64KEY;
	else if (a == Arch::X86) FLAG = KEY_WOW64_32KEY;

	if (RegOpenKeyEx(m_hKey, subkey, 0, KEY_READ | FLAG, &hKey) != ERROR_SUCCESS) {
		return nullptr;
	}
	else {
		return std::shared_ptr<RegistryKey>(new RegistryKey(hKey, a));
	}
}

vector<wstring> RegistryKey::GetSubKeyNames() {
	vector<wstring> ret;
	ret.reserve(SubKeySizeDefault);

	LONG lRet;
	DWORD dwIndex = 0;
	DWORD cbName = MAX_PATH;
	WCHAR szSubKeyName[MAX_PATH];
	while ((lRet = RegEnumKeyEx(
		m_hKey,
		dwIndex,
		szSubKeyName,
		&cbName,
		NULL,
		NULL,
		NULL,
		NULL)) != ERROR_NO_MORE_ITEMS)
	{
		if (lRet == ERROR_SUCCESS) {
			ret.emplace_back(szSubKeyName);
		}
		cbName = MAX_PATH;
		dwIndex++;
	}
	return std::move(ret);
}

wstring RegistryKey::GetValue(LPCWSTR query)
{
	WCHAR Value[MAX_PATH]{};
	DWORD dwSize = sizeof(Value);
	DWORD dwType;
	if (RegQueryValueEx(m_hKey, query, NULL, &dwType, (LPBYTE)&Value, &dwSize) == ERROR_SUCCESS)
	{
		if (dwType == REG_DWORD) {
			DWORD* ret = (DWORD*)Value;
			return to_wstring(*ret);
		}
		else if (dwType == REG_SZ) return wstring(Value);
		else if (dwType == REG_EXPAND_SZ) {
			WCHAR Expanded[MAX_PATH]{};
			ExpandEnvironmentStrings(Value, Expanded, MAX_PATH);
			return wstring(Expanded);
		}
	}
	else {
		return L"";
	}
}

Arch RegistryKey::arch() {
	return m_keyArch;
}


