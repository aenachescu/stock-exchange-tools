#include "bvb_scraper.h"

#include <iostream>
#include <magic_enum.hpp>

int main()
{
    CBvbScraper bvb_scraper;

    auto r = bvb_scraper.GetIndexes();
    if (! r) {
        std::cout << "failed to get indexes: "
                  << magic_enum::enum_name(r.error()) << std::endl;
        return -1;
    }

    std::cout << "Indexes:" << std::endl;
    for (const auto& i : r.value()) {
        std::cout << i << std::endl;
    }

    return 0;
}
