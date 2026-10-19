#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>

#include "Common.h"

static_assert(std::is_same_v<CellValue, std::int64_t>);
static_assert(sizeof(CellValue) == 8);
static_assert(std::numeric_limits<CellValue>::is_signed);

int main()
{
    const CellValue minimum = std::numeric_limits<CellValue>::lowest();
    const CellValue maximum = std::numeric_limits<CellValue>::max();

    if (minimum >= 0 || maximum <= 0)
    {
        std::cout << "FAILED\n";
        return 1;
    }

    std::cout << "ALL PASSED\n";
    return 0;
}