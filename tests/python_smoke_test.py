import pathlib
import sys


if len(sys.argv) != 2:
    raise SystemExit("usage: python_smoke_test.py <module-directory>")

sys.path.insert(0, sys.argv[1])

import petri_core  # noqa: E402


NET = {
    "name": "python_smoke",
    "places": [
        {"id": "p1", "label": "P1", "tokens": 1},
        {"id": "p2", "label": "P2", "tokens": 0},
    ],
    "transitions": [
        {"id": "t1", "label": "T1", "fire_time": 0.5},
    ],
    "arcs": [
        {"id": "a1", "source": "p1", "target": "t1", "weight": 1},
        {"id": "a2", "source": "t1", "target": "p2", "weight": 1},
    ],
}


loaded = petri_core.load_petri_net_from_json(NET)
assert loaded["status"] == "ok"
assert loaded["initial_marking"] == {"p1": 1, "p2": 0}

enabled = petri_core.get_enabled_transitions(NET)
assert enabled == ["t1"]

next_marking = petri_core.fire_transition(NET, "t1")
assert next_marking == {"p1": 0, "p2": 1}

simulation = petri_core.run_simulation(NET, {"max_steps": 1})
assert simulation["status"] == "ok"
assert simulation["final_marking"] == {"p1": 0, "p2": 1}

algorithm = petri_core.run_algorithm(
    NET,
    {
        "algorithm": "bfs",
        "max_depth": 4,
        "target_marking": {"p2": 1},
    },
)
assert algorithm["status"] == "ok"
assert algorithm["found"] is True

selection = petri_core.select_algorithm(
    NET,
    {
        "algorithms": ["bfs", "dijkstra"],
        "weights": {"path_cost": 1.0},
        "max_depth": 4,
        "target_marking": {"p2": 1},
    },
)
assert selection["best_algorithm"] in {"bfs", "dijkstra"}
assert len(selection["comparison"]) == 2

print(f"petri_core smoke test passed from {pathlib.Path(sys.argv[1]).resolve()}")
