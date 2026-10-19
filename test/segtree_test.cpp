#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "segtree/SegmentTrees.h"

int main() {
    std::mt19937_64 rng(12345);
    int failures = 0;

    for (int iter = 0; iter < 300; ++iter) {
        int R = rng() % 20 + 1, C = rng() % 20 + 1;

        std::vector<std::unique_ptr<SegTree>> trees;
        trees.push_back(std::make_unique<SumSegTree>(R, C));
        trees.push_back(std::make_unique<MinSegTree>(R, C));
        trees.push_back(std::make_unique<MaxSegTree>(R, C));

        const CellValue NONE = std::numeric_limits<CellValue>::min();
        std::vector<std::vector<CellValue>> grid(R, std::vector<CellValue>(C, NONE));

        for (int step = 0; step < 200; ++step) {
            if (rng() % 2) {
                int r = rng() % R, c = rng() % C;
                CellValue v = (CellValue)(rng() % 2001) - 1000;
                grid[r][c] = v;
                for (auto& t : trees) t->update(r, c, v);
            } else {
                int lx = rng() % R, rx = rng() % R, ly = rng() % C, ry = rng() % C;
                if (lx > rx) std::swap(lx, rx);
                if (ly > ry) std::swap(ly, ry);

                CellValue s = 0, mn = std::numeric_limits<CellValue>::max(),
                          mx = std::numeric_limits<CellValue>::lowest();
                for (int i = lx; i <= rx; ++i)
                    for (int j = ly; j <= ry; ++j)
                        if (grid[i][j] != NONE) {
                            s += grid[i][j];
                            mn = std::min(mn, grid[i][j]);
                            mx = std::max(mx, grid[i][j]);
                        }

                if (trees[0]->get(lx, ly, rx, ry) != s) ++failures;
                if (trees[1]->get(lx, ly, rx, ry) != mn) ++failures;
                if (trees[2]->get(lx, ly, rx, ry) != mx) ++failures;
            }
        }
    }

    std::cout << (failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string(failures)) << "\n";
    return failures != 0;
}