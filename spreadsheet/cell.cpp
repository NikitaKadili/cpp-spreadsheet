#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::Impl {
public:
    virtual Value GetValue(SheetInterface& /*sheet*/) const = 0;
    virtual std::string GetText() const = 0;

    virtual bool IsEmpty() const = 0;

    virtual ~Impl() = default;

    virtual std::vector<Position> GetReferencedCells() const = 0;
};
/**
 * Пустая ячейка
*/
class Cell::EmptyImpl : public Cell::Impl {
public:
    Value GetValue(SheetInterface& /*sheet*/) const override {
        return "";
    }
    std::string GetText() const override {
        return "";
    }

    bool IsEmpty() const override { return true; }

    std::vector<Position> GetReferencedCells() const override { return {}; }
};
/**
 * Текстовая ячейка
*/
class Cell::TextImpl : public Cell::Impl {
public:
    explicit TextImpl(std::string text, int pos = 0)
        : text_(std::move(text))
        , value_pos_(pos)
    {}

    Value GetValue(SheetInterface& /*sheet*/) const override {
        return text_.substr(value_pos_);
    }
    std::string GetText() const override {
        return text_;
    }

    bool IsEmpty() const override { return false; }

    std::vector<Position> GetReferencedCells() const override { return {}; }

private:
    std::string text_; // Тест ячейки
    int value_pos_ = 0; // Позиция начала значения ячейки
};
/**
 * Формульная ячейка
*/
class Cell::FormulaImpl : public Cell::Impl {
public:
    explicit FormulaImpl(std::string text)
        : formula_ptr_(ParseFormula(std::move(text)))
    {}

    Value GetValue(SheetInterface& sheet) const override {
        const FormulaInterface::Value& value = formula_ptr_->Evaluate(sheet);

        // Если значение содержит тип double - возвращаем его
        if (std::holds_alternative<double>(value)) {
            return std::get<double>(value);
        }

        // В противном случае выражение содержит FormulaError
        return std::get<FormulaError>(value);
    }
    std::string GetText() const override {
        return FORMULA_SIGN + formula_ptr_->GetExpression();
    }

    bool IsEmpty() const override { return false; }

    std::vector<Position> GetReferencedCells() const override { 
        return formula_ptr_->GetReferencedCells(); 
    }

private:
    // Указатель на объект FormulaInterface
    std::unique_ptr<FormulaInterface> formula_ptr_;
};

Cell::Cell(Sheet& sheet) : impl_(new EmptyImpl), sheet_(sheet) {}
Cell::~Cell() {}

void Cell::Set(std::string text) {
    // Если содержимое ячейки идентично text - ничего не делаем
    if (text == GetText()) {
        return;
    }

    // Если строка пустая - создаем пустую ячейку
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
    }
    // Если первый символ строки - символ начала формулы, и строка имеет 
    // больше одного символа - создаем формульную ячейку
    else if (text[0] == FORMULA_SIGN && text.size() > 1) {
        // Создаем временный указатель на формульную реализацию ячейки
        std::unique_ptr<Impl> temp = std::make_unique<FormulaImpl>(text.substr(1));

        // Если ячейка содержит циклические зависимости 
        // - выбрасываем CircularDependencyException
        std::unordered_set<const Cell*> visited_cells;
        if (IsCyclic(temp->GetReferencedCells(), visited_cells)) {
            throw CircularDependencyException("Circular dependency detected");
        }

        std::swap(impl_, temp);
    }
    // В остальных случаях ячейка будет считать текстовой
    else {
        impl_ = std::make_unique<TextImpl>(
            std::move(text),
            text[0] == ESCAPE_SIGN ? 1 : 0
        );
    }

    // Инвалидируем кэш в необходимых ячейках
    InvalidateCache();
    // Обновляем списки зависимостей
    UpdateDepencies();
}

/**
 * Очищает ячейку (меняет тип ячейки на пустую)
*/
void Cell::Clear() {
    Set("");
}

/**
 * Возвращает результат вычисления для формульной ячейки,
 * текст для текстовой ячейки
*/
Cell::Value Cell::GetValue() const {
    // Если у ячейки нет кэша - создаем его
    if (!cache_) {
        cache_ = impl_->GetValue(sheet_);
    }

    return cache_.value();
}
/**
 * Возвращает содержимое ячейки
*/
std::string Cell::GetText() const {
    return impl_->GetText();
}
/**
 * Возвращает вектор позиций ячеек, от которых зависит текущая ячейка
*/
std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

/**
 * Возвращает true, если вектор зависит от других ячеек
*/
bool Cell::IsReferenced() const {
    return !current_depends_on_.empty();
}
/**
 * Вовращает true, если имеются ячейки, которые зависят от текущей
*/
bool Cell::HasDependencies() const {
    return !depends_on_current_.empty();
}

/**
 * Возвращает true, если ячейка пустая
*/
bool Cell::IsEmpty() const {
    return impl_->IsEmpty();
}
/**
 * Возвращает true, если граф связей ячейки содержит циклы
*/
bool Cell::IsCyclic(const std::vector<Position>& cells_to_check,
        std::unordered_set<const Cell*>& visited_cells) const
{
    for (Position pos : cells_to_check) {
        const Cell* cell = reinterpret_cast<Cell*>(sheet_.GetCell(pos));

        // Если указатель на ячейку является указателем на начальную ячейку 
        // - возвращаем true
        if (cell == this) {
            return true;
        }

        // Если ячейка является листом - пропускаем итерацию
        if (cell == nullptr
            || cell->IsEmpty())
        {
            continue;
        }

        // Если ячейка уже посещалась - пропускаем итерацию
        if (visited_cells.find(cell) != visited_cells.end()) {
            continue;
        }
        visited_cells.insert(cell);

        // Если метод ContainsCircularReferences для зависимостей cell 
        // вернул true - возвращаем также true
        if (IsCyclic(cell->GetReferencedCells(), visited_cells)) {
            return true;
        }
    }

    return false;
}

/**
 * Инвалидирует кэш у текущей и зависящих от нее ячеек
*/
void Cell::InvalidateCache() {
    // Если кэш не был создан, или уже был инвалидирован - не делаем ничего
    if (!cache_) {
        return;
    }

    // Инвалидируем кэш
    cache_.reset();

    // Запускаем инвалидацию кэша у всех зависящих ячеек
    for (Cell* dep_cell : depends_on_current_) {
        dep_cell->InvalidateCache();
    }
}
/**
 * Обновляет списки зависимостей
*/
void Cell::UpdateDepencies() {
    // Удаляем текущую ячейку из старых списков зависимостей
    for (Cell* cell : current_depends_on_) {
        cell->depends_on_current_.erase(this);
    }

    // Обновляем список зависимостей текущей ячейки
    current_depends_on_.clear();
    for (Position pos : GetReferencedCells()) {
        Cell* cell = reinterpret_cast<Cell*>(sheet_.GetOrCreateCell(pos));
        current_depends_on_.insert(cell);
    }

    // Вносим текущую ячейку в новые списки зависимостей
    for (Cell* cell : current_depends_on_) {
        cell->depends_on_current_.insert(this);
    }
}