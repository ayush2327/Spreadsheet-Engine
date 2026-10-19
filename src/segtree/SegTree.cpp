#include "segtree/SegTree.h"

#include <cassert>

namespace
{
    int nextPowerOfTwo(int n)
    {
        int p = 1;
        while (p < n)
            p <<= 1;
        return p;
    }
}

SegTree::SegTree(int rowLen, int colLen)
    : rowLen(rowLen),
      colLen(colLen),
      totalRowNodes(nextPowerOfTwo(rowLen)),
      totalColNodes(nextPowerOfTwo(colLen)) {}

void SegTree::build()
{
    segTree.assign(2 * totalRowNodes, SegTreeNode(2 * totalColNodes, identity()));
    built_ = true;
}

void SegTree::merge(SegTreeNode &parent, const SegTreeNode &left, const SegTreeNode &right, int colLeaf) const
{
    for (int c = colLeaf; c >= 1; c >>= 1)
    {
        parent.seg[c] = combine(left.seg[c], right.seg[c]);
    }
}

void SegTree::updateRow(int rowNode, int colLeaf, CellValue x)
{
    auto &seg = segTree[rowNode].seg;
    seg[colLeaf] = x;
    for (int j = colLeaf >> 1; j >= 1; j >>= 1)
    {
        seg[j] = combine(seg[2 * j], seg[2 * j + 1]);
    }
}

void SegTree::update(int row, int col, CellValue x)
{
    assert(built_ && "derived constructor must call build()");
    assert(0 <= row && row < rowLen && 0 <= col && col < colLen);

    int i = row + totalRowNodes;
    int j = col + totalColNodes;
    updateRow(i, j, x);

    for (i >>= 1; i >= 1; i >>= 1)
    {
        merge(segTree[i], segTree[2 * i], segTree[2 * i + 1], j);
    }
}

CellValue SegTree::getFromRow(const SegTreeNode &rowNode, int l, int r) const
{
    CellValue res = identity();
    while (l <= r)
    {
        if (l & 1)
            res = combine(res, rowNode.seg[l++]);
        if (r % 2 == 0)
            res = combine(res, rowNode.seg[r--]);
        l >>= 1;
        r >>= 1;
    }
    return res;
}

CellValue SegTree::get(int lx, int ly, int rx, int ry) const
{
    assert(built_ && "derived constructor must call build()");
    if (lx > rx || ly > ry)
        return identity();
    assert(0 <= lx && rx < rowLen && 0 <= ly && ry < colLen);

    CellValue res = identity();
    lx += totalRowNodes;
    rx += totalRowNodes;
    ly += totalColNodes;
    ry += totalColNodes;

    while (lx <= rx)
    {
        if (lx & 1)
            res = combine(res, getFromRow(segTree[lx++], ly, ry));
        if (rx % 2 == 0)
            res = combine(res, getFromRow(segTree[rx--], ly, ry));
        lx >>= 1;
        rx >>= 1;
    }
    return res;
}