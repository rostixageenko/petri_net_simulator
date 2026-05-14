# Документация к коду

Документ описывает текущее состояние репозитория `petri_net_simulator`. Здесь перечислены только те модули, файлы, классы и функции, которые уже есть в коде. Запланированные, но отсутствующие части вынесены в конец отдельным списком.

## 1. Структура проекта

В корне проекта находятся файлы управления сборкой, описание JSON-контракта, исходный код C++-ядра, пример входной сети Петри и тесты.

```text
petri_net_simulator/
  AGENTS.md
  API_CONTRACT.md
  CMakeLists.txt
  CODEX_INIT_PROMPT.md
  PLAN.md
  PROJECT.md
  README.md
  examples/
    mutex.json
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
      logging_metrics/
        metrics_logger.hpp
      runtime/
        algorithm_runtime.hpp
      service_adapter/
        json_api.hpp
  src/
    algorithms/
      algorithms.cpp
    algorithm_selector/
      algorithm_selector.cpp
    cli/
      petri_cli.cpp
    core_pn/
      interpreter.cpp
      petri_net.cpp
    graph_models/
      directed_graph.cpp
      reachability.cpp
    logging_metrics/
      metrics_logger.cpp
    runtime/
      algorithm_runtime.cpp
    service_adapter/
      json_api.cpp
  tests/
    algorithm_selector_tests.cpp
    algorithms_tests.cpp
    core_pn_tests.cpp
    integration_tests.cpp
    metrics_logger_tests.cpp
    runtime_tests.cpp
    service_adapter_tests.cpp
    test_main.cpp
  docs/
    CODE_DOCUMENTATION.md
  logs/
    .gitkeep
  build/
```

Папка `build/` уже присутствует в рабочем каталоге, но это не исходный код. Она создаётся CMake и содержит сгенерированные файлы Visual Studio, собранные исполняемые файлы, библиотеку и логи тестов.

Основные папки проекта:

- `include/petri/` содержит публичные заголовочные файлы проекта. В них объявлены структуры данных, классы и функции.
- `src/` содержит реализации функций и методов, объявленных в `include/petri/`.
- `tests/` содержит тесты на `doctest`.
- `examples/` содержит пример JSON-описания сети Петри.
- `docs/` содержит документацию к проекту.
- `logs/` содержит файл `.gitkeep` и используется для JSONL-журнала метрик запусков. Сами `.jsonl` и `.csv` файлы игнорируются Git.
- `include/nlohmann/json.hpp` содержит подключённую header-only библиотеку для работы с JSON.
- `include/doctest/doctest.h` содержит подключённый header-only фреймворк тестирования.

## 2. Назначение основных файлов

### Файлы в корне проекта

`CMakeLists.txt` отвечает за сборку проекта. В нём задаётся стандарт C++20, создаётся библиотека `petri_core`, исполняемый файл `petri_cli`, тестовый исполняемый файл `petri_tests` и регистрируется тест `petri_tests` для CTest. Этот файл связывает все `.cpp`-файлы из `src/` с заголовками из `include/`.

`API_CONTRACT.md` описывает JSON-форматы входной сети Петри, запроса симуляции, ответа симуляции, запроса алгоритма, ответа алгоритма и ошибок. Текущий код в `src/service_adapter/json_api.cpp` реализует именно этот тип обмена данными, но не все упомянутые в документе режимы уже поддержаны. Например, в коде поддержан только `graph_mode = "reachability_graph"`.

`README.md`, `PROJECT.md`, `PLAN.md` и `CODEX_INIT_PROMPT.md` содержат общее описание проекта, план и рабочий контекст. Эти файлы не участвуют в сборке программы.

`AGENTS.md` содержит инструкции для Codex по работе с проектом. В сборке и выполнении программы он не участвует.

### Пример входных данных

`examples/mutex.json` содержит пример сети Петри для задачи взаимного исключения двух процессов. В файле есть:

- 7 мест: состояния первого процесса, состояния второго процесса и место `mutex`;
- 6 переходов: запрос, вход в критическую секцию и выход для каждого процесса;
- 16 дуг между местами и переходами.

Этот файл используется в тестах `tests/core_pn_tests.cpp`, `tests/integration_tests.cpp`, `tests/service_adapter_tests.cpp` и в CLI-приложении.

### Общие типы результата

`include/petri/common/result.hpp` объявляет:

- `Error` — структуру ошибки с кодом, сообщением и дополнительными деталями;
- `Result<T>` — контейнер результата операции, который хранит либо значение типа `T`, либо ошибку;
- `Result<void>` — специальную версию результата для операций без возвращаемого значения;
- `make_error()` — вспомогательную функцию для создания `Error`.

Этот файл используется почти всеми основными модулями: моделью сети Петри, интерпретатором, графом достижимости, алгоритмами, runtime и JSON-адаптером.

### Модель сети Петри

`include/petri/core_pn/model.hpp` объявляет простые структуры предметной области:

- `Place` — место сети Петри;
- `Transition` — переход;
- `ArcDirection` — направление дуги;
- `Arc` — дуга;
- `Marking` — маркировка, то есть набор токенов по местам;
- `SimulationEvent` — событие одного шага симуляции.

Этот файл подключается в `petri_net.hpp`, `interpreter.hpp` и `reachability.hpp`.

`include/petri/core_pn/petri_net.hpp` объявляет класс `PetriNet`. Он хранит места, переходы, дуги, индексы по id, входные дуги переходов и выходные дуги переходов. Через этот класс выполняется загрузка JSON, валидация сети, получение начальной маркировки, проверка разрешённых переходов и срабатывание переходов.

`src/core_pn/petri_net.cpp` реализует методы класса `PetriNet`. Именно здесь проверяются обязательные поля JSON, уникальность id, корректность количества токенов, корректность веса дуги, существование концов дуги и запрет дуг вида `place -> place` и `transition -> transition`.

### Интерпретатор

`include/petri/core_pn/interpreter.hpp` объявляет:

- `SimulationStrategy` — стратегию выбора перехода;
- `SimulationParams` — параметры запуска симуляции;
- `SimulationRun` — результат серии шагов;
- `parse_strategy()` и `strategy_to_string()`;
- класс `Interpreter`.

