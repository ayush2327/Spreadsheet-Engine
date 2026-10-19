#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "ExpressionEvaluator.h"

class TestContext final : public ExpressionContext
{
public:
    TestContext()
        : parser(5, 5),
          values(6, std::vector<CellValue>(6, 0)),
          assigned(6, std::vector<bool>(6, false)) {}

    void setCell(const CellRef &cell, CellValue value)
    {
        values[cell.row][cell.col] = value;
        assigned[cell.row][cell.col] = true;
    }

    bool getCellValue(const CellRef &cell, CellValue &value) const override
    {
        if (cell.row < 1 || cell.row > 5 || cell.col < 1 || cell.col > 5)
            return false;
        value = values[cell.row][cell.col];
        return true;
    }

    CellValue queryRange(RangeOperation operation, const CellRange &range) const override
    {
        CellValue sum = 0;
        CellValue squares = 0;
        CellValue minimum = std::numeric_limits<CellValue>::max();
        CellValue maximum = std::numeric_limits<CellValue>::lowest();

        for (int row = range.start.row; row <= range.end.row; ++row)
        {
            for (int col = range.start.col; col <= range.end.col; ++col)
            {
                CellValue value = values[row][col];
                sum += value;
                squares += value * value;
                if (assigned[row][col])
                {
                    minimum = std::min(minimum, value);
                    maximum = std::max(maximum, value);
                }
            }
        }

        if (operation == RangeOperation::Sum)
            return sum;
        if (operation == RangeOperation::Min)
            return minimum;
        if (operation == RangeOperation::Max)
            return maximum;
        return squares;
    }

    const CellParser &cellParser() const { return parser; }

private:
    CellParser parser;
    std::vector<std::vector<CellValue>> values;
    std::vector<std::vector<bool>> assigned;
};

void check(bool condition, const std::string &name, int &failures)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << name << '\n';
        ++failures;
    }
}

int main()
{
    int failures = 0;
    TestContext context;
    context.setCell({1, 1}, 10);
    context.setCell({1, 2}, 20);
    context.setCell({1, 3}, -10);

    ExpressionEvaluator evaluator(context, context.cellParser());
    auto expect = [&](std::string_view expression, CellValue expected)
    {
        auto result = evaluator.evaluate(expression);
        check(result.has_value() && *result == expected,
              std::string(expression), failures);
    };
    auto reject = [&](std::string_view expression)
    {
        check(!evaluator.evaluate(expression).has_value(),
              "reject " + std::string(expression), failures);
    };

    // Operator precedence and parentheses.
    expect("1 + 2 * 3", 7);
    expect("1 * 2 + 3", 5);
    expect("(1 + 2) * 3", 9);
    expect("A1 + B1 * 2", 50);
    expect("(A1 + B1) * 2", 60);
    expect("A1 + B1 * 2 - 5 / 2", 48);

    // Unary operators and integer division.
    expect("-5 + +2 * 4", 3);
    expect("-10 / 3", -3);
    expect("2 * -3 + 10", 4);
    expect("- 5", -5);
    expect("--5", 5);
    expect("-9223372036854775808", std::numeric_limits<CellValue>::lowest());
    expect("-9223372036854775808 + 1", std::numeric_limits<CellValue>::lowest() + 1);

    // Range functions.
    expect("SUM(A1:B1)", 30);
    expect("MIN(A1:B1)", 10);
    expect("MAX(A1:B1)", 20);
    expect("AVG(A1:B1)", 15);
    expect("STDEV(A1:B1)", 5);
    expect("SUM(A1)", 10);
    expect("AVG(A1:B2)", 7);
    expect(" SUM ( A1 : B1 ) ", 30);
    expect("SUM(A1:B1) + 2 * 3", 36);
    expect("STDEV(B1:C1)", 15);

    // Dependency collection includes repeated references and every cell in a range.
    std::vector<CellRef> references;
    auto rangeResult = evaluator.evaluate("SUM(A1:B1)", &references);
    check(rangeResult.has_value() && references.size() == 2 &&
              references[0].row == 1 && references[0].col == 1 &&
              references[1].row == 1 && references[1].col == 2,
          "range dependencies collected", failures);

    auto cellResult = evaluator.evaluate("A1 + A1", &references);
    check(cellResult.has_value() && references.size() == 2 &&
              references[0].col == 1 && references[1].col == 1,
          "repeated cell dependencies collected", failures);

    // Invalid syntax and arithmetic errors must fail safely.
    reject("");
    reject("1 / 0");
    reject("A0 + 1");
    reject("SUM(A1:B9)");
    reject("UNKNOWN(A1)");
    reject("(1 + 2");
    reject("1 + 2 xyz");
    reject("9223372036854775808");
    reject("-9223372036854775809");
    reject("--9223372036854775808");
    reject("9223372036854775807 + 1");
    reject("-9223372036854775808 - 1");
    reject("-9223372036854775808 / -1");
    reject("3037000500 * 3037000500");
    reject("SUM(A1:B1");
    reject("SUM(A1:B1))");
    reject("sum(A1:B1)");
    reject("A 1");

    if (failures == 0)
        std::cout << "ALL PASSED\n";
    else
        std::cout << "FAILURES: " << failures << '\n';
    return failures != 0;
}
