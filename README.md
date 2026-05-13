# Automated Petri Net Modeling and Interpretation Subsystem

Рабочее название репозитория: `petri-core`.

Проект реализует C++-ядро для автоматизированной подсистемы моделирования и интерпретации сетей Петри. Ядро должно принимать описание сети Петри в JSON, валидировать модель, выполнять пошаговую или автоматическую интерпретацию, строить граф состояний/маркировок и запускать алгоритмы DFS, BFS и Dijkstra. Результаты возвращаются в Python/клиент-серверную часть в JSON-формате для анализа, сбора метрик и визуализации.

## Роль этого репозитория

Этот репозиторий относится к части «разработчик 2»: C++/архитектура/ядро.

Зона ответственности:

- `core_pn` — объектная модель сети Петри: места, переходы, дуги, маркировки, правила срабатывания.
- `graph_models` — графовые структуры: adjacency list, FSF, BSF, преобразования, сериализация.
- `algorithms` / `algorithm_runtime` — единый интерфейс запуска алгоритмов и встроенные реализации `dfs`, `bfs`, `dijkstra`.
- `algorithm_selector` — базовый выбор алгоритма по типу задачи и метрикам. В первой версии допускается упрощённая эвристика.
- `logging_metrics` — сбор времени выполнения, числа просмотренных состояний/рёбер, длины найденного пути, статуса выполнения.
- `storage_api` — сохранение/загрузка моделей, запусков и результатов в JSON/CSV. База данных может быть отложена на следующую итерацию.
- `python_bindings` — Python API через `pybind11`.
- `service_adapter` — тонкий слой для интеграции с клиент-серверной частью одногруппника через JSON.

## Минимальная вертикальная версия

Первая рабочая версия должна уметь:

1. Принять JSON модели сети Петри с полями `places`, `transitions`, `arcs`.
2. Провалидировать ссылки дуг и веса.
3. Сформировать начальную маркировку.
4. Определить разрешённые переходы.
5. Выполнить один шаг интерпретации или серию шагов.
6. Вернуть список событий: какой переход сработал, маркировка до/после, время, разрешённые переходы.
7. Построить ограниченный граф достижимости маркировок (`max_states`, `max_depth`).
8. Запустить `dfs`, `bfs` или `dijkstra` по графу достижимости либо по обычному графу.
9. Вернуть результат алгоритма и метрики в JSON.

## Предлагаемая структура репозитория

```text
petri-core/
  CMakeLists.txt
  AGENTS.md
  README.md
  PROJECT.md
  PLAN.md
  API_CONTRACT.md
  examples/
    mutex.json
  include/petri/
    core_pn/
    graph_models/
    algorithms/
    runtime/
    metrics/
    bindings/
  src/
    core_pn/
    graph_models/
    algorithms/
    runtime/
    metrics/
    bindings/
    service_adapter/
  tests/
    core_pn/
    graph_models/
    algorithms/
    integration/
  python/
    petri_core/
  docs/
```

## Технологический стек по умолчанию

- C++20.
- CMake.
- `nlohmann/json` для JSON.
- `pybind11` для Python-биндингов.
- `Catch2` или `doctest` для тестов.
- `std::chrono` для замеров времени.
- `std::priority_queue` для Dijkstra.

## Быстрый старт после генерации кода

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

После появления Python-биндингов:

```bash
python -m pip install -e .
python examples/run_mutex.py
```

## Первый запрос к Codex

Открой файл `CODEX_INIT_PROMPT.md`, скопируй его содержимое в новый поток Codex и попроси начать с вертикального среза: JSON → PetriNet → step simulation → BFS/DFS/Dijkstra → JSON result → tests.
