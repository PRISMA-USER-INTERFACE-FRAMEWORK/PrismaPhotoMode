#pragma once

#include "RuntimeSupport.h"
#include <string_view>

namespace PPM::Screenshot
{
    void Install(RuntimeSupport::Family family);
    bool Request(std::string_view frame);
}
