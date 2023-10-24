#include "bvb_scraper.h"

#include <iostream>

int main()
{
    CBvbScraper bvb_scraper;

    std::cout << "hello world" << std::endl;

    auto r = bvb_scraper.GetIndexes();
    if (! r) {
        std::cout << "error" << std::endl;
    } else {
        std::cout << "success" << std::endl;
    }

    return 0;
}
