#include "SegmentTrees.h"

#include <algorithm>
#include <limits>

SumSegTree::SumSegTree(int rowLen, int colLen) : SegTree(rowLen, colLen) { build(); }

CellValue SumSegTree::combine(CellValue x, CellValue y) const { return x + y; }

CellValue SumSegTree::identity() const { return 0; }

MinSegTree::MinSegTree(int rowLen, int colLen) : SegTree(rowLen, colLen) { build(); }

CellValue MinSegTree::combine(CellValue x, CellValue y) const { return std::min(x, y); }

CellValue MinSegTree::identity() const { return std::numeric_limits<CellValue>::max(); }

MaxSegTree::MaxSegTree(int rowLen, int colLen) : SegTree(rowLen, colLen) { build(); }

CellValue MaxSegTree::combine(CellValue x, CellValue y) const { return std::max(x, y); }

CellValue MaxSegTree::identity() const { return std::numeric_limits<CellValue>::lowest(); }
