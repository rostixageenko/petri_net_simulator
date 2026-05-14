# API_CONTRACT.md — JSON-контракты интеграции

## 1. Входная модель сети Петри

```json
{
  "name": "Mutex: two processes contending for a shared resource",
  "places": [
    {"id": "p1_idle", "label": "P1 idle", "tokens": 1}
  ],
  "transitions": [
    {"id": "t1_request", "label": "P1 request", "fire_time": 0.2}
  ],
  "arcs": [
    {"id": "a1", "source": "p1_idle", "target": "t1_request", "weight": 1},
    {"id": "a2", "source": "t1_request", "target": "p1_wait", "weight": 1}
  ]
}
```

### Правила

- `id` должен быть уникальным внутри своей категории.
- `source` и `target` должны ссылаться на существующее место или переход.
- Дуга должна соединять место и переход.
- `weight` — положительное целое число.
- `tokens` — неотрицательное целое число.
- `fire_time` — неотрицательное число. Если отсутствует, считать `1.0`.

## 2. Запрос симуляции

```json
{
  "request_id": "run-001",
  "mode": "simulate",
  "net": {"...": "Petri net JSON"},
  "params": {
    "max_steps": 20,
    "strategy": "first_enabled",
    "stop_on_deadlock": true,
    "return_events": true
  }
}
```

### Допустимые стратегии первой версии

- `first_enabled` — выбирать первый разрешённый переход по внутреннему порядку.
- `by_id` — использовать `transition_id` из параметров одного шага.
- `round_robin` — выбирать следующий разрешённый переход циклически.

### Поля `params`

- `max_steps` — неотрицательное целое число. Если поле отсутствует, выполняется один шаг.
- `strategy` — строка: `first_enabled`, `by_id` или `round_robin`. Если поле отсутствует, используется `first_enabled`.
- `transition_id` — строка, обязательна для стратегии `by_id`.
- `stop_on_deadlock` — boolean. Если `true`, симуляция останавливается при тупике.
- `return_events` — boolean. Если `false`, ответ содержит пустой массив `events`, но финальная маркировка и метрики всё равно возвращаются.

## 3. Ответ симуляции

```json
{
  "request_id": "run-001",
  "status": "ok",
  "model_name": "Mutex: two processes contending for a shared resource",
  "initial_marking": {
    "p1_idle": 1,
    "p1_wait": 0
  },
  "final_marking": {
    "p1_idle": 0,
    "p1_wait": 1
  },
  "events": [
    {
      "step": 1,
      "time": 0.2,
      "fired_transition": "t1_request",
      "marking_before": {"p1_idle": 1, "p1_wait": 0},
      "marking_after": {"p1_idle": 0, "p1_wait": 1},
      "enabled_before": ["t1_request"],
      "enabled_after": ["t1_enter"]
    }
  ],
  "metrics": {
    "elapsed_ms": 0.14,
    "steps_executed": 1,
    "deadlock": false
  }
}
```

## 4. Запрос алгоритма

```json
{
  "request_id": "alg-001",
  "mode": "algorithm",
  "net": {"...": "Petri net JSON"},
  "params": {
    "graph_mode": "reachability_graph",
    "algorithm": "bfs",
    "max_states": 1000,
    "max_depth": 20,
    "source": "initial",
    "target_marking": {
      "p1_cs": 1
    }
  }
}
```

### `graph_mode`

- `reachability_graph` — вершины являются маркировками, рёбра — срабатываниями переходов.
- `structural_graph` — зарезервирован контрактом, но в текущем библиотечном адаптере ещё не поддержан. Запрос с таким значением возвращает `INVALID_JSON`.

### Поля `params`

- `algorithm` — строка: `dfs`, `bfs` или `dijkstra`. Если поле отсутствует, используется `bfs`.
- `max_states` — неотрицательное целое число, ограничение числа вершин графа достижимости.
- `max_depth` — неотрицательное целое число, ограничение глубины построения графа достижимости.
- `source` — строка. Значение `initial` означает начальную маркировку, иначе ожидается id вершины графа достижимости.
- `target` — строка с id вершины графа достижимости.
- `target_marking` — объект `{place_id: tokens}`. Все `place_id` должны существовать в сети, а значения должны быть неотрицательными целыми числами. Поле используется, если `target` не задан.

## 5. Ответ алгоритма

```json
{
  "request_id": "alg-001",
  "status": "ok",
  "algorithm": "bfs",
  "graph_mode": "reachability_graph",
  "found": true,
  "path": [
    {
      "vertex_id": "m0",
      "marking": {"p1_idle": 1, "p1_wait": 0}
    },
    {
      "vertex_id": "m1",
      "via_transition": "t1_request",
      "marking": {"p1_idle": 0, "p1_wait": 1}
    }
  ],
  "metrics": {
    "elapsed_ms": 0.51,
    "visited_vertices": 12,
    "scanned_edges": 18,
    "path_length": 1,
    "path_cost": 0.2
  },
  "graph": {
    "vertices": 12,
    "edges": 18
  }
}
```

## 6. Ошибка

```json
{
  "request_id": "alg-001",
  "status": "error",
  "error": {
    "code": "INVALID_ARC_ENDPOINT",
    "message": "Arc a12 references unknown node mutex2",
    "details": {
      "arc_id": "a12",
      "field": "source"
    }
  }
}
```

## 7. Коды ошибок первой версии

- `INVALID_JSON`
- `DUPLICATE_ID`
- `UNKNOWN_NODE_ID`
- `INVALID_ARC_ENDPOINT`
- `INVALID_ARC_DIRECTION`
- `INVALID_ARC_WEIGHT`
- `INVALID_TOKEN_COUNT`
- `UNKNOWN_TRANSITION_ID`
- `TRANSITION_NOT_ENABLED`
- `UNKNOWN_ALGORITHM`
- `NEGATIVE_EDGE_WEIGHT`
- `STATE_LIMIT_EXCEEDED`
- `TARGET_NOT_FOUND`

### Типовые причины ошибок

- Некорректная сеть возвращает код из валидации модели: например `INVALID_ARC_WEIGHT`, `UNKNOWN_NODE_ID`, `INVALID_ARC_DIRECTION`, `INVALID_TOKEN_COUNT`.
- Неизвестный алгоритм возвращает `UNKNOWN_ALGORITHM`.
- Недостижимая цель в алгоритмическом запросе возвращает `TARGET_NOT_FOUND`.
- Превышение `max_states` при построении графа достижимости возвращает `STATE_LIMIT_EXCEEDED`.
- Некорректные типы полей запроса, например строка вместо `max_steps`, возвращают `INVALID_JSON` с уточнением в `details.field`.
