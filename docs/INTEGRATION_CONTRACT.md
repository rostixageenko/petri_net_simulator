# Integration Contract For Client-Server Layer

This document describes the JSON contract exposed by the C++ core through the library adapter. It does not define an HTTP server. A client-server layer can pass the same JSON objects to `petri::handle_request()` or `petri::handle_request_json()` and return the resulting JSON to the UI.

## Entry Points

- C++ object API: `petri::handle_request(const nlohmann::json& request)`.
- C++ string API: `petri::handle_request_json(const std::string& request_json)`.
- Python binding API: `petri_core.run_simulation(net, params)` and `petri_core.run_algorithm(net, params)` use the same request and response fields.

Every top-level request is a JSON object with:

- `request_id`: optional string copied into the response.
- `mode`: required string, either `simulate` or `algorithm`.
- `net`: required Petri net object.
- `params`: optional object with mode-specific parameters.

## Petri Net JSON

```json
{
  "name": "demo_net",
  "places": [
    {"id": "p_start", "label": "Start", "tokens": 1},
    {"id": "p_done", "label": "Done", "tokens": 0}
  ],
  "transitions": [
    {"id": "t_finish", "label": "Finish", "fire_time": 0.5}
  ],
  "arcs": [
    {"id": "a1", "source": "p_start", "target": "t_finish", "weight": 1},
    {"id": "a2", "source": "t_finish", "target": "p_done", "weight": 1}
  ]
}
```

Rules:

- Place, transition and arc ids must be unique inside their own groups.
- Arc endpoints must reference an existing place or transition.
- An arc must connect a place to a transition or a transition to a place.
- `tokens` must be a non-negative integer.
- `weight` must be a positive integer.
- `fire_time` must be a non-negative number. If omitted, the core uses `1.0`.

## Simulation Request

```json
{
  "request_id": "sim-001",
  "mode": "simulate",
  "net": {
    "name": "demo_net",
    "places": [
      {"id": "p_start", "label": "Start", "tokens": 1},
      {"id": "p_done", "label": "Done", "tokens": 0}
    ],
    "transitions": [
      {"id": "t_finish", "label": "Finish", "fire_time": 0.5}
    ],
    "arcs": [
      {"id": "a1", "source": "p_start", "target": "t_finish", "weight": 1},
      {"id": "a2", "source": "t_finish", "target": "p_done", "weight": 1}
    ]
  },
  "params": {
    "max_steps": 1,
    "strategy": "first_enabled",
    "stop_on_deadlock": true,
    "return_events": true
  }
}
```

Simulation params:

- `max_steps`: non-negative integer, default `1`.
- `strategy`: `first_enabled`, `by_id`, or `round_robin`, default `first_enabled`.
- `transition_id`: string used by `strategy = "by_id"`.
- `stop_on_deadlock`: boolean, default `true`.
- `return_events`: boolean, default `true`.

## Simulation Response

```json
{
  "request_id": "sim-001",
  "status": "ok",
  "model_name": "demo_net",
  "initial_marking": {
    "p_start": 1,
    "p_done": 0
  },
  "final_marking": {
    "p_start": 0,
    "p_done": 1
  },
  "events": [
    {
      "step": 1,
      "time": 0.5,
      "fired_transition": "t_finish",
      "marking_before": {
        "p_start": 1,
        "p_done": 0
      },
      "marking_after": {
        "p_start": 0,
        "p_done": 1
      },
      "enabled_before": ["t_finish"],
      "enabled_after": []
    }
  ],
  "metrics": {
    "elapsed_ms": 0.05,
    "steps_executed": 1,
    "deadlock": false
  }
}
```

## BFS Request

```json
{
  "request_id": "bfs-001",
  "mode": "algorithm",
  "net": {
    "name": "demo_net",
    "places": [
      {"id": "p_start", "label": "Start", "tokens": 1},
      {"id": "p_done", "label": "Done", "tokens": 0}
    ],
    "transitions": [
      {"id": "t_finish", "label": "Finish", "fire_time": 0.5}
    ],
    "arcs": [
      {"id": "a1", "source": "p_start", "target": "t_finish", "weight": 1},
      {"id": "a2", "source": "t_finish", "target": "p_done", "weight": 1}
    ]
  },
  "params": {
    "graph_mode": "reachability_graph",
    "algorithm": "bfs",
    "max_states": 20,
    "max_depth": 4,
    "source": "initial",
    "target_marking": {
      "p_done": 1
    }
  }
}
```

