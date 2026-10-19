#pragma once

#include <cctype>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Common.h"

struct CellRef
{
    int row = 0;
    int col = 0;
};

struct CellRange
{
    CellRef start;
    CellRef end;
};

enum class RangeFunction
{
    Sum,
    Min,
    Max,
    Avg,
    Stdev
};

// Everything that deals with text lives here: trimming, column names

class Parser
{
public:
    Parser(int rows, int cols) : rows_(rows), cols_(cols) {}

    static bool isSpace(char ch) { return std::isspace(static_cast<unsigned char>(ch)) != 0; }
    static bool isUpper(char ch) { return ch >= 'A' && ch <= 'Z'; }
    static bool isDigit(char ch) { return ch >= '0' && ch <= '9'; }
    static bool isAlnum(char ch) { return isUpper(ch) || isDigit(ch) || (ch >= 'a' && ch <= 'z'); }
    static bool isOperator(char ch) { return ch == '+' || ch == '-' || ch == '*' || ch == '/'; }

    static bool startsWith(std::string_view text, std::string_view prefix)
    {
        return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
    }

    static std::string_view trimView(std::string_view text)
    {
        std::size_t first = 0, last = text.size();
        while (first < last && isSpace(text[first]))
            ++first;
        while (last > first && isSpace(text[last - 1]))
            --last;
        return text.substr(first, last - first);
    }

    static std::string trim(std::string_view text) { return std::string(trimView(text)); }

    static int columnNameToNumber(std::string_view name)
    {
        if (name.empty())
            return -1;
        long long number = 0;
        for (char ch : name)
        {
            if (!isUpper(ch))
                return -1;
            number = number * 26 + (ch - 'A' + 1);
            if (number > std::numeric_limits<int>::max())
                return -1;
        }
        return static_cast<int>(number);
    }

    static std::string columnNumberToName(int number)
    {
        std::string name;
        while (number > 0)
        {
            --number; // shift to 0-based digit
            name.insert(name.begin(), static_cast<char>('A' + number % 26));
            number /= 26;
        }
        return name;
    }

    static std::optional<CellValue> parseInteger(std::string_view text)
    {
        text = trimView(text);
        bool negative = false;
        if (!text.empty() && text.front() == '-')
        {
            negative = true;
            text.remove_prefix(1);
        }
        if (text.empty())
            return std::nullopt;

        using U = unsigned long long;
        const U limit = negative ? static_cast<U>(std::numeric_limits<CellValue>::max()) + 1
                                 : static_cast<U>(std::numeric_limits<CellValue>::max());
        U magnitude = 0;
        for (char ch : text)
        {
            if (!isDigit(ch))
                return std::nullopt;
            U digit = static_cast<U>(ch - '0');
            if (magnitude > (limit - digit) / 10)
                return std::nullopt;
            magnitude = magnitude * 10 + digit;
        }
        if (negative)
            return static_cast<CellValue>(0 - magnitude);
        return static_cast<CellValue>(magnitude);
    }

    bool parseCellReference(std::string_view text, CellRef &cell) const
    {
        text = trimView(text);
        std::size_t i = 0;
        while (i < text.size() && isUpper(text[i]))
            ++i;

        int col = columnNameToNumber(text.substr(0, i));
        std::string_view digits = text.substr(i);
        if (col < 1 || col > cols_ || digits.empty())
            return false;

        long long row = 0;
        for (char ch : digits)
        {
            if (!isDigit(ch))
                return false;
            row = row * 10 + (ch - '0');
            if (row > rows_)
                return false;
        }
        if (row < 1)
            return false;

        cell.row = static_cast<int>(row);
        cell.col = col;
        return true;
    }

    bool parseRange(std::string_view text, CellRange &range) const
    {
        text = trimView(text);
        std::size_t colon = text.find(':');
        if (colon == std::string_view::npos)
        {
            CellRef cell;
            if (!parseCellReference(text, cell))
                return false;
            range = {cell, cell};
            return true;
        }
        if (text.find(':', colon + 1) != std::string_view::npos)
            return false;

        CellRef start, end;
        if (!parseCellReference(text.substr(0, colon), start) ||
            !parseCellReference(text.substr(colon + 1), end))
            return false;
        if (start.row > end.row || start.col > end.col)
            return false;

        range = {start, end};
        return true;
    }

    std::vector<CellRef> findReferences(std::string_view expression) const
    {
        std::vector<CellRef> references;
        std::size_t i = 0;
        while (i < expression.size())
        {
            if (!isAlnum(expression[i]))
            {
                ++i;
                continue;
            }
            std::size_t start = i;
            while (i < expression.size() && isAlnum(expression[i]))
                ++i;
            CellRef cell;
            if (parseCellReference(expression.substr(start, i - start), cell))
                references.push_back(cell);
        }
        return references;
    }

    static bool parseFunctionCall(std::string_view expression, RangeFunction &function, std::string &arguments)
    {
        expression = trimView(expression);
        if (expression.size() < 3 || expression.back() != ')')
            return false;
        std::size_t open = expression.find('(');
        if (open == std::string_view::npos)
            return false;

        std::string_view name = trimView(expression.substr(0, open));
        if (name == "SUM")
            function = RangeFunction::Sum;
        else if (name == "MIN")
            function = RangeFunction::Min;
        else if (name == "MAX")
            function = RangeFunction::Max;
        else if (name == "AVG")
            function = RangeFunction::Avg;
        else if (name == "STDEV")
            function = RangeFunction::Stdev;
        else
            return false;

        arguments = std::string(expression.substr(open + 1, expression.size() - open - 2));
        return true;
    }

    static std::size_t findBinaryOperator(std::string_view expression)
    {
        bool expectOperand = true;
        for (std::size_t i = 0; i < expression.size(); ++i)
        {
            char ch = expression[i];
            if (isSpace(ch))
                continue;
            if (isOperator(ch))
            {
                if (!expectOperand)
                    return i;
                expectOperand = true;
            }
            else
            {
                expectOperand = false;
            }
        }
        return std::string_view::npos;
    }

private:
    int rows_;
    int cols_;
};