`src/core_pn/interpreter.cpp` реализует пошаговую интерпретацию. Интерпретатор хранит текущую маркировку, номер шага, текущее модельное время и курсор для стратегии `round_robin`. Для изменения маркировки он использует методы `PetriNet`.

### Графовые модели

`include/petri/graph_models/directed_graph.hpp` объявляет:

- `GraphEdge` — ориентированное ребро графа;
- `DirectedGraph` — ориентированный граф на списках смежности.

`src/graph_models/directed_graph.cpp` реализует добавление вершин, добавление рёбер, проверку существования вершины, получение исходящих рёбер и поиск ребра между двумя вершинами.

`include/petri/graph_models/reachability.hpp` объявляет:

- `ReachabilityOptions` — ограничения построения графа достижимости;
- `ReachabilityGraph` — граф достижимости вместе с маркировками вершин и глубинами;
- `build_reachability_graph()` — построение ограниченного графа достижимости;
- `find_marking_vertex()` — поиск вершины по частичной маркировке;
- `reachability_graph_to_json()` — сериализацию графа достижимости в JSON.

`src/graph_models/reachability.cpp` реализует построение графа достижимости. Вершины графа соответствуют маркировкам, а рёбра соответствуют срабатываниям переходов. Вес ребра берётся из `transition.fire_time`.

### Алгоритмы

`include/petri/algorithms/algorithms.hpp` объявляет единый интерфейс алгоритмов:

- `AlgorithmParams` — параметры запуска алгоритма;
- `AlgorithmMetrics` — метрики выполнения;
- `AlgorithmResult` — результат алгоритма;
- `run_dfs()`;
- `run_bfs()`;
- `run_dijkstra()`.

`src/algorithms/algorithms.cpp` реализует DFS, BFS и алгоритм Дейкстры для `DirectedGraph`. Все три функции возвращают `Result<AlgorithmResult>`, то есть либо результат с путём и метриками, либо структурированную ошибку.

### Селектор алгоритмов

`include/petri/algorithm_selector/algorithm_selector.hpp` объявляет слой выбора алгоритма поверх runtime:

- `SelectionWeights` — веса критериев скоринга: время выполнения, длина пути, стоимость пути и число посещённых вершин;
- `AlgorithmCandidateReport` — строка сравнения для одного алгоритма;
- `AlgorithmSelectionRequest` — задача, параметры, список алгоритмов и веса выбора;
- `AlgorithmSelectionReport` — итоговый отчёт со всеми кандидатами, лучшим алгоритмом и лучшим результатом;
- `score_algorithm_result()` считает взвешенный балл результата;
- `select_algorithm()` запускает несколько алгоритмов через `AlgorithmRegistry` и выбирает результат с минимальным баллом;
- `algorithm_selection_report_to_json()` формирует JSON-отчёт с таблицей сравнения.

`src/algorithm_selector/algorithm_selector.cpp` реализует запуск набора алгоритмов на одной задаче. Если список алгоритмов пуст, используются все алгоритмы из переданного реестра. Ошибка отдельного алгоритма, например `UNKNOWN_ALGORITHM`, записывается в таблицу сравнения как строка со статусом `error`, но не останавливает сравнение остальных кандидатов. Если в `AlgorithmTask.context.log_metrics` включено логирование, каждый запуск кандидата проходит через runtime и сохраняет метрики тем же механизмом `logging_metrics`.

### Runtime выбора алгоритма

`include/petri/runtime/algorithm_runtime.hpp` объявляет единый runtime-интерфейс алгоритмов:

- `AlgorithmRunContext` — контекст запуска для логирования метрик;
- `AlgorithmTask` — описание задачи алгоритма: имя алгоритма, граф и контекст;
- `Algorithm` — базовый интерфейс алгоритма с методами `name()` и `run(graph, params)`;
- `AlgorithmRunner` — функциональный тип для регистрации алгоритма без отдельного класса;
- `AlgorithmRegistry` — реестр алгоритмов;
- `create_builtin_algorithm_registry()` создаёт реестр со встроенными алгоритмами;
- `available_algorithms()` возвращает список встроенных алгоритмов;
- `run(task, params)` запускает задачу через встроенный реестр;
- `run_algorithm(name, graph, params)` оставлен как совместимый фасад.

`src/runtime/algorithm_runtime.cpp` реализует реестр алгоритмов. Встроенный реестр регистрирует:

- `"dfs"` вызывает `run_dfs()`;
- `"bfs"` вызывает `run_bfs()`;
- `"dijkstra"` вызывает `run_dijkstra()`;
- неизвестное имя возвращает ошибку `UNKNOWN_ALGORITHM`.

`AlgorithmRegistry` умеет регистрировать объект `Algorithm` или функциональный `AlgorithmRunner`. Это оставляет архитектурное место для будущих пользовательских алгоритмов и плагинов без реализации механизма загрузки плагинов в текущей версии.

Если в `AlgorithmTask.context.log_metrics` установлено `true`, runtime создаёт запись `RunMetricsRecord` и сохраняет её через модуль `logging_metrics`. Для успешного запуска записываются метрики из `AlgorithmResult.metrics`; для ошибки неизвестного алгоритма или ошибки выполнения записывается `found = false` и сообщение ошибки.

Этот слой используется JSON-адаптером, чтобы внешний запрос мог указывать алгоритм строкой и запускать его через единый интерфейс.

### JSON-адаптер

`include/petri/service_adapter/json_api.hpp` объявляет класс и функции:

- `JsonServiceAdapter` — статeless-класс библиотечного адаптера с методами `handle()` и `handle_json()`;
- `error_response()`;
- `simulate_request()`;
- `algorithm_request()`;
- `handle_request()`;
- `handle_request_json()`.

`src/service_adapter/json_api.cpp` реализует внешний JSON API. Он принимает JSON-запрос, достаёт из него сеть Петри, создаёт `PetriNet`, запускает симуляцию или алгоритм и формирует JSON-ответ. Адаптер не содержит HTTP-сервера и предназначен для вызова из C++-кода, будущих Python-биндингов или тонкого серверного слоя.

### CLI

`src/cli/petri_cli.cpp` содержит простое консольное приложение. Оно принимает путь к JSON-файлу сети Петри и необязательный режим:

