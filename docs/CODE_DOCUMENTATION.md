# Документация к коду

Этот документ описывает текущее состояние репозитория `petri_net_simulator`. Описание основано на реально существующих файлах, классах, структурах и функциях проекта. Возможности, которых нет в коде, вынесены в раздел ограничений и не описываются как готовые.

## 1. Общая цель проекта

Проект реализует C++20-ядро автоматизированной подсистемы моделирования и интерпретации сетей Петри.

Ядро принимает описание сети Петри в JSON, проверяет корректность модели, строит внутреннее представление `PetriNet`, выполняет пошаговую симуляцию, строит ограниченный граф достижимости маркировок и запускает графовые алгоритмы DFS, BFS и Dijkstra. Результаты возвращаются в JSON-формате, удобном для визуализации, Python-интеграции и будущего серверного слоя.

В текущем коде уже есть:

- модель сети Петри;
- валидация JSON-модели;
- интерпретатор пошагового выполнения;
- построение ограниченного графа достижимости;
- графовые представления `DirectedGraph`, adjacency list, FSF и BSF;
- алгоритмы DFS, BFS и Dijkstra;
- runtime-реестр алгоритмов;
- простой выборщик алгоритма по метрикам;
- JSON service adapter без HTTP-сервера;
- логирование метрик в JSONL;
- файловый `storage_api` для моделей, результатов запусков и метрик;
- опциональные Python bindings через `pybind11`;
- CLI-пример и набор тестов.

## 2. Структура проекта

Текущая структура исходников выглядит так:

```text
petri_net_simulator/
  AGENTS.md
  API_CONTRACT.md
  CMakeLists.txt
  CODEX_INIT_PROMPT.md
  PLAN.md
  PROJECT.md
  README.md
  docs/
    CODE_DOCUMENTATION.md
    GITHUB_AND_CODEX_SETUP.md
    INTEGRATION_CONTRACT.md
  examples/
    mutex.json
    python_demo.py
  include/
    doctest/
      doctest.h
    nlohmann/
      json.hpp
    petri/
      algorithms/
        algorithms.hpp
      algorithm_selector/
        algorithm_selector.hpp
      common/
        result.hpp
      core_pn/
        interpreter.hpp
        model.hpp
        petri_net.hpp
      graph_models/
        directed_graph.hpp
        reachability.hpp
        star_forms.hpp
      logging_metrics/
        metrics_logger.hpp
      runtime/
        algorithm_runtime.hpp
      service_adapter/
        json_api.hpp
      storage_api/
        storage_api.hpp
  logs/
    .gitkeep
  src/
    algorithms/
      algorithms.cpp
    algorithm_selector/
      algorithm_selector.cpp
    bindings/
      python_bindings.cpp
    cli/
      petri_cli.cpp
    core_pn/
      interpreter.cpp
      petri_net.cpp
    graph_models/
      directed_graph.cpp
      reachability.cpp
      star_forms.cpp
    logging_metrics/
      metrics_logger.cpp
    runtime/
      algorithm_runtime.cpp
    service_adapter/
      json_api.cpp
    storage_api/
      storage_api.cpp
  tests/
    algorithm_selector_tests.cpp
    algorithms_tests.cpp
    core_pn_tests.cpp
    graph_models_tests.cpp
    integration_contract_tests.cpp
    integration_tests.cpp
    metrics_logger_tests.cpp
    python_smoke_test.py
    runtime_tests.cpp
    service_adapter_tests.cpp
    storage_api_tests.cpp
    test_main.cpp
```

Папка `build/` может существовать после запуска CMake. Это сгенерированная папка сборки, а не исходный код. В ней находятся файлы Visual Studio, библиотека `petri_core.lib`, исполняемые файлы `petri_cli.exe`, `petri_tests.exe`, а при успешной сборке Python bindings - модуль `petri_core`.

### Основные папки

`include/` содержит публичные заголовочные файлы. В них объявлены классы, структуры и функции, которые используются между модулями.

`src/` содержит реализации C++-кода из заголовочных файлов.

`tests/` содержит тесты на `doctest` и Python smoke test для опционального модуля `petri_core`.

`examples/` содержит демонстрационные входные данные и Python-пример.

`docs/` содержит документацию и интеграционные контракты.

`logs/` содержит `.gitkeep`, чтобы папка логов существовала в репозитории. По умолчанию метрики пишутся в `logs/metrics.jsonl`.

### Важные файлы в корне

`CMakeLists.txt` управляет сборкой. Он создаёт библиотеку `petri_core`, CLI-приложение `petri_cli`, тестовый бинарник `petri_tests` и, если найдены Python3 development files и `pybind11`, Python-модуль `petri_core`.

`README.md` описывает проект, реализованные модули, команды сборки и ограничения. Это справочный файл, он не участвует в компиляции.

`API_CONTRACT.md` описывает JSON-контракт запросов и ответов.

`PROJECT.md`, `PLAN.md`, `CODEX_INIT_PROMPT.md`, `AGENTS.md` содержат проектный контекст, план и рабочие инструкции. Они полезны для понимания дипломной задачи, но не подключаются к коду.

