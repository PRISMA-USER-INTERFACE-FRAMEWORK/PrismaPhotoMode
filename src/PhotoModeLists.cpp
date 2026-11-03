#include "PhotoModeLists.h"
#include "RuntimeSupport.h"

#include <RE/B/BGSArtObject.h>
#include <RE/B/BGSKeyword.h>
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

        using Entries = std::vector<std::pair<std::string, std::uint32_t>>;

        std::string EntriesJson(Entries a_entries)
        {
            std::ranges::sort(a_entries);
            if (a_entries.size() > kMaxFormEntries) a_entries.resize(kMaxFormEntries);

            std::string json{ "[" };
            bool first = true;
            for (const auto& [name, id] : a_entries) {
                if (!first) json += ',';
                first = false;
                json += std::format(R"({{"id":{},"name":")", id);
                AppendEscaped(json, name);
                json += "\"}";
            }
            json += ']';
            return json;
        }

        template <class T>
        std::string FormArray()
        {
            Entries entries;
            if (auto* handler = RE::TESDataHandler::GetSingleton()) {
                for (auto* form : handler->GetFormArray<T>()) {
                    if (!form) continue;
                    const char* editorId = form->GetFormEditorID();
                    entries.emplace_back(editorId && *editorId ? std::string{ editorId } : std::format("{:08X}", form->GetFormID()), form->GetFormID());
                }
            }
            return EntriesJson(std::move(entries));
        }

        std::string ExpressionArray()
        {
            constexpr std::string_view kPrefix{ "AnimFaceArchetype" };
            Entries entries;
            if (RuntimeSupport::ExpressionsAvailable()) {
                if (const auto keywords = RE::BGSKeyword::GetTypedKeywords()) {
                    for (auto* keyword : (*keywords)[std::to_underlying(RE::KeywordType::kAnimFaceArchetype)]) {
                        if (!keyword) continue;
                        const char* editorId = keyword->GetFormEditorID();
                        std::string name = editorId && *editorId ? std::string{ editorId } : std::format("{:08X}", keyword->GetFormID());
                        if (name.starts_with(kPrefix) && name.size() > kPrefix.size()) name.erase(0, kPrefix.size());
                        entries.emplace_back(std::move(name), keyword->GetFormID());
                    }
                }
            }
            return EntriesJson(std::move(entries));
        }

        std::string OverlayCategories(const std::filesystem::path& a_root)
        {
            namespace fs = std::filesystem;
            std::error_code ec;
            std::vector<fs::path> categories;
            for (const auto& entry : fs::directory_iterator(a_root, ec)) {
                if (entry.is_directory(ec)) categories.push_back(entry.path());
            }
            std::ranges::sort(categories);

            std::string json{ "[" };
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
        return std::string{ R"({"idles":)" } + FormArray<RE::TESIdleForm>() + R"(,"arts":)" + FormArray<RE::BGSArtObject>() + R"(,"shaders":)" + FormArray<RE::TESEffectShader>() + R"(,"expressions":)" + ExpressionArray() + "}";
    }

    std::string BuildPick(const char* a_name)
    {
        std::string json{ R"({"name":")" };
        AppendEscaped(json, a_name ? a_name : "");
        return json + "\"}";
    }

    std::string BuildOverlays()
    {
        const std::filesystem::path root{ kOverlayRoot };
        return std::string{ R"({"frames":)" } + OverlayCategories(root / "frames") + R"(,"textures":)" + OverlayCategories(root / "textures") + "}";
    }
}
