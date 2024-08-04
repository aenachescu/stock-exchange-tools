#ifndef STOCK_EXCHANGE_TOOLS_ASSET_TYPE_H
#define STOCK_EXCHANGE_TOOLS_ASSET_TYPE_H

#include <cstdint>

enum class AssetType : std::uint8_t
{
    Unknown,
    Money,
    Bonds,
    Stock,
};

#endif // STOCK_EXCHANGE_TOOLS_ASSET_TYPE_H
