#include <exception>
#include <iostream>
#include <string>

#include "Sheet.h"

int main(int argc, char *argv[])
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int rows = 0, cols = 0;
    try
    {
        if (argc != 3)
            throw std::invalid_argument("argc");
        rows = std::stoi(argv[1]);
        cols = std::stoi(argv[2]);
        if (rows < 1 || cols < 1)
            throw std::invalid_argument("size");
    }
    catch (const std::exception &)
    {
        std::cerr << "Usage: ./sheet <rows> <columns>\n";
        return 1;
    }

    Sheet sheet(rows, cols);
    sheet.run();
    return 0;
}