```text
petri_cli <net.json> [simulate|bfs|dfs|dijkstra]
```

Если режим равен `simulate`, CLI формирует запрос симуляции. Если указан `bfs`, `dfs` или `dijkstra`, CLI формирует запрос алгоритма по графу достижимости. Затем CLI вызывает `petri::handle_request()` и печатает JSON-ответ в консоль.

### Тесты

`tests/test_main.cpp` подключает `doctest` и создаёт точку входа для всех тестов.

`tests/core_pn_tests.cpp` проверяет загрузку примера `mutex.json`, валидацию некорректных дуг и токенов, поиск разрешённых переходов, срабатывание перехода и работу интерпретатора.

`tests/algorithms_tests.cpp` проверяет BFS, DFS, Dijkstra и ошибку неизвестного алгоритма.

`tests/algorithm_selector_tests.cpp` проверяет взвешенный выбор BFS по длине пути, Dijkstra по стоимости пути, JSON-отчёт сравнения и строку ошибки для неизвестного алгоритма.

`tests/integration_tests.cpp` проверяет совместную работу сети Петри, графа достижимости и алгоритмов.

`tests/metrics_logger_tests.cpp` проверяет создание записи метрик, сохранение и чтение JSONL-файла, а также автоматическую запись успешного и ошибочного запуска через JSON service adapter.

`tests/runtime_tests.cpp` проверяет список встроенных алгоритмов в runtime, запуск через `AlgorithmTask`, регистрацию функционального алгоритма в `AlgorithmRegistry`, ошибку `UNKNOWN_ALGORITHM` и запись метрик runtime.

`tests/service_adapter_tests.cpp` проверяет JSON-ответы симуляции, JSON-ответы алгоритма и структурированную ошибку для некорректного запроса.

## 3. Подробное описание кода

### `include/petri/common/result.hpp`

Файл задаёт единый способ возвращать успешный результат или ошибку без смешивания разных подходов.

`struct Error` содержит:

- `code` — машинно-читаемый код ошибки, например `INVALID_JSON` или `UNKNOWN_ALGORITHM`;
- `message` — человекочитаемое сообщение;
- `details` — словарь дополнительных строковых деталей.

`template <typename T> class Result` содержит:

- приватное поле `std::optional<T> value_`;
- приватное поле `Error error_`.

Основные методы `Result<T>`:

- `static success(T value)` создаёт успешный результат и сохраняет значение;
- `static failure(Error error)` создаёт ошибочный результат;
- `has_value()` сообщает, есть ли значение;
- `operator bool()` позволяет писать `if (result)`;
- `value()` возвращает сохранённое значение или выбрасывает `std::logic_error`, если результата нет;
- `error()` возвращает ошибку или выбрасывает `std::logic_error`, если результат успешный.

`Result<void>` нужен для проверок, где нет полезного возвращаемого значения. В текущем коде он используется при валидации параметров алгоритма.

`make_error()` создаёт `Error` из кода, сообщения и необязательных деталей.

Роль файла: он связывает все модули общим форматом ошибок. Благодаря этому `PetriNet`, алгоритмы и JSON-адаптер могут одинаково передавать ошибки наверх.

### `include/petri/core_pn/model.hpp`

Файл содержит базовые структуры сети Петри.

`struct Place` описывает место:

- `id` — внутренний идентификатор места;
- `label` — подпись для отображения;
- `tokens` — начальное количество токенов.

`struct Transition` описывает переход:

- `id` — внутренний идентификатор перехода;
- `label` — подпись для отображения;
- `fire_time` — длительность или стоимость срабатывания. По умолчанию равна `1.0`.

`enum class ArcDirection` описывает направление дуги:

- `PlaceToTransition` — дуга из места в переход;
- `TransitionToPlace` — дуга из перехода в место.

`struct Arc` описывает дугу:

- `id` — идентификатор дуги;
- `source` — id исходного узла;
- `target` — id целевого узла;
- `weight` — вес дуги, по умолчанию `1`;
- `direction` — направление дуги.

`class Marking` хранит текущую маркировку сети:

- приватное поле `std::vector<int> tokens_` хранит токены в порядке массива `places_` внутри `PetriNet`.

Основные методы `Marking`:

- конструктор `Marking(std::vector<int> tokens)` принимает вектор токенов;
- `tokens() const` возвращает константную ссылку на вектор;
- `tokens()` возвращает изменяемую ссылку;
- `at(index)` возвращает количество токенов по индексу места;
- `set(index, value)` записывает количество токенов по индексу;
- `size()` возвращает число мест в маркировке;
- `key()` формирует строковый ключ вида `1|0|0`. Он используется при построении графа достижимости, чтобы определять, встречалась ли такая маркировка раньше;
- `operator==` сравнивает две маркировки по вектору токенов.

`struct SimulationEvent` описывает один шаг симуляции:

- `step` — номер шага;
- `time` — модельное время после срабатывания перехода;
- `fired_transition` — id сработавшего перехода;
- `marking_before` — маркировка до шага;
- `marking_after` — маркировка после шага;
- `enabled_before` — переходы, разрешённые до шага;
- `enabled_after` — переходы, разрешённые после шага.

Роль файла: он задаёт простые типы, на которых строятся `PetriNet`, `Interpreter`, граф достижимости и JSON-ответы.

### `include/petri/logging_metrics/metrics_logger.hpp` и `src/logging_metrics/metrics_logger.cpp`

Эти файлы реализуют модуль `logging_metrics`, который формирует и сохраняет метрики запусков симуляции и алгоритмов.

Публичные структуры:

- `RunMetricsInput` — входные данные для создания записи метрик;
- `RunMetricsRecord` — готовая запись метрик.

Запись содержит:

- `request_id`;
- `algorithm_name` — имя алгоритма или `simulation` для симуляции;
- `task_type` — тип задачи, например `simulation` или `reachability_graph`;
- `place_count`, `transition_count`, `arc_count`;
- `built_state_count`;
- `scanned_edge_count`;
- `elapsed_ms`;
- `found`;
- `path_cost`;
- `path_length`;
- `error_message`.

Публичные функции:

