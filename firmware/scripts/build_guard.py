Import("env")
import subprocess

subprocess.run(["node", "scripts/generate-contract.mjs", "--check"], check=True)

def deny_upload(*args, **kwargs):
    raise RuntimeError("Deployment blocked: HW-01/HW-02 and physical qualification unresolved. Build only; do not upload.")

for target in ("upload", "uploadfs", "uploadfsota"):
    env.AddPreAction(target, deny_upload)
