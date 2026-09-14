export type Floor = { floor: number; name: string; position: string | null };
export type Parameter = {
  number: number;
  name: string;
  displayName: string;
  units: string;
  min: number;
  max: number;
  scaleDivisor: number;
  writable: boolean;
  access: string;
  rawValue?: number | null;
  ageMs?: number | null;
  supported?: boolean;
};
export function validateParameterCatalog(value: unknown): Parameter[] {
  if (
    !object(value) ||
    value.apiVersion !== 1 ||
    !Array.isArray(value.definitions) ||
    value.definitions.length > 17
  )
    throw new Error("invalid_parameter_catalog");
  const ids = new Set<number>();
  for (const p of value.definitions) {
    if (
      !object(p) ||
      !integer(p.number) ||
      p.number > 16 ||
      ids.has(p.number) ||
      !["name", "displayName", "units", "access"].every(
        (k) => typeof p[k] === "string",
      ) ||
      !["min", "max", "scaleDivisor"].every((k) => integer(p[k])) ||
      Number(p.scaleDivisor) < 1 ||
      Number(p.max) < Number(p.min) ||
      typeof p.writable !== "boolean"
    )
      throw new Error("invalid_parameter_catalog");
    ids.add(p.number);
  }
  const definitions = value.definitions as Parameter[];
  if (value.readings === undefined) return definitions;
  if (
    typeof value.available !== "boolean" ||
    !Array.isArray(value.readings) ||
    value.readings.length > 17
  )
    throw new Error("invalid_parameter_readings");
  const seen = new Set<number>();
  for (const r of value.readings) {
    if (
      !object(r) ||
      !integer(r.number) ||
      !ids.has(r.number) ||
      seen.has(r.number) ||
      typeof r.supported !== "boolean" ||
      (r.rawValue !== null && (!integer(r.rawValue) || r.rawValue > 9999)) ||
      (r.ageMs !== null && !integer(r.ageMs)) ||
      (r.rawValue === null) !== (r.ageMs === null)
    )
      throw new Error("invalid_parameter_readings");
    seen.add(r.number);
  }
  return definitions.map((p) => {
    const r = (value.readings as Record<string, unknown>[]).find(
      (r) => r.number === p.number,
    );
    return {
      ...p,
      supported: r?.supported as boolean | undefined,
      rawValue: value.available ? (r?.rawValue as number | null) : null,
      ageMs: value.available ? (r?.ageMs as number | null) : null,
    };
  });
}
export type Status = {
  apiVersion: 1;
  contractVersion: number;
  bootId: string;
  sequence: number;
  uptimeMs: number;
  sampleAgeMs: number;
  state: string;
  deploymentReady: boolean;
  position: string;
  positionValid: boolean;
  currentFloor: number | null;
  targetFloor: number | null;
  motionAllowed: boolean;
  canMoveUp: boolean;
  canMoveDown: boolean;
  stoppedConfirmed: boolean;
  safetyOk: boolean;
  home: boolean;
  upperLimit: boolean | null;
  lowerLimit: boolean | null;
  serviceKey: boolean | null;
  inputsQualified: boolean;
  manualControl?: {
    active: boolean;
    hold: boolean;
    up: boolean;
    down: boolean;
    safetyHealthy: boolean;
    inputsKnown: boolean;
    requestedDirection: number;
    motionEnabled: boolean;
  };
  vfd: {
    commsEnabled: boolean;
    healthy: boolean;
    stopTransmitted: boolean;
    stopAcknowledged: boolean;
    monitorStopped: boolean;
    communicationFault?: boolean;
    stopRefreshMs?: number;
    replyTimeoutMs?: number;
    watchdog?: {
      state: string;
      timeoutMs: number | null;
      ageMs: number | null;
    };
  };
  fault: string;
  blockedReasons: string[];
  capabilities: Record<string, boolean>;
  floors: Floor[];
  light: { on: boolean; feedbackVerified: boolean };
  telemetry: null | {
    ageMs: number;
    frequency: number;
    current: number;
    busVolts: number;
    temperature: number;
  };
  calibration: { valid: boolean; distance: number | null };
  devices: Record<string, string>;
  network: { mode: string; stationIp: string; apIp: string; hostname: string };
  authConfigured: boolean;
};
function object(v: unknown): v is Record<string, unknown> {
  return !!v && typeof v === "object" && !Array.isArray(v);
}
function integer(v: unknown): v is number {
  return typeof v === "number" && Number.isSafeInteger(v) && v >= 0;
}
function count(v: unknown): v is string {
  if (typeof v !== "string" || !/^-?(0|[1-9][0-9]{0,18})$/.test(v))
    return false;
  return (
    BigInt(v) >= BigInt("-9223372036854775808") &&
    BigInt(v) <= BigInt("9223372036854775807")
  );
}
export function validateStatus(v: unknown): Status {
  const fail = () => {
    throw new Error("invalid_controller_status");
  };
  if (!object(v)) return fail();
  if (v.apiVersion !== 1 || v.contractVersion !== 2) return fail();
  if (typeof v.bootId !== "string" || !/^[0-9a-f]{16}$/.test(v.bootId))
    return fail();
  for (const k of ["sequence", "uptimeMs", "sampleAgeMs"])
    if (!integer(v[k])) return fail();
  for (const k of [
    "deploymentReady",
    "positionValid",
    "motionAllowed",
    "canMoveUp",
    "canMoveDown",
    "stoppedConfirmed",
    "safetyOk",
    "home",
    "authConfigured",
    "inputsQualified",
  ])
    if (typeof v[k] !== "boolean") return fail();
  for (const k of ["upperLimit", "lowerLimit", "serviceKey"])
    if (v[k] !== null && typeof v[k] !== "boolean") return fail();
  if (v.manualControl !== undefined) {
    const m = v.manualControl;
    if (
      !object(m) ||
      ![
        "active",
        "hold",
        "up",
        "down",
        "safetyHealthy",
        "inputsKnown",
        "motionEnabled",
      ].every((k) => typeof m[k] === "boolean") ||
      ![-1, 0, 1].includes(m.requestedDirection as number)
    )
      return fail();
    if (
      m.requestedDirection !== 0 &&
      (!m.active ||
        !m.hold ||
        !m.safetyHealthy ||
        !m.inputsKnown ||
        m.up === m.down ||
        (m.requestedDirection === 1) !== m.up)
    )
      return fail();
    if (m.motionEnabled && !v.deploymentReady) return fail();
  }
  if (
    !object(v.vfd) ||
    ![
      "commsEnabled",
      "healthy",
      "stopTransmitted",
      "stopAcknowledged",
      "monitorStopped",
    ].every((k) => typeof (v.vfd as Record<string, unknown>)[k] === "boolean")
  )
    return fail();
  if (
    v.vfd.monitorStopped &&
    (!v.vfd.healthy || !v.vfd.commsEnabled || v.telemetry === null)
  )
    return fail();
  if (v.stoppedConfirmed && !v.vfd.monitorStopped) return fail();
  if (
    v.vfd.communicationFault !== undefined &&
    (typeof v.vfd.communicationFault !== "boolean" ||
      (v.vfd.communicationFault && v.vfd.healthy))
  )
    return fail();
  for (const k of ["stopRefreshMs", "replyTimeoutMs"])
    if (v.vfd[k] !== undefined && (!integer(v.vfd[k]) || Number(v.vfd[k]) < 1))
      return fail();
  if (v.vfd.watchdog !== undefined) {
    const w = v.vfd.watchdog;
    if (
      !object(w) ||
      ![
        "unknown",
        "disabled",
        "too-short",
        "outside-policy",
        "within-software-policy",
      ].includes(String(w.state)) ||
      (w.timeoutMs !== null &&
        (!integer(w.timeoutMs) ||
          w.timeoutMs > 59900 ||
          w.timeoutMs % 100 !== 0)) ||
      (w.ageMs !== null && !integer(w.ageMs)) ||
      (w.timeoutMs === null) !== (w.ageMs === null)
    )
      return fail();
    if (
      w.state !== "unknown" &&
      (w.timeoutMs === null || w.ageMs === null || Number(w.ageMs) > 10000)
    )
      return fail();
    if (w.state === "disabled" && w.timeoutMs !== 0) return fail();
    if (
      w.state === "too-short" &&
      !(Number(w.timeoutMs) > 0 && Number(w.timeoutMs) < 1000)
    )
      return fail();
    if (w.state === "within-software-policy" && w.timeoutMs !== 1000)
      return fail();
    if (w.state === "outside-policy" && !(Number(w.timeoutMs) > 1000))
      return fail();
  }
  if (
    v.inputsQualified &&
    (v.upperLimit === null ||
      v.lowerLimit === null ||
      v.serviceKey === null ||
      (v.upperLimit && v.lowerLimit))
  )
    return fail();
  if (
    ![
      "unknown-position",
      "idle",
      "moving",
      "homing",
      "service",
      "calibrating",
      "stopping",
      "fault",
    ].includes(String(v.state))
  )
    return fail();
  if (!count(v.position) || typeof v.fault !== "string") return fail();
  for (const k of ["currentFloor", "targetFloor"])
    if (v[k] !== null && ![1, 2, 3].includes(v[k] as number)) return fail();
  if (
    !Array.isArray(v.blockedReasons) ||
    !v.blockedReasons.every((r) => typeof r === "string")
  )
    return fail();
  if (
    !object(v.capabilities) ||
    !Object.values(v.capabilities).every((c) => typeof c === "boolean")
  )
    return fail();
  for (const cap of ["status", "motion", "light", "network", "logs"])
    if (typeof v.capabilities[cap] !== "boolean") return fail();
  if (
    !Array.isArray(v.floors) ||
    v.floors.length !== 3 ||
    !v.floors.every(
      (f, i) =>
        object(f) &&
        f.floor === i + 1 &&
        typeof f.name === "string" &&
        f.name.length <= 64 &&
        (f.position === null || count(f.position)),
    )
  )
    return fail();
  if (
    !object(v.light) ||
    typeof v.light.on !== "boolean" ||
    typeof v.light.feedbackVerified !== "boolean"
  )
    return fail();
  if (
    !object(v.calibration) ||
    typeof v.calibration.valid !== "boolean" ||
    (v.calibration.distance !== null && !integer(v.calibration.distance))
  )
    return fail();
  if (
    v.telemetry !== null &&
    (!object(v.telemetry) ||
      !["ageMs", "frequency", "current", "busVolts", "temperature"].every((k) =>
        integer((v.telemetry as Record<string, unknown>)[k]),
      ))
  )
    return fail();
  if (
    !object(v.devices) ||
    !Object.values(v.devices).every((d) => typeof d === "string")
  )
    return fail();
  if (
    !object(v.network) ||
    !["mode", "stationIp", "apIp", "hostname"].every(
      (k) => typeof (v.network as Record<string, unknown>)[k] === "string",
    )
  )
    return fail();
  if (
    v.motionAllowed &&
    (!v.deploymentReady ||
      !v.inputsQualified ||
      !v.positionValid ||
      !v.safetyOk ||
      !v.stoppedConfirmed ||
      v.state !== "idle" ||
      v.blockedReasons.length ||
      !v.capabilities.motion ||
      !v.calibration.valid ||
      v.upperLimit === null ||
      v.lowerLimit === null ||
      v.serviceKey !== false)
  )
    return fail();
  if (v.canMoveUp && (!v.motionAllowed || v.upperLimit !== false))
    return fail();
  if (v.canMoveDown && (!v.motionAllowed || v.lowerLimit !== false))
    return fail();
  if (!v.positionValid && v.currentFloor !== null) return fail();
  return v as Status;
}
export function isFresh(
  status: Status | null,
  receivedAt: number,
  now: number,
) {
  return (
    !!status &&
    now >= receivedAt &&
    now - receivedAt < 2500 &&
    status.sampleAgeMs <= 100
  );
}
export class StatusTracker {
  private last: Status | null = null;
  accept(value: unknown): Status {
    const next = validateStatus(value);
    if (
      this.last?.bootId === next.bootId &&
      next.sequence <= this.last.sequence
    )
      throw new Error("replayed_controller_status");
    this.last = next;
    return next;
  }
}
export async function jsonRequest(
  path: string,
  token: string,
  fields?: Record<string, string>,
  signal?: AbortSignal,
): Promise<unknown> {
  const controller = new AbortController();
  const abort = () => controller.abort();
  signal?.addEventListener("abort", abort, { once: true });
  if (signal?.aborted) controller.abort();
  const timer = setTimeout(abort, 1800);
  try {
    const response = await fetch(path, {
      method: fields ? "POST" : "GET",
      headers: {
        ...(token ? { "X-Lift-Api-Token": token } : {}),
        ...(fields
          ? { "Content-Type": "application/x-www-form-urlencoded" }
          : {}),
      },
      body: fields ? new URLSearchParams(fields) : undefined,
      signal: controller.signal,
      cache: "no-store",
    });
    if (!response.headers.get("content-type")?.includes("application/json"))
      throw new Error("invalid_content_type");
    const reader = response.body?.getReader();
    if (!reader) throw new Error("empty_response");
    const decoder = new TextDecoder("utf-8", { fatal: true });
    let text = "",
      bytes = 0;
    try {
      for (;;) {
        const { done, value } = await reader.read();
        if (done) break;
        bytes += value.byteLength;
        if (bytes > 32768) {
          controller.abort();
          throw new Error("response_too_large");
        }
        text += decoder.decode(value, { stream: true });
      }
      text += decoder.decode();
    } finally {
      reader.releaseLock();
    }
    const data: unknown = JSON.parse(text);
    if (!response.ok)
      throw new Error(
        object(data) && typeof data.error === "string"
          ? data.error
          : `http_${response.status}`,
      );
    return data;
  } finally {
    clearTimeout(timer);
    signal?.removeEventListener("abort", abort);
  }
}