- `create_metrics_record(input)` создаёт запись метрик;
- `metrics_record_to_json(record)` преобразует запись в JSON;
- `metrics_record_from_json(data)` восстанавливает запись из JSON;
- `default_metrics_log_path()` возвращает путь `logs/metrics.jsonl`;
- `append_metrics_record(record)` добавляет запись в стандартный JSONL-журнал;
- `append_metrics_record(record, path)` добавляет запись в указанный JSONL-файл;
- `read_metrics_records()` читает записи из стандартного JSONL-журнала;
- `read_metrics_records(path)` читает записи из указанного JSONL-файла.

Формат хранения — JSONL: каждая строка файла является отдельным JSON-объектом метрик. Перед записью модуль создаёт родительскую папку, если её ещё нет. Ошибки записи и чтения возвращаются через `Result` с кодом `METRICS_LOG_ERROR`; ошибки разбора JSONL возвращаются с кодом `INVALID_JSON`.

JSON service adapter вызывает этот модуль для каждого запуска `simulate_request()` и `algorithm_request()`. Успешные запуски записываются с пустым `error_message`; ошибочные запуски записываются с `found = false` и человекочитаемым сообщением ошибки.

### `include/petri/core_pn/petri_net.hpp` и `src/core_pn/petri_net.cpp`

Эти файлы реализуют центральную модель сети Петри.

`class PetriNet` хранит:

- `name_` — имя сети;
- `places_` — список мест;
- `transitions_` — список переходов;
- `arcs_` — список дуг;
- `place_index_by_id_` — быстрый поиск индекса места по id;
- `transition_index_by_id_` — быстрый поиск индекса перехода по id;
- `input_arcs_by_transition_` — для каждого перехода список входных мест с весами дуг;
- `output_arcs_by_transition_` — для каждого перехода список выходных мест с весами дуг.

Внутренняя структура `WeightedPlace` содержит:

- `place_index` — индекс места в `places_`;
- `weight` — вес дуги;
- `arc_id` — id дуги.

Основные публичные методы `PetriNet`:

- `from_json(const nlohmann::json& data)` создаёт сеть Петри из JSON.
- `load_from_file(const std::string& path)` читает JSON из файла и вызывает `from_json()`.
- `name()` возвращает имя сети.
- `places()` возвращает список мест.
- `transitions()` возвращает список переходов.
- `arcs()` возвращает список дуг.
- `initial_marking()` строит начальную маркировку из поля `tokens` каждого места.
- `marking_to_json(const Marking& marking)` преобразует маркировку в объект JSON вида `{ "p1": 1, "p2": 0 }`.
- `enabled_transitions(const Marking& marking)` возвращает список id переходов, которые разрешены в данной маркировке.
- `is_enabled(const Marking& marking, const std::string& transition_id)` проверяет один переход.
- `fire_transition(const Marking& marking, const std::string& transition_id)` атомарно выполняет переход и возвращает новую маркировку.
- `has_place(id)` проверяет наличие места.
- `has_transition(id)` проверяет наличие перехода.
- `place_index(id)` возвращает индекс места или `std::nullopt`.
- `transition_index(id)` возвращает индекс перехода или `std::nullopt`.
- `transition_fire_time(transition_id)` возвращает `fire_time` перехода. Если id неизвестен, метод возвращает `1.0`.
- `marking_matches(marking, partial_marking)` проверяет, подходит ли маркировка под частичный JSON-образец. Образец должен содержать хотя бы одно известное место; неизвестные id мест не считаются совпадением.

`from_json()` выполняет основную валидацию входной сети:

- корневой JSON должен быть объектом;
- поля `places`, `transitions` и `arcs` должны быть массивами;
- `id` места, перехода и дуги должен быть строкой;
- id внутри своей категории не должен быть пустым или повторяться;
- `tokens` должен быть неотрицательным целым числом;
- `fire_time` должен быть неотрицательным числом;
- `weight` дуги должен быть положительным целым числом;
- `source` и `target` дуги должны ссылаться на существующие место или переход;
- дуга должна соединять место и переход, а не два места и не два перехода.

`rebuild_indices()` пересобирает внутренние индексы. Он заполняет `place_index_by_id_`, `transition_index_by_id_`, `input_arcs_by_transition_` и `output_arcs_by_transition_`. Этот метод вызывается после чтения мест и переходов, а затем ещё раз после чтения дуг.

`enabled_transitions()` проходит по всем переходам и проверяет входные дуги. Переход считается разрешённым, если во всех входных местах достаточно токенов с учётом веса дуги.

`fire_transition()` сначала вызывает `is_enabled()`. Если переход неизвестен или не разрешён, возвращается ошибка. Если переход можно выполнить, метод копирует текущую маркировку, затем вычитает токены из входных мест и прибавляет токены в выходные места. Это реализует атомарное срабатывание перехода.

Роль файлов: `PetriNet` является основой всей программы. Без него не работает ни симуляция, ни граф достижимости, ни алгоритмы над графом достижимости.

### `include/petri/core_pn/interpreter.hpp` и `src/core_pn/interpreter.cpp`

Эти файлы реализуют пошаговую симуляцию сети Петри.

`enum class SimulationStrategy` содержит стратегии выбора перехода:

- `FirstEnabled` — выбрать первый разрешённый переход;
- `ById` — выбрать переход по заданному id;
- `RoundRobin` — циклически выбирать следующий разрешённый переход.

`struct SimulationParams` содержит:

- `max_steps` — максимальное число шагов, по умолчанию `1`;
- `strategy` — стратегия выбора перехода;
- `stop_on_deadlock` — останавливать ли запуск при тупике;
- `return_events` — возвращать ли события шагов;
- `transition_id` — id перехода для стратегии `ById` или явного шага.

`struct SimulationRun` содержит:

- `initial_marking` — маркировку перед запуском;
- `final_marking` — маркировку после запуска;
- `events` — список событий;
- `steps_executed` — число выполненных шагов;
- `elapsed_ms` — реальное время выполнения симуляции в миллисекундах;
- `deadlock` — признак тупика.

Функция `parse_strategy()` принимает строку из JSON и возвращает `SimulationStrategy`. Поддерживаются строки `first_enabled`, `by_id` и `round_robin`.

Функция `strategy_to_string()` делает обратное преобразование из enum в строку.

`class Interpreter` хранит:

