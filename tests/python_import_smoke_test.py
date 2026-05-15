import pathlib
import sys


if len(sys.argv) != 2:
    raise SystemExit("usage: python_import_smoke_test.py <module-directory>")

module_dir = pathlib.Path(sys.argv[1]).resolve()
sys.path.insert(0, str(module_dir))

import petri_core  # noqa: E402


required_functions = [
    "load_petri_net_from_json",
    "get_enabled_transitions",
    "fire_transition",
    "run_simulation",
    "run_algorithm",
    "select_algorithm",
]

missing = [name for name in required_functions if not hasattr(petri_core, name)]
if missing:
    raise AssertionError(f"petri_core is missing bindings: {missing}")

print(f"petri_core import smoke test passed from {module_dir}")
