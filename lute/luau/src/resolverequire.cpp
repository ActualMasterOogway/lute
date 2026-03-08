#include "lute/resolverequire.h"

#include "lute/batteriesvfs.h"
#include "lute/clibatteries.h"
#include "lute/filevfs.h"
#include "lute/lutevfs.h"
#include "lute/lutemodules.h"
#include "lute/modulepath.h"
#include "lute/stdlib.h"
#include "lute/stdlibvfs.h"

#include "Luau/Common.h"
#include "Luau/FileUtils.h"
#include "Luau/RequireNavigator.h"

#include "lua.h"
#include "lualib.h"

#include <optional>
#include <string>

// LuteVfsContext
class LuteVfsContext : public Luau::Require::NavigationContext
{
public:
    LuteVfsContext(std::string requirerChunkname);

    NavigateResult resetToRequirer() override;
    NavigateResult jumpToAlias(const std::string& path) override;

    NavigateResult toParent() override;
    NavigateResult toChild(const std::string& component) override;

    ConfigStatus getConfigStatus() const override;

    ConfigBehavior getConfigBehavior() const override;
    std::optional<std::string> getAlias(const std::string& alias) const override;
    std::optional<std::string> getConfig() const override;

    FileVfs fileVfs;
    StdLibVfs stdLibVfs;
    LuteVfs luteVfs;
    BatteriesVfs batteriesVfs;

    enum class VFSType
    {
        Disk,
        Std,
        Lute,
        Batteries,
    };
    VFSType vfsType = VFSType::Disk;

    std::string requirerChunkname;
};

using NC = Luau::Require::NavigationContext;

static NC::NavigateResult convert(NavigationStatus status)
{
    NC::NavigateResult result = NC::NavigateResult::NotFound;
    switch (status)
    {
    case NavigationStatus::Success:
        result = NC::NavigateResult::Success;
        break;
    case NavigationStatus::Ambiguous:
        result = NC::NavigateResult::Ambiguous;
        break;
    case NavigationStatus::NotFound:
        result = NC::NavigateResult::NotFound;
        break;
    }
    return result;
}

static NC::ConfigStatus convert(ConfigStatus status)
{
    NC::ConfigStatus result = NC::ConfigStatus::Ambiguous;
    switch (status)
    {
    case ConfigStatus::Absent:
        result = NC::ConfigStatus::Absent;
        break;
    case ConfigStatus::Ambiguous:
        result = NC::ConfigStatus::Ambiguous;
        break;
    case ConfigStatus::PresentJson:
        result = NC::ConfigStatus::PresentJson;
        break;
    case ConfigStatus::PresentLuau:
        result = NC::ConfigStatus::PresentLuau;
        break;
    }
    return result;
}

LuteVfsContext::LuteVfsContext(std::string requirerChunkname)
    : requirerChunkname(std::move(requirerChunkname))
{
}

NC::NavigateResult LuteVfsContext::resetToRequirer()
{
    if (requirerChunkname.rfind("@std", 0) == 0)
    {
        vfsType = VFSType::Std;
        return convert(stdLibVfs.resetToPath(requirerChunkname));
    }
    else if (requirerChunkname.rfind("@lute", 0) == 0)
    {
        vfsType = VFSType::Lute;
        return convert(luteVfs.resetToPath(requirerChunkname));
    }
    else if (requirerChunkname.rfind("@batteries", 0) == 0)
    {
        vfsType = VFSType::Batteries;
        return convert(batteriesVfs.resetToPath(requirerChunkname));
    }
    else
    {
        vfsType = VFSType::Disk;
        return convert(fileVfs.resetToPath(requirerChunkname));
    }
}

NC::NavigateResult LuteVfsContext::jumpToAlias(const std::string& path)
{
    if (path.rfind("@std", 0) == 0)
    {
        vfsType = VFSType::Std;
        return convert(stdLibVfs.resetToPath(path));
    }
    else if (path.rfind("@lute", 0) == 0)
    {
        vfsType = VFSType::Lute;
        return convert(luteVfs.resetToPath(path));
    }
    else if (path.rfind("@batteries", 0) == 0)
    {
        vfsType = VFSType::Batteries;
        return convert(batteriesVfs.resetToPath(path));
    }
    else
    {
        vfsType = VFSType::Disk;
        return convert(fileVfs.resetToPath(path));
    }
}

NC::NavigateResult LuteVfsContext::toParent()
{
    switch (vfsType)
    {
    case VFSType::Disk:
        return convert(fileVfs.toParent());
    case VFSType::Std:
        return convert(stdLibVfs.toParent());
    case VFSType::Lute:
        return convert(luteVfs.toParent());
    case VFSType::Batteries:
        return convert(batteriesVfs.toParent());
    }
    return NC::NavigateResult::NotFound;
}