## 3. Описание каждого основного файла

### `include/petri/common/result.hpp`

Файл нужен для единого способа возвращать либо успешный результат, либо ошибку.

Содержит:

- `struct Error`;
- шаблон `Result<T>`;
- специализацию `Result<void>`;
- функцию `make_error()`.

`Error` принимает код ошибки, сообщение и карту дополнительных деталей. `Result<T>` возвращает значение типа `T` или ошибку. Этот файл используется почти всеми модулями, потому что ошибки в проекте передаются через `Result`, а не через разные несовместимые механизмы.

### `include/petri/core_pn/model.hpp`

Файл объявляет базовые типы сети Петри.

Содержит:

- `Place`;
- `Transition`;
- `ArcDirection`;
- `Arc`;
- `Marking`;
- `SimulationEvent`.

Эти типы используются в `PetriNet`, `Interpreter`, графе достижимости и JSON-ответах.

### `include/petri/core_pn/petri_net.hpp` и `src/core_pn/petri_net.cpp`

Эти файлы описывают и реализуют класс `PetriNet`.

`PetriNet` нужен для хранения сети Петри и правил её выполнения. Он принимает JSON, валидирует места, переходы и дуги, строит внутренние индексы, определяет разрешённые переходы и выполняет срабатывание перехода.

Основные данные:

- имя сети;
- список мест;
- список переходов;
- список дуг;
- индекс места по id;
- индекс перехода по id;
- входные дуги каждого перехода;
- выходные дуги каждого перехода.

Файл взаимодействует с `model.hpp`, `result.hpp`, `nlohmann/json.hpp`, `interpreter.cpp`, `reachability.cpp`, `json_api.cpp`, Python bindings и тестами.

### `include/petri/core_pn/interpreter.hpp` и `src/core_pn/interpreter.cpp`

Эти файлы реализуют интерпретатор сети Петри.

Содержат:

- `SimulationStrategy`;
- `SimulationParams`;
- `SimulationRun`;
- `parse_strategy()`;
- `strategy_to_string()`;
- класс `Interpreter`.

Интерпретатор принимает `PetriNet` и текущую маркировку, выбирает разрешённый переход по стратегии, выполняет шаг через `PetriNet::fire_transition()` и формирует `SimulationEvent`. Для серии шагов используется `Interpreter::run()`.

### `include/petri/graph_models/directed_graph.hpp` и `src/graph_models/directed_graph.cpp`

Файлы реализуют простой ориентированный граф.

Содержат:

- `GraphEdge`;
- `DirectedGraph`.

`DirectedGraph` хранит вершины и списки исходящих рёбер. Он используется алгоритмами DFS, BFS, Dijkstra, графом достижимости и графовыми формами FSF/BSF.

### `include/petri/graph_models/reachability.hpp` и `src/graph_models/reachability.cpp`

Файлы строят граф достижимости маркировок сети Петри.

Содержат:

- `ReachabilityOptions`;
- `ReachabilityGraph`;
- `build_reachability_graph()`;
- `find_marking_vertex()`;
- `reachability_graph_to_json()`.

Граф достижимости строится из `PetriNet`: вершины соответствуют маркировкам, рёбра соответствуют срабатываниям переходов, вес ребра равен `transition.fire_time`.

### `include/petri/graph_models/star_forms.hpp` и `src/graph_models/star_forms.cpp`

Файлы реализуют дополнительные представления графа:

- adjacency list;
- forward star form, FSF;
- backward star form, BSF.

Содержат структуры `AdjacencyListEdge`, `AdjacencyListRow`, `AdjacencyList`, `ForwardStarForm`, `BackwardStarForm`, функции преобразования из `DirectedGraph` и `ReachabilityGraph`, а также функции сериализации и десериализации JSON.

Эти представления уже реализованы и покрыты тестами, но сами DFS/BFS/Dijkstra сейчас работают с `DirectedGraph`, а не напрямую с FSF/BSF.

### `include/petri/algorithms/algorithms.hpp` и `src/algorithms/algorithms.cpp`

Файлы объявляют и реализуют алгоритмы:

- `run_dfs()`;
- `run_bfs()`;
- `run_dijkstra()`.

Общий вход алгоритмов:

- `DirectedGraph`;
- `AlgorithmParams`, где есть `source_vertex_id` и необязательный `target_vertex_id`.

Общий выход:

- `AlgorithmResult` с именем алгоритма, статусом, признаком найденного пути, путём и метриками.

Метрики хранятся в `AlgorithmMetrics`: время, посещённые вершины, просмотренные рёбра, длина пути и стоимость пути.

### `include/petri/runtime/algorithm_runtime.hpp` и `src/runtime/algorithm_runtime.cpp`

Файлы реализуют единый runtime-интерфейс алгоритмов.

Содержат:

- `AlgorithmRunContext`;
- `AlgorithmTask`;
- абстрактный класс `Algorithm`;
- тип `AlgorithmRunner`;
- класс `AlgorithmRegistry`;
- `create_builtin_algorithm_registry()`;
- `available_algorithms()`;
- `run()`;
- `run_algorithm()`.

