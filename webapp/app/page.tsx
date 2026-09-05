"use client";

import { useCallback, useEffect, useMemo, useRef, useState } from "react";

type View = "overview" | "setup" | "drive" | "remotes" | "logs" | "system";

type LiftStatus = {
  state: string;
  position: number;
  target: number;
  targetFloor: number;
  normalRunTenthsHz: number;
  safetyOk: boolean;
  home: boolean;
  lowerLimit: boolean;
  upperLimit: boolean;
  lastVfdCommand: string;
  lastFault: string;
  networkMode: string;
  stationIp: string;
  apIp: string;
};

type EventItem = {
  id: number;
  time: string;
  title: string;
  detail: string;
  level: "ok" | "info" | "warn";
};

type FloorConfig = { floor: number; name: string; position: number };
type RemoteRecord = { id: string; name: string; lastSeen: string; lastCommand: string; seen: boolean };

const defaultFloors: FloorConfig[] = [
  { floor: 1, name: "Lower deck", position: 0 },
  { floor: 2, name: "Main deck", position: 18420 },
  { floor: 3, name: "Upper deck", position: 39250 },
];

const demoStatus: LiftStatus = {
  state: "idle",
  position: 18420,
  target: 18420,
  targetFloor: 2,
  normalRunTenthsHz: 450,
  safetyOk: true,
  home: false,
  lowerLimit: false,
  upperLimit: false,
  lastVfdCommand: "S00A0000",
  lastFault: "",
  networkMode: "station",
  stationIp: "192.168.4.28",
  apIp: "",
};

const initialEvents: EventItem[] = [
  { id: 1, time: "10:42:18", title: "Arrived at Landing 2", detail: "Travel complete · 18,420 counts", level: "ok" },
  { id: 2, time: "10:41:56", title: "Motion accepted", detail: "Landing 2 · Hall station", level: "info" },
  { id: 3, time: "08:15:03", title: "Controller online", detail: "Normal restart · Position restored", level: "info" },
  { id: 4, time: "Yesterday", title: "Configuration saved", detail: "Run speed updated by installer", level: "warn" },
];

const navItems: { id: View; label: string; symbol: string }[] = [
  { id: "overview", label: "Overview", symbol: "⌂" },
  { id: "setup", label: "Lift setup", symbol: "⌁" },
  { id: "drive", label: "Drive & calibration", symbol: "◫" },
  { id: "remotes", label: "RF remotes", symbol: "⌁" },
  { id: "logs", label: "Event log", symbol: "≡" },
  { id: "system", label: "System", symbol: "⚙" },
];

function titleCase(value: string) {
  return value.replaceAll("_", " ").replace(/\b\w/g, (letter) => letter.toUpperCase());
}

function SafetyPill({ ok, label }: { ok: boolean; label: string }) {
  return <span className={`safety-pill ${ok ? "is-ok" : "is-warn"}`}><i />{label}</span>;
}

function Metric({ label, value, unit, accent }: { label: string; value: string; unit?: string; accent?: boolean }) {
  return (
    <div className="metric">
      <span>{label}</span>
      <strong className={accent ? "accent" : ""}>{value}<small>{unit}</small></strong>
    </div>
  );
}

function Toggle({ checked, onChange, label }: { checked: boolean; onChange: () => void; label: string }) {
  return (
    <button className={`toggle ${checked ? "on" : ""}`} onClick={onChange} role="switch" aria-checked={checked} aria-label={label}>
      <span />
    </button>
  );
}