## Dijkstra Request

```json
{
  "request_id": "dijkstra-001",
  "mode": "algorithm",
  "net": {
    "name": "demo_net",
    "places": [
      {"id": "p_start", "label": "Start", "tokens": 1},
      {"id": "p_done", "label": "Done", "tokens": 0}
    ],
    "transitions": [
      {"id": "t_finish", "label": "Finish", "fire_time": 0.5}
    ],
    "arcs": [
      {"id": "a1", "source": "p_start", "target": "t_finish", "weight": 1},
      {"id": "a2", "source": "t_finish", "target": "p_done", "weight": 1}
    ]
  },
  "params": {
    "graph_mode": "reachability_graph",
    "algorithm": "dijkstra",
    "max_states": 20,
    "max_depth": 4,
    "source": "initial",
    "target_marking": {
      "p_done": 1
    }
  }
}
```

Algorithm params:

- `graph_mode`: currently only `reachability_graph`.
- `algorithm`: `dfs`, `bfs`, or `dijkstra`. Default is `bfs`.
- `max_states`: maximum number of reachability states. Default is `1000`.
- `max_depth`: maximum reachability graph depth. Default is `20`.
- `source`: `initial` or a reachability vertex id such as `m0`.
- `target`: optional reachability vertex id.
- `target_marking`: optional object `{place_id: token_count}`. Used when `target` is absent.

## Algorithm Response With Route, Metrics And States

```json
{
  "request_id": "dijkstra-001",
  "status": "ok",
  "algorithm": "dijkstra",
  "graph_mode": "reachability_graph",
  "found": true,
  "path": [
    {
      "vertex_id": "m0",
      "marking": {
        "p_start": 1,
        "p_done": 0
      }
    },
    {
      "vertex_id": "m1",
      "via_transition": "t_finish",
      "marking": {
        "p_start": 0,
        "p_done": 1
      }
    }
  ],
  "metrics": {
    "elapsed_ms": 0.08,
    "visited_vertices": 2,
    "scanned_edges": 1,
    "path_length": 1,
    "path_cost": 0.5
  },
  "graph": {
    "vertices": 2,
    "edges": 1
  }
}
```

Notes for visualization:

- `path` is ordered from source to target.
- Each path item has `vertex_id` and `marking`.
- Each path item after the first also has `via_transition`, the transition fired to reach that state.
- `metrics.path_length` is the number of graph edges in the route.
- `metrics.path_cost` is the sum of edge weights. For reachability graphs, each edge weight is the transition `fire_time`.
- `graph.vertices` and `graph.edges` describe the bounded reachability graph actually built for this request.

## Error Response

```json
{
  "request_id": "bfs-001",
  "status": "error",
  "error": {
    "code": "TARGET_NOT_FOUND",
    "message": "Target is not reachable from source",
    "details": {
      "source": "m1",
      "target": "m0",
      "algorithm": "bfs"
    }
  }
}
```

Important error codes:

- `INVALID_JSON`: malformed request or wrong field type.
- `INVALID_ARC_ENDPOINT`: arc references an unknown place or transition.
- `INVALID_ARC_DIRECTION`: arc connects place to place or transition to transition.
- `INVALID_ARC_WEIGHT`: arc weight is not a positive integer.
- `INVALID_TOKEN_COUNT`: place token count is negative.
- `UNKNOWN_ALGORITHM`: algorithm is not registered.
- `TARGET_NOT_FOUND`: requested target marking or vertex is not reachable.
- `STATE_LIMIT_EXCEEDED`: bounded reachability construction exceeded `max_states`.

## Adapter Boundary

The core does not open sockets and does not own HTTP concerns. The client-server layer should:

1. Receive HTTP or UI payload.
2. Build one of the JSON request objects described above.
3. Call `handle_request()` or `handle_request_json()`.
4. Return the JSON response unchanged, or wrap it in the server framework's envelope if needed.
