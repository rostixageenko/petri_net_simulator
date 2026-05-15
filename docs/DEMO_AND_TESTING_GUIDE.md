# Инструкция по запуску, тестированию и демонстрации проекта

Этот файл нужен как практическая шпаргалка: как открыть проект, собрать C++-ядро, запустить тесты и показать работу сети Петри научному руководителю или комиссии.

Команды ниже рассчитаны на Windows, VS Code и PowerShell. Выполнять их нужно из корня репозитория `petri_net_simulator`.

## 1. Что есть в проекте

Проект собирается через `CMakeLists.txt`.

После сборки создаются основные исполняемые файлы:

- `build\Debug\petri_cli.exe` - простой консольный запуск одного режима: `simulate`, `dfs`, `bfs`, `dijkstra`;
- `build\Debug\demo_runner.exe` - удобный демонстрационный сценарий, который показывает модель, симуляцию, граф достижимости, DFS, BFS, Dijkstra и JSON-ответ;
- `build\Debug\petri_tests.exe` - C++-тесты.

Главные папки:

- `include\` - заголовочные файлы C++;
- `src\` - реализации C++-модулей;
- `tests\` - тесты;
- `examples\` - демонстрационные JSON-сети и Python-пример;
- `docs\` - документация;
- `logs\` - место для JSONL-метрик;
- `data\` - место для файлового хранилища, если использовать `storage_api`.

Главные примеры:

- `examples\mutex.json` - более содержательная сеть Петри про взаимное исключение двух процессов;
- `examples\simple_chain.json` - максимально простая сеть: стартовое место, один переход, конечное место.

## 2. Как открыть проект в VS Code

1. Открой PowerShell.
2. Перейди в папку проекта:

```powershell
cd "C:\Users\37529\OneDrive\Рабочий стол\УЧЁБА\ДИПЛОМ\Диплом\petri_net_simulator"
```

3. Открой VS Code:

```powershell
code .
```

Если команда `code` не найдена, открой VS Code вручную и выбери:

```text
File -> Open Folder -> petri_net_simulator
```

## 3. Как проверить, что Git работает

Проверить текущую ветку и изменения:

```powershell
git status
```

Ожидаемый хороший результат перед началом демонстрации:

```text
On branch main
nothing to commit, working tree clean
```

Посмотреть удалённый репозиторий:

```powershell
git remote -v
```

Посмотреть последние коммиты:

```powershell
git log --oneline -5
```

Если Git пишет `dubious ownership`, можно выполнить команду, которую он сам предлагает, или запускать Git из той же учётной записи, где создан репозиторий.

## 4. Как сгенерировать build-файлы

Для обычной демонстрации C++-ядра Python bindings можно выключить. Так сборка не будет зависеть от `pybind11`.

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
```

Если `cmake` не найден в `PATH`, используй CMake из Visual Studio:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
```

Успешная генерация заканчивается строкой примерно такого вида:

```text
Build files have been written to: .../petri_net_simulator/build
```

## 5. Как собрать проект

```powershell
cmake --build build --config Debug
```

Если `cmake` не найден:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
```

После успешной сборки должны появиться:

```text
build\Debug\petri_core.lib
build\Debug\petri_cli.exe
build\Debug\demo_runner.exe
build\Debug\petri_tests.exe
```

Проверить можно так:

```powershell
Get-ChildItem build\Debug\*.exe
```

## 6. Как запустить все тесты

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Или через CTest из Visual Studio:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Успешный результат выглядит так:

```text
100% tests passed, 0 tests failed
```

Если тест упал, CTest покажет имя теста и вывод ошибки. Для этого и нужен параметр `--output-on-failure`.

## 7. Как запустить пример сети Петри

Самый удобный демонстрационный запуск:

```powershell
.\build\Debug\demo_runner.exe
```

Без аргументов `demo_runner` загружает:

```text
examples\mutex.json
```

Запуск на простейшей сети:

```powershell
.\build\Debug\demo_runner.exe examples\simple_chain.json
```