Runtime регистрирует встроенные `dfs`, `bfs`, `dijkstra`, запускает алгоритм по имени и умеет записывать метрики через `logging_metrics`, если в `AlgorithmRunContext` включён `log_metrics`.

### `include/petri/algorithm_selector/algorithm_selector.hpp` и `src/algorithm_selector/algorithm_selector.cpp`

Файлы реализуют выбор алгоритма по взвешенной оценке метрик.

Содержат:

- `SelectionWeights`;
- `AlgorithmCandidateReport`;
- `AlgorithmSelectionRequest`;
- `AlgorithmSelectionReport`;
- `score_algorithm_result()`;
- `select_algorithm()`;
- `algorithm_selection_report_to_json()`.

Выборщик запускает несколько алгоритмов на одной задаче, считает score по весам `elapsed_ms`, `path_length`, `path_cost`, `visited_vertices` и выбирает лучший eligible-результат.

### `include/petri/logging_metrics/metrics_logger.hpp` и `src/logging_metrics/metrics_logger.cpp`

Файлы реализуют логирование метрик запусков.

Содержат:

- `RunMetricsInput`;
- `RunMetricsRecord`;
- `create_metrics_record()`;
- `metrics_record_to_json()`;
- `metrics_record_from_json()`;
- `default_metrics_log_path()`;
- `append_metrics_record()`;
- `read_metrics_records()`.

Метрики пишутся в JSONL. Путь по умолчанию: `logs/metrics.jsonl`.

### `include/petri/storage_api/storage_api.hpp` и `src/storage_api/storage_api.cpp`

Файлы реализуют файловое хранилище.

Содержат:

- `StorageConfig`;
- `StoredArtifact`;
- `models_directory()`;
- `runs_directory()`;
- `metrics_directory()`;
- `generate_run_id()`;
- `save_model()`;
- `load_model()`;
- `save_run_result()`;
- `load_run_result()`;
- `save_metrics()`.

По умолчанию данные сохраняются в папке `data/`: модели в `data/models`, результаты запусков в `data/runs`, метрики в `data/metrics`. Этот модуль существует как отдельный API и не вызывается автоматически из `service_adapter`.

### `include/petri/service_adapter/json_api.hpp` и `src/service_adapter/json_api.cpp`

Файлы реализуют библиотечный JSON service adapter.

Содержат:

- класс `JsonServiceAdapter`;
- `error_response()`;
- `simulate_request()`;
- `algorithm_request()`;
- `handle_request()`;
- `handle_request_json()`.

Adapter принимает JSON-запрос, создаёт `PetriNet`, запускает симуляцию или алгоритм и возвращает JSON-ответ. Это не HTTP-сервер: он не открывает порт и не работает с сетью.

### `src/bindings/python_bindings.cpp`

Файл реализует опциональный Python-модуль `petri_core` через `pybind11`.

Экспортируемые функции:

- `load_petri_net_from_json(net)`;
- `get_enabled_transitions(net, marking=None)`;
- `fire_transition(net, transition_id, marking=None)`;
- `run_simulation(net, params=None)`;
- `run_algorithm(net, params=None)`;
- `select_algorithm(net, params=None)`.

Python bindings принимают Python dict/list или JSON-строку, преобразуют данные в `nlohmann::json`, вызывают C++-ядро и возвращают Python dict/list.

### `src/cli/petri_cli.cpp`

Файл реализует консольный запуск.

Команда:

```text
petri_cli <net.json> [simulate|bfs|dfs|dijkstra]
```

CLI читает JSON сети из файла, формирует запрос к `petri::handle_request()` и печатает JSON-ответ.

### `examples/mutex.json`

Файл содержит демонстрационную сеть Петри для взаимного исключения двух процессов. В ней 7 мест, 6 переходов и 16 дуг.

### `examples/python_demo.py`

Файл демонстрирует вызов Python-модуля `petri_core`: загрузку сети, получение разрешённых переходов, срабатывание перехода, симуляцию, запуск BFS и выбор алгоритма.

### `tests/`

Тесты покрывают основные модули:

- `core_pn_tests.cpp` проверяет загрузку сети, валидацию, разрешённость переходов, срабатывание и интерпретатор;
- `algorithms_tests.cpp` проверяет DFS, BFS, Dijkstra;
- `runtime_tests.cpp` проверяет реестр алгоритмов и логирование через runtime;
- `algorithm_selector_tests.cpp` проверяет выбор лучшего алгоритма;
- `graph_models_tests.cpp` проверяет adjacency list, FSF, BSF и JSON-сериализацию;
- `integration_tests.cpp` проверяет связку `PetriNet -> reachability_graph -> algorithm`;
- `integration_contract_tests.cpp` проверяет форму JSON-ответов для интеграции;
- `service_adapter_tests.cpp` проверяет JSON adapter и ошибки;
- `metrics_logger_tests.cpp` проверяет JSONL-метрики;
- `storage_api_tests.cpp` проверяет файловое сохранение моделей, результатов и метрик;
- `python_smoke_test.py` проверяет Python bindings, если они собраны;
- `test_main.cpp` подключает `doctest` main.

