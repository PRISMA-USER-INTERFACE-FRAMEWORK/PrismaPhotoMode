#include "PhotoModeLists.h"

#include <RE/B/BGSArtObject.h>
#include <RE/T/TESDataHandler.h>
#include <RE/T/TESEffectShader.h>
#include <RE/T/TESIdleForm.h>
#include <RE/T/TESImageSpaceModifier.h>
#include <RE/T/TESWeather.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <format>
#include <string_view>
#include <utility>
#include <vector>

namespace PPM::PhotoModeLists
{
    namespace
    {
        constexpr char kOverlayRoot[] = "Data/PrismaUI_F4/views/PrismaPhotoMode/overlays";
        constexpr std::size_t kMaxFormEntries = 2000;

        void AppendEscaped(std::string& a_json, std::string_view a_text)
        {
            for (const char c : a_text) {
                if (c == '"' || c == '\\') {
                    a_json += '\\';
                    a_json += c;
                } else if (static_cast<unsigned char>(c) >= 0x20) {
                    a_json += c;
                }
            }
        }

        bool IsPng(const std::filesystem::path& a_path)
        {
            auto extension = a_path.extension().string();
            std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return extension == ".png";
        }

        template <class T>
        std::string FormArray()
        {
            std::vector<std::pair<std::string, std::uint32_t>> entries;
            if (auto* handler = RE::TESDataHandler::GetSingleton()) {
                for (auto* form : handler->GetFormArray<T>()) {
                    if (!form) continue;
                    const char* editorId = form->GetFormEditorID();
                    entries.emplace_back(editorId && *editorId ? std::string{ editorId } : std::format("{:08X}", form->GetFormID()), form->GetFormID());
                }
            }
            std::ranges::sort(entries);
            if (entries.size() > kMaxFormEntries) entries.resize(kMaxFormEntries);

            std::string json{ "[" };
            bool first = true;
            for (const auto& [name, id] : entries) {
                if (!first) json += ',';
                first = false;
                json += std::format(R"({{"id":{},"name":")", id);
                AppendEscaped(json, name);
                json += "\"}";
            }
            json += ']';
            return json;
        }
    }

    std::string Build()
    {
        return std::string{ R"({"weathers":)" } + FormArray<RE::TESWeather>() + "}";
    }

    std::string BuildFilters()
    {
        return std::string{ R"({"filters":)" } + FormArray<RE::TESImageSpaceModifier>() + "}";
    }

    std::string BuildCharacterForms()
    {
        return std::string{ R"({"idles":)" } + FormArray<RE::TESIdleForm>() + R"(,"arts":)" + FormArray<RE::BGSArtObject>() + R"(,"shaders":)" + FormArray<RE::TESEffectShader>() + "}";
    }

    std::string BuildPick(const char* a_name)
    {
        std::string json{ R"({"name":")" };
        AppendEscaped(json, a_name ? a_name : "");
        return json + "\"}";
    }

    std::string BuildOverlays()
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        std::vector<fs::path> categories;
        for (const auto& entry : fs::directory_iterator(kOverlayRoot, ec)) {
            if (entry.is_directory(ec)) categories.push_back(entry.path());
        }
        std::ranges::sort(categories);

        std::string json{ R"({"categories":[)" };
        bool firstCategory = true;
        for (const auto& category : categories) {
            std::vector<std::string> names;
            for (const auto& entry : fs::directory_iterator(category, ec)) {
                if (entry.is_regular_file(ec) && IsPng(entry.path())) names.push_back(entry.path().filename().string());
            }
            std::ranges::sort(names);

            if (!firstCategory) json += ',';
            firstCategory = false;
            json += R"({"name":")";
            AppendEscaped(json, category.filename().string());
            json += R"(","overlays":[)";
            for (std::size_t i = 0; i < names.size(); ++i) {
                if (i) json += ',';
                json += '"';
                AppendEscaped(json, names[i]);
                json += '"';
            }
            json += "]}";
        }
        json += "]}";
        return json;
    }
}
