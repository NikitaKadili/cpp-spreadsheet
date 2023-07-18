#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <memory>

class Cell;

class Sheet : public SheetInterface {
public:
    ~Sheet() {}

    void SetCell(Position pos, std::string text) override;

    CellInterface* GetCell(Position pos) override;
    const CellInterface* GetCell(Position pos) const override;

    CellInterface* GetOrCreateCell(Position pos);

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    Size print_size_ = { 0, 0 }; // Размер печатной области таблицы. По умолчанию (0, 0)
    Size fact_size_ = { 0, 0 }; // Фактический размер таблицы. По умолчанию (0, 0)

    std::vector<std::vector<std::unique_ptr<Cell>>> table_; // Таблица
};