## 4. Подробное описание кода

### Обработка ошибок

Единый механизм ошибок находится в `include/petri/common/result.hpp`.

`Error` содержит:

- `code` - машинный код ошибки;
- `message` - понятное сообщение;
- `details` - дополнительные данные в виде `std::map<std::string, std::string>`.

`Result<T>` хранит либо значение `T`, либо `Error`. Успешный результат создаётся через `Result<T>::success(value)`, ошибочный через `Result<T>::failure(error)`.

Метод `value()` выбрасывает `std::logic_error`, если попытаться получить значение из ошибочного результата. Метод `error()` выбрасывает `std::logic_error`, если попытаться получить ошибку из успешного результата. В нормальном коде перед этим вызывается `has_value()` или используется `if (result)`.

Типичные коды ошибок:

- `INVALID_JSON`;
- `DUPLICATE_ID`;
- `UNKNOWN_NODE_ID`;
- `INVALID_ARC_ENDPOINT`;
- `INVALID_ARC_DIRECTION`;
- `INVALID_ARC_WEIGHT`;
- `INVALID_TOKEN_COUNT`;
- `UNKNOWN_TRANSITION_ID`;
- `TRANSITION_NOT_ENABLED`;
- `UNKNOWN_ALGORITHM`;
- `NEGATIVE_EDGE_WEIGHT`;
- `STATE_LIMIT_EXCEEDED`;
- `TARGET_NOT_FOUND`;
- `INVALID_ALGORITHM`;
- `INVALID_ALGORITHM_TASK`;
- `METRICS_LOG_ERROR`;
- `STORAGE_ERROR`;
- `INVALID_STORAGE_ID`.

### Модель сети Петри

`Place` содержит:

- `id`;
- `label`;
- `tokens`.

`Transition` содержит:

- `id`;
- `label`;
- `fire_time`.

`Arc` содержит:

- `id`;
- `source`;
- `target`;
- `weight`;
- `direction`.

`ArcDirection` может быть:

- `PlaceToTransition`;
- `TransitionToPlace`.

`Marking` хранит вектор токенов `tokens_`. Порядок токенов соответствует порядку мест в `PetriNet::places_`.

Методы `Marking`:

- `tokens() const` возвращает константный вектор токенов;
- `tokens()` возвращает изменяемый вектор;
- `at(index)` возвращает число токенов по индексу;
- `set(index, value)` меняет число токенов;
- `size()` возвращает размер вектора;
- `key()` строит строковый ключ маркировки, например `1|0|2`;
- `operator==` сравнивает маркировки.

`SimulationEvent` содержит номер шага, время, id сработавшего перехода, маркировку до и после шага, а также списки разрешённых переходов до и после шага.

### `PetriNet`

Класс `PetriNet` объявлен в `petri_net.hpp` и реализован в `petri_net.cpp`.

Поля класса:

- `name_` - имя сети;
- `places_` - места;
- `transitions_` - переходы;
- `arcs_` - дуги;
- `place_index_by_id_` - словарь `place id -> index`;
- `transition_index_by_id_` - словарь `transition id -> index`;
- `input_arcs_by_transition_` - входные дуги каждого перехода;
- `output_arcs_by_transition_` - выходные дуги каждого перехода.

Внутренняя структура `WeightedPlace` хранит:

- индекс места;
- вес дуги;
- id дуги.

Метод `from_json(data)` принимает `nlohmann::json` и возвращает `Result<PetriNet>`. Он проверяет:

- корень должен быть объектом;
- `name`, если задан, должен быть строкой;
- `places`, `transitions`, `arcs` должны быть массивами;
- элементы массивов должны быть объектами;
- id мест, переходов и дуг должны быть строками;
- id не должны быть пустыми или повторяться внутри своей группы;
- `tokens` должен быть неотрицательным целым числом;
- `fire_time` должен быть неотрицательным числом;
- `weight` должен быть положительным целым числом;
- `source` и `target` дуги должны ссылаться на существующие место или переход;
- дуга должна соединять место с переходом или переход с местом.

Метод `load_from_file(path)` читает JSON из файла и вызывает `from_json()`.

Метод `rebuild_indices()` пересобирает индексы и списки входных/выходных дуг. Он вызывается после чтения мест и переходов, а затем после чтения дуг.

Метод `initial_marking()` строит `Marking` из начальных токенов мест.

Метод `marking_to_json(marking)` возвращает JSON-объект `{place_id: token_count}`.

Метод `enabled_transitions(marking)` возвращает id всех переходов, для которых во всех входных местах достаточно токенов.

Метод `is_enabled(marking, transition_id)` проверяет один переход и возвращает `Result<bool>`.

Метод `fire_transition(marking, transition_id)` выполняет атомарное срабатывание: сначала вычитает токены из входных мест, затем добавляет токены в выходные места. Если переход неизвестен или не разрешён, возвращается ошибка.

Методы `has_place()`, `has_transition()`, `place_index()`, `transition_index()` используются для валидации и быстрого доступа.

Метод `transition_fire_time(transition_id)` возвращает `fire_time`; для неизвестного id возвращает `1.0`.

