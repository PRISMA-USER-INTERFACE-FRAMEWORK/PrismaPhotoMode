import runpy
from pathlib import Path

tests = Path(__file__).resolve().parent
runpy.run_path(str(tests / "static_checks.py"), run_name="__main__")
runpy.run_path(str(tests / "runtime_policy_tests.py"), run_name="__main__")
runpy.run_path(str(tests / "test_mcm_config.py"), run_name="__main__")
