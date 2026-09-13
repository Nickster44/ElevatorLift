import fs from "node:fs";
import assert from "node:assert/strict";
import { chromium } from "playwright";
const fixture = JSON.parse(
  fs.readFileSync(new URL("./fixtures/status.json", import.meta.url), "utf8"),
);
const browser = await chromium.launch({
  headless: true,
});
async function interrupt() {
  await browser.close();
  process.exit(1);
}
process.once("SIGINT", interrupt);
process.once("SIGTERM", interrupt);
const output = new URL("../test-results/", import.meta.url);
fs.mkdirSync(output, { recursive: true });
try {
  for (const viewport of [
    { width: 1440, height: 1000 },
    { width: 390, height: 844 },
  ]) {
    const page = await browser.newPage({ viewport });
    let mode = "offline",
      stops = 0;
    const errors = [];
    page.on("pageerror", (e) => errors.push(e.message));
    await page.route("**/api/**", async (route) => {
      const path = new URL(route.request().url()).pathname;
      if (path === "/api/stop") {
        ++stops;
        await route.fulfill({
          status: 503,
          contentType: "application/json",
          body: JSON.stringify({
            error: "stop_delivery_unavailable_hardware_inhibited",
          }),
        });
        return;
      }
      if (mode === "offline") {
        await route.abort();
        return;
      }
      if (path === "/api/vfd/parameters") {
        await route.fulfill({
          contentType: "application/json",
          body: JSON.stringify({
            apiVersion: 1,
            available: false,
            definitions: [
              {
                number: 4,
                name: "DEC",
                displayName: "Deceleration ramp",
                units: "s",
                min: 1,
                max: 599,
                scaleDivisor: 10,
                writable: true,
                access: "installer",
              },
              {
                number: 13,
                name: "RELE",
                displayName: "Expansion relay / input (readback unresolved)",
                units: "raw",
                min: 0,
                max: 1,
                scaleDivisor: 1,
                writable: false,
                access: "locked",
              },
            ],
          }),
        });
        return;
      }
      await route.fulfill({
        contentType: "application/json",
        body: JSON.stringify(
          mode === "malformed"
            ? {}
            : { ...fixture, sequence: ++fixture.sequence },
        ),
      });
    });
    await page.goto("http://127.0.0.1:5178/");
    await page
      .getByRole("status")
      .filter({ hasText: "Disconnected" })
      .waitFor();
    assert.equal(
      await page.getByRole("button", { name: "Call floor 1" }).isEnabled(),
      false,
    );
    await page.getByRole("button", { name: "Controlled lift stop" }).click();
    await page
      .getByRole("alert")
      .filter({ hasText: "stop_delivery_unavailable" })
      .waitFor();
    assert.equal(stops, 1);
    mode = "online";
    await page
      .getByRole("status")
      .filter({ hasText: "Controller connected" })
      .waitFor();
    assert.equal(
      (await page.getByText("Unknown", { exact: true }).count()) > 0,
      true,
    );
    mode = "malformed";
    await page
      .getByRole("status")
      .filter({ hasText: "invalid_controller_status" })
      .waitFor();
    assert.equal(
      await page.getByRole("button", { name: "Call floor 3" }).isEnabled(),
      false,
    );
    mode = "online";
    await page
      .getByRole("status")
      .filter({ hasText: "Controller connected" })
      .waitFor();
    await page.screenshot({
      path: new URL(`console-${viewport.width}.png`, output).pathname.replace(
        /^\/([A-Za-z]:)/,
        "$1",
      ),
      fullPage: true,
    });
    assert.equal(
      await page.evaluate(
        () => document.documentElement.scrollWidth > innerWidth,
      ),
      false,
    );
    for (const view of ["Drive", "Setup", "Remotes", "Logs", "System"]) {
      await page.getByRole("button", { name: view, exact: true }).click();
      if (view === "Drive") {
        await page
          .getByRole("button", { name: "Read parameter catalog" })
          .click();
        await page.getByText("04 Deceleration ramp").waitFor();
      }
      assert.equal(
        await page.evaluate(
          () => document.documentElement.scrollWidth > innerWidth,
        ),
        false,
      );
    }
    assert.deepEqual(errors, []);
    await page.close();
    console.log(
      `PASS browser ${viewport.width}: disconnect, STOP transport, malformed status, reconnect, six views, overflow`,
    );
  }
} finally {
  await browser.close();
  process.off("SIGINT", interrupt);
  process.off("SIGTERM", interrupt);
}
