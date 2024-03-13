#ifndef STOCK_EXCHANGE_TOOLS_CLI_UTILS_H
#define STOCK_EXCHANGE_TOOLS_CLI_UTILS_H

#include <string>
#include <vector>

using Table = std::vector<std::vector<std::string>>;

void print_table(const Table& table);

#endif // STOCK_EXCHANGE_TOOLS_CLI_UTILS_H
