#pragma once

#include <vector>

#include "Common.h"

struct SegTreeNode
{
    std::vector<CellValue> seg;

    SegTreeNode() = default;
    SegTreeNode(int size, CellValue fill) : seg(size, fill) {}
};