Запуск на mutex-сети явно:

```powershell
.\build\Debug\demo_runner.exe examples\mutex.json
```

## 8. Как запустить симуляцию

Через удобный demo runner:

```powershell
.\build\Debug\demo_runner.exe examples\mutex.json
```

В выводе найди раздел:

```text
=== Simulation: round_robin, max 4 steps ===
```

Там показано:

- номер шага;
- какой переход сработал;
- модельное время;
- маркировка до шага;
- маркировка после шага.

Через короткий CLI:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json simulate
```

`petri_cli` выводит JSON-ответ целиком.

## 9. Как запустить DFS, BFS и Dijkstra

Через demo runner все три алгоритма запускаются одной командой:

```powershell
.\build\Debug\demo_runner.exe examples\mutex.json
```

В выводе найди раздел:

```text
=== Algorithms ===
```

Там будут строки для:

- `dfs`;
- `bfs`;
- `dijkstra`.

Отдельные команды через `petri_cli`:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json dfs
.\build\Debug\petri_cli.exe examples\mutex.json bfs
.\build\Debug\petri_cli.exe examples\mutex.json dijkstra
```

На простейшей сети:

```powershell
.\build\Debug\petri_cli.exe examples\simple_chain.json bfs
.\build\Debug\petri_cli.exe examples\simple_chain.json dijkstra
```

## 10. Как получить JSON-результат

`petri_cli` сразу печатает JSON:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json bfs
```

`demo_runner` тоже показывает JSON-ответ в конце. Найди раздел:

```text
=== JSON response from service_adapter (BFS) ===
```

Если нужно сохранить JSON в файл:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json bfs > bfs_result.json
```

Посмотреть сохранённый файл:

```powershell
Get-Content bfs_result.json
```

## 11. Где смотреть метрики и сохранённые результаты

Метрики JSON service adapter и runtime пишутся в JSONL-файл:

```text
logs\metrics.jsonl
```

Посмотреть последние записи:

```powershell
Get-Content logs\metrics.jsonl -Tail 5
```

Если файла нет, значит в текущей сессии ещё не было запуска, который пишет метрики. Запусти:

```powershell
.\build\Debug\demo_runner.exe examples\simple_chain.json
```

Файловое хранилище `storage_api` реализовано в коде. Оно умеет сохранять:

- модели в `data\models\`;
- результаты запусков в `data\runs\`;
- метрики в `data\metrics\`.

Важно: `demo_runner` и `petri_cli` автоматически не сохраняют полный JSON-ответ в `data\runs`. Они только выводят результат в консоль. Полное сохранение через `storage_api` используется отдельными функциями C++ и покрыто тестами.

## 12. Что делать, если возникла ошибка

### `cmake` не найден

Используй полный путь к CMake из Visual Studio:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
```

### Ошибка Python или pybind11 при CMake

Для демонстрации C++-ядра выключи Python bindings:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
```

Это нормально: Python-модуль опциональный.

### Ошибка сборки из-за Visual Studio или Windows SDK

Проверь, что установлен Visual Studio 2022 с компонентом C++.

Также можно заново создать папку сборки:

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
cmake --build build --config Debug
```

### Тесты не запускаются и CTest просит конфигурацию

Для Visual Studio generator обязательно указывай `-C Debug`:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

### `demo_runner.exe` не найден

