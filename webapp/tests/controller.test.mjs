import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import {
  validateStatus,
  isFresh,
  jsonRequest,
  StatusTracker,
  validateParameterCatalog,
} from "../lib/controller.ts";
const fixture = () =>
  JSON.parse(
    fs.readFileSync(new URL("./fixtures/status.json", import.meta.url), "utf8"),
  );
test("parameter metadata is validated separately from live values", () => {
  const p = {
    number: 4,
    name: "DEC",
    displayName: "Deceleration ramp",
    units: "s",
    min: 1,
    max: 599,
    scaleDivisor: 10,
    writable: true,
    access: "installer",
  };
  assert.equal(
    validateParameterCatalog({ apiVersion: 1, definitions: [p] })[0].number,
    4,
  );
  assert.throws(() =>
    validateParameterCatalog({ apiVersion: 1, definitions: [p, p] }),
  );
  assert.throws(() =>
    validateParameterCatalog({
      apiVersion: 1,
      definitions: [{ ...p, scaleDivisor: 0 }],
    }),
  );
});
test("inhibited contract is accepted without manufactured position or telemetry", () => {
  const s = validateStatus(fixture());
  assert.equal(s.currentFloor, null);
  assert.equal(s.motionAllowed, false);
  assert.equal(s.telemetry, null);
});
test("malformed, incompatible and contradictory responses are rejected", () => {
  for (const value of [
    {},
    null,
    { apiVersion: 1 },
    { ...fixture(), apiVersion: 2 },
    { ...fixture(), motionAllowed: true },
    { ...fixture(), currentFloor: 2 },
    { ...fixture(), position: "9223372036854775808" },
    { ...fixture(), telemetry: { frequency: 0 } },
  ])
    assert.throws(() => validateStatus(value));
});
test("status loss cannot create readiness; old samples expire", () => {
  assert.equal(isFresh(null, 0, 0), false);
  assert.equal(isFresh(validateStatus(fixture()), 1000, 3500), false);
  assert.equal(
    isFresh(validateStatus({ ...fixture(), sampleAgeMs: 101 }), 1000, 1100),
    false,
  );
});
test("duplicate snapshots are rejected but a new boot is accepted as new state", () => {
  const tracker = new StatusTracker();
  tracker.accept(fixture());
  assert.throws(() => tracker.accept(fixture()), /replayed/);
  assert.equal(
    tracker.accept({ ...fixture(), bootId: "abcdef0123456789" }).positionValid,
    false,
  );
});
test("STOP always uses transport; rejection never returns local success", async () => {
  const original = globalThis.fetch;
  const calls = [];
  globalThis.fetch = async (url, options) => {
    calls.push([url, options]);
    return new Response(
      JSON.stringify({ error: "stop_delivery_unavailable_hardware_inhibited" }),
      { status: 503, headers: { "content-type": "application/json" } },
    );
  };
  try {
    await assert.rejects(
      jsonRequest("/api/stop", "token", {}),
      /stop_delivery_unavailable/,
    );
    assert.equal(calls[0][0], "/api/stop");
    assert.equal(calls[0][1].method, "POST");
  } finally {
    globalThis.fetch = original;
  }
});
test("disconnect then reconnect returns only validated server state", async () => {
  const original = globalThis.fetch;
  let fail = true;
  globalThis.fetch = async () => {
    if (fail) throw Error("offline");
    return new Response(JSON.stringify(fixture()), {
      headers: { "content-type": "application/json" },
    });
  };
  try {
    await assert.rejects(jsonRequest("/api/status", ""), /offline/);
    fail = false;
    assert.equal(
      validateStatus(await jsonRequest("/api/status", "")).state,
      "unknown-position",
    );
  } finally {
    globalThis.fetch = original;
  }
});
test("invalid content type, malformed JSON and oversized responses reject", async () => {
  const original = globalThis.fetch;
  try {
    for (const [body, type] of [
      ["{}", "text/html"],
      ["not-json", "application/json"],
      [" ".repeat(32769), "application/json"],
    ]) {
      globalThis.fetch = async () =>
        new Response(body, { headers: { "content-type": type } });
      await assert.rejects(jsonRequest("/api/status", ""));
    }
  } finally {
    globalThis.fetch = original;
  }
});
