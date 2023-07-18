#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <forward_list>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const { return category_; }
bool FormulaError::operator==(FormulaError rhs) const { return category_ == rhs.GetCategory(); }

std::string_view FormulaError::ToString() const {
    if (category_ == FormulaError::Category::Div0) {
        return "#DIV/0!"sv;
    }

    if (category_ == FormulaError::Category::Value) {
        return "#VALUE!"sv;
    }

    return "#REF!"sv;
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {

class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) 
    try
        : ast_(ParseFormulaAST(expression))
    {
        Position prev_cell = Position::NONE;
        for (Position cell : ast_.GetCells()) {
            if (cell.IsValid()
                && !(cell == prev_cell))
            {
                referenced_cells_.push_back(std::move(cell));
                prev_cell = referenced_cells_.back();
            }
        }
    }
    catch (std::exception& ex) {
        throw FormulaException(ex.what());
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        }
        catch (FormulaError& error) {
            return error;
        }
    }

    std::string GetExpression() const override {
        std::stringstream ss;
        ast_.PrintFormula(ss);
        return ss.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        return referenced_cells_;
    }

private:
    FormulaAST ast_;
    std::vector<Position> referenced_cells_;
};

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}