- ссылку `net_` на `PetriNet`;
- текущую `marking_`;
- `step_index_` — номер последнего выполненного шага;
- `current_time_` — модельное время;
- `round_robin_cursor_` — позицию для циклического выбора перехода.

Конструктор `Interpreter(const PetriNet& net)` начинает с начальной маркировки сети.

Конструктор `Interpreter(const PetriNet& net, Marking marking)` начинает с переданной маркировки.

`marking()` возвращает текущую маркировку.

`current_time()` возвращает текущее модельное время.

`choose_transition()` выбирает переход из списка разрешённых:

- если список пустой, возвращает ошибку `TRANSITION_NOT_ENABLED`;
- если передан конкретный `transition_id` или выбрана стратегия `ById`, проверяет, что этот переход разрешён;
- для `RoundRobin` ищет следующий разрешённый переход, начиная с `round_robin_cursor_`;
- для `FirstEnabled` возвращает первый id из списка разрешённых.

`step()` выполняет один шаг:

1. Сохраняет маркировку до шага.
2. Получает список разрешённых переходов через `PetriNet::enabled_transitions()`.
3. Выбирает переход через `choose_transition()`.
4. Получает новую маркировку через `PetriNet::fire_transition()`.
5. Увеличивает модельное время на `PetriNet::transition_fire_time()`.
6. Обновляет текущую маркировку.
7. Формирует `SimulationEvent`.

`run()` выполняет серию шагов до `max_steps`. Перед каждым шагом он проверяет, есть ли разрешённые переходы. Если переходов нет и `stop_on_deadlock = true`, запуск останавливается и ставится `deadlock = true`. В конце метод возвращает `SimulationRun`.

Роль файлов: интерпретатор превращает статическую модель `PetriNet` в динамическое выполнение: шаги, события, финальную маркировку и метрики запуска.

### `include/petri/graph_models/directed_graph.hpp` и `src/graph_models/directed_graph.cpp`

Эти файлы реализуют простой ориентированный граф.

`struct GraphEdge` содержит:

- `id` — идентификатор ребра;
- `source` — id исходной вершины;
- `target` — id целевой вершины;
- `label` — подпись ребра. В графе достижимости здесь хранится id перехода сети Петри;
- `weight` — вес ребра.

`class DirectedGraph` хранит:

- `index_by_id_` — индекс вершины по её id;
- `vertex_ids_` — список id вершин;
- `adjacency_` — список исходящих рёбер для каждой вершины;
- `edge_count_` — количество рёбер.

Методы `DirectedGraph`:

- `add_vertex(id)` добавляет вершину, если её ещё нет;
- `add_edge(edge)` добавляет ребро и автоматически добавляет его исходную и целевую вершины;
- `has_vertex(id)` проверяет наличие вершины;
- `vertex_ids()` возвращает список вершин;
- `edges_from(id)` возвращает исходящие рёбра вершины. Если вершины нет, возвращается пустой список;
- `edge_between(source, target)` ищет первое ребро между двумя вершинами;
- `vertex_count()` возвращает число вершин;
- `edge_count()` возвращает число рёбер.

Роль файлов: `DirectedGraph` является общей структурой для DFS, BFS, Dijkstra и графа достижимости.

### `include/petri/graph_models/reachability.hpp` и `src/graph_models/reachability.cpp`

Эти файлы строят граф достижимости маркировок сети Петри.

`struct ReachabilityOptions` содержит:

- `max_states` — максимальное число вершин, по умолчанию `1000`;
- `max_depth` — максимальная глубина обхода, по умолчанию `20`.

`struct ReachabilityGraph` содержит:

- `graph` — объект `DirectedGraph`;
- `initial_vertex_id` — id вершины начальной маркировки;
- `markings` — соответствие `vertex_id -> Marking`;
- `depths` — соответствие `vertex_id -> depth`.

`build_reachability_graph()` принимает `PetriNet` и `ReachabilityOptions`. Если `max_states` равен нулю, функция возвращает ошибку `STATE_LIMIT_EXCEEDED`. Затем она создаёт начальную вершину `m0` по `net.initial_marking()` и обходит достижимые маркировки через очередь.

Для каждой текущей маркировки функция:

1. Проверяет ограничение `max_depth`.
2. Получает разрешённые переходы через `PetriNet::enabled_transitions()`.
3. Для каждого перехода строит новую маркировку через `PetriNet::fire_transition()`.
4. Добавляет новую вершину, если такой маркировки ещё не было.
5. Добавляет ребро `GraphEdge` из текущей вершины в новую.

Id вершин имеют вид `m0`, `m1`, `m2` и так далее. Id рёбер имеют вид `e0`, `e1`, `e2` и так далее. В поле `label` ребра записывается id перехода, а в поле `weight` записывается `transition.fire_time`.

`find_marking_vertex()` ищет вершину графа достижимости по частичной маркировке. Например, JSON `{ "p1_cs": 1 }` означает: найти любую вершину, где в месте `p1_cs` находится один токен.

`reachability_graph_to_json()` преобразует граф достижимости в JSON с массивами `vertices` и `edges`. В текущем JSON API эта функция объявлена и реализована, но основной ответ `algorithm_request()` возвращает только количество вершин и рёбер, а не полный граф.

Роль файлов: они переводят поведение сети Петри в обычный ориентированный граф, по которому затем можно запускать DFS, BFS и Dijkstra.

### `include/petri/algorithms/algorithms.hpp` и `src/algorithms/algorithms.cpp`

Эти файлы реализуют три алгоритма над `DirectedGraph`.

`struct AlgorithmParams` содержит:

- `source_vertex_id` — id начальной вершины;
- `target_vertex_id` — необязательный id целевой вершины.

Если цель не задана, DFS, BFS и Dijkstra возвращают порядок обхода как путь.

`struct AlgorithmMetrics` содержит:

- `elapsed_ms` — время выполнения алгоритма в миллисекундах;
- `visited_vertices` — число посещённых вершин;
- `scanned_edges` — число просмотренных рёбер;
- `path_length` — длина найденного пути в рёбрах;
- `path_cost` — стоимость найденного пути.

`struct AlgorithmResult` содержит:

- `algorithm` — имя алгоритма;
- `status` — строковый статус, например `ok` или `target_not_found`;
- `found` — найден ли путь;
- `path` — список id вершин;
- `metrics` — метрики выполнения.

