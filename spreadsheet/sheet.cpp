#include "sheet.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

/**
 * Задает значение ячейке по адресу pos
*/
void Sheet::SetCell(Position pos, std::string text) {
    // Если позиция невалидная - выбрасываем исключение
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid set position");
    }

    // Если ячейка не была создана - создаем ее
    if (GetCell(pos) == nullptr) {
        // Обновляем размер таблицы и саму таблицу при необходимости
        if (static_cast<int>(table_.size()) < pos.row + 1) {
            table_.resize(pos.row + 1);

            for (int i = fact_size_.rows; i < pos.row + 1; ++i) {
                table_[i].resize(fact_size_.cols);
            }

            fact_size_.rows = pos.row + 1;
        }
        if (static_cast<int>(table_[0].size()) < pos.col + 1) {
            for (auto& row : table_) {
                row.resize(pos.col + 1);
            }
            fact_size_.cols = pos.col + 1;
        }

        table_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }

    table_[pos.row][pos.col].get()->Set(text);

    // Если ячейка непустая и новый размер непустой ячейки 
    // больше размера печатной области - обновляем размер печатной области
    if (!table_[pos.row][pos.col].get()->IsEmpty()) {
        print_size_.rows = std::max(print_size_.rows, pos.row + 1);
        print_size_.cols = std::max(print_size_.cols, pos.col + 1);
    }
}

/**
 * Возвращает указатель на ячейку
*/
CellInterface* Sheet::GetCell(Position pos) {
    // Если позиция невалидная - выбрасываем исключение
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid get position");
    }

    // Если позиция выходит за пределы области таблицы - возвращаем nullptr
    if (pos.row >= fact_size_.rows || pos.col >= fact_size_.cols) {
        return nullptr;
    }

    // Если pos указывает на пустую ячейку - возвращаем nullptr
    if (table_[pos.row][pos.col] == nullptr) {
        return nullptr;
    }

    return table_[pos.row][pos.col].get();
}
/**
 * Возвращает константный указатель на ячейку
*/
const CellInterface* Sheet::GetCell(Position pos) const {
    // Если позиция невалидная - выбрасываем исключение
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid get position");
    }

    // Если позиция выходит за пределы области таблицы - возвращаем nullptr
    if (pos.row >= fact_size_.rows || pos.col >= fact_size_.cols) {
        return nullptr;
    }

    // Если pos указывает на пустую ячейку - возвращаем nullptr
    if (table_[pos.row][pos.col] == nullptr) {
        return nullptr;
    }

    return table_[pos.row][pos.col].get();
}

/**
 * Возвращает указатель на ячейку, а в случае ее отсутствия 
 * - создает пустую и возвращает указатель на нее
 * (Метод необходим для создания корректных списков зависимостей ячеек)
*/
CellInterface* Sheet::GetOrCreateCell(Position pos) {
    // Если позиция невалидная - выбрасываем исключение
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid get position");
    }

    // Создаем ячейку, если она не была создана
    if (GetCell(pos) == nullptr) {
        SetCell(pos, "");
    }

    return table_[pos.row][pos.col].get();
}

/**
 * Очищает ячейку по адресу pos
*/
void Sheet::ClearCell(Position pos) {
    // Если pos указывает на пустую ячейку - ничего не делаем
    if (GetCell(pos) == nullptr) {
        return;
    }

    table_[pos.row][pos.col].get()->Clear();

    // Если ячейка не входит ни в один список зависимостей - удаляем ее
    if (!table_[pos.row][pos.col].get()->HasDependencies()) {
        table_[pos.row][pos.col] = nullptr;
    }

    // Итерируемся по строкам, пока не найдем непустую строку
    for (; print_size_.rows > 0; --print_size_.rows) {
        bool is_found = false;

        for (int i = 0; i < print_size_.cols; ++i) {
            if (GetCell({ print_size_.rows - 1, i }) != nullptr) {
                is_found = true;
                break;
            }
        }

        if (is_found) {
            break;
        }
    }

    // Итерируемся по столбцам, пока не найдем непустой столбец
    for (; print_size_.cols > 0; --print_size_.cols) {
        bool is_found = false;

        for (int i = 0; i < print_size_.rows; ++i) {
            if (GetCell({ i, print_size_.cols - 1 }) != nullptr) {
                is_found = true;
                break;
            }
        }

        if (is_found) {
            break;
        }
    }
}

/**
 * Возвращает размер печатной области таблицы
*/
Size Sheet::GetPrintableSize() const {
    return print_size_;
}

std::ostream& operator<<(std::ostream& os, const CellInterface::Value& val) {
    if (std::holds_alternative<std::string>(val)) {
        os << std::get<std::string>(val);
    }
    else if (std::holds_alternative<double>(val)) {
        os << std::get<double>(val);
    }
    else {
        os << std::get<FormulaError>(val);
    }

    return os;
}
/**
 * Выводит значения ячеек таблицы
*/
void Sheet::PrintValues(std::ostream& output) const {
    for (int y = 0; y < print_size_.rows; ++y) {
        for (int x = 0; x < print_size_.cols; ++x) {
            // Выводим табуляцию перед всеми элементами, кроме первого
            if (x != 0) {
                output << '\t';
            }

            // Если ячейка имеет ненулевой указатель - выводим значение
            if (table_[y][x] != nullptr) {
                output << table_[y][x]->GetValue();
            }
        }

        output << '\n';
    }
}
/**
 * Выводит содержимое ячеек таблицы
*/
void Sheet::PrintTexts(std::ostream& output) const {
    for (int y = 0; y < print_size_.rows; ++y) {
        for (int x = 0; x < print_size_.cols; ++x) {
            // Выводим табуляцию перед всеми элементами, кроме первого
            if (x != 0) {
                output << '\t';
            }

            // Если ячейка имеет ненулевой указатель - выводим содержимое
            if (table_[y][x] != nullptr) {
                output << table_[y][x]->GetText();
            }
        }

        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}