Метод `marking_matches(marking, partial_marking)` проверяет, соответствует ли маркировка частичному JSON-описанию. В текущей версии частичная маркировка должна быть непустой, все её place id должны существовать, а значения должны совпасть.

### Интерпретатор

`SimulationStrategy` задаёт стратегию выбора перехода:

- `FirstEnabled` - первый разрешённый переход;
- `ById` - переход с заданным id;
- `RoundRobin` - циклический выбор следующего разрешённого перехода.

`SimulationParams` содержит:

- `max_steps`;
- `strategy`;
- `stop_on_deadlock`;
- `return_events`;
- `transition_id`.

`SimulationRun` содержит:

- `initial_marking`;
- `final_marking`;
- `events`;
- `steps_executed`;
- `elapsed_ms`;
- `deadlock`.

`parse_strategy(value)` принимает строку `first_enabled`, `by_id` или `round_robin`. Для неизвестной строки возвращает `INVALID_JSON`.

`Interpreter` хранит:

- ссылку на `PetriNet`;
- текущую маркировку;
- номер шага;
- текущее модельное время;
- курсор `round_robin`.

`Interpreter::step()` принимает необязательный `transition_id` и стратегию. Метод получает разрешённые переходы, выбирает один переход, вызывает `PetriNet::fire_transition()`, обновляет маркировку и формирует `SimulationEvent`.

`Interpreter::run(params)` выполняет до `max_steps` шагов. Если разрешённых переходов нет, устанавливается `deadlock`. Время выполнения измеряется через `std::chrono::steady_clock`.

### Графы и граф достижимости

`GraphEdge` содержит:

- `id`;
- `source`;
- `target`;
- `label`;
- `weight`.

В графе достижимости `label` хранит id перехода сети Петри, а `weight` хранит `fire_time`.

`DirectedGraph` хранит:

- `index_by_id_`;
- `vertex_ids_`;
- `adjacency_`;
- `edge_count_`.

Методы:

- `add_vertex(id)`;
- `add_edge(edge)`;
- `has_vertex(id)`;
- `vertex_ids()`;
- `edges_from(id)`;
- `edge_between(source, target)`;
- `vertex_count()`;
- `edge_count()`.

`ReachabilityOptions` содержит `max_states` и `max_depth`.

`ReachabilityGraph` содержит:

- `DirectedGraph graph`;
- `initial_vertex_id`;
- `markings`;
- `depths`.

`build_reachability_graph(net, options)` строит ограниченный граф достижимости. Начальная вершина получает id `m0`. Новые вершины получают id `m1`, `m2` и далее. Рёбра получают id `e0`, `e1` и далее. Повторная маркировка определяется через `Marking::key()`.

`find_marking_vertex()` ищет вершину по целевой маркировке.

`reachability_graph_to_json()` сериализует вершины, глубины, маркировки и рёбра графа достижимости.

### Adjacency list, FSF и BSF

`AdjacencyList` содержит список вершин и строки смежности. Каждая строка `AdjacencyListRow` содержит id вершины и исходящие рёбра `AdjacencyListEdge`.

`ForwardStarForm` содержит:

- `vertex_ids`;
- `row_offsets`;
- `edge_ids`;
- `target_vertex_ids`;
- `target_indices`;
- `labels`;
- `weights`.

`BackwardStarForm` содержит:

- `vertex_ids`;
- `row_offsets`;
- `edge_ids`;
- `source_vertex_ids`;
- `source_indices`;
- `labels`;
- `weights`.

Функции `adjacency_list_from_graph()`, `forward_star_from_graph()`, `backward_star_from_graph()` преобразуют `DirectedGraph` в соответствующие формы.

Функции `*_from_reachability_graph()` делают то же самое для `ReachabilityGraph`.

Функции `*_to_json()` и `*_from_json()` сериализуют и валидируют эти формы. Валидация проверяет массивы, индексы вершин, размеры массивов рёбер и монотонность `row_offsets`.

### DFS, BFS и Dijkstra

Алгоритмы объявлены в `algorithms.hpp` и реализованы в `algorithms.cpp`.

`AlgorithmParams` содержит:

- `source_vertex_id`;
- необязательный `target_vertex_id`.

`AlgorithmMetrics` содержит:

- `elapsed_ms`;
- `visited_vertices`;
- `scanned_edges`;
- `path_length`;
- `path_cost`.

`AlgorithmResult` содержит:

- `algorithm`;
- `status`;
- `found`;
- `path`;
- `metrics`.

Перед запуском каждый алгоритм вызывает `validate_source()`. Если исходная вершина отсутствует, возвращается `UNKNOWN_NODE_ID`. Если целевая вершина задана и отсутствует, возвращается `TARGET_NOT_FOUND`.

`run_bfs()` использует `std::queue`, множество посещённых вершин и карту родителей. BFS подходит для кратчайшего пути по числу рёбер.

`run_dfs()` использует `std::stack`, множество посещённых вершин и карту родителей. DFS подходит для обхода и поиска любого достижимого пути.