export default function Home() {
  const [view, setView] = useState<View>("overview");
  const [status, setStatus] = useState<LiftStatus>(demoStatus);
  const [events, setEvents] = useState<EventItem[]>(initialEvents);
  const [demoMode, setDemoMode] = useState(false);
  const [lastUpdated, setLastUpdated] = useState(new Date());
  const [toast, setToast] = useState("");
  const [serviceMode, setServiceMode] = useState(false);
  const [lightOn, setLightOn] = useState(false);
  const [floors, setFloors] = useState<FloorConfig[]>(defaultFloors);
  const [apiToken, setApiToken] = useState(() => typeof window === "undefined" ? "" : window.localStorage.getItem("lift-api-token") ?? "");
  const [menuOpen, setMenuOpen] = useState(false);

  const apiFetch = useCallback(async (path: string, init?: RequestInit) => {
    const headers = new Headers(init?.headers);
    if (apiToken) headers.set("X-Lift-Api-Token", apiToken);
    const response = await fetch(path, { ...init, headers });
    const contentType = response.headers.get("content-type") || "";
    if (!response.ok || !contentType.includes("application/json")) throw new Error("Controller unavailable");
    return response.json();
  }, [apiToken]);

  const refresh = useCallback(async () => {
    try {
      const next = await apiFetch("/api/status");
      setStatus(next);
      setDemoMode(false);
    } catch {
      setDemoMode(true);
    } finally {
      setLastUpdated(new Date());
    }
  }, [apiFetch]);

  useEffect(() => {
    const kickoff = window.setTimeout(refresh, 0);
    const timer = window.setInterval(refresh, 3000);
    return () => {
      window.clearTimeout(kickoff);
      window.clearInterval(timer);
    };
  }, [refresh]);

  useEffect(() => {
    if (!toast) return;
    const timer = window.setTimeout(() => setToast(""), 3200);
    return () => window.clearTimeout(timer);
  }, [toast]);

  const currentFloor = useMemo(() => {
    if (status.position < 8000) return 1;
    if (status.position < 27000) return 2;
    return 3;
  }, [status.position]);
  const currentFloorConfig = floors.find((floor) => floor.floor === currentFloor) ?? floors[0];

  async function moveTo(floor: number) {
    if (!status.safetyOk || status.state !== "idle") {
      setToast("Move unavailable: controller is not ready");
      return;
    }
    if (demoMode) {
      setStatus((current) => ({ ...current, state: "moving", targetFloor: floor, target: floor === 1 ? 0 : floor === 2 ? 18420 : 39250 }));
      setEvents((current) => [{ id: Date.now(), time: "Just now", title: "Motion requested", detail: `Landing ${floor} · Web console demo`, level: "info" }, ...current]);
      setToast(`Demo request accepted for Landing ${floor}`);
      window.setTimeout(() => setStatus((current) => ({ ...current, state: "idle", position: current.target, targetFloor: floor })), 1800);
      return;
    }
    try {
      await apiFetch(`/api/move?floor=${floor}`, { method: "POST" });
      setToast(`Request accepted for Landing ${floor}`);
      refresh();
    } catch {
      setToast("Controller rejected the move request");
    }
  }

  async function stopLift() {
    if (demoMode) {
      setStatus((current) => ({ ...current, state: "idle" }));
      setToast("Demo stop request sent");
      return;
    }
    try {
      await apiFetch("/api/stop", { method: "POST" });
      setToast("Stop request sent");
      refresh();
    } catch {
      setToast("Stop request could not be confirmed");
    }
  }

  async function toggleLight() {
    if (demoMode) {
      setLightOn((current) => !current);
      setToast(`Lift light turned ${lightOn ? "off" : "on"}`);
      return;
    }
    try {
      const result = await apiFetch("/api/light/toggle", { method: "POST" });
      setLightOn(Boolean(result.on));
      setToast(`Lift light turned ${result.on ? "on" : "off"}`);
    } catch {
      setToast("Light command could not be confirmed");
    }
  }

  function saveToken() {
    window.localStorage.setItem("lift-api-token", apiToken);
    setToast("API token saved on this device");
  }

  return (
    <main className="app-shell">
      <aside className={`sidebar ${menuOpen ? "open" : ""}`}>
        <div className="brand">
          <div className="brand-mark"><span>▲</span><span>▼</span></div>
          <div><strong>Elevator Lift</strong><small>CONTROL SYSTEM</small></div>
        </div>
        <nav aria-label="Primary navigation">
          {navItems.map((item) => (
            <button key={item.id} className={view === item.id ? "active" : ""} onClick={() => { setView(item.id); setMenuOpen(false); }}>
              <span className="nav-symbol">{item.symbol}</span>{item.label}
            </button>
          ))}
        </nav>
        <div className="controller-card">
          <div><i className={demoMode ? "amber" : ""} /><span>{demoMode ? "Preview data" : "Controller online"}</span></div>
          <small>{demoMode ? "API not connected" : status.stationIp || status.apIp}</small>
        </div>
        <div className="sidebar-foot">REV A · WEB CONSOLE<br />Firmware interface v0.1</div>
      </aside>

      <section className="workspace">
        <header className="topbar">
          <button className="menu-button" onClick={() => setMenuOpen(!menuOpen)} aria-label="Toggle navigation">☰</button>
          <div>
            <span className="eyebrow">LIFT / {view.toUpperCase()}</span>
            <h1>{navItems.find((item) => item.id === view)?.label}</h1>
          </div>
          <div className="topbar-actions">
            <span className="updated">Updated {lastUpdated.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" })}</span>
            <button className="icon-button" onClick={refresh} aria-label="Refresh status">↻</button>
          </div>
        </header>

        <div className="content">
          {view === "overview" && (
            <>
              <section className={`safety-banner ${status.safetyOk ? "safe" : "unsafe"}`}>
                <div className="shield">{status.safetyOk ? "✓" : "!"}</div>
                <div><span>SAFETY CIRCUIT</span><strong>{status.safetyOk ? "All interlocks healthy" : "Motion authority removed"}</strong></div>
                <div className="safety-checks">
                  <SafetyPill ok={status.safetyOk} label="Loop closed" />
                  <SafetyPill ok={!status.upperLimit} label="Upper limit clear" />
                  <SafetyPill ok={!status.lowerLimit} label="Lower limit clear" />
                </div>
              </section>

              <div className="overview-grid">
                <section className="card lift-card">
                  <div className="card-heading"><div><span className="section-label">LIVE POSITION</span><h2>{currentFloorConfig.name}</h2></div><span className={`state-badge ${status.state}`}>{titleCase(status.state)}</span></div>
                  <div className="shaft-wrap">
                    <div className="shaft">
                      {[3, 2, 1].map((floor) => (
                        <div className={`shaft-floor ${currentFloor === floor ? "current" : ""}`} key={floor}>
                          <span>L{floor}</span><i /><b>{currentFloor === floor ? "CAR" : floors.find((item) => item.floor === floor)?.name}</b>
                        </div>
                      ))}
                    </div>
                    <div className="position-detail">
                      <span>POSITION COUNTER</span>
                      <strong>{status.position.toLocaleString()}</strong>
                      <small>encoder counts</small>
                      <div className="target-line"><span>Target</span><b>{status.state === "idle" ? "—" : `Landing ${status.targetFloor}`}</b></div>
                    </div>
                  </div>
                  <div className="motion-note"><i />{status.state === "idle" ? "Car is stationary and ready" : `Lift is ${titleCase(status.state)}`}</div>
                </section>

                <section className="card call-card">
                  <div className="card-heading"><div><span className="section-label">CONTROL</span><h2>Lift controls</h2></div><small>Five-button remote layout</small></div>
                  <div className="floor-buttons">
                    {[3, 2, 1].map((floor) => (
                      <button key={floor} onClick={() => moveTo(floor)} disabled={status.state !== "idle" || !status.safetyOk || currentFloor === floor}>
                        <span>{floor}</span>
                        <div><strong>{floors.find((item) => item.floor === floor)?.name}</strong><small>{currentFloor === floor ? "Lift is here" : `Landing ${floor}`}</small></div>
                        <b>{currentFloor === floor ? "HERE" : "→"}</b>
                      </button>
                    ))}
                  </div>
                  <div className="utility-controls">
                    <button className="control-stop" onClick={stopLift}><span>■</span><div><strong>Stop</strong><small>Controlled lift stop</small></div></button>
                    <button className={`control-light ${lightOn ? "active" : ""}`} onClick={toggleLight}><span>☀</span><div><strong>Lift light</strong><small>{lightOn ? "Currently on" : "Currently off"}</small></div></button>
                  </div>
                  <div className="service-row"><div><strong>Service controls</strong><small>Enable jog and maintenance actions</small></div><Toggle checked={serviceMode} onChange={() => setServiceMode(!serviceMode)} label="Service controls" /></div>
                </section>

                <section className="card telemetry-card">
                  <div className="card-heading"><div><span className="section-label">EM01 DRIVE</span><h2>Drive telemetry</h2></div><span className="link-state"><i />Communicating</span></div>
                  <div className="metrics-grid">
                    <Metric label="Output frequency" value="0.0" unit="Hz" accent />
                    <Metric label="Motor current" value="0.0" unit="A" />
                    <Metric label="DC bus" value="326" unit="V" />
                    <Metric label="Drive temp." value="34" unit="°C" />
                  </div>
                  <div className="command-line"><span>LAST COMMAND</span><code>{status.lastVfdCommand || "No command recorded"}</code></div>
                </section>

                <section className="card activity-card">
                  <div className="card-heading"><div><span className="section-label">RECENT ACTIVITY</span><h2>Event timeline</h2></div><button className="text-button" onClick={() => setView("logs")}>View all</button></div>
                  <div className="timeline">
                    {events.slice(0, 4).map((event) => (
                      <div className="event" key={event.id}><i className={event.level} /><time>{event.time}</time><div><strong>{event.title}</strong><small>{event.detail}</small></div></div>
                    ))}
                  </div>
                </section>
              </div>
            </>
          )}

          {view === "setup" && <SetupPanel floors={floors} setFloors={setFloors} setToast={setToast} />}
          {view === "drive" && <DrivePanel setToast={setToast} />}
          {view === "remotes" && <RemotesPanel setToast={setToast} />}
          {view === "logs" && <LogsPanel events={events} />}
          {view === "system" && <SystemPanel status={status} apiToken={apiToken} setApiToken={setApiToken} saveToken={saveToken} setToast={setToast} />}
        </div>
      </section>
      {menuOpen && <button className="scrim" onClick={() => setMenuOpen(false)} aria-label="Close navigation" />}
      {toast && <div className="toast" role="status"><i />{toast}</div>}
    </main>
  );
}

function PanelIntro({ kicker, title, description }: { kicker: string; title: string; description: string }) {
  return <div className="panel-intro"><span className="section-label">{kicker}</span><h2>{title}</h2><p>{description}</p></div>;
}

function SetupPanel({ floors, setFloors, setToast }: { floors: FloorConfig[]; setFloors: (floors: FloorConfig[]) => void; setToast: (message: string) => void }) {
  const [editing, setEditing] = useState(false);
  const [draftFloors, setDraftFloors] = useState<FloorConfig[]>(floors);

  function updateFloor(floorNumber: number, field: "name" | "position", value: string) {
    setDraftFloors((current) => current.map((floor) => floor.floor === floorNumber ? { ...floor, [field]: field === "position" ? Number(value.replace(/,/g, "")) || 0 : value } : floor));
  }

  function saveFloors() {
    if (draftFloors.some((floor) => !floor.name.trim())) {
      setToast("Every landing needs a nickname");
      return;
    }
    setFloors(draftFloors);
    setEditing(false);
    setToast("Landing names and positions saved in preview");
  }

  return (
    <div className="panel-page">
      <PanelIntro kicker="CONTROLLER SETTINGS" title="Lift setup" description="Core motion values and landing positions. Changes are validated by the controller before they are stored." />
      <div className="settings-grid">
        <section className="card settings-card"><div className="card-heading"><h2>Travel speeds</h2><span className="state-badge idle">Validated</span></div><label>Normal run speed <span><input defaultValue="45.0" inputMode="decimal" /> Hz</span></label><label>Service jog speed <span><input defaultValue="10.0" inputMode="decimal" /> Hz</span></label><label>Homing speed <span><input defaultValue="8.0" inputMode="decimal" /> Hz</span></label><button className="primary-button" onClick={() => setToast("Speed settings staged for controller validation")}>Save speed settings</button></section>
        <section className="card settings-card"><div className="card-heading"><h2>Landing positions & names</h2><button className="text-button" onClick={() => { setDraftFloors(floors); setEditing(!editing); }}>{editing ? "Cancel" : "Edit positions"}</button></div>{draftFloors.map((floor) => <div className={`landing-row ${editing ? "editing" : ""}`} key={floor.floor}><b>{floor.floor}</b><div>{editing ? <><input aria-label={`Landing ${floor.floor} nickname`} value={floor.name} onChange={(event) => updateFloor(floor.floor, "name", event.target.value)} /><span className="position-input"><input aria-label={`Landing ${floor.floor} position`} inputMode="numeric" value={floor.position} onChange={(event) => updateFloor(floor.floor, "position", event.target.value)} /> counts</span></> : <><strong>{floor.name}</strong><small>{floor.position.toLocaleString()} encoder counts</small></>}</div><span className="safety-pill is-ok"><i />Set</span></div>)}{editing && <button className="primary-button" onClick={saveFloors}>Save landing configuration</button>}</section>
        <section className="card settings-card wide"><div className="card-heading"><div><span className="section-label">HOMING</span><h2>Position recovery</h2></div></div><div className="notice"><b>Home reed switch configured near Landing 3</b><p>After position is lost, the controller travels upward at homing speed until the top home reference is confirmed. The upper final limit remains independent and must not be used as the normal home reference.</p></div><button className="secondary-button" onClick={() => setToast("Homing can only start from the connected controller")}>Start guarded homing</button></section>
      </div>
    </div>
  );
}

function DrivePanel({ setToast }: { setToast: (message: string) => void }) {
  const [showAll, setShowAll] = useState(false);
  const parameters = [
    ["03", "ACC", "Acceleration ramp", "1.0 s", "Installer"], ["04", "DEC", "Deceleration ramp", "1.0 s", "Installer"],
    ["06", "IN", "Nominal motor current", "3.6 A", "Installer"], ["07", "TSOVRA", "Overload duration", "0 s", "Installer"],
    ["08", "PSOVRA", "Overload threshold", "150 %", "Installer"], ["10", "FMAX", "Maximum frequency", "120.0 Hz", "Advanced"],
    ["00", "OVERV", "Overvoltage alarm", "390 Vdc", "Advanced"], ["01", "UNDER", "Undervoltage alarm", "200 Vdc", "Advanced"],
    ["02", "TEMPALL", "Temperature alarm", "80 °C", "Installer"], ["05", "BOOST", "Voltage boost", "8 raw", "Advanced"],
    ["09", "VMAX", "Rated-voltage frequency", "50.0 Hz", "Advanced"], ["11", "FMIN", "Minimum frequency", "0.0 Hz", "Advanced"],
    ["12", "TIME", "Serial timeout", "0.1 s", "Installer"], ["13", "RELE", "Expansion relay", "Off", "Advanced"],
    ["14", "POT1", "Analog input 1", "—", "Read only"], ["15", "POT2", "Analog input 2", "—", "Read only"],
    ["16", "DAC", "Analog output", "—", "Read only"],
  ];
  const visibleParameters = showAll ? parameters : parameters.slice(0, 6);
  return (
    <div className="panel-page">
      <PanelIntro kicker="EM01 VFD" title="Drive & calibration" description="Monitor drive configuration and maintain the measured stopping-distance model used for accurate landings." />
      <section className="calibration-hero card"><div><span className="section-label">STOP CALIBRATION</span><h2>Calibration is current</h2><p>Measured July 8, 2026 at <b>45.0 Hz</b>. A run-speed or deceleration change will mark this record stale.</p></div><div className="cal-number"><strong>1,284</strong><span>counts stopping distance</span></div><button className="primary-button" onClick={() => setToast("Calibration workflow requires an idle, connected controller")}>Run new calibration</button></section>
      <section className="card load-limit-card"><div><span className="section-label">LOAD PROTECTION</span><h2>Current & overload limits</h2><p>These thresholds protect the lift from sustained overload or mechanical binding. They are not a certified weight measurement.</p></div><div className="load-values"><label>Nominal current <span><input defaultValue="3.6" inputMode="decimal" /> A</span></label><label>Trip threshold <span><input defaultValue="150" inputMode="numeric" /> %</span></label><label>Allowed duration <span><input defaultValue="2" inputMode="numeric" /> s</span></label></div><button className="secondary-button" onClick={() => setToast("Load limits staged for stopped-only VFD validation")}>Save limits</button></section>
      <section className="card parameter-card"><div className="card-heading"><div><span className="section-label">PARAMETER REGISTER</span><h2>VFD parameters</h2></div><button className="text-button" onClick={() => setShowAll(!showAll)}>{showAll ? "Show common" : "Show all 17"}</button></div><div className="parameter-header"><span>Number</span><span>Parameter</span><span>Current value</span><span>Access</span><span /></div>{visibleParameters.map((parameter) => <div className="parameter-row" key={parameter[0]}><code>P{parameter[0]}</code><div><strong>{parameter[2]}</strong><small>{parameter[1]}</small></div><b>{parameter[3]}</b><span>{parameter[4]}</span><button className="text-button" onClick={() => setToast(`${parameter[1]} editor requires a stopped, connected drive`)}>{parameter[4] === "Read only" ? "Read" : "Edit"}</button></div>)}</section>
    </div>
  );
}

function RemotesPanel({ setToast }: { setToast: (message: string) => void }) {
  const [remotes, setRemotes] = useState<RemoteRecord[]>([
    { id: "82A1-4C09", name: "Primary remote", lastSeen: "Today, 10:41", lastCommand: "Floor 2", seen: true },
    { id: "19C4-71E2", name: "Spare remote", lastSeen: "July 8, 18:22", lastCommand: "Light toggle", seen: true },
  ]);
  const [learning, setLearning] = useState(false);
  const [eraseArmed, setEraseArmed] = useState(false);
  const eraseTimer = useRef<number | null>(null);

  useEffect(() => {
    if (!learning) return;
    const timer = window.setTimeout(() => { setLearning(false); setToast("Learn window ended after 17 seconds"); }, 17000);
    return () => window.clearTimeout(timer);
  }, [learning, setToast]);

  function startLearning() {
    setLearning((current) => !current);
    setToast(learning ? "Learn line released" : "Learn line requested high; press any button on the new remote");
  }

  function renameRemote(id: string) {
    const remote = remotes.find((item) => item.id === id);
    if (!remote) return;
    const name = window.prompt("Remote nickname", remote.name);
    if (!name?.trim()) return;
    setRemotes((current) => current.map((item) => item.id === id ? { ...item, name: name.trim() } : item));
    setToast("Remote nickname updated in the WebUI registry");
  }

  function beginEraseHold() {
    eraseTimer.current = window.setTimeout(() => {
      setRemotes([]);
      setEraseArmed(false);
      setToast("Decoder erase-all sequence completed");
    }, 10000);
  }

  function cancelEraseHold() {
    if (eraseTimer.current !== null) window.clearTimeout(eraseTimer.current);
    eraseTimer.current = null;
  }

  return <div className="panel-page"><PanelIntro kicker="418 MHZ RECEIVER + MS DECODER" title="RF remotes" description="Each five-button remote can request Floor 1, Floor 2, Floor 3, Stop, or Light toggle. The WebUI registry uses TX_ID observations to associate a nickname and audit history with each detected remote." /><div className={`learn-banner ${learning ? "active" : ""}`}><div><span className="learn-indicator"><i /></span><div><strong>{learning ? "Learn mode active" : "Decoder ready"}</strong><small>{learning ? "17-second window · waiting for a valid remote transmission" : `${remotes.length} observed remote profiles · decoder capacity 40 addresses`}</small></div></div><button className={learning ? "secondary-button" : "primary-button"} onClick={startLearning}>{learning ? "Cancel learn mode" : "Pair a remote"}</button></div><section className="card table-card"><div className="remote-table-head"><span>Remote</span><span>Last command</span><span>Last seen</span><span /></div>{remotes.map((remote) => <div className="remote-row" key={remote.id}><div className="remote-icon">⌁</div><div><strong>{remote.name}</strong><small>TX ID · {remote.seen ? remote.id : "Hidden until observed"}</small></div><span>{remote.lastCommand}</span><span>{remote.lastSeen}</span><button className="text-button" onClick={() => renameRemote(remote.id)}>Rename</button></div>)}{remotes.length === 0 && <div className="empty-state"><strong>No remote profiles observed</strong><small>Pair a remote or wait for a learned transmitter to send a command.</small></div>}</section><section className="card decoder-note"><div><span className="section-label">DECODER MEMORY</span><h2>Erase learned addresses</h2><p>The decoder cannot remove one address at a time. Erasing holds LEARN high for ten seconds and clears all learned addresses; WebUI nicknames become unlinked until their TX IDs are observed again.</p></div>{!eraseArmed ? <button className="danger-outline" onClick={() => setEraseArmed(true)}>Prepare erase all</button> : <button className="hold-button" onPointerDown={beginEraseHold} onPointerUp={cancelEraseHold} onPointerLeave={cancelEraseHold} onPointerCancel={cancelEraseHold}>Hold continuously for 10 seconds</button>}</section></div>;
}

function LogsPanel({ events }: { events: EventItem[] }) {
  return <div className="panel-page"><PanelIntro kicker="AUDIT & DIAGNOSTICS" title="Event log" description="Motion, safety, configuration, network, and drive events recorded by the controller." /><div className="filter-row"><button className="filter active">All events</button><button className="filter">Motion</button><button className="filter">Safety</button><button className="filter">Configuration</button><button className="secondary-button">Export CSV</button></div><section className="card log-table"><div className="log-header"><span>Time</span><span>Event</span><span>Details</span><span>Source</span></div>{events.map((event) => <div className="log-row" key={event.id}><time>{event.time}</time><strong><i className={event.level} />{event.title}</strong><span>{event.detail}</span><code>{event.id % 2 ? "controller" : "web_ui"}</code></div>)}</section></div>;
}

function SystemPanel({ status, apiToken, setApiToken, saveToken, setToast }: { status: LiftStatus; apiToken: string; setApiToken: (token: string) => void; saveToken: () => void; setToast: (message: string) => void }) {
  const [recoveryOpen, setRecoveryOpen] = useState(false);
  return <div className="panel-page"><PanelIntro kicker="NETWORK & MAINTENANCE" title="System" description="Connection details, local API access, configuration portability, and guarded service-recovery actions." /><div className="settings-grid"><section className="card settings-card"><div className="card-heading"><h2>Network</h2><span className="state-badge idle">Online</span></div><div className="detail-list"><div><span>Mode</span><b>{titleCase(status.networkMode)}</b></div><div><span>Address</span><b>{status.stationIp || status.apIp || "Not connected"}</b></div><div><span>Hostname</span><b>lift.local</b></div><div><span>Wi-Fi band</span><b>2.4 GHz</b></div></div><button className="secondary-button" onClick={() => setToast("Network editor will use /api/network")}>Edit network</button></section><section className="card settings-card"><div className="card-heading"><h2>API authentication</h2></div><p className="helper">Stored only in this browser and sent as the X-Lift-Api-Token header.</p><label>Controller API token <span className="full-input"><input type="password" value={apiToken} onChange={(event) => setApiToken(event.target.value)} placeholder="Enter token" /></span></label><button className="primary-button" onClick={saveToken}>Save on this device</button></section><section className="card settings-card wide"><div className="card-heading"><div><span className="section-label">CONFIGURATION</span><h2>Backup & restore</h2></div></div><p className="helper">Exports controller settings, VFD parameters, landing names and positions, calibration, network preferences, and remote nickname profiles. Learned decoder addresses are hardware memory and are not copied.</p><div className="backup-warning"><strong>Remote profile reconciliation</strong><span>During restore, nicknames remain pending and their IDs stay hidden until that transmitter is observed. Unknown or removed remotes are never auto-paired.</span></div><div className="action-row"><button className="secondary-button" onClick={() => setToast("Complete configuration backup prepared")}>Back up configuration</button><button className="secondary-button" onClick={() => setToast("Restore preview will validate version, IDs, and conflicts")}>Restore from file</button></div></section><section className="card recovery-card wide"><div><span className="section-label">INSTALLER SERVICE RECOVERY</span><h2>Restricted safety-loop recovery</h2><p>This mode is only for repositioning after a diagnosed field failure. It does not bypass emergency stop, hardwired final limits, VFD faults, or the independent motion-authority chain.</p></div><button className="danger-outline" onClick={() => setRecoveryOpen(!recoveryOpen)}>{recoveryOpen ? "Close recovery panel" : "Open recovery panel"}</button>{recoveryOpen && <div className="recovery-panel"><div className="recovery-steps"><span><b>1</b> Installer re-authentication</span><span><b>2</b> Select failed monitored device and record reason</span><span><b>3</b> Cabinet-local hold-to-run input required</span><span><b>4</b> Low-speed jog only, with automatic timeout and full audit log</span></div><div className="recovery-lock"><strong>Web control alone cannot enable recovery motion</strong><small>A physical keyed/service input and continuous local hold-to-run control are required by the proposed firmware and hardware strategy.</small><button className="hold-button" disabled>Awaiting physical service input</button></div></div>}</section><section className="card settings-card wide"><div className="card-heading"><div><span className="section-label">MAINTENANCE</span><h2>Controller actions</h2></div></div><div className="action-row"><button className="danger-outline" onClick={() => setToast("Reboot requires confirmation on the connected controller")}>Reboot controller</button></div></section></div></div>;
}
