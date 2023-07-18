#include "common.h"

#include <cctype>
#include <cmath>
#include <regex>
#include <sstream>

const int LETTERS = 26;
// const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = {-1, -1};

/**
 * Возвращает true, если размеры равны
*/
bool Size::operator==(Size rhs) const {
    return std::tie(rows, cols) == std::tie(rhs.rows, rhs.cols);
}

/**
 * Возвращает true, если позиции указывают на одну ячейку
*/
bool Position::operator==(const Position rhs) const {
    return std::tie(row, col) == std::tie(rhs.row, rhs.col);
}
/**
 * Возвращает true, если позиция lhs меньше rhs
*/
bool Position::operator<(const Position rhs) const {
    return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

/**
 * Возвращает true, если позиция валидна
*/
bool Position::IsValid() const {
    return (row >= 0 && col >= 0)
        && (row < MAX_ROWS && col < MAX_COLS);
}

/**
 * Преобразует цифровое обозначение столбца в буквенное
*/
std::string ConvertNumsToColumnLiterals(int col) {
    std::string result = ""; // Результирующая строка
    result.reserve(MAX_POS_LETTER_COUNT);

    // Итерируемся, пока col >= 0, вставляем символы в начало строки
    for (; col >= 0; col = (col / LETTERS) - 1) {
        result.insert(result.begin(), 'A' + (col % LETTERS));
    }

    return result;
}
/**
 * Преобразует объект типа Position в string
*/
std::string Position::ToString() const {
    // Если позиция невалидная - возваращем пустую строку
    if (!IsValid()) {
        return "";
    }

    return ConvertNumsToColumnLiterals(col) + std::to_string(row + 1);
}

/**
 * Преобразует буквенное обозначение столбца в цифровое
*/
static int ConvertColumnLiteralsToNums(std::string_view col_chars) {
    int result = 0;

    // Итерируемся по символам, преобразуем при помощи 
    // стандартной формулы приведения систем исчисления
    for (int i = 0; i < static_cast<int>(col_chars.size()); ++i) {
        result += (col_chars[i] - 'A' + 1) * pow(LETTERS, col_chars.size() - i - 1);
    }

    return result;
}
/**
 * Возвращает объект типа Position из str
*/
Position Position::FromString(std::string_view str) {
    // Если строка пуста или больше 8-ми символов - возвращаем невалидную позицию
    if (str.size() <= 1 || str.size() > 8) {
        return Position::NONE;
    }

    int row = -1; // Номер строки
    int col = -1; // Номер столбца

    // Регулярное выражение номера ячейки
    static std::regex pos_reg("([A-Z]{1,3})([0-9]{1,5})");
    std::smatch m;
    std::string text = std::string(str);

    // Если строка не соответствует регулярному выражению - возвращаем невалидную позицию
    if (!std::regex_match(text, m, pos_reg)) {
        return Position::NONE;
    }

    // Преобразуем номер строки в int
    row = std::stoi(m[2].str());
    // Если номер строки нулевой или превышает максимально допустимое значение 
    // - возвращаем невалидную позицию
    if (0 >= row || row > MAX_ROWS) {
        return Position::NONE;
    }

    // Если количество символов столбца больше MAX_POS_LETTER_COUNT - возвращаем невалидную позицию
    if (m[1].str().size() > MAX_POS_LETTER_COUNT) {
        return Position::NONE;
    }

    // Преобразуем номер столбца в int
    col = ConvertColumnLiteralsToNums(m[1].str());
    // Если номер столбца нулевой или превышает максимально допустимое значение 
    // - возвращаем невалидную позицию
    if (0 >= col || col > MAX_COLS) {
        return Position::NONE;
    }

    return { row - 1, col - 1 };
}