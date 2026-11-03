#include "PhotoModeLists.h"

#include <RE/T/TESDataHandler.h>
#include <RE/T/TESWeather.h>

#include <format>
#include <string_view>

namespace PPM::PhotoModeLists
{
    namespace
    {
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
}
