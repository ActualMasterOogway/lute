#pragma once

#include <optional>
#include <string>

struct lua_State;

std::optional<std::string> resolveRequire(std::string requirePath, std::string requirerChunkname, std::string* error);
std::optional<std::string> readSourceFromVfs(const std::string& name);

int resolverequire_luau(lua_State* L);
