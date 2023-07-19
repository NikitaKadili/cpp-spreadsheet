#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <functional>
#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;
    bool HasDependencies() const;

    bool IsEmpty() const;
    bool IsCyclic(const std::vector<Position>& cells_to_check,
        std::unordered_set<const Cell*>& visited_cells) const;

    void InvalidateCache();

    void UpdateDepencies();

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> impl_;

    Sheet& sheet_; // Ссылка на таблицу, в которой находится ячейка

    mutable std::optional<Value> cache_; // Значение кэша текущей ячейки
    std::unordered_set<Cell*> depends_on_current_; // Указатели на ячейки, которые зависят от текущей
    std::unordered_set<Cell*> current_depends_on_; // Указатели на ячейки, от которых зависит текущая
};