Внутри `algorithms.cpp` есть вспомогательные функции:

- `elapsed_ms()` считает прошедшее время;
- `reconstruct_path()` восстанавливает путь от источника к цели по словарю родителей;
- `path_cost()` считает стоимость пути по весам рёбер;
- `make_traversal_result()` собирает общий `AlgorithmResult`;
- `validate_source()` проверяет, что исходная вершина существует, и что целевая вершина существует, если она задана.

`run_bfs()` реализует поиск в ширину:

- принимает `DirectedGraph` и `AlgorithmParams`;
- проверяет исходную и целевую вершины;
- использует `std::queue`;
- посещает вершины по слоям;
- запоминает родителей вершин;
- для невзвешенного графа находит путь с минимальным числом рёбер;
- возвращает `AlgorithmResult` с путём и метриками.

`run_dfs()` реализует поиск в глубину:

- принимает `DirectedGraph` и `AlgorithmParams`;
- использует `std::stack`;
- идёт как можно глубже по рёбрам;
- запоминает родителей вершин;
- возвращает найденный путь до цели или порядок обхода, если цель не задана.

`run_dijkstra()` реализует алгоритм Дейкстры:

- принимает `DirectedGraph` и `AlgorithmParams`;
- использует `std::priority_queue`;
- хранит расстояния от исходной вершины;
- выбирает вершину с минимальной текущей стоимостью;
- обновляет расстояния до соседей;
- возвращает кратчайший по весам путь;
- если встречает отрицательный вес ребра, возвращает ошибку `NEGATIVE_EDGE_WEIGHT`.

Роль файлов: они дают готовые алгоритмы анализа графа. В проекте они применяются к графу достижимости маркировок сети Петри.

### `include/petri/runtime/algorithm_runtime.hpp` и `src/runtime/algorithm_runtime.cpp`

Файл `algorithm_runtime.hpp` объявляет единый интерфейс запуска алгоритмов:

- `AlgorithmRunContext` хранит контекст запуска для метрик: `request_id`, `task_type`, размеры сети, число построенных состояний, флаг `log_metrics` и необязательный путь к файлу метрик;
- `AlgorithmTask` хранит имя алгоритма, указатель на `DirectedGraph` и `AlgorithmRunContext`;
- `Algorithm` задаёт общий интерфейс алгоритма: `name()` и `run(graph, params)`;
- `AlgorithmRunner` позволяет зарегистрировать алгоритм как функциональный объект;
- `AlgorithmRegistry` хранит зарегистрированные алгоритмы и запускает их через единый метод `run(task, params)`;
- `create_builtin_algorithm_registry()` создаёт реестр со встроенными `dfs`, `bfs`, `dijkstra`;
- `available_algorithms()` возвращает список доступных встроенных алгоритмов;
- `run(task, params)` запускает задачу через встроенный реестр;
- `run_algorithm(name, graph, params)` оставлен для совместимости со старым кодом.

Файл `algorithm_runtime.cpp` реализует `AlgorithmRegistry`. Метод `register_algorithm()` принимает либо объект `Algorithm`, либо пару `name + AlgorithmRunner`. Это оставляет место для будущих пользовательских алгоритмов: внешний код сможет создать свой реестр и добавить в него новые реализации без изменения встроенных DFS/BFS/Dijkstra.

При запуске runtime проверяет:

- что в `AlgorithmTask` есть граф;
- что имя алгоритма зарегистрировано;
- что выбранный алгоритм вернул успешный `AlgorithmResult`.

Если имя неизвестно, возвращается ошибка `UNKNOWN_ALGORITHM`. Если `AlgorithmTask.context.log_metrics = true`, runtime записывает результат или ошибку запуска через `logging_metrics`.

Роль файлов: runtime даёт единую точку регистрации, получения списка и запуска алгоритмов. JSON-адаптеру не нужно напрямую выбирать между `run_dfs()`, `run_bfs()` и `run_dijkstra()`.

### `include/petri/service_adapter/json_api.hpp` и `src/service_adapter/json_api.cpp`

Эти файлы реализуют внешний JSON API проекта.

Публичные функции:

- `JsonServiceAdapter::handle(request)` принимает объект JSON, выбирает режим и возвращает JSON-ответ;
- `JsonServiceAdapter::handle_json(request_json)` принимает строку JSON и возвращает строку JSON-ответа;
- `error_response(request_id, error)` создаёт JSON-ответ со статусом `error`;
- `simulate_request(request)` обрабатывает запрос с `mode = "simulate"`;
- `algorithm_request(request)` обрабатывает запрос с `mode = "algorithm"`;
- `handle_request(request)` — совместимый фасад, который вызывает `JsonServiceAdapter::handle()`;
- `handle_request_json(request_json)` — совместимый фасад, который вызывает `JsonServiceAdapter::handle_json()`.

Внутренние вспомогательные функции:

- `request_id_from()` достаёт `request_id`, если он есть;
- `net_counts_from_request()` и `net_counts_from_net()` получают размеры сети для записи метрик;
- `algorithm_from_request()` и `graph_mode_from_request()` достают значения для журнала метрик даже при ошибочных запросах;
- `log_metrics_record()` создаёт запись `RunMetricsRecord` и добавляет её в `logs/metrics.jsonl`;
- `event_to_json()` преобразует `SimulationEvent` в JSON;
- `algorithm_metrics_to_json()` преобразует `AlgorithmMetrics` в JSON;
- `params_from_request()` проверяет, что `params` отсутствует или является объектом;
- `read_size_t_field()`, `read_bool_field()`, `read_string_field()` и `read_optional_string_field()` проверяют типы параметров до запуска ядра;
- `validate_target_marking()` проверяет, что `target_marking` является непустым объектом, ссылается только на существующие места и содержит неотрицательные целые значения токенов;
- `path_to_json()` преобразует путь по вершинам графа достижимости в массив JSON с маркировками и переходами;
- `net_from_request()` проверяет наличие поля `net` и вызывает `PetriNet::from_json()`.

`simulate_request()` работает так:

