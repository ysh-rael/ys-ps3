#include "calibration/profile_store.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace ps3translator
{
    namespace
    {
        const char* KindToText(BindingKind kind)
        {
            switch (kind)
            {
            case BindingKind::Bit: return "bit";
            case BindingKind::Hat: return "hat";
            case BindingKind::Analog: return "analog";
            default: return "none";
            }
        }

        BindingKind TextToKind(const std::string& text)
        {
            if (text == "bit") return BindingKind::Bit;
            if (text == "hat") return BindingKind::Hat;
            if (text == "analog") return BindingKind::Analog;
            return BindingKind::None;
        }
    }

    void WriteProfile(std::ostream& out, const DeviceProfile& profile)
    {
        // Formato: <chave> <tipo> <byte> <mask> <value> <rest> <full>
        out << "# PS3Translator profile v1\n"
            << "vendor " << profile.vendorId << '\n'
            << "product " << profile.productId << '\n';
        for (std::size_t i = 0; i < kControlCount; ++i)
        {
            const Binding& b = profile.bindings[i];
            out << GetControlInfo(static_cast<ControlId>(i)).key << ' ' << KindToText(b.kind) << ' ' << b.byteIndex
                << ' ' << unsigned{ b.mask } << ' ' << unsigned{ b.value } << ' ' << unsigned{ b.rest }
                << ' ' << unsigned{ b.full } << '\n';
        }
    }

    std::optional<DeviceProfile> ReadProfile(std::istream& in)
    {
        DeviceProfile profile;
        bool sawVendor = false;
        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }
            std::istringstream fields(line);
            std::string key;
            fields >> key;

            if (key == "vendor" || key == "product")
            {
                unsigned int value = 0;
                fields >> value;
                (key == "vendor" ? profile.vendorId : profile.productId) = static_cast<std::uint16_t>(value);
                sawVendor = sawVendor || key == "vendor";
                continue;
            }

            for (std::size_t i = 0; i < kControlCount; ++i)
            {
                if (key != GetControlInfo(static_cast<ControlId>(i)).key)
                {
                    continue;
                }
                std::string kind;
                unsigned int byteIndex = 0, mask = 0, value = 0, rest = 0, full = 0;
                if (fields >> kind >> byteIndex >> mask >> value >> rest >> full)
                {
                    Binding& b = profile.bindings[i];
                    b.kind = TextToKind(kind);
                    b.byteIndex = static_cast<std::uint16_t>(byteIndex);
                    b.mask = static_cast<std::uint8_t>(mask);
                    b.value = static_cast<std::uint8_t>(value);
                    b.rest = static_cast<std::uint8_t>(rest);
                    b.full = static_cast<std::uint8_t>(full);
                }
                break;
            }
        }
        if (!sawVendor)
        {
            return std::nullopt;
        }
        return profile;
    }

    std::filesystem::path ProfilesDirectory()
    {
        char* appData = nullptr;
        std::size_t length = 0;
        std::filesystem::path base;
        if (_dupenv_s(&appData, &length, "APPDATA") == 0 && appData != nullptr)
        {
            base = appData;
            std::free(appData);
        }
        else
        {
            base = std::filesystem::current_path();
        }
        return base / "PS3Translator" / "profiles";
    }

    std::filesystem::path ProfilePath(std::uint16_t vendorId, std::uint16_t productId)
    {
        char name[32]{};
        std::snprintf(name, sizeof(name), "%04X_%04X.profile", vendorId, productId);
        return ProfilesDirectory() / name;
    }

    bool ProfileExists(std::uint16_t vendorId, std::uint16_t productId)
    {
        std::error_code ignored;
        return std::filesystem::exists(ProfilePath(vendorId, productId), ignored);
    }

    bool SaveProfile(const DeviceProfile& profile, std::wstring& error)
    {
        std::error_code ec;
        std::filesystem::create_directories(ProfilesDirectory(), ec);
        std::ofstream file(ProfilePath(profile.vendorId, profile.productId), std::ios::trunc);
        if (ec || !file)
        {
            error = L"Não foi possível gravar o perfil em " + ProfilesDirectory().wstring();
            return false;
        }
        WriteProfile(file, profile);
        return static_cast<bool>(file);
    }

    std::optional<DeviceProfile> LoadProfile(std::uint16_t vendorId, std::uint16_t productId)
    {
        std::ifstream file(ProfilePath(vendorId, productId));
        if (!file)
        {
            return std::nullopt;
        }
        return ReadProfile(file);
    }
}
