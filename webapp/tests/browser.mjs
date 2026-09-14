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
      stops = 0,
      stopAccepted = false;
    const errors = [];
    page.on("pageerror", (e) => errors.push(e.message));
    await page.route("**/api/**", async (route) => {
      const path = new URL(route.request().url()).pathname;
      if (path === "/api/stop") {
        ++stops;
        if (stopAccepted) {
          await route.fulfill({
            status: 202,
            contentType: "application/json",
            body: JSON.stringify({
              ...fixture,
              sequence: ++fixture.sequence,
              vfd: {
                ...fixture.vfd,
                stopTransmitted: true,
                stopAcknowledged: true,
              },
            }),
          });
          return;
        }
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
            available: true,
            readings: [
              { number: 4, supported: true, rawValue: 10, ageMs: 0 },
              { number: 13, supported: false, rawValue: null, ageMs: null },
            ],
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
            : {
                ...fixture,
                sequence: ++fixture.sequence,
                vfd: {
                  ...fixture.vfd,
                  communicationFault: false,
                  stopRefreshMs: 300,
                  replyTimeoutMs: 150,
                  watchdog: { state: "disabled", timeoutMs: 0, ageMs: 0 },
                },
              },
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
    stopAccepted = true;
    await page.getByText("Manual inputs: No direction requested").waitFor();
    await page.getByRole("button", { name: "Controlled lift stop" }).click();
    await page
      .getByRole("alert")
      .filter({ hasText: "physical stopping is unconfirmed" })
      .waitFor();
    assert.equal(stops, 2);
    assert.equal(
      await page.getByRole("button", { name: "Call floor 1" }).isEnabled(),
      false,
    );
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
        await page.getByText("Drive watchdog: 0 ms (disabled)").waitFor();
        await page
          .getByRole("button", { name: "Read parameter catalog" })
          .click();
        await page.getByText("04 Deceleration ramp").waitFor();
        await page.getByRole("cell", { name: "1 s", exact: true }).waitFor();
        await page
          .getByRole("cell", { name: "Unsupported", exact: true })
          .waitFor();
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
