import {
  readdir,
  readFile,
  mkdir,
  copyFile,
  writeFile,
  unlink,
} from "node:fs/promises";
import { createHash } from "node:crypto";
import { join, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";
const source = fileURLToPath(new URL("../dist/embedded/", import.meta.url));
const target = fileURLToPath(new URL("../../firmware/data/", import.meta.url));
const files = [];
let previous = [];
try {
  previous = JSON.parse(
    await readFile(join(target, "bundle-manifest.json"), "utf8"),
  ).files;
} catch (error) {
  if (error.code !== "ENOENT") throw error;
}
async function visit(dir, relative = "") {
  for (const entry of await readdir(dir, { withFileTypes: true })) {
    const name = join(relative, entry.name);
    if (entry.isDirectory()) await visit(join(dir, entry.name), name);
    else {
      const bytes = await readFile(join(source, name));
      files.push({
        path: name.replaceAll("\\", "/"),
        bytes: bytes.length,
        sha256: createHash("sha256").update(bytes).digest("hex"),
      });
    }
  }
}
await visit(source);
if (
  !files.some((f) => f.path === "index.html") ||
  files.reduce((n, f) => n + f.bytes, 0) > 2 * 1024 * 1024
)
  throw Error("Invalid/oversized embedded bundle");
for (const f of files) {
  const dest = join(target, f.path);
  await mkdir(join(dest, ".."), { recursive: true });
  await copyFile(join(source, f.path), dest);
}
await writeFile(
  join(target, "bundle-manifest.json"),
  JSON.stringify({ apiVersion: 1, contractVersion: 1, files }, null, 2),
);
for (const old of previous) {
  const path = resolve(target, old.path);
  if (!path.startsWith(resolve(target) + sep))
    throw Error("Unsafe prior manifest path");
  if (!files.some((f) => f.path === old.path))
    await unlink(path).catch((error) => {
      if (error.code !== "ENOENT") throw error;
    });
}
console.log(
  "Staged " +
    files.length +
    " assets. Build filesystem only; uploads remain blocked.",
);