NC::NavigateResult LuteVfsContext::toChild(const std::string& component)
{
    switch (vfsType)
    {
    case VFSType::Disk:
        return convert(fileVfs.toChild(component));
    case VFSType::Std:
        return convert(stdLibVfs.toChild(component));
    case VFSType::Lute:
        return convert(luteVfs.toChild(component));
    case VFSType::Batteries:
        return convert(batteriesVfs.toChild(component));
    }
    return NC::NavigateResult::NotFound;
}

NC::ConfigStatus LuteVfsContext::getConfigStatus() const
{
    switch (vfsType)
    {
    case VFSType::Disk:
        return convert(fileVfs.getConfigStatus());
    case VFSType::Std:
        return convert(stdLibVfs.getConfigStatus());
    case VFSType::Lute:
        return convert(luteVfs.getConfigStatus());
    case VFSType::Batteries:
        return convert(batteriesVfs.getConfigStatus());
    }
    return NC::ConfigStatus::Absent;
}

NC::ConfigBehavior LuteVfsContext::getConfigBehavior() const
{
    return NC::ConfigBehavior::GetConfig;
}

std::optional<std::string> LuteVfsContext::getAlias(const std::string& alias) const
{
    if (alias == "std")
        return "@std";
    if (alias == "lute")
        return "@lute";
    if (alias == "batteries")
        return "@batteries";
    return std::nullopt;
}

std::optional<std::string> LuteVfsContext::getConfig() const
{
    switch (vfsType)
    {
    case VFSType::Disk:
        return fileVfs.getConfig();
    case VFSType::Std:
        return stdLibVfs.getConfig();
    case VFSType::Lute:
        return luteVfs.getConfig();
    case VFSType::Batteries:
        return batteriesVfs.getConfig();
    }
    return std::nullopt;
}

// ErrorCapturer
class ErrorCapturer : public Luau::Require::ErrorHandler
{
public:
    void reportError(std::string message) override;
    std::optional<std::string> error = std::nullopt;
};

void ErrorCapturer::reportError(std::string message)
{
    error = message;
}

// Public API
std::optional<std::string> resolveRequire(std::string requirePath, std::string requirerChunkname, std::string* error)
{
    if (requirerChunkname.empty() || requirerChunkname[0] != '@')
    {
        if (error)
            *error = "requirer chunkname must start with '@'";
        return std::nullopt;
    }

    LuteVfsContext context{requirerChunkname.substr(1)};
    ErrorCapturer errorCapturer{};

    Luau::Require::Navigator navigator{context, errorCapturer};
    Luau::Require::Navigator::Status status = navigator.navigate(requirePath);

    if (status == Luau::Require::Navigator::Status::ErrorReported)
    {
        if (error && errorCapturer.error)
            *error = *errorCapturer.error;
        return std::nullopt;
    }

    std::optional<std::string> result = std::nullopt;
    switch (context.vfsType)
    {
    case LuteVfsContext::VFSType::Disk:
        result = context.fileVfs.getAbsoluteFilePath();
        break;
    case LuteVfsContext::VFSType::Std:
        result = context.stdLibVfs.getIdentifier();
        break;
    case LuteVfsContext::VFSType::Lute:
        result = context.luteVfs.getIdentifier();
        break;
    case LuteVfsContext::VFSType::Batteries:
        result = context.batteriesVfs.getIdentifier();
        break;
    }

    return result;
}

std::optional<std::string> readSourceFromVfs(const std::string& name)
{
    if (name.rfind("@std", 0) == 0)
    {
        StdLibModuleResult result = getStdLibModule(name);
        if (result.type == StdLibModuleType::Module)
            return std::string(result.contents);
    }
    else if (name.rfind("@lute", 0) == 0)
    {
        LuteModuleResult result = getLuteModule(name);
        if (result.type == LuteModuleType::Module)
            return std::string(result.contents);
    }
    else if (name.rfind("@batteries", 0) == 0)
    {
        BatteryModuleResult result = getBatteryModule(name);
        if (result.type == BatteryModuleType::Module)
            return std::string(result.contents);
    }

    return readFile(name);
}

int resolverequire_luau(lua_State* L)
{
    std::string requirePath = luaL_checkstring(L, 1);
    std::string requirerChunkname = luaL_checkstring(L, 2);

    std::string error;
    std::optional<std::string> absolutePath = resolveRequire(requirePath, requirerChunkname, &error);
    if (!absolutePath)
        luaL_error(L, "%s", error.c_str());

    lua_pushlstring(L, absolutePath->c_str(), absolutePath->size());
    return 1;
}
