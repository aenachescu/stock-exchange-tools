#ifndef STOCK_EXCHANGE_TOOLS_ACTIVITY_TYPE_H
#define STOCK_EXCHANGE_TOOLS_ACTIVITY_TYPE_H

enum class ActivityType
{
    Unknown,
    Buy,
    Sell,
    Dividend,
    Deposit,
    AssetTransfer,
    Tax,
    Out,
};

#endif // STOCK_EXCHANGE_TOOLS_ACTIVITY_TYPE_H
