import { preview } from "vite";
import { spawn } from "node:child_process";
import { fileURLToPath } from "node:url";
import { setTimeout as delay } from "node:timers/promises";

const root = fileURLToPath(new URL("../", import.meta.url));
const url = "http://127.0.0.1:5178/";
const cancellation = new AbortController();
let server;
let child;
let forceStop;
let interrupted = false;
let exitCode = 0;
function interrupt() {
  interrupted = true;
  process.exitCode = 1;
  cancellation.abort();
  child?.kill("SIGTERM");
  if (child && !forceStop) {
    forceStop = setTimeout(() => child.kill("SIGKILL"), 5000);
  }
}
process.on("SIGINT", interrupt);
process.on("SIGTERM", interrupt);
const deadline = setTimeout(interrupt, 120_000);
try {
  server = await preview({
    configFile: fileURLToPath(new URL("../vite.embedded.config.ts", import.meta.url)),
    preview: { host: "127.0.0.1", port: 5178, strictPort: true, open: false },
  });
  const readyDeadline = Date.now() + 10_000;
  while (true) {
    cancellation.signal.throwIfAborted();
    try {
      const response = await fetch(url, {
        signal: AbortSignal.any([cancellation.signal, AbortSignal.timeout(1000)]),
      });
      await response.arrayBuffer();
      if (response.ok) break;
    } catch (error) {
      if (cancellation.signal.aborted) throw error;
    }
    if (Date.now() >= readyDeadline) throw new Error("Embedded preview readiness timed out");
    await delay(100, undefined, { signal: cancellation.signal });
  }
  cancellation.signal.throwIfAborted();
  child = spawn(process.execPath, ["tests/browser.mjs"], {
    cwd: root,
    stdio: "inherit",
    shell: false,
  });
  const code = await new Promise((resolve, reject) => {
    child.once("error", reject);
    child.once("exit", (code) => resolve(code ?? 1));
  });
  if (interrupted) throw new Error("Browser test interrupted or exceeded 120 seconds");
  if (code !== 0) throw new Error(`Browser test exited with code ${code}`);
} catch (error) {
  console.error(error.message);
  exitCode = 1;
  process.exitCode = exitCode;
} finally {
  clearTimeout(deadline);
  clearTimeout(forceStop);
  if (server) {
    server.httpServer.closeAllConnections();
    await server.close();
  }
  process.off("SIGINT", interrupt);
  process.off("SIGTERM", interrupt);
}
process.exitCode = exitCode;
