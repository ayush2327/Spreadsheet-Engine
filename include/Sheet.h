#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Common.h"
#include "Parser.h"
#include "SegmentTrees.h"

// The spreadsheet: cell storage, range queries (segment trees),
// dependency graph, formula evaluation, input and output.
class Sheet
{
public:
    Sheet(int rows, int cols)
        : rows_(rows),
          cols_(cols),
          parser_(rows, cols),
          values_(rows + 1, std::vector<CellValue>(cols + 1, 0)),
          errors_(rows + 1, std::vector<char>(cols + 1, 0)),
          // Held through the base-class pointer: every call goes through virtual dispatch.
          sumTree_(std::make_unique<SumSegTree>(rows + 1, cols + 1)),
          minTree_(std::make_unique<MinSegTree>(rows + 1, cols + 1)),
          maxTree_(std::make_unique<MaxSegTree>(rows + 1, cols + 1)),
          squareSumTree_(std::make_unique<SumSegTree>(rows + 1, cols + 1))
    {
    }

    void run()
    {
        output();
        std::cout << "[0.0] (ok) > " << std::flush;

        std::string line;
        while (std::getline(std::cin, line))
        {
            std::string input = Parser::trim(line);
            if (input == "q")
                break;

            auto start = std::chrono::high_resolution_clock::now();

            std::string error;
            bool success;
            if (input.size() == 1)
                success = scroll(input);
            else if (input.find('=') != std::string::npos)
                success = assign(input, error);
            else
                success = false;

            std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - start;
            std::ostringstream status;
            status << std::fixed << std::setprecision(1) << "[" << elapsed.count() << "]"
                   << (success ? " (ok) > " : " (unrecognized cmd) > ");

            if (!error.empty())
                std::cout << error << '\n';
            output();
            std::cout << status.str() << std::flush;
        }
    }

    void output() const
    {
        int lastCol = std::min(startCol_ + 9, cols_);
        int lastRow = std::min(startRow_ + 9, rows_);

        std::cout << std::setw(5) << Parser::columnNumberToName(startCol_);
        for (int c = startCol_ + 1; c <= lastCol; ++c)
            std::cout << std::setw(4) << Parser::columnNumberToName(c);
        std::cout << '\n';

        for (int r = startRow_; r <= lastRow; ++r)
        {
            std::cout << r;
            for (int c = startCol_; c <= lastCol; ++c)
            {
                if (errors_[r][c])
                    std::cout << std::setw(4) << "ERR";
                else
                    std::cout << std::setw(4) << values_[r][c];
            }
            std::cout << '\n';
        }
    }

    bool scroll(std::string_view command)
    {
        if (command == "w")
            startRow_ = std::max(1, startRow_ - 10);
        else if (command == "s")
            startRow_ = std::min(rows_, startRow_ + 10);
        else if (command == "a")
            startCol_ = std::max(1, startCol_ - 10);
        else if (command == "d")
            startCol_ = std::min(cols_, startCol_ + 10);
        else
            return false;
        return true;
    }

    bool assign(std::string_view command, std::string &error)
    {
        error.clear();
        std::size_t eq = command.find('=');
        if (eq == std::string_view::npos)
            return false;

        CellRef target;
        if (!parser_.parseCellReference(command.substr(0, eq), target))
            return false;

        std::string expression = Parser::trim(command.substr(eq + 1));
        if (expression.empty())
            return false;
        NodeId node = nodeId(target);

        if (auto constant = Parser::parseInteger(expression))
        {
            removeFormula(node);
            storeCell(target, *constant, false);
            propagate(node);
            return true;
        }

        auto value = evaluateExpression(expression, error);
        if (!value)
            return false;

        std::vector<CellRef> references = parser_.findReferences(expression);

        std::set<NodeId> dependents;
        collectDependents(node, dependents);
        for (const CellRef &ref : references)
        {
            if (dependents.count(nodeId(ref)))
            {
                error = "Circular formula detected : " + cellName(ref) + " depends on " + cellName(target);
                return false;
            }
        }

        removeFormula(node);
        for (const CellRef &ref : references)
        {
            graph_[nodeId(ref)].insert({node, expression});
            revGraph_[node].insert({nodeId(ref), expression});
        }
        storeCell(target, *value, false);
        propagate(node);
        return true;
    }

private:
    using NodeId = std::int64_t;
    using Edge = std::pair<NodeId, std::string>; // (other node, formula as edge weight)

    //  cell storage
    void storeCell(const CellRef &cell, CellValue value, bool isError)
    {
        values_[cell.row][cell.col] = value;
        errors_[cell.row][cell.col] = isError;
        sumTree_->update(cell.row, cell.col, value);
        minTree_->update(cell.row, cell.col, value);
        maxTree_->update(cell.row, cell.col, value);
        squareSumTree_->update(cell.row, cell.col, value * value);
    }

    std::string cellName(const CellRef &cell) const
    {
        return Parser::columnNumberToName(cell.col) + std::to_string(cell.row);
    }

