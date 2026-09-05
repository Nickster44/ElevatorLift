import react from "@vitejs/plugin-react";
import { defineConfig } from "vite";
import { resolve } from "node:path";
import { fileURLToPath } from "node:url";

const projectRoot = fileURLToPath(new URL(".", import.meta.url));

export default defineConfig({
  root: resolve(projectRoot, "embedded"),
  base: "/",
  plugins: [react()],
  build: {
    outDir: resolve(projectRoot, "dist/embedded"),
    emptyOutDir: true,
    minify: true,
    sourcemap: false,
    assetsInlineLimit: 2048,
    rollupOptions: {
      output: {
        entryFileNames: "assets/app-[hash].js",
        assetFileNames: "assets/[name]-[hash][extname]",
      },
    },
  },
});
