#include <iostream>
#include <string>

#include "Sheet.h"

void check(bool condition, const std::string &name, int &failures)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << name << '\n';
        ++failures;
    }
}

CellRef parse_cell(const Sheet &sheet, const std::string &name)
{
    CellRef cell;
    if (!sheet.cell_parser().parseCellReference(name, cell))
        return {};
    return cell;
}

bool assign(Sheet &sheet, const std::string &cellName, const std::string &expression, int &failures)
{
    CellRef cell = parse_cell(sheet, cellName);
    std::string error;
    bool result = sheet.assign_cell(cell, expression, &error);
    check(result, cellName + " = " + expression + (result ? "" : " (" + error + ")"), failures);
    return result;
}

int main()
{
    int failures = 0;
    Sheet sheet(5, 53);

    assign(sheet, "A1", "10", failures);
    assign(sheet, "B1", "20", failures);
    assign(sheet, "C1", "A1 + B1", failures);
    assign(sheet, "D1", "C1 * 2", failures);
    check(sheet.cell_value(parse_cell(sheet, "C1")) == 30, "initial formula", failures);
    check(sheet.cell_value(parse_cell(sheet, "D1")) == 60, "dependent formula", failures);

    assign(sheet, "A1", "5", failures);
    check(sheet.cell_value(parse_cell(sheet, "C1")) == 25, "dependent recalculation", failures);
    check(sheet.cell_value(parse_cell(sheet, "D1")) == 50, "transitive recalculation", failures);

    assign(sheet, "C1", "B1 + 1", failures);
    assign(sheet, "A1", "7", failures);
    check(sheet.cell_value(parse_cell(sheet, "C1")) == 21, "old dependency removed", failures);
    assign(sheet, "B1", "30", failures);
    check(sheet.cell_value(parse_cell(sheet, "C1")) == 31, "new dependency recalculated", failures);
    check(sheet.cell_value(parse_cell(sheet, "D1")) == 62, "downstream after rewire", failures);

    assign(sheet, "E1", "A1 + B1 * 2", failures);
    assign(sheet, "F1", "(A1 + B1) * 2", failures);
    check(sheet.cell_value(parse_cell(sheet, "E1")) == 67, "operator precedence", failures);
    check(sheet.cell_value(parse_cell(sheet, "F1")) == 74, "parentheses", failures);

    assign(sheet, "G1", "SUM(A1:B1)", failures);
    assign(sheet, "H1", "AVG(A1:B1)", failures);
    assign(sheet, "I1", "STDEV(A1:B1)", failures);
    assign(sheet, "J1", "MIN(A1:B1)", failures);
    assign(sheet, "K1", "MAX(A1:B1)", failures);
    check(sheet.cell_value(parse_cell(sheet, "G1")) == 37, "SUM function", failures);
    check(sheet.cell_value(parse_cell(sheet, "H1")) == 18, "AVG function", failures);
    check(sheet.cell_value(parse_cell(sheet, "I1")) == 11, "STDEV function", failures);
    check(sheet.cell_value(parse_cell(sheet, "J1")) == 7, "MIN function", failures);
    check(sheet.cell_value(parse_cell(sheet, "K1")) == 30, "MAX function", failures);

    std::string cycleError;
    bool cycleAccepted = sheet.assign_cell(parse_cell(sheet, "C1"), "D1 + 1", &cycleError);
    check(!cycleAccepted && cycleError == "Circular formula detected.", "reject circular formula", failures);
    check(sheet.cell_value(parse_cell(sheet, "C1")) == 31, "cycle rejection keeps old value", failures);
    check(sheet.cell_value(parse_cell(sheet, "D1")) == 62, "cycle rejection keeps dependent value", failures);

    std::string invalidError;
    bool invalidAccepted = sheet.assign_cell(parse_cell(sheet, "A1"), "1 / 0", &invalidError);
    check(!invalidAccepted, "reject division by zero assignment", failures);
    check(sheet.cell_value(parse_cell(sheet, "A1")) == 7, "invalid assignment leaves old value", failures);

    assign(sheet, "L1", "1", failures);
    assign(sheet, "M1", "10 / L1", failures);
    check(sheet.cell_value(parse_cell(sheet, "M1")) == 10, "division formula initial value", failures);
    assign(sheet, "L1", "0", failures);
    check(sheet.cell_has_error(parse_cell(sheet, "M1")), "dependent division error is marked", failures);
    assign(sheet, "L1", "2", failures);
    check(!sheet.cell_has_error(parse_cell(sheet, "M1")), "dependent error clears", failures);
    check(sheet.cell_value(parse_cell(sheet, "M1")) == 5, "dependent formula recovers", failures);

    assign(sheet, "BA2", "42", failures);
    check(sheet.cell_value(parse_cell(sheet, "BA2")) == 42, "base-26 column names above Z", failures);

    if (failures == 0)
        std::cout << "ALL PASSED\n";
    else
        std::cout << "FAILURES: " << failures << '\n';
    return failures != 0;
}