    //  graph logic
    NodeId nodeId(const CellRef &cell) const { return static_cast<NodeId>(cell.row - 1) * cols_ + cell.col; }

    CellRef cellOf(NodeId id) const
    {
        return {static_cast<int>((id - 1) / cols_ + 1), static_cast<int>((id - 1) % cols_ + 1)};
    }

    // Everything that (transitively) depends on u, including u itself.
    void collectDependents(NodeId u, std::set<NodeId> &visited) const
    {
        if (!visited.insert(u).second)
            return;
        auto it = graph_.find(u);
        if (it == graph_.end())
            return;
        for (const Edge &edge : it->second)
            collectDependents(edge.first, visited);
    }

    // Drop the edges that come from the node's previous formula.
    void removeFormula(NodeId node)
    {
        auto it = revGraph_.find(node);
        if (it == revGraph_.end())
            return;
        for (const Edge &edge : it->second)
            graph_[edge.first].erase({node, edge.second});
        revGraph_.erase(it);
    }

    // Re-evaluate every formula that depends on u.
    void propagate(NodeId u)
    {
        auto it = graph_.find(u);
        if (it == graph_.end())
            return;
        for (const Edge &edge : it->second)
        {
            CellRef cell = cellOf(edge.first);
            std::string ignored;
            auto value = evaluateExpression(edge.second, ignored);
            storeCell(cell, value.value_or(0), !value.has_value());
            propagate(edge.first);
        }
    }

    //  evaluation
    std::optional<CellValue> evaluateOperand(std::string_view token, std::string &error) const
    {
        token = Parser::trimView(token);
        if (auto number = Parser::parseInteger(token))
            return number;

        CellRef cell;
        if (parser_.parseCellReference(token, cell))
        {
            if (errors_[cell.row][cell.col])
            {
                error = "Cell " + cellName(cell) + " contains an error.";
                return std::nullopt;
            }
            return values_[cell.row][cell.col];
        }

        error = "Invalid operand '" + std::string(token) + "'.";
        return std::nullopt;
    }

    std::optional<CellValue> evaluateFunction(RangeFunction function, std::string_view rangeText, std::string &error) const
    {
        CellRange range;
        if (!parser_.parseRange(rangeText, range))
        {
            error = "Invalid range '" + std::string(rangeText) + "'.";
            return std::nullopt;
        }

        const int r1 = range.start.row, c1 = range.start.col;
        const int r2 = range.end.row, c2 = range.end.col;
        const CellValue count = static_cast<CellValue>(r2 - r1 + 1) * (c2 - c1 + 1);

        switch (function)
        {
        case RangeFunction::Sum:
            return sumTree_->get(r1, c1, r2, c2);
        case RangeFunction::Min:
            return minTree_->get(r1, c1, r2, c2);
        case RangeFunction::Max:
            return maxTree_->get(r1, c1, r2, c2);
        case RangeFunction::Avg:
            return sumTree_->get(r1, c1, r2, c2) / count;
        case RangeFunction::Stdev:
        {
            double mean = static_cast<double>(sumTree_->get(r1, c1, r2, c2)) / count;
            double meanSquare = static_cast<double>(squareSumTree_->get(r1, c1, r2, c2)) / count;
            double variance = std::max(0.0, meanSquare - mean * mean);
            return static_cast<CellValue>(std::sqrt(variance));
        }
        }
        return std::nullopt;
    }

    std::optional<CellValue> evaluateExpression(std::string_view expression, std::string &error) const
    {
        std::string s = Parser::trim(expression);
        if (s.empty())
        {
            error = "Empty expression.";
            return std::nullopt;
        }

        if (auto number = Parser::parseInteger(s))
            return number;

        RangeFunction function;
        std::string arguments;
        if (Parser::parseFunctionCall(s, function, arguments))
            return evaluateFunction(function, arguments, error);

        std::size_t pos = Parser::findBinaryOperator(s);
        if (pos == std::string::npos)
            return evaluateOperand(s, error);

        auto left = evaluateOperand(std::string_view(s).substr(0, pos), error);
        if (!left)
            return std::nullopt;
        auto right = evaluateOperand(std::string_view(s).substr(pos + 1), error);
        if (!right)
            return std::nullopt;

        switch (s[pos])
        {
        case '+':
            return *left + *right;
        case '-':
            return *left - *right;
        case '*':
            return *left * *right;
        case '/':
            if (*right == 0)
            {
                error = "Error: Division by zero. Please enter a valid expression.";
                return std::nullopt;
            }
            return *left / *right;
        }
        return std::nullopt;
    }

private:
    int rows_, cols_;
    int startRow_ = 1, startCol_ = 1;

    Parser parser_;
    std::vector<std::vector<CellValue>> values_;
    std::vector<std::vector<char>> errors_;

    std::unique_ptr<SegTree> sumTree_, minTree_, maxTree_, squareSumTree_;

    std::map<NodeId, std::set<Edge>> graph_;
    std::map<NodeId, std::set<Edge>> revGraph_;
};
