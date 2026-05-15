# petri_net_simulator

Репозиторий содержит C++20-ядро для дипломного проекта "Автоматизированная подсистема моделирования и интерпретации сетей Петри". Ядро принимает сеть Петри в JSON, валидирует модель, выполняет симуляцию, строит ограниченный граф достижимости, запускает DFS/BFS/Dijkstra, собирает метрики и возвращает JSON-ответы для Python, визуализации или будущего серверного слоя.

HTTP-сервер и GUI в этом репозитории не реализуются. Текущая точка интеграции - библиотечный JSON adapter и опциональный Python-модуль через `pybind11`.

## Текущее состояние проекта

Реализованные модули:

- `core_pn` - модель сети Петри: `Place`, `Transition`, `Arc`, `Marking`, `PetriNet`, правила разрешённости и атомарного срабатывания переходов.
- `graph_models` - `DirectedGraph`, список смежности, FSF, BSF, построение ограниченного графа достижимости и JSON-сериализация графовых представлений.
- `algorithms` - DFS, BFS и Dijkstra для ориентированного графа.
- `runtime` - единый интерфейс алгоритмов, реестр алгоритмов, встроенная регистрация `dfs`, `bfs`, `dijkstra`.
- `algorithm_selector` - запуск нескольких алгоритмов на одной задаче, взвешенный скоринг по времени, длине пути, стоимости пути и числу посещённых вершин.
- `logging_metrics` - JSONL-журнал метрик запусков алгоритмов и симуляций.
- `service_adapter` - библиотечный JSON API без HTTP-сервера.
- `storage_api` - файловое хранилище JSON-моделей, JSON-результатов запусков и JSONL-метрик в `data/`.
- `bindings` - опциональный Python-модуль `petri_core` через `pybind11`.
- `cli` - консольный пример `petri_cli`.

Основные документы:

- `API_CONTRACT.md` - базовый JSON-контракт ядра.
- `docs/INTEGRATION_CONTRACT.md` - прикладной контракт для клиент-серверной части одногруппника.
- `docs/CODE_DOCUMENTATION.md` - подробное описание текущего кода.
- `AGENTS.md` - рабочие правила для Codex.
- `PLAN.md` - этапный план разработки.

## Структура репозитория

```text
petri_net_simulator/
  CMakeLists.txt
  AGENTS.md
  API_CONTRACT.md
  PLAN.md
  PROJECT.md
  README.md
  docs/
    CODE_DOCUMENTATION.md
    INTEGRATION_CONTRACT.md
  examples/
    mutex.json
    python_demo.py
  include/
    doctest/
    nlohmann/
    petri/
      algorithms/
      algorithm_selector/
      common/
      core_pn/
      graph_models/
      logging_metrics/
      runtime/
      service_adapter/
      storage_api/
  src/
    algorithms/
    algorithm_selector/
    bindings/
    cli/
    core_pn/
    graph_models/
    logging_metrics/
    runtime/
    service_adapter/
    storage_api/
  tests/
```

## Сборка на Windows через CMake

Минимальная сборка C++-ядра и тестов:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

В среде Visual Studio `cmake.exe` может находиться не в `PATH`. В таком случае можно вызвать его полным путём, например:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DCMAKE_BUILD_TYPE=Debug
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Если `pybind11` или Python development files не установлены, CMake выводит предупреждение и пропускает Python-модуль. C++-ядро, CLI и C++-тесты при этом продолжают собираться.

## Сборка Python-модуля

Python-биндинги собираются в модуль `petri_core`.

```bash
python -m pip install pybind11
python -m pybind11 --cmakedir
```

Передайте путь из второй команды в `pybind11_DIR`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=ON -Dpybind11_DIR="<path-from-python-m-pybind11-cmakedir>"
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

На Windows собранный модуль обычно лежит в `build/Debug`:

```powershell
$env:PYTHONPATH = (Resolve-Path build\Debug).Path
python examples/python_demo.py
```

## Быстрый запуск и демонстрация

Минимальная проверка C++-ядра без Python-биндингов:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DPETRI_BUILD_PYTHON_BINDINGS=OFF
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Демонстрационный запуск:

```powershell
.\build\Debug\demo_runner.exe examples\simple_chain.json
.\build\Debug\demo_runner.exe examples\mutex.json
```

Отдельный JSON-ответ алгоритма:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json bfs
```

Подробный сценарий для защиты находится в `docs/DEMO_AND_TESTING_GUIDE.md`.

## Примеры запуска

CLI-симуляция:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json simulate
```

BFS по графу достижимости:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json bfs
```

Dijkstra по графу достижимости:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json dijkstra
```

Python-пример:

```powershell
$env:PYTHONPATH = (Resolve-Path build\Debug).Path
python examples/python_demo.py
```

Python-пример требует предварительно собранный модуль `petri_core`.

## Ограничения текущей версии

- Поддерживаются обычные сети Петри. Ингибиторные, цветные, стохастические и временные расширения сверх `fire_time` не реализованы.
- JSON adapter поддерживает только `graph_mode = "reachability_graph"`. Режим `structural_graph` зарезервирован, но пока возвращает ошибку.
- Граф достижимости строится ограниченно через `max_states` и `max_depth`.
- DFS/BFS/Dijkstra работают с уже построенным графом; оптимизации Dijkstra через FSF/BSF подготовлены структурами данных, но сам алгоритм пока использует `DirectedGraph`.
- Python-модуль собирается только при наличии `pybind11` и Python development files.
- HTTP-сервер, GUI и база данных не входят в текущую версию.
- Файловое хранилище использует JSON/JSONL в папке `data/`; оно не является транзакционной базой данных.

## Что можно описать в дипломной записке

- Предметную модель сети Петри: места, переходы, дуги, маркировки и правила срабатывания.
- Валидацию входной JSON-модели и структурированные ошибки.
- Пошаговую интерпретацию сети Петри и стратегии выбора перехода.
- Построение ограниченного графа достижимости маркировок.
- Представления графов: adjacency list, FSF и BSF.
- Реализацию DFS, BFS и Dijkstra, а также различие критериев оптимальности.
- Единый runtime-интерфейс алгоритмов и реестр встроенных алгоритмов.
- Сбор метрик запусков и сохранение в JSONL.
- Взвешенный выбор алгоритма на основе метрик.
- Библиотечный JSON adapter как интеграционный слой для визуализации и серверной части.
- Python-биндинги через `pybind11` как способ вызвать C++-ядро из Python.
- Файловое хранилище моделей, результатов запусков и метрик без отдельной СУБД.

## Проверка перед сдачей

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Для проверки "с нуля" удобно использовать отдельную папку сборки:

```powershell
cmake -S . -B out\fresh-build -DCMAKE_BUILD_TYPE=Debug
cmake --build out\fresh-build --config Debug
ctest --test-dir out\fresh-build -C Debug --output-on-failure
```

