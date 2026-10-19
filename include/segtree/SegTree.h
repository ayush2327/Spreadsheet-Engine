#pragma once

#include <vector>

#include "Common.h"
#include "segtree/SegTreeNode.h"

class SegTree
{
public:
    SegTree(int rowLen, int colLen);
    virtual ~SegTree() = default;

    virtual void update(int row, int col, CellValue x);
    CellValue get(int lx, int ly, int rx, int ry) const;

protected:
    virtual CellValue combine(CellValue x, CellValue y) const = 0;
    virtual CellValue identity() const = 0;

    virtual void merge(SegTreeNode &parent, const SegTreeNode &left, const SegTreeNode &right, int colLeaf) const;
    virtual void build();
    virtual void updateRow(int rowNode, int colLeaf, CellValue x);
    CellValue getFromRow(const SegTreeNode &rowNode, int l, int r) const;

protected:
    int rowLen, colLen;
    int totalRowNodes, totalColNodes;
    std::vector<SegTreeNode> segTree;

private:
    bool built_ = false;
};
