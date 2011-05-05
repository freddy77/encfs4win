#include <windows.h>
#include <stdio.h>
#include <string>
#include <stdexcept>

using namespace std;

static const char rootKeyName[] = "SOFTWARE\\EncFS";

static HKEY RootKey()
{
	HKEY root = NULL;
	if (!root) {
		if (RegCreateKeyEx(HKEY_CURRENT_USER, rootKeyName, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &root, NULL) != ERROR_SUCCESS) {
			root = NULL;
			throw runtime_error("accessing configuration");
		}
	}
	return root;
}

class AutoDeleteKey
{
public:
	operator HKEY() { return key; }
	AutoDeleteKey(HKEY hkey):key(hkey) {};
	~AutoDeleteKey() { RegCloseKey(key); }
private:
	HKEY key;
};

void SaveConfig(const std::string& name, const std::string& entry, const std::string& value)
{
	HKEY hkey;
	if (RegCreateKeyEx(RootKey(), name.c_str(), 0, NULL, 0, KEY_SET_VALUE, NULL, &hkey, NULL) != ERROR_SUCCESS)
		throw runtime_error("writing configuration");
	AutoDeleteKey key(hkey);
	if (RegSetValueEx(key, entry.c_str(), 0, REG_SZ, (const BYTE*) value.c_str(), value.length()+1) != ERROR_SUCCESS)
		throw runtime_error("writing configuration");
}

std::string LoadConfig(const std::string& name, const std::string& entry)
{
	HKEY hkey;
	if (RegOpenKeyEx(RootKey(), name.c_str(), 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
		throw runtime_error("reading configuration");
	AutoDeleteKey key(hkey);
	char buf[512];
	DWORD type, len = sizeof(buf);
	if (RegQueryValueEx(key, entry.c_str(), NULL, &type, (BYTE*) buf, &len) != ERROR_SUCCESS || type != REG_SZ)
		throw runtime_error("reading configuration");
	return buf;
}

void EnumConfig(void (*proc)(const std::string& name, void* param), void *param)
{
	DWORD i;
	for (i = 0; ; ++i) {
		char name[128];
		DWORD len = sizeof(name);
		if (RegEnumKeyEx(RootKey(), i, name, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			break;
		proc(name, param);
	}
}

void DeleteConfig(const std::string& name)
{
	RegDeleteKey(RootKey(), name.c_str());
}

std::string ConfigNewName()
{
	for (unsigned n = 1; n < 100; ++n) {
		char name[20];
		sprintf(name, "Drive%u", n);

		HKEY hkey;
		if (RegCreateKeyEx(RootKey(), name, 0, NULL, 0, KEY_QUERY_VALUE, NULL, &hkey, NULL) != ERROR_SUCCESS)
			break;
		AutoDeleteKey key(hkey);

		DWORD num;
		if (RegQueryInfoKey(key, NULL, NULL, NULL, NULL, NULL, NULL, &num, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			break;
		if (!num)
			return name;
	}
	throw runtime_error("writing configuration");
}