Значит проект ещё не собран или CMake был запущен до добавления demo runner. Выполни:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
cmake --build build --config Debug
```

### JSON-модель не загружается

Проверь, что команда выполняется из корня проекта и путь к JSON правильный:

```powershell
Test-Path examples\mutex.json
Test-Path examples\simple_chain.json
```

## 13. Сценарий демонстрации для защиты

### Шаг 1. Открыть проект в VS Code

```powershell
cd "C:\Users\37529\OneDrive\Рабочий стол\УЧЁБА\ДИПЛОМ\Диплом\petri_net_simulator"
code .
```

Что сказать:

```text
Это репозиторий с C++20-ядром подсистемы моделирования и интерпретации сетей Петри.
```

### Шаг 2. Показать структуру проекта

```powershell
Get-ChildItem
Get-ChildItem include\petri
Get-ChildItem src
Get-ChildItem tests
Get-ChildItem examples
```

Что показать в VS Code:

- `include\petri\core_pn` - модель сети Петри;
- `src\core_pn` - реализация модели и интерпретатора;
- `src\graph_models` - графы и граф достижимости;
- `src\algorithms` - DFS, BFS, Dijkstra;
- `src\service_adapter` - JSON API;
- `tests` - автотесты.

### Шаг 3. Показать пример входного JSON сети Петри

```powershell
Get-Content examples\simple_chain.json
Get-Content examples\mutex.json
```

Что сказать:

```text
Входная сеть задаётся JSON-объектом: places, transitions и arcs. Дуги соединяют место с переходом или переход с местом.
```

### Шаг 4. Собрать проект через CMake

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
cmake --build build --config Debug
```

Если `cmake` не найден:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
```

Показать результат:

```powershell
Get-ChildItem build\Debug\*.exe
```

### Шаг 5. Запустить тесты

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Ожидаемый результат:

```text
100% tests passed, 0 tests failed
```

Что сказать:

```text
Тесты проверяют загрузку JSON, правила срабатывания переходов, симуляцию, граф достижимости, DFS/BFS/Dijkstra, JSON adapter, метрики и storage_api.
```

### Шаг 6. Запустить пример симуляции

```powershell
.\build\Debug\demo_runner.exe examples\simple_chain.json
```

Что показать:

- начальную маркировку;
- доступный переход;
- шаг симуляции;
- финальную маркировку.

Потом можно показать более сложную сеть:

```powershell
.\build\Debug\demo_runner.exe examples\mutex.json
```

### Шаг 7. Показать результат работы DFS/BFS/Dijkstra

В `demo_runner` найди раздел:

```text
=== Algorithms ===
```

Отдельные команды:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json dfs
.\build\Debug\petri_cli.exe examples\mutex.json bfs
.\build\Debug\petri_cli.exe examples\mutex.json dijkstra
```

Что сказать:

```text
Алгоритмы запускаются по графу достижимости. Вершины графа - это маркировки сети Петри, а рёбра - срабатывания переходов.
```

### Шаг 8. Показать JSON-ответ

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json bfs
```

Или сохранить в файл:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json bfs > bfs_result.json
Get-Content bfs_result.json
```

Что сказать:

```text
JSON-ответ содержит путь, маркировки, переходы, метрики и размер построенного графа. Такой формат можно передать визуализации или Python-части.
```

### Шаг 9. Объяснить связь с визуализацией и Python-частью

Команда для демонстрации Python, если модуль собран:

```powershell
$env:PYTHONPATH = (Resolve-Path build\Debug).Path
python examples\python_demo.py
```

Если Python-модуль не собран, объяснение всё равно простое:

```text
Ядро не зависит от GUI. Оно принимает JSON и возвращает JSON. Поэтому визуализатор или серверная часть могут вызвать service adapter и отобразить полученные маркировки, переходы и путь.
```

### Шаг 10. Объяснить, что реализовано как C++-ядро

Краткий текст:

```text
В моей части реализовано C++-ядро: модель сети Петри, валидация JSON, пошаговый интерпретатор, построение графа достижимости, алгоритмы DFS/BFS/Dijkstra, единый runtime запуска алгоритмов, сбор метрик, JSON service adapter, файловый storage_api и опциональные Python bindings.
```

## 14. Минимальный набор команд перед защитой

Если нужно быстро всё проверить:

```powershell
cd "C:\Users\37529\OneDrive\Рабочий стол\УЧЁБА\ДИПЛОМ\Диплом\petri_net_simulator"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\demo_runner.exe examples\simple_chain.json
.\build\Debug\demo_runner.exe examples\mutex.json
.\build\Debug\petri_cli.exe examples\mutex.json bfs
Get-Content logs\metrics.jsonl -Tail 5
```

