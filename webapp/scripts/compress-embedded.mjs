import { gzipSync } from "node:zlib";
import { readdir, readFile, stat, writeFile } from "node:fs/promises";
import { join, relative } from "node:path";
import { fileURLToPath } from "node:url";

const root = fileURLToPath(new URL("../dist/embedded/", import.meta.url));
const compressible = new Set([".html", ".css", ".js", ".json"]);
let rawTotal = 0;
let gzipTotal = 0;

async function visit(directory) {
  for (const name of await readdir(directory)) {
    if (name.endsWith(".gz")) continue;
    const path = join(directory, name);
    const info = await stat(path);
    if (info.isDirectory()) {
      await visit(path);
      continue;
    }
    const extension = name.slice(name.lastIndexOf("."));
    if (!compressible.has(extension)) continue;
    const contents = await readFile(path);
    const compressed = gzipSync(contents, { level: 9 });
    await writeFile(`${path}.gz`, compressed);
    rawTotal += contents.length;
    gzipTotal += compressed.length;
    console.log(`${relative(root, path)}: ${contents.length} B -> ${compressed.length} B gzip`);
  }
}

await visit(root);
console.log(`Embedded web assets: ${rawTotal} B raw, ${gzipTotal} B gzip`);
