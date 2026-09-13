"""Exercise upload denial without running PlatformIO or touching a serial port."""
import os
import runpy
from pathlib import Path


class FakeEnvironment:
    def __init__(self):
        self.actions = {}

    def AddPreAction(self, target, callback):
        self.actions[target] = callback


root = Path(__file__).resolve().parents[1]
os.chdir(root)
env = FakeEnvironment()
runpy.run_path(str(root / "scripts/build_guard.py"), init_globals={"env": env, "Import": lambda _: None})
assert set(env.actions) == {"upload", "uploadfs", "uploadfsota"}
for target, callback in env.actions.items():
    try:
        callback()
    except RuntimeError as error:
        assert "Deployment blocked" in str(error)
    else:
        raise AssertionError(f"{target} was not denied")
print("PASS upload guards: fake build environment only, no device access")
