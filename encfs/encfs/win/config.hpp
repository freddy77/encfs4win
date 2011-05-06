
struct Config
{
	static void Save(const std::string& name, const std::string& entry, const std::string& value);
	static std::string Load(const std::string& name, const std::string& entry);
	static void Enum(void (*proc)(const std::string& name, void* param), void *param);
	static void Delete(const std::string& name);
	static std::string NewName();
};
