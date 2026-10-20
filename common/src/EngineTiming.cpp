#include "idk/EngineTiming.hpp"
#include "libidk/cfgparser.hpp"
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <cstring>

namespace idk
{
    namespace
    {
        bool validRate(double value, double minimum, bool allowZero)
        {
            return std::isfinite(value) && ((allowZero && value == 0) || (value >= minimum && value <= 1000));
        }
    }

    bool EngineTimingConfig::valid() const
    {
        return validRate(fixedRateHz, 4, false) && validRate(inputRateHz, 1, true) &&
               validRate(renderRateHz, 1, true) && validRate(physicsRateHz, 30, false);
    }

    const char *EngineTimingConfig::loadFile(const char *path, bool optional)
    {
        CfgParser parser;
        errno = 0;
        if (!parser.load(path))
            return optional && errno == ENOENT ? nullptr : parser.error();
        const auto *section = parser.find("timing");
        if (!section)
            return nullptr;
        if (section->isLeaf())
            return "timing must be a section";
        const auto &timing = *static_cast<const CfgParser::TreeNode *>(section);
        EngineTimingConfig candidate = *this;
        struct Setting
        {
            const char *name;
            double *value;
            double minimum;
            bool allowZero;
        };
        Setting settings[] = {
            {"fixed_hz", &candidate.fixedRateHz, 4, false},
            {"input_hz", &candidate.inputRateHz, 1, true},
            {"render_hz", &candidate.renderRateHz, 1, true},
            {"physics_hz", &candidate.physicsRateHz, 30, false}};
        for (const auto &[name, node] : timing)
        {
            if (name != "fixed_hz" && name != "input_hz" && name != "render_hz" &&
                name != "physics_hz" && name != "vsync")
                return "Unknown setting in [timing]";
            if (!node->isLeaf())
                return "Timing settings must be values, not sections";
        }
        for (const auto &setting : settings)
        {
            const auto *node = timing.find(setting.name);
            if (!node)
                continue;
            const char *text = static_cast<const CfgParser::TreeLeaf *>(node)->getValue();
            const char *end = text + std::strlen(text);
            auto result = std::from_chars(text, end, *setting.value);
            if (result.ec != std::errc{} || result.ptr != end ||
                !validRate(*setting.value, setting.minimum, setting.allowZero))
                return setting.name;
        }
        if (const auto *node = timing.find("vsync"))
        {
            const char *text = static_cast<const CfgParser::TreeLeaf *>(node)->getValue();
            if (std::strcmp(text, "0") && std::strcmp(text, "1"))
                return "vsync";
            candidate.vsync = text[0] == '1';
        }
        *this = candidate;
        return nullptr;
    }
}
