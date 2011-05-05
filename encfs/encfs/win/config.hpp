
void SaveConfig(const std::string& name, const std::string& entry, const std::string& value);
std::string LoadConfig(const std::string& name, const std::string& entry);
void EnumConfig(void (*proc)(const std::string& name, void* param), void *param);
void DeleteConfig(const std::string& name);
std::string ConfigNewName();

