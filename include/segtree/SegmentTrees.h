#pragma once

#include "segtree/SegTree.h"

class SumSegTree final : public SegTree
{
public:
    SumSegTree(int rowLen, int colLen);

private:
    CellValue combine(CellValue x, CellValue y) const override;
    CellValue identity() const override;
};

class MinSegTree final : public SegTree
{
public:
    MinSegTree(int rowLen, int colLen);

private:
    CellValue combine(CellValue x, CellValue y) const override;
    CellValue identity() const override;
};

class MaxSegTree final : public SegTree
{
public:
    MaxSegTree(int rowLen, int colLen);

private:
    CellValue combine(CellValue x, CellValue y) const override;
    CellValue identity() const override;
};
