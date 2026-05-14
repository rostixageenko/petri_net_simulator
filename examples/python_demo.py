import petri_core


net = {
    "name": "python_demo",
    "places": [
        {"id": "p_start", "label": "Start", "tokens": 1},
        {"id": "p_done", "label": "Done", "tokens": 0},
    ],
    "transitions": [
        {"id": "t_finish", "label": "Finish", "fire_time": 0.25},
    ],
    "arcs": [
        {"id": "a1", "source": "p_start", "target": "t_finish", "weight": 1},
        {"id": "a2", "source": "t_finish", "target": "p_done", "weight": 1},
    ],
}


print("Loaded:", petri_core.load_petri_net_from_json(net))
print("Enabled:", petri_core.get_enabled_transitions(net))
print("After fire:", petri_core.fire_transition(net, "t_finish"))

simulation = petri_core.run_simulation(net, {"max_steps": 1})
print("Simulation:", simulation)

algorithm = petri_core.run_algorithm(
    net,
    {
        "algorithm": "bfs",
        "target_marking": {"p_done": 1},
        "max_depth": 4,
    },
)
print("Algorithm:", algorithm)

selection = petri_core.select_algorithm(
    net,
    {
        "algorithms": ["bfs", "dijkstra"],
        "weights": {"path_cost": 1.0},
        "target_marking": {"p_done": 1},
        "max_depth": 4,
    },
)
print("Selection:", selection)