1. Получает `request_id`.
2. Загружает сеть через `net_from_request()`.
3. Проверяет объект `params` и читает параметры `max_steps`, `strategy`, `stop_on_deadlock`, `return_events`, `transition_id`.
4. Создаёт `Interpreter`.
5. Вызывает `Interpreter::run()`.
6. Возвращает JSON с начальными и финальными маркировками, событиями и метриками.

`algorithm_request()` работает так:

1. Получает `request_id`.
2. Загружает сеть через `net_from_request()`.
3. Проверяет объект `params` и читает параметры `graph_mode`, `algorithm`, `max_states`, `max_depth`, `source`, `target`, `target_marking`.
4. Проверяет, что `graph_mode` равен `reachability_graph`. Другие режимы сейчас не поддержаны.
5. Если задан `target_marking`, проверяет id мест и типы значений.
6. Строит граф достижимости через `build_reachability_graph()`.
7. Определяет начальную вершину. Значение `source = "initial"` заменяется на `reachability.initial_vertex_id`.
8. Если задан `target_marking`, ищет целевую вершину через `find_marking_vertex()`.
9. Создаёт `AlgorithmTask` с контекстом метрик и запускает алгоритм через runtime-функцию `run(task, params)`.
10. Если алгоритм не нашёл путь к заданной цели, возвращает ошибку `TARGET_NOT_FOUND`.
11. Возвращает JSON с путём, метриками и количеством вершин и рёбер графа.

`JsonServiceAdapter::handle()` требует строковое поле `mode`. Значение `simulate` направляет запрос в `simulate_request()`, значение `algorithm` — в `algorithm_request()`. Остальные значения возвращают ошибку. Функция `handle_request()` оставлена как совместимый свободный фасад.

`JsonServiceAdapter::handle_json()` нужен для интеграции через строковый JSON: он парсит входную строку и возвращает красиво отформатированный JSON с отступом 2 пробела. Функция `handle_request_json()` оставлена как совместимый свободный фасад.

Все публичные входы JSON-адаптера перехватывают исключения парсинга и ошибок доступа к JSON и возвращают структурированный ответ `status = "error"`. Для некорректной сети возвращается код из слоя `PetriNet`, для неизвестного алгоритма — `UNKNOWN_ALGORITHM`, для превышения лимита графа достижимости — `STATE_LIMIT_EXCEEDED`, для недостижимой цели — `TARGET_NOT_FOUND`.

Роль файлов: это слой интеграции между внешним JSON-контрактом и внутренними C++-модулями.

### `src/cli/petri_cli.cpp`

Файл содержит функцию `main()`.

CLI принимает аргументы:

- первый аргумент — путь к JSON-файлу сети Петри;
- второй необязательный аргумент — режим `simulate`, `bfs`, `dfs` или `dijkstra`.

Если файл нельзя открыть или JSON некорректен, CLI пишет ошибку в `stderr` и возвращает код `2`.

Для режима `simulate` CLI формирует запрос:

- `mode = "simulate"`;
- `max_steps = 8`;
- `strategy = "round_robin"`;
- `stop_on_deadlock = true`;
- `return_events = true`.

Для режимов `bfs`, `dfs`, `dijkstra` CLI формирует запрос алгоритма:

- `mode = "algorithm"`;
- `graph_mode = "reachability_graph"`;
- `algorithm` равен выбранному режиму;
- `max_states = 100`;
- `max_depth = 10`;
- `source = "initial"`;
- `target_marking = { "p1_cs": 1 }`.

Затем CLI вызывает `petri::handle_request()` и печатает JSON-ответ.

Роль файла: это простой способ вручную проверить ядро без отдельного Python-слоя или HTTP-сервиса.

## 4. Поток данных в программе

Логический поток данных выглядит так:

```text
JSON сети Петри
  -> загрузка и валидация
  -> модель PetriNet
  -> интерпретатор или граф достижимости
  -> DFS/BFS/Dijkstra
  -> JSON-результат
```

В текущем коде есть два основных режима: `simulate` и `algorithm`. Они используют одну и ту же модель `PetriNet`, но идут разными ветками.

### Вход через JSON API

Внешняя точка входа находится в `src/service_adapter/json_api.cpp`.

Если запрос уже представлен как `nlohmann::json`, вызывается:

```cpp
petri::handle_request(request)
```

Если запрос приходит строкой, вызывается:

```cpp
petri::handle_request_json(request_json)
```

`handle_request_json()` парсит строку через `nlohmann::json::parse()` и передаёт объект в `handle_request()`.

`handle_request()` смотрит поле `mode`:

- `simulate` — вызывает `simulate_request()`;
- `algorithm` — вызывает `algorithm_request()`;
- другое значение — возвращает JSON-ошибку.

### Загрузка JSON сети Петри в `PetriNet`

Обе ветки сначала вызывают внутреннюю функцию `net_from_request()` из `src/service_adapter/json_api.cpp`.

`net_from_request()` проверяет, что в запросе есть поле `net`, и вызывает:

```cpp
PetriNet::from_json(request.at("net"))
```

Метод `PetriNet::from_json()` находится в `src/core_pn/petri_net.cpp`. Он читает:

- `name`;
- `places`;
- `transitions`;
- `arcs`.

После чтения и проверки данных создаётся объект `PetriNet`. В нём построены индексы мест и переходов, а также списки входных и выходных дуг для каждого перехода.

### Ветка симуляции: `PetriNet -> Interpreter -> JSON`

Если `mode = "simulate"`, функция `simulate_request()`:

1. Загружает `PetriNet`.
2. Читает параметры симуляции из `params`.
3. Создаёт объект:

```cpp
Interpreter interpreter(net);
```

4. Запускает:

```cpp
interpreter.run(params)
```

`Interpreter::run()` находится в `src/core_pn/interpreter.cpp`. Он многократно вызывает `Interpreter::step()`.

`Interpreter::step()` использует методы `PetriNet`:

- `enabled_transitions()` — чтобы узнать, какие переходы можно выполнить;
- `fire_transition()` — чтобы получить новую маркировку;
- `transition_fire_time()` — чтобы увеличить модельное время.

После завершения `simulate_request()` преобразует результат в JSON:

- `initial_marking`;
- `final_marking`;
- `events`;
- `metrics.elapsed_ms`;
- `metrics.steps_executed`;
- `metrics.deadlock`.

