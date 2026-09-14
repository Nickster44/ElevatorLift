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
test("watchdog configuration is separate from reply timing and rejects contradictory status", () => {
  const s = fixture();
  const w = { state: "within-software-policy", timeoutMs: 1000, ageMs: 10 };
  const vfd = {
    ...s.vfd,
    watchdog: w,
    stopRefreshMs: 300,
    replyTimeoutMs: 150,
    communicationFault: false,
  };
  assert.equal(validateStatus({ ...s, vfd }).vfd.watchdog.timeoutMs, 1000);
  for (const bad of [
    { ...w, timeoutMs: 200 },
    { ...w, ageMs: 10001 },
    { ...w, state: "disabled" },
    { ...w, timeoutMs: null },
    { ...w, timeoutMs: 59901 },
  ])
    assert.throws(() =>
      validateStatus({ ...s, vfd: { ...vfd, watchdog: bad } }),
    );
  assert.throws(() =>
    validateStatus({
      ...s,
      vfd: { ...vfd, communicationFault: true, healthy: true },
    }),
  );
  assert.equal(
    validateStatus({
      ...s,
      vfd: { ...vfd, watchdog: { state: "disabled", timeoutMs: 0, ageMs: 0 } },
    }).motionAllowed,
    false,
  );
});
test("manual direction intent is not motion permission and release removes intent", () => {
  const base = fixture();
  const manual = {
    active: true,
    hold: true,
    up: true,
    down: false,
    safetyHealthy: true,
    inputsKnown: true,
    requestedDirection: 1,
    motionEnabled: false,
  };
  const active = validateStatus({ ...base, manualControl: manual });
  assert.equal(active.motionAllowed, false);
  assert.equal(active.manualControl.requestedDirection, 1);
  for (const field of ["active", "hold", "up", "safetyHealthy", "inputsKnown"])
    assert.throws(() =>
      validateStatus({ ...base, manualControl: { ...manual, [field]: false } }),
    );
  assert.throws(() =>
    validateStatus({ ...base, manualControl: { ...manual, down: true } }),
  );
  assert.throws(() =>
    validateStatus({
      ...base,
      manualControl: { ...manual, motionEnabled: true },
    }),
  );
  assert.equal(
    validateStatus({
      ...base,
      manualControl: { ...manual, hold: false, requestedDirection: 0 },
    }).manualControl.requestedDirection,
    0,
  );
});
test("v2 wiring and STOP acknowledgements do not confer motion authority", () => {
  const value = {
    ...fixture(),
    upperLimit: false,
    lowerLimit: false,
    serviceKey: true,
    vfd: {
      commsEnabled: true,
      healthy: false,
      stopTransmitted: true,
      stopAcknowledged: true,
      monitorStopped: false,
    },
  };
  const status = validateStatus(value);
  assert.equal(status.inputsQualified, false);
  assert.equal(status.stoppedConfirmed, false);
  assert.equal(status.motionAllowed, false);
  assert.throws(() => validateStatus({ ...value, stoppedConfirmed: true }));
  assert.throws(() => validateStatus({ ...value, contractVersion: 1 }));
  assert.throws(() =>
    validateStatus({ ...value, inputsQualified: true, upperLimit: null }),
  );
  assert.throws(() =>
    validateStatus({
      ...value,
      inputsQualified: true,
      upperLimit: true,
      lowerLimit: true,
    }),
  );
  assert.throws(() =>
    validateStatus({ ...value, vfd: { ...value.vfd, monitorStopped: true } }),
  );
});
test("diagnostic parameter cache validates values, ages and unsupported entries", () => {
  const p = {
    number: 4,
    name: "DEC",
    displayName: "Deceleration",
    units: "s",
    min: 1,
    max: 599,
    scaleDivisor: 10,
    writable: true,
    access: "installer",
  };
  const value = {
    apiVersion: 1,
    available: true,
    definitions: [p],
    readings: [{ number: 4, supported: true, rawValue: 10, ageMs: 20 }],
  };
  assert.equal(validateParameterCatalog(value)[0].rawValue, 10);
  assert.equal(
    validateParameterCatalog({ ...value, available: false })[0].rawValue,
    null,
  );
  for (const r of [
    { ...value.readings[0], ageMs: -1 },
    { ...value.readings[0], rawValue: 10000 },
    { ...value.readings[0], rawValue: null },
    { ...value.readings[0], number: 3 },
  ])
    assert.throws(() => validateParameterCatalog({ ...value, readings: [r] }));
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
