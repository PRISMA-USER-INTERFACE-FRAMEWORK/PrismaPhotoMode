#include "PhotoModeLists.h"

#include <RE/T/TESDataHandler.h>
#include <RE/T/TESImageSpaceModifier.h>
#include <RE/T/TESWeather.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <string_view>
#include <vector>

namespace PPM::PhotoModeLists
{
    namespace
    {
        constexpr char kOverlayRoot[] = "Data/PrismaUI_F4/views/PrismaPhotoMode/overlays";

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

        template <class T>
        std::string FormArray()
        {
            std::string json{ "[" };
            if (auto* handler = RE::TESDataHandler::GetSingleton()) {
                bool first = true;
                for (auto* form : handler->GetFormArray<T>()) {
                    if (!form) continue;
                    if (!first) json += ',';
                    first = false;
                    const char* editorId = form->GetFormEditorID();
                    const auto name = editorId && *editorId ? std::string{ editorId } : std::format("{:08X}", form->GetFormID());
                    json += std::format(R"({{"id":{},"name":")", form->GetFormID());
                    AppendEscaped(json, name);
                    json += "\"}";
                }
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
                if (entry.is_regular_file(ec) && entry.path().extension().string() == ".png") names.push_back(entry.path().filename().string());
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