`run_dijkstra()` использует `std::priority_queue`, расстояния и карту родителей. Он учитывает веса рёбер. Если встречает отрицательный вес, возвращает `NEGATIVE_EDGE_WEIGHT`.

Вспомогательные функции:

- `reconstruct_path()` восстанавливает путь;
- `path_cost()` считает стоимость пути;
- `make_traversal_result()` собирает общий результат;
- `elapsed_ms()` считает время.

### Algorithm runtime

`AlgorithmRunContext` хранит служебный контекст запуска:

- `request_id`;
- `task_type`;
- количество мест, переходов и дуг;
- число построенных состояний;
- флаг `log_metrics`;
- путь к файлу метрик.

`AlgorithmTask` хранит имя алгоритма, указатель на граф и контекст.

Абстрактный класс `Algorithm` задаёт интерфейс:

- `name()`;
- `run(graph, params)`.

`AlgorithmRegistry` хранит зарегистрированные алгоритмы в `std::map`.

Методы `AlgorithmRegistry`:

- `register_algorithm(shared_ptr<Algorithm>)`;
- `register_algorithm(name, runner)`;
- `has_algorithm(name)`;
- `available_algorithms()`;
- `run(task, params)`.

`create_builtin_algorithm_registry()` регистрирует `dfs`, `bfs`, `dijkstra`.

`run_algorithm(name, graph, params)` остаётся удобной короткой функцией, которая создаёт `AlgorithmTask` и запускает встроенный runtime.

Если включён `task.context.log_metrics`, runtime пишет запись через `append_metrics_record()`.

### Algorithm selector

`SelectionWeights` задаёт веса критериев:

- `elapsed_ms`;
- `path_length`;
- `path_cost`;
- `visited_vertices`.

`AlgorithmSelectionRequest` содержит:

- `AlgorithmTask`;
- `AlgorithmParams`;
- список алгоритмов;
- веса;
- флаг `require_found`.

`select_algorithm()` запускает кандидатов, собирает `AlgorithmCandidateReport` и выбирает лучший результат с минимальным score.

`score_algorithm_result()` считает score как сумму выбранных метрик с весами.

`algorithm_selection_report_to_json()` возвращает JSON с:

- `best_algorithm`;
- `criteria`;
- `comparison`;
- `best_result`.

### Метрики

`RunMetricsInput` и `RunMetricsRecord` содержат:

- `request_id`;
- `algorithm_name`;
- `task_type`;
- количество мест, переходов и дуг;
- количество построенных состояний;
- количество просмотренных рёбер;
- время;
- найден ли путь;
- стоимость пути;
- длина пути;
- текст ошибки.

`create_metrics_record()` переносит данные из input в record.

`metrics_record_to_json()` формирует JSON.

`metrics_record_from_json()` читает JSON и валидирует поля.

`append_metrics_record()` добавляет строку JSON в JSONL-файл.

`read_metrics_records()` читает JSONL-файл и возвращает записи.

В `service_adapter` метрики пишутся автоматически для симуляций и алгоритмических запросов. В `runtime` метрики пишутся, если включить `AlgorithmRunContext::log_metrics`.

### Storage API

`StorageConfig` содержит корневую папку, по умолчанию `data`.

`StoredArtifact` содержит id сохранённого объекта и путь к файлу.

`models_directory()`, `runs_directory()`, `metrics_directory()` возвращают подпапки хранилища.

`generate_run_id()` создаёт id запуска на основе времени и случайного числа.

`save_model(model_id, model)` сохраняет JSON-модель в `data/models/<id>.json`.

`save_model(model)` использует `model.name`, если оно есть, иначе создаёт id.

`load_model(model_id)` читает модель.

`save_run_result(run_result)` сохраняет JSON-результат запуска в `data/runs/<run_id>.json`.

`load_run_result(run_id)` читает результат.

`save_metrics(run_id, metrics)` сохраняет объект или массив метрик в JSONL-файл `data/metrics/<run_id>.jsonl`.

Id очищаются функцией `sanitize_storage_id()`: недопустимые символы заменяются на `_`, начальные и конечные точки удаляются. Пустой id возвращает `INVALID_STORAGE_ID`.

### JSON service adapter

`JsonServiceAdapter::handle(request)` принимает JSON-объект. Он проверяет поле `mode` и направляет запрос:

- `simulate` в `simulate_request()`;
- `algorithm` в `algorithm_request()`.

`JsonServiceAdapter::handle_json(request_json)` принимает строку, парсит JSON и возвращает строку JSON.

`simulate_request()`:

1. Читает `request_id`.
2. Загружает `PetriNet` из поля `net`.
3. Валидирует `params`.
4. Создаёт `Interpreter`.
5. Выполняет `Interpreter::run()`.
6. Возвращает JSON с `initial_marking`, `final_marking`, `events`, `metrics`.
7. Записывает метрики в `logs/metrics.jsonl`.

`algorithm_request()`:

1. Читает `request_id`.
2. Загружает `PetriNet`.
3. Валидирует `params`.
4. Проверяет `graph_mode`. Сейчас поддерживается только `reachability_graph`.
5. Строит `ReachabilityGraph`.
6. Определяет source и target.
7. Создаёт `AlgorithmTask`.
8. Запускает runtime через `run(task, algorithm_params)`.
9. Возвращает JSON с путём, метриками и размером графа.
10. Runtime записывает метрики, потому что `task.context.log_metrics = true`.

`error_response()` возвращает единый JSON ошибки.

### Python bindings

Python bindings строятся только если CMake нашёл Python3 development files и `pybind11`.

Функция `py_to_json()` принимает Python-объект. Если это строка, она парсится как JSON. Иначе объект сериализуется через Python-модуль `json`.

Функция `json_to_py()` возвращает Python dict/list через `json.loads()`.

Функция `net_from_python()` создаёт `PetriNet` из Python-объекта.

Функция `marking_from_python()` строит `Marking`. Если маркировка не передана, используется начальная маркировка.

Экспортируемые функции модуля `petri_core`:

- `load_petri_net_from_json`;
- `get_enabled_transitions`;
- `fire_transition`;
- `run_simulation`;
- `run_algorithm`;
- `select_algorithm`.

Ошибки `Result` превращаются в `py::value_error` с кодом и сообщением.

## 5. Поток данных

### Общая схема

Реальный поток данных в текущем коде:

```text
JSON сети Петри
  -> net_from_request()
  -> PetriNet::from_json()
  -> валидация
  -> PetriNet
  -> simulate_request() или algorithm_request()
  -> JSON-ответ
  -> запись метрик в logs/metrics.jsonl
```

Важно: в текущем коде симуляция и алгоритмы являются двумя режимами JSON API. Режим `simulate` идёт через `Interpreter`. Режим `algorithm` строит граф достижимости напрямую из `PetriNet`, а не через объект `Interpreter`.

### Поток симуляции

```text
handle_request()
  -> simulate_request()
  -> net_from_request()
  -> PetriNet::from_json()
  -> Interpreter::run()
  -> Interpreter::step()
  -> PetriNet::enabled_transitions()
  -> PetriNet::fire_transition()
  -> event_to_json()
  -> JSON response
  -> append_metrics_record()
```

Входные данные: JSON с `mode = "simulate"`, полем `net` и параметрами `max_steps`, `strategy`, `stop_on_deadlock`, `return_events`, `transition_id`.

Выходные данные: JSON со статусом, именем модели, начальной и финальной маркировкой, событиями и метриками.

### Поток алгоритмов

```text
handle_request()
  -> algorithm_request()
  -> net_from_request()
  -> PetriNet::from_json()
  -> build_reachability_graph()
  -> DirectedGraph inside ReachabilityGraph
  -> run(task, params)
  -> AlgorithmRegistry::run()
  -> run_dfs() / run_bfs() / run_dijkstra()
  -> path_to_json()
  -> JSON response
  -> append_metrics_record()
```

Входные данные: JSON с `mode = "algorithm"`, полем `net` и параметрами `graph_mode`, `algorithm`, `max_states`, `max_depth`, `source`, `target` или `target_marking`.

Выходные данные: JSON с алгоритмом, путём, метриками и размером построенного графа.

### Поток построения графа достижимости

`build_reachability_graph()` начинает с `net.initial_marking()`. Для каждой маркировки:

1. Получает разрешённые переходы через `enabled_transitions()`.
2. Для каждого перехода получает новую маркировку через `fire_transition()`.
3. Проверяет, встречалась ли такая маркировка по `Marking::key()`.
4. Добавляет вершину, если маркировка новая.
5. Добавляет ребро с label = transition id и weight = `fire_time`.

Ограничения задаются `max_states` и `max_depth`.

### Поток метрик

Для симуляции `simulate_request()` напрямую вызывает внутреннюю функцию логирования и пишет запись в `logs/metrics.jsonl`.

Для алгоритмов `algorithm_request()` создаёт `AlgorithmTask` с `log_metrics = true`. После запуска алгоритма `AlgorithmRegistry::run()` пишет метрики через `append_metrics_record()`.

Для ручного логирования можно использовать `logging_metrics` напрямую.

### Поток сохранения результатов

`storage_api` реализован, но не подключён автоматически к `service_adapter`.

Если вызывающий код хочет сохранить модель, результат или метрики, он должен отдельно вызвать:

- `save_model()`;
- `save_run_result()`;
- `save_metrics()`.

Поэтому текущий JSON API возвращает результат и пишет метрики, но не сохраняет автоматически полный JSON-ответ в `data/runs`.

### Поток Python bindings

```text
Python dict/list/string
  -> py_to_json()
  -> C++ функции PetriNet / service_adapter / selector
  -> nlohmann::json
  -> json_to_py()
  -> Python dict/list
```

Python-функции `run_simulation()` и `run_algorithm()` формируют такие же запросы, как JSON service adapter.

## 6. Как собрать проект

Команды выполняются из корня репозитория `petri_net_simulator`.

### Генерация build-файлов

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

Если `cmake` не найден в `PATH`, можно использовать CMake из Visual Studio:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

### Сборка

```powershell
cmake --build build --config Debug
```