События шагов преобразуются через внутреннюю функцию `event_to_json()`.

### Ветка алгоритма: `PetriNet -> ReachabilityGraph -> DFS/BFS/Dijkstra -> JSON`

Если `mode = "algorithm"`, функция `algorithm_request()`:

1. Загружает `PetriNet`.
2. Проверяет `graph_mode`.
3. Создаёт `ReachabilityOptions`.
4. Вызывает:

```cpp
build_reachability_graph(net, options)
```

Построение графа достижимости реализовано в `src/graph_models/reachability.cpp`.

Важно: в текущем коде граф достижимости строится напрямую через методы `PetriNet`, а не через класс `Interpreter`. Для каждой маркировки вызываются:

- `PetriNet::enabled_transitions()`;
- `PetriNet::fire_transition()`;
- `PetriNet::transition_fire_time()`.

В результате получается `ReachabilityGraph`, внутри которого:

- `graph` — обычный `DirectedGraph`;
- вершины `m0`, `m1`, `m2` соответствуют маркировкам;
- рёбра `e0`, `e1`, `e2` соответствуют срабатываниям переходов;
- вес ребра равен `fire_time` перехода.

После построения графа функция `algorithm_request()` определяет начальную и целевую вершины:

- если `source = "initial"`, используется `reachability.initial_vertex_id`;
- если задан `target`, он используется как id вершины;
- если задан `target_marking`, вызывается `find_marking_vertex()`.

Затем создаётся `AlgorithmParams` и вызывается:

```cpp
run_algorithm(algorithm, reachability.graph, algorithm_params)
```

Функция `run_algorithm()` находится в `src/runtime/algorithm_runtime.cpp`. Она выбирает одну из функций:

- `run_bfs()` из `src/algorithms/algorithms.cpp`;
- `run_dfs()` из `src/algorithms/algorithms.cpp`;
- `run_dijkstra()` из `src/algorithms/algorithms.cpp`.

Результат алгоритма содержит путь по id вершин графа достижимости. JSON-адаптер преобразует этот путь через `path_to_json()`: для каждой вершины добавляется её маркировка, а для вершин после первой добавляется `via_transition`, то есть переход, по которому пришли в эту маркировку.

Финальный JSON-ответ алгоритма содержит:

- `request_id`;
- `status`;
- `algorithm`;
- `graph_mode`;
- `found`;
- `path`;
- `metrics`;
- `graph.vertices`;
- `graph.edges`.

### Вход через CLI

Файл `src/cli/petri_cli.cpp` даёт альтернативный вход. Он читает JSON сети из файла, сам формирует объект запроса и затем вызывает тот же `petri::handle_request()`, что и внешний JSON API.

Поэтому CLI не содержит отдельной логики моделирования. Он только подготавливает запрос и печатает ответ.

## 5. Сборка и тесты

Проект собирается через CMake на Windows. Команды нужно выполнять из корня репозитория `petri_net_simulator`.

### Генерация build-файлов

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

Эта команда создаёт или обновляет папку `build/`. В текущей конфигурации там появляются файлы Visual Studio, включая решение:

```text
build/petri_net_simulator.sln
```

### Сборка

```bash
cmake --build build
```

После сборки основные результаты находятся в:

```text
build/Debug/petri_core.lib
build/Debug/petri_cli.exe
build/Debug/petri_tests.exe
```

`petri_core.lib` — статическая библиотека с основным кодом.

`petri_cli.exe` — консольная программа для ручного запуска.

`petri_tests.exe` — исполняемый файл с тестами.

### Запуск тестов

```bash
ctest --test-dir build --output-on-failure
```

CTest запускает тест `petri_tests`, который зарегистрирован в `CMakeLists.txt`. Рабочая директория теста установлена на корень проекта, поэтому тесты могут читать `examples/mutex.json`.

Логи CTest появляются в:

```text
build/Testing/Temporary/
```

Например, подробный лог последнего запуска обычно находится в:

```text
build/Testing/Temporary/LastTest.log
```

### Какие тесты сейчас есть

`core_pn_tests.cpp` проверяет:

- загрузку `examples/mutex.json`;
- начальную маркировку;
- валидацию направления дуг;
- валидацию количества токенов;
- поиск разрешённых переходов;
- атомарное срабатывание перехода;
- запись событий интерпретатора;
- автоматический запуск симуляции.

`algorithms_tests.cpp` проверяет:

- BFS для поиска кратчайшего невзвешенного пути;
- DFS для достижения цели;
- Dijkstra для учёта весов рёбер;
- ошибку `UNKNOWN_ALGORITHM`.

`algorithm_selector_tests.cpp` проверяет:

- выбор BFS при приоритете минимальной длины пути;
- выбор Dijkstra при приоритете минимальной стоимости пути;
- формирование JSON-отчёта с таблицей сравнения;
- сохранение ошибочного кандидата в отчёте при неизвестном имени алгоритма.

`integration_tests.cpp` проверяет:

- построение графа достижимости для `mutex.json`;
- поиск вершины по маркировке;
- запуск BFS по графу достижимости;
- использование `fire_time` как веса рёбер для Dijkstra.

`service_adapter_tests.cpp` проверяет:

- JSON-ответ симуляции;
- JSON-ответ алгоритма;
- JSON-ответ Dijkstra с метрикой стоимости пути;
- ошибку некорректных параметров симуляции;
- ошибку некорректной сети;
- ошибку неизвестного алгоритма;
- ошибку недостижимой цели;
- ошибку превышения лимита состояний;
- строковый вход `handle_request_json()` для некорректного JSON.

## 6. Что ещё не реализовано в текущем коде

Эти части могут быть запланированы в общем проекте, но в текущем состоянии репозитория их исходного кода нет:

- Python-биндинги через `pybind11`.
- HTTP-сервис или отдельный серверный адаптер.
- Отдельные представления FSF и BSF для графов.
- Режим `structural_graph` в JSON API. Сейчас `algorithm_request()` принимает только `reachability_graph`.
- Ингибиторные, цветные и стохастические сети Петри.
- Хранение запусков или результатов в базе данных.

Это важно учитывать при использовании документации: описанные выше классы и функции уже реализованы, а перечисленные в этом разделе возможности пока отсутствуют.
