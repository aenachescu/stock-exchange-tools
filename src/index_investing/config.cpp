#include "config.h"

#include "string_utils.h"

#include <fstream>

#define DEF_SETTER(option, field)                                              \
    static Setter option = [](Config& cfg, const std::string& val) -> void {   \
        cfg.field = val;                                                       \
    };                                                                         \
    m_setters.insert({#option, option});

Config::Config()
{
    DEF_SETTER(broker, m_broker);
    DEF_SETTER(tradeville_user, m_tradevilleUser);
    DEF_SETTER(tradeville_password, m_tradevillePass);
    DEF_SETTER(stock_exchange, m_stockExchange);
    DEF_SETTER(index, m_indexName);
    DEF_SETTER(index_adjustment_date, m_indexAdjustmentDate);
    DEF_SETTER(index_adjustment_reason, m_indexAdjustmentReason);
}

Error Config::LoadConfig()
{
    std::ifstream file(kFileName);
    if (! file) {
        return Error::FileNotFound;
    }

    std::string line;
    std::vector<std::string> tokens;

    while (std::getline(file, line)) {
        if (line.empty() == true) {
            continue;
        }

        tokens = split_string(line, ": ");
        if (tokens.size() != 2) {
            return Error::InvalidData;
        }

        auto it = m_setters.find(tokens[0]);
        if (it == m_setters.end()) {
            return Error::InvalidArg;
        }

        it->second(*this, tokens[1]);
    }

    return Error::NoError;
}