Или полным путём:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
```

### Запуск тестов

Для Visual Studio generator нужно указывать конфигурацию:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

Или полным путём:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

### Где искать результаты сборки

Для Debug-сборки основные файлы находятся в:

```text
build/Debug/petri_core.lib
build/Debug/petri_cli.exe
build/Debug/petri_tests.exe
```

Если Python bindings собраны, модуль `petri_core` также находится в папке конфигурации сборки, обычно `build/Debug`.

Логи CTest находятся в:

```text
build/Testing/Temporary/
```

Метрики запусков по умолчанию пишутся в:

```text
logs/metrics.jsonl
```

Файлы `storage_api` по умолчанию создаются в:

```text
data/models/
data/runs/
data/metrics/
```

### Запуск примеров

Симуляция через CLI:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json simulate
```

BFS через CLI:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json bfs
```

DFS через CLI:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json dfs
```

Dijkstra через CLI:

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json dijkstra
```

Python-пример, если собран модуль `petri_core`:

```powershell
$env:PYTHONPATH = (Resolve-Path build\Debug).Path
python examples\python_demo.py
```

Если `pybind11` или Python development files не найдены, CMake выводит предупреждение и пропускает сборку Python-модуля. C++-ядро и C++-тесты при этом продолжают собираться.

## 7. Как использовать проект для демонстрации

Для защиты удобно показать три сценария.

### 1. Сборка и тесты

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Это показывает, что проект собирается и все тесты проходят.

### 2. Симуляция сети Петри

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json simulate
```

В ответе видно:

- начальную маркировку;
- финальную маркировку;
- список событий;
- сработавшие переходы;
- метрики симуляции.

### 3. Поиск пути в графе достижимости

```powershell
.\build\Debug\petri_cli.exe examples\mutex.json bfs
.\build\Debug\petri_cli.exe examples\mutex.json dijkstra
```

Можно показать, что BFS ищет путь по числу шагов, а Dijkstra учитывает веса рёбер, которые в графе достижимости берутся из `fire_time`.

### 4. Python-интеграция

Если собран модуль `petri_core`:

```powershell
$env:PYTHONPATH = (Resolve-Path build\Debug).Path
python examples\python_demo.py
```

Этот пример показывает, что C++-ядро можно вызывать из Python и получать обычные Python dict/list.

## 8. Что уже реализовано

Реально реализованные возможности:

- C++20/CMake-проект.
- Статическая библиотека `petri_core`.
- CLI-приложение `petri_cli`.
- Базовая модель сети Петри: места, переходы, дуги, маркировки.
- Загрузка сети Петри из JSON.
- Валидация JSON-модели.
- Проверка разрешённых переходов.
- Атомарное срабатывание перехода.
- Пошаговая симуляция.
- Стратегии симуляции `first_enabled`, `by_id`, `round_robin`.
- JSON-ответы симуляции.
- Ориентированный граф `DirectedGraph`.
- Ограниченный граф достижимости.
- Поиск вершины графа достижимости по целевой маркировке.
- Adjacency list, FSF и BSF.
- JSON-сериализация и десериализация графовых форм.
- DFS.
- BFS.
- Dijkstra для неотрицательных весов.
- Единый результат алгоритмов с метриками.
- Runtime-реестр алгоритмов.
- Регистрация пользовательского алгоритма через runner.
- Логирование метрик runtime.
- Algorithm selector с weighted scoring.
- JSON service adapter для `simulate` и `algorithm`.
- Единый формат JSON-ошибок.
- Автоматическая запись метрик service adapter в `logs/metrics.jsonl`.
- Файловый `storage_api` для моделей, запусков и метрик.
- Опциональные Python bindings через `pybind11`.
- Python demo.
- C++ unit/integration tests.
- Python smoke test, если Python-модуль собран.

## 9. Что ещё не реализовано или ограничено

Ограничения текущей версии:

- HTTP-сервер не реализован. Есть только библиотечный JSON adapter.
- GUI и визуализация не входят в этот репозиторий.
- В JSON adapter поддержан только `graph_mode = "reachability_graph"`. `structural_graph` пока возвращает ошибку.
- Ингибиторные, цветные и стохастические сети Петри не реализованы.
- Расширения временных сетей сверх поля `fire_time` не реализованы.
- Граф достижимости ограничен параметрами `max_states` и `max_depth`; полный бесконечный граф не строится.
- DFS/BFS/Dijkstra работают с `DirectedGraph`. FSF/BSF уже реализованы как представления данных, но алгоритмы напрямую их не используют.
- `storage_api` не подключён автоматически к JSON adapter. Полные JSON-ответы не сохраняются в `data/runs` без отдельного вызова `save_run_result()`.
- Python-модуль собирается только при наличии `pybind11` и Python development files.
- Нет транзакционной базы данных. Хранилище использует обычные JSON и JSONL-файлы.
- Нет отдельного сетевого протокола. Интеграция выполняется через JSON-объекты, строки JSON, CLI или Python bindings.

Эти ограничения важны для пояснительной записки: они показывают границы текущего реализованного ядра и отделяют готовую функциональность от будущих направлений развития.
