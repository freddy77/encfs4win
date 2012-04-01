#include "i18n.h"

struct Config
{
	static void Save(const std::string& name, const std::tstring& entry, const std::tstring& value);
	static std::tstring Load(const std::string& name, const std::tstring& entry);
	static void Enum(void (*proc)(const std::string& name, void* param), void *param);
	static void Delete(const std::string& name);
	static std::string NewName();
	static void SaveGlobal(const std::string& entry, int value);
	static int LoadGlobal(const std::string& entry);
};
