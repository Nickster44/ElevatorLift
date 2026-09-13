"use client";
import { useEffect, useRef, useState } from "react";
import {
  isFresh,
  jsonRequest,
  validateStatus,
  validateParameterCatalog,
  StatusTracker,
  type Parameter,
  type Status,
} from "../lib/controller";

type View = "Overview" | "Drive" | "Setup" | "Remotes" | "Logs" | "System";
const views: View[] = [
  "Overview",
  "Drive",
  "Setup",
  "Remotes",
  "Logs",
  "System",
];
export default function Home() {
  const [view, setView] = useState<View>("Overview");
  const [status, setStatus] = useState<Status | null>(null);
  const [received, setReceived] = useState(0);
  const [now, setNow] = useState(0);
  const [connection, setConnection] = useState("Connecting");
  const [message, setMessage] = useState("");
  const [token, setToken] = useState("");
  const [busy, setBusy] = useState(false);
  const [logs, setLogs] = useState<unknown[]>([]);
  const [parameters, setParameters] = useState<Parameter[]>([]);
  const [ssid, setSsid] = useState("");
  const [password, setPassword] = useState("");
  const mounted = useRef(false);
  useEffect(() => {
    mounted.current = true;
    let active = true;
    let timer: ReturnType<typeof setTimeout>;
    const abort = new AbortController();
    const tracker = new StatusTracker();
    const poll = async () => {
      try {
        const value = tracker.accept(
          await jsonRequest("/api/status", "", undefined, abort.signal),
        );
        if (active) {
          setStatus(value);
          setReceived(performance.now());
          setConnection(value.sampleAgeMs > 100 ? "Stale" : "Connected");
        }
      } catch (error) {
        if (active)
          setConnection(
            `Disconnected: ${error instanceof Error ? error.message : "request_failed"}`,
          );
      }
      if (active) timer = setTimeout(poll, 1000);
    };
    void poll();
    const clock = setInterval(() => setNow(performance.now()), 250);
    return () => {
      active = false;
      mounted.current = false;
      abort.abort();
      clearTimeout(timer);
      clearInterval(clock);
    };
  }, []);
  const fresh = connection === "Connected" && isFresh(status, received, now);
  const can = (cap: string) => fresh && status?.capabilities[cap] === true;
  async function command(
    path: string,
    fields: Record<string, string>,
    kind: "status" | "network" = "status",
  ) {
    if(path!=="/api/stop"&&(connection!=="Connected"||!isFresh(status,received,performance.now()))) {
      setMessage("Request blocked: controller status is stale or disconnected.");
      return;
    }
    setBusy(true);
    setMessage("");
    try {
      const value = await jsonRequest(path, token, fields);
      if (!mounted.current) return;
      if (kind === "status") {
        // Commands never invent a motion transition. A later poll owns the display.
        validateStatus(value);
        setMessage("Controller accepted the request; awaiting fresh status.");
      } else {
        if (
          !value ||
          typeof value !== "object" ||
          !("saved" in value) ||
          value.saved !== true ||
          !("restartRequired" in value) ||
          value.restartRequired !== true
        )
          throw new Error("invalid_network_response");
        setPassword("");
        setMessage(
          "Network saved. Restart required; remote reboot is unavailable in this hardware profile.",
        );
      }
    } catch (error) {
      if (mounted.current)
        setMessage(
          `Request not confirmed: ${error instanceof Error ? error.message : "request_failed"}`,
        );
    } finally {
      if (mounted.current) setBusy(false);
    }
  }
  async function readLogs() {
    try {
      const data = await jsonRequest("/api/logs/recent", token);
      if (
        !data ||
        typeof data !== "object" ||
        !("apiVersion" in data) ||
        data.apiVersion !== 1 ||
        !("events" in data) ||
        !Array.isArray(data.events) ||
        data.events.length > 16
      )
        throw new Error("invalid_log_response");
      setLogs(data.events);
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "logs_unavailable");
    }
  }
  async function readParameters() {
    try {
      setParameters(
        validateParameterCatalog(
          await jsonRequest("/api/vfd/parameters", token),
        ),
      );
    } catch (error) {
      setMessage(
        error instanceof Error
          ? error.message
          : "parameter_catalog_unavailable",
      );
    }
  }
  const telemetry =
    fresh && status?.telemetry && status.telemetry.ageMs <= 500
      ? status.telemetry
      : null;
  return (
    <div className="console-shell">
      <aside className="console-nav">
        <h1>Elevator Lift</h1>
        <nav aria-label="Controller views">
          {views.map((v) => (
            <button
              key={v}
              aria-current={view === v ? "page" : undefined}
              onClick={() => setView(v)}
            >
              {v}
            </button>
          ))}
        </nav>
        <p role="status">
          {fresh
            ? "Controller connected"
            : connection === "Connected"
              ? "Stale"
              : connection}
        </p>
      </aside>
      <main className="console-main">
        <header className="console-header">
          <div>
            <h2>{view}</h2>
            <small>
              {received
                ? `Last valid response: ${Math.max(0, Math.floor((now - received) / 1000))} s ago`
                : "No valid controller response"}
            </small>
          </div>
          <button
            className="stop-button"
            aria-label="Controlled lift stop"
            onClick={() => void command("/api/stop", {})}
          >
            STOP
          </button>
        </header>
        <section
          className={`permission ${fresh && status?.motionAllowed ? "allowed" : "blocked"}`}
          aria-label="Motion permission"
        >
          <strong>SAFETY CIRCUIT</strong>
          <p>
            {!fresh
              ? "Motion status unavailable"
              : status?.motionAllowed
                ? "Motion permitted"
                : "Motion inhibited"}
          </p>
          {fresh &&
            status?.blockedReasons.map((reason) => (
              <div key={reason}>{reason}</div>
            ))}
          {fresh && status?.fault && <p>Latched fault: {status.fault}</p>}
        </section>
        {message && (
          <p role="alert" className="request-result">
            {message}
          </p>
        )}
        {view === "Overview" && (
          <>
            <section className="console-section">
              <h3>Lift controls</h3>
              <div className="floor-list">
                {[3, 2, 1].map((f) => {
                  const floor = status?.floors.find((x) => x.floor === f);
                  return (
                    <div key={f} className="floor-row">
                      <span
                        className={`landing-indicator ${fresh && status?.currentFloor === f ? "occupied" : ""}`}
                      >
                        {f}
                      </span>
                      <div>
                        <strong>{floor?.name ?? `Landing ${f}`}</strong>
                        <small>
                          {fresh && status?.currentFloor === f
                            ? "At landing"
                            : floor?.position !== null &&
                                floor?.position !== undefined
                              ? `${floor.position} counts`
                              : "Not commissioned"}
                        </small>
                      </div>
                      <button
                        disabled={
                          busy ||
                          !can("motion") ||
                          !status?.motionAllowed ||
                          status.currentFloor === f
                        }
                        onClick={() =>
                          void command("/api/move", { floor: String(f) })
                        }
                      >
                        Call floor {f}
                      </button>
                    </div>
                  );
                })}
              </div>
            </section>
            <section className="console-section">
              <h3>Controller state</h3>
              <dl className="metrics">
                <div>
                  <dt>Motion</dt>
                  <dd>{fresh ? status?.state : "Unknown"}</dd>
                </div>
                <div>
                  <dt>Position</dt>
                  <dd>
                    {fresh && status?.positionValid
                      ? `${status.position} counts`
                      : "Unknown"}
                  </dd>
                </div>
                <div>
                  <dt>Stopped</dt>
                  <dd>
                    {fresh && status?.stoppedConfirmed
                      ? "Confirmed"
                      : "Unconfirmed"}
                  </dd>
                </div>
                <div>
                  <dt>Light output command</dt>
                  <dd>
                    <label>
                      <input
                        type="checkbox"
                        checked={fresh && status?.light.on === true}
                        disabled={busy || !can("light")}
                        onChange={(e) =>
                          void command("/api/light", {
                            on: String(e.target.checked),
                          })
                        }
                      />
                      {fresh ? (status?.light.on ? "On" : "Off") : "Unknown"}
                    </label>
                  </dd>
                </div>
              </dl>
            </section>
          </>
        )}
        {view === "Drive" && (
          <>
            <section className="console-section">
              <h3>VFD telemetry</h3>
              <dl className="metrics">
                {(
                  ["frequency", "current", "busVolts", "temperature"] as const
                ).map((k) => (
                  <div key={k}>
                    <dt>{k}</dt>
                    <dd>{telemetry ? telemetry[k] : "Unavailable"}</dd>
                  </div>
                ))}
              </dl>
            </section>
            <section className="console-section">
              <h3>Stopping-distance calibration</h3>
              <p>
                {fresh && status?.calibration.valid
                  ? `${status.calibration.distance} counts`
                  : "Not valid"}
              </p>
              <button disabled>Calibration unavailable</button>
              <h3>Drive parameters</h3>
              <p>
                Live read/write unavailable: UART enable hardware dependency.
              </p>
              <button disabled={!fresh} onClick={() => void readParameters()}>
                Read parameter catalog
              </button>
              <div className="parameter-table">
                <table>
                  <thead>
                    <tr>
                      <th>Parameter</th>
                      <th>Range</th>
                      <th>Live value</th>
                    </tr>
                  </thead>
                  <tbody>
                    {parameters.map((p) => (
                      <tr key={p.number}>
                        <td>
                          {String(p.number).padStart(2, "0")} {p.displayName}
                          <small>
                            {p.name} / {p.access}
                          </small>
                        </td>
                        <td>
                          {p.min / p.scaleDivisor} - {p.max / p.scaleDivisor}{" "}
                          {p.units}
                        </td>
                        <td>Unavailable</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </section>
          </>
        )}
        {view === "Setup" && (
          <section className="console-section">
            <h3>Floors and motion settings</h3>
            <p>Commissioning unavailable in this hardware profile.</p>
            <button disabled>Set floor positions</button>{" "}
            <button disabled>Home to upper reference</button>
            <h3>Service controls</h3>
            <p>Physical service-key assignment unresolved. No web override.</p>
          </section>
        )}
        {view === "Remotes" && (
          <section className="console-section">
            <h3>418 MHz remotes</h3>
            <p>Decoder capture and pairing are not enabled in this profile.</p>
            <p>
              TX_ID is a reusable learned slot. No permanent transmitter
              identity is available.
            </p>
            <button disabled>Learn unavailable</button>{" "}
            <button disabled>Erase unavailable</button>
          </section>
        )}
        {view === "Logs" && (
          <section className="console-section">
            <h3>Durable events</h3>
            <button disabled={!can("logs")} onClick={() => void readLogs()}>
              Refresh recent events
            </button>
            {logs.length ? (
              <pre>{JSON.stringify(logs, null, 2)}</pre>
            ) : (
              <p>No events loaded.</p>
            )}
          </section>
        )}
        {view === "System" && (
          <>
            <section className="console-section">
              <h3>Access</h3>
              <label>
                API token
                <input
                  type="password"
                  value={token}
                  autoComplete="off"
                  onChange={(e) => setToken(e.target.value)}
                />
              </label>
              <p>
                {fresh
                  ? status?.authConfigured
                    ? "Authentication configured"
                    : "Write access disabled: no configured token"
                  : "Authentication state unavailable"}
              </p>
            </section>
            <section className="console-section">
              <h3>Wi-Fi network</h3>
              <form
                onSubmit={(e) => {
                  e.preventDefault();
                  void command("/api/network", { ssid, password }, "network");
                }}
              >
                <label>
                  SSID
                  <input
                    value={ssid}
                    maxLength={32}
                    onChange={(e) => setSsid(e.target.value)}
                  />
                </label>
                <label>
                  Password
                  <input
                    type="password"
                    value={password}
                    maxLength={63}
                    autoComplete="new-password"
                    onChange={(e) => setPassword(e.target.value)}
                  />
                </label>
                <button disabled={busy || !can("network")}>Save network</button>
              </form>
              <dl>
                <dt>Local hostname</dt>
                <dd>{fresh ? status?.network.hostname : "Unavailable"}</dd>
                <dt>Mode</dt>
                <dd>{fresh ? status?.network.mode : "Unknown"}</dd>
              </dl>
            </section>
            <section className="console-section">
              <h3>Devices</h3>
              {fresh && status ? (
                Object.entries(status.devices).map(([name, state]) => (
                  <p key={name}>
                    {name}: {state}
                  </p>
                ))
              ) : (
                <p>Unavailable</p>
              )}
              <button disabled>Reboot unavailable</button>{" "}
              <button disabled>Restore unavailable</button>
            </section>
          </>
        )}
      </main>
    </div>
  );
}
