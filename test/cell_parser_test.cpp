#include <iostream>
#include <string>

#include "parser/CellParser.h"

void check(bool condition, const std::string &testName, int &failures)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << testName << '\n';
        ++failures;
    }
}

int main()
{
    int failures = 0;

    check(CellParser::columnNameToNumber("A") == 1, "column A", failures);
    check(CellParser::columnNameToNumber("Z") == 26, "column Z", failures);
    check(CellParser::columnNameToNumber("AA") == 27, "column AA", failures);
    check(CellParser::columnNameToNumber("AZ") == 52, "column AZ", failures);
    check(CellParser::columnNameToNumber("BA") == 53, "column BA", failures);
    check(CellParser::columnNameToNumber("ZZZ") == 18278, "column ZZZ", failures);
    check(CellParser::columnNameToNumber("") == -1, "empty column", failures);
    check(CellParser::columnNameToNumber("A1") == -1, "invalid column characters", failures);
    check(CellParser::columnNameToNumber("a") == -1, "lowercase column", failures);
    check(CellParser::columnNameToNumber("ZZZZZZZZZZZZ") == -1, "column overflow", failures);

    check(CellParser::columnNumberToName(1) == "A", "number 1 to column", failures);
    check(CellParser::columnNumberToName(26) == "Z", "number 26 to column", failures);
    check(CellParser::columnNumberToName(27) == "AA", "number 27 to column", failures);
    check(CellParser::columnNumberToName(18278) == "ZZZ", "number 18278 to column", failures);
    check(CellParser::columnNumberToName(0).empty(), "invalid column number", failures);
    for (int column = 1; column <= 18278; ++column)
    {
        std::string name = CellParser::columnNumberToName(column);
        if (CellParser::columnNameToNumber(name) != column)
        {
            check(false, "column conversion round-trip", failures);
            break;
        }
    }

    CellParser parser(1000, 18278);
    CellRef cell;
    check(parser.parseCellReference("A1", cell) && cell.row == 1 && cell.col == 1,
          "parse A1", failures);
    check(parser.parseCellReference(" ZZZ999 ", cell) && cell.row == 999 && cell.col == 18278,
          "parse trimmed ZZZ999", failures);
    check(parser.parseCellReference("B005", cell) && cell.row == 5 && cell.col == 2,
          "leading zero row", failures);
    check(!parser.parseCellReference("A0", cell), "reject row zero", failures);
    check(!parser.parseCellReference("A1001", cell), "reject out-of-range row", failures);
    check(!parser.parseCellReference("ZZZZ1", cell), "reject out-of-range column", failures);
    check(!parser.parseCellReference("A", cell), "reject missing row", failures);
    check(!parser.parseCellReference("1A", cell), "reject reversed reference", failures);
    check(!parser.parseCellReference("A-1", cell), "reject negative row", failures);
    check(!parser.parseCellReference("A999999999999999999999", cell), "reject row overflow", failures);
    check(!parser.parseCellReference("A 1", cell), "reject internal space", failures);

    CellRange range;
    check(parser.parseRange("B2", range) &&
              range.start.row == 2 && range.start.col == 2 &&
              range.end.row == 2 && range.end.col == 2,
          "single-cell range", failures);
    check(parser.parseRange("A1:C3", range) &&
              range.start.row == 1 && range.start.col == 1 &&
              range.end.row == 3 && range.end.col == 3,
          "rectangle range", failures);
    check(parser.parseRange(" A1 : C3 ", range), "trim range endpoints", failures);
    check(!parser.parseRange("A1:C1001", range), "reject invalid range endpoint", failures);
    check(!parser.parseRange("C1:A3", range), "reject reversed columns", failures);
    check(!parser.parseRange("A3:A1", range), "reject reversed rows", failures);
    check(!parser.parseRange("A1:B2:C3", range), "reject multiple colons", failures);
    check(!parser.parseRange(":A1", range), "reject missing start", failures);
    check(!parser.parseRange("A1:", range), "reject missing end", failures);
    check(!parser.parseRange("", range), "reject empty range", failures);

    if (failures == 0)
        std::cout << "ALL PASSED\n";
    else
        std::cout << "FAILURES: " << failures << '\n';

    return failures != 0;
}
