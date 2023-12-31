# Электронная таблица
## Краткое описание
Таблица является упрощенным аналогом существующих решений: Excel, Google Sheets (бэк-енд). Ячейки таблицы могут содержать значения и формулы. Формулы, в свою очередь, помимо простых бинарных и унарных операций, могут содержать ссылки на сами ячейки. Программа умеет выводить печатную область таблицы в необходимый поток вывода. 

Для генерации лексического и семантического анализаторов применен ANTLR 4. При попытке создания некорректной ячейки (циклические ссылки, некорректные формулы и т.д.), в ней выводится соответствующее ошибке предупреждение.
## Компиляция
Сборка проекта осуществляется при помощи CMake. Перед сборкой в папке с файлом `CMakeLists.txt` необходимо предварительно расположить актуальный .jar файл ANTLR4, скорретировать название этого файла в переменной `ANTLR_EXECUTABLE` файла `CMakeLists.txt`, в переменной `ANTLR_EXECUTABLE` файла `FindANTLR.cmake`. Команды для сборки:
```
cmake -B./build -G "Unix Makefiles"
cmake --build build
```
## Использование
Пример использования:
```
auto sheet = CreateSheet();

sheet->SetCell("E2"_pos, "=E4");
sheet->SetCell("E4"_pos, "27");
sheet->SetCell("X9"_pos, "=M6");
sheet->SetCell("M6"_pos, "Ready");

sheet->PrintValues(std::cout);
```
## Системные требования
* C++17 (STL)
* g++ с поддержкой 17-го стандарта (также, возможно применения иных компиляторов C++ с поддержкой необходимого стандарта)
* OpenJDK
* ANTLR4
## Планы по доработке
* Создать десктопное приложение
* Внедрить возможность сохранять/загружать таблицы (предположительно в формате .xml)
## Стек
* C++17
* ANTLR4
* CMake 3.8
