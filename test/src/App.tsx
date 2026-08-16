import { useState, useEffect, useCallback, createContext, useContext, useRef } from 'react'
import { AreaChart, Area, XAxis, YAxis, Tooltip, ResponsiveContainer, ReferenceLine, CartesianGrid } from 'recharts'

// ── Theme ─────────────────────────────────────────────────────────────────────

type Theme = 'dark' | 'light'
const ThemeCtx = createContext<{ theme: Theme; toggle: () => void }>({ theme: 'dark', toggle: () => {} })
function useTheme() { return useContext(ThemeCtx) }

function tok(theme: Theme) {
  if (theme === 'dark') return {
    bg: '#0b0f1a', surface: '#111827', surface2: '#1a2236',
    border: 'rgba(255,255,255,0.07)', borderStrong: 'rgba(255,255,255,0.13)',
    text: '#f1f5f9', textSub: '#9ca3af', textMuted: '#6b7280', textDim: '#374151',
    accent: '#22d3ee', accentDim: 'rgba(34,211,238,0.1)', accentBorder: 'rgba(34,211,238,0.2)',
    grid: 'rgba(34,211,238,0.025)', inputBg: '#0b0f1a', chartGrid: 'rgba(255,255,255,0.05)',
  }
  return {
    bg: '#e8edf4', surface: '#f6f8fb', surface2: '#dde3ec',
    border: 'rgba(0,0,0,0.07)', borderStrong: 'rgba(0,0,0,0.13)',
    text: '#0f172a', textSub: '#475569', textMuted: '#94a3b8', textDim: '#cbd5e1',
    accent: '#0891b2', accentDim: 'rgba(8,145,178,0.1)', accentBorder: 'rgba(8,145,178,0.25)',
    grid: 'rgba(8,145,178,0.035)', inputBg: '#dde3ec', chartGrid: 'rgba(0,0,0,0.05)',
  }
}

// ── Types ─────────────────────────────────────────────────────────────────────

interface SensorReading {
  id: string; label: string; value: number; unit: string
  min: number; max: number; goodMax: number; warnMax: number
  connected: boolean; port: string; addr: string; lastUpdated: number
  history: { time: string; value: number }[]
}
interface HourlySlice { time: string; temp: number; feelsLike: number; precipProb: number; precip: number; cloudCover: number; wind: number; radiation: number; wmoCode: number }
interface DailySlice { date: string; maxTemp: number; sunrise: string; sunset: string; daylightHrs: number; precipHours: number; radiationSum: number; wmoCode: number }
interface WeatherData {
  temp: number; feelsLike: number; humidity: number; windSpeed: number; cloudCover: number
  condition: string; icon: string; location: string
  hourly: HourlySlice[]; daily: DailySlice[]
  sunrise: string; sunset: string; loading: boolean; error: string | null
}
interface WiFiNetwork { ssid: string; strength: number; secured: boolean }
interface Settings { refreshRate: number; alertsEnabled: boolean }
interface QuoteData { id: number; quote: string; author: string }
interface SavedQuote { id: number; quote: string; author: string; savedAt: string }

type BlowupKind = 'quote' | 'sensor' | 'firetv' | 'weather' | 'ports' | 'sdcard' | 'hourly'
type BlowupState =
  | { kind: 'quote' }
  | { kind: 'sensor'; id: string }
  | { kind: 'firetv' }
  | { kind: 'weather'; chartIdx: number }
  | { kind: 'ports' }
  | { kind: 'sdcard' }
  | { kind: 'hourly' }

// ── Helpers ───────────────────────────────────────────────────────────────────

function wmoLabel(code: number): { label: string; icon: string } {
  if (code === 0) return { label: 'Clear Sky', icon: '☀️' }
  if (code === 1) return { label: 'Mainly Clear', icon: '🌤️' }
  if (code === 2) return { label: 'Partly Cloudy', icon: '⛅' }
  if (code === 3) return { label: 'Overcast', icon: '☁️' }
  if (code <= 48) return { label: 'Foggy', icon: '🌫️' }
  if (code <= 55) return { label: 'Drizzle', icon: '🌦️' }
  if (code <= 65) return { label: 'Rain', icon: '🌧️' }
  if (code <= 77) return { label: 'Snow', icon: '❄️' }
  if (code <= 82) return { label: 'Rain Showers', icon: '🌦️' }
  if (code <= 86) return { label: 'Snow Showers', icon: '🌨️' }
  return { label: 'Thunderstorm', icon: '⛈️' }
}
function fmtTime(iso: string) { if (!iso) return '—'; return new Date(iso).toLocaleTimeString('en-US', { hour: 'numeric', minute: '2-digit', hour12: true }) }
function fmtDayLabel(iso: string) { return new Date(iso).toLocaleDateString('en-US', { weekday: 'short', month: 'numeric', day: 'numeric' }) }

// Moon phase calculation (no API needed — pure astronomy)
const KNOWN_NEW_MOON_JD = 2451550.26  // Jan 6 2000 18:14 UTC
const SYNODIC_MONTH = 29.53058867

function getMoonPhase(date = new Date()): { phase: number; name: string; illumination: number } {
  const jd = date.getTime() / 86400000 + 2440587.5
  const raw = ((jd - KNOWN_NEW_MOON_JD) % SYNODIC_MONTH + SYNODIC_MONTH) % SYNODIC_MONTH
  const phase = raw / SYNODIC_MONTH
  const illumination = Math.round((1 - Math.cos(phase * 2 * Math.PI)) / 2 * 100)
  let name = 'New Moon'
  if (phase > 0.0625 && phase <= 0.1875) name = 'Waxing Crescent'
  else if (phase > 0.1875 && phase <= 0.3125) name = 'First Quarter'
  else if (phase > 0.3125 && phase <= 0.4375) name = 'Waxing Gibbous'
  else if (phase > 0.4375 && phase <= 0.5625) name = 'Full Moon'
  else if (phase > 0.5625 && phase <= 0.6875) name = 'Waning Gibbous'
  else if (phase > 0.6875 && phase <= 0.8125) name = 'Last Quarter'
  else if (phase > 0.8125 && phase <= 0.9375) name = 'Waning Crescent'
  return { phase, name, illumination }
}

function MoonSVG({ phase, size = 44 }: { phase: number; size?: number }) {
  const r = size * 0.42
  const cx = size / 2, cy = size / 2
  const top = { x: cx, y: cy - r }
  const bot = { x: cx, y: cy + r }
  const litColor = '#fde68a'
  const darkColor = '#1e293b'
  const strokeColor = '#334155'

  let path: string
  const termRx = Math.abs(r * Math.cos(phase * 2 * Math.PI))

  if (phase < 0.02 || phase > 0.98) {
    // New moon
    return (
      <svg viewBox={`0 0 ${size} ${size}`} width={size} height={size}>
        <circle cx={cx} cy={cy} r={r} fill={darkColor} stroke={strokeColor} strokeWidth={0.8} />
      </svg>
    )
  }
  if (Math.abs(phase - 0.5) < 0.02) {
    // Full moon
    return (
      <svg viewBox={`0 0 ${size} ${size}`} width={size} height={size}>
        <circle cx={cx} cy={cy} r={r} fill={litColor} stroke="#fbbf24" strokeWidth={0.5} />
      </svg>
    )
  }
  if (phase < 0.5) {
    // Waxing: right side lit
    const sweep2 = phase < 0.25 ? 0 : 1
    path = `M ${top.x} ${top.y} A ${r} ${r} 0 0 1 ${bot.x} ${bot.y} A ${termRx} ${r} 0 0 ${sweep2} ${top.x} ${top.y} Z`
  } else {
    // Waning: left side lit
    const sweep2 = phase < 0.75 ? 0 : 1
    path = `M ${top.x} ${top.y} A ${r} ${r} 0 0 0 ${bot.x} ${bot.y} A ${termRx} ${r} 0 0 ${sweep2} ${top.x} ${top.y} Z`
  }
  return (
    <svg viewBox={`0 0 ${size} ${size}`} width={size} height={size}>
      <circle cx={cx} cy={cy} r={r} fill={darkColor} stroke={strokeColor} strokeWidth={0.8} />
      <path d={path} fill={litColor} opacity={0.92} />
    </svg>
  )
}

function generateHistory(base: number, count = 48): { time: string; value: number }[] {
  const now = new Date()
  return Array.from({ length: count }, (_, i) => {
    const t = new Date(now.getTime() - (count - 1 - i) * 30 * 60 * 1000)
    const jitter = (Math.random() - 0.5) * 0.15
    const day = Math.sin((t.getHours() - 6) * Math.PI / 12) * 0.18
    const v = Math.max(0, base * (1 + day + jitter))
    return { time: `${String(t.getHours()).padStart(2,'0')}:${String(t.getMinutes()).padStart(2,'0')}`, value: parseFloat(v.toFixed(v < 1 ? 3 : 0)) }
  })
}

function getStatus(v: number, gm: number, wm: number): 'good' | 'warn' | 'danger' { return v <= gm ? 'good' : v <= wm ? 'warn' : 'danger' }
const SC = { good: '#22c55e', warn: '#f59e0b', danger: '#ef4444' }
const SL = { good: 'GOOD', warn: 'MODERATE', danger: 'POOR' }

function computeAQI(sensors: SensorReading[]) {
  const a = sensors.filter(s => s.connected)
  if (!a.length) return { score: 0, label: 'NO DATA', color: '#6b7280' }
  const avg = a.reduce((acc, s) => acc + Math.min(1, (s.value - s.min) / (s.max - s.min)), 0) / a.length
  const score = Math.round((1 - avg) * 100)
  if (score >= 80) return { score, label: 'EXCELLENT', color: '#22c55e' }
  if (score >= 60) return { score, label: 'GOOD', color: '#84cc16' }
  if (score >= 40) return { score, label: 'MODERATE', color: '#f59e0b' }
  if (score >= 20) return { score, label: 'POOR', color: '#f97316' }
  return { score, label: 'HAZARDOUS', color: '#ef4444' }
}

const WIFI_NETWORKS: WiFiNetwork[] = [
  { ssid: 'HomeNet_5G', strength: 95, secured: true },
  { ssid: 'TP-Link_2.4G', strength: 72, secured: true },
  { ssid: 'Xfinity_WiFi', strength: 58, secured: false },
  { ssid: 'Neighbor_Net', strength: 41, secured: true },
]

const INITIAL_SENSORS: SensorReading[] = [
  // SGP40 — VOC index
  { id: 'voc', label: 'VOC', value: 68, unit: 'idx', min: 0, max: 500, goodMax: 100, warnMax: 250, connected: true, port: 'I2C', addr: '0x59', lastUpdated: Date.now(), history: generateHistory(68) },
  // SHT31 — Humidity + Temp (combined sensor)
  { id: 'hum', label: 'Humidity', value: 54.2, unit: '%RH', min: 0, max: 100, goodMax: 60, warnMax: 80, connected: true, port: 'I2C', addr: '0x44', lastUpdated: Date.now(), history: generateHistory(54.2) },
  { id: 'temp', label: 'Temp', value: 72.8, unit: '°F', min: 32, max: 120, goodMax: 77, warnMax: 86, connected: true, port: 'I2C', addr: '0x44', lastUpdated: Date.now(), history: generateHistory(72.8) },
  // MiCS-4514 — multi-gas
  { id: 'co', label: 'CO', value: 4.2, unit: 'ppm', min: 1, max: 1000, goodMax: 35, warnMax: 200, connected: true, port: 'I2C', addr: '0x75', lastUpdated: Date.now(), history: generateHistory(4.2) },
  { id: 'no2', label: 'NO₂', value: 0.08, unit: 'ppm', min: 0.05, max: 10, goodMax: 0.1, warnMax: 0.5, connected: true, port: 'I2C', addr: '0x75', lastUpdated: Date.now(), history: generateHistory(0.08) },
  { id: 'eth', label: 'C₂H₅OH', value: 18, unit: 'ppm', min: 10, max: 500, goodMax: 50, warnMax: 200, connected: true, port: 'I2C', addr: '0x75', lastUpdated: Date.now(), history: generateHistory(18) },
  { id: 'h2', label: 'H₂', value: 6, unit: 'ppm', min: 1, max: 1000, goodMax: 50, warnMax: 300, connected: true, port: 'I2C', addr: '0x75', lastUpdated: Date.now(), history: generateHistory(6) },
  { id: 'nh3', label: 'NH₃', value: 2.1, unit: 'ppm', min: 1, max: 500, goodMax: 25, warnMax: 50, connected: true, port: 'I2C', addr: '0x75', lastUpdated: Date.now(), history: generateHistory(2.1) },
  { id: 'ch4', label: 'CH₄', value: 1080, unit: 'ppm', min: 1000, max: 5000, goodMax: 1500, warnMax: 3000, connected: true, port: 'I2C', addr: '0x75', lastUpdated: Date.now(), history: generateHistory(1080) },
]

const DEFAULT_SETTINGS: Settings = { refreshRate: 3, alertsEnabled: true }

// ── Sun Face AQI ──────────────────────────────────────────────────────────────

function AQISunFace({ score, color, size = 64 }: { score: number; color: string; size?: number }) {
  const sm = Math.max(-0.5, Math.min(1, (score - 30) / 70))
  const cx = 40, cy = 40, face = 22, mxL = 31, mxR = 49, mBaseY = 47, mCtrlY = mBaseY + sm * 8
  const rays = Array.from({ length: 8 }, (_, i) => {
    const a = (i * 45 * Math.PI) / 180
    return { x1: cx + Math.cos(a) * 27, y1: cy + Math.sin(a) * 27, x2: cx + Math.cos(a) * 36, y2: cy + Math.sin(a) * 36 }
  })
  return (
    <svg viewBox="0 0 80 80" width={size} height={size} style={{ flexShrink: 0 }}>
      <circle cx={cx} cy={cy} r={34} fill={color} opacity="0.08" />
      {rays.map((r, i) => <line key={i} x1={r.x1} y1={r.y1} x2={r.x2} y2={r.y2} stroke={color} strokeWidth="2.5" strokeLinecap="round" opacity="0.75" />)}
      <circle cx={cx} cy={cy} r={face} fill={color} opacity="0.88" />
      <circle cx={cx - 7} cy={cy - 3} r="2.5" fill="rgba(0,0,0,0.55)" />
      <circle cx={cx + 7} cy={cy - 3} r="2.5" fill="rgba(0,0,0,0.55)" />
      {sm < 0.1 && <><circle cx={cx - 7} cy={cy - 4.5} r={1} fill="rgba(0,0,0,0.3)" /><circle cx={cx + 7} cy={cy - 4.5} r={1} fill="rgba(0,0,0,0.3)" /></>}
      <path d={`M ${mxL} ${mBaseY} Q 40 ${mCtrlY} ${mxR} ${mBaseY}`} fill="none" stroke="rgba(0,0,0,0.55)" strokeWidth="2" strokeLinecap="round" />
    </svg>
  )
}

// ── WiFi Bar ──────────────────────────────────────────────────────────────────

function WifiBar({ strength, color = '#22d3ee' }: { strength: number; color?: string }) {
  return (
    <div style={{ display: 'flex', gap: 2, alignItems: 'flex-end', height: 12 }}>
      {[25, 50, 75, 100].map((t, i) => (
        <div key={i} style={{ width: 3, height: 3 + i * 3, borderRadius: 1, background: strength >= t ? color : 'rgba(128,128,128,0.2)' }} />
      ))}
    </div>
  )
}

// ── WiFi Screen ───────────────────────────────────────────────────────────────

function WiFiScreen({ onConnect }: { onConnect: (ssid: string) => void }) {
  const { theme } = useTheme()
  const t = tok(theme)
  const [sel, setSel] = useState<WiFiNetwork | null>(null)
  const [pw, setPw] = useState('')
  const [busy, setBusy] = useState(false)
  const [err, setErr] = useState('')

  const doConnect = () => {
    if (!sel) return
    if (sel.secured && !pw) { setErr('Password required'); return }
    setErr(''); setBusy(true)
    setTimeout(() => { setBusy(false); onConnect(sel.ssid) }, 1800)
  }

  return (
    <div style={{ width: 800, height: 400, background: t.bg, display: 'flex', alignItems: 'center', justifyContent: 'center', fontFamily: 'Outfit, sans-serif', overflow: 'hidden', position: 'relative' }}>
      <div style={{ position: 'absolute', inset: 0, backgroundImage: `linear-gradient(${t.grid} 1px,transparent 1px),linear-gradient(90deg,${t.grid} 1px,transparent 1px)`, backgroundSize: '40px 40px', pointerEvents: 'none' }} />
      <div style={{ display: 'flex', gap: 20, width: 680, position: 'relative', zIndex: 1 }}>
        <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', width: 150, gap: 10 }}>
          <AQISunFace score={85} color="#22d3ee" size={76} />
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 13, color: t.accent, letterSpacing: '0.14em', textAlign: 'center' }}>AIRWATCH<br />PRO</div>
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted, letterSpacing: '0.08em', textAlign: 'center' }}>ESP32-S3 · v3.2.1</div>
        </div>
        <div style={{ flex: 1, background: t.surface, border: `1px solid ${t.border}`, borderRadius: 14, overflow: 'hidden' }}>
          <div style={{ padding: '9px 14px', borderBottom: `1px solid ${t.border}`, fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.textMuted, letterSpacing: '0.12em' }}>CONNECT TO WIFI</div>
          <div style={{ maxHeight: 240, overflowY: 'auto' }}>
            {WIFI_NETWORKS.map(net => (
              <button key={net.ssid} onClick={() => { setSel(net); setPw(''); setErr('') }}
                style={{ width: '100%', padding: '9px 14px', background: sel?.ssid === net.ssid ? t.accentDim : 'transparent', border: 'none', borderBottom: `1px solid ${t.border}`, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 10, textAlign: 'left' }}>
                <WifiBar strength={net.strength} color={t.accent} />
                <span style={{ flex: 1, fontSize: 13, color: t.text, fontWeight: 500 }}>{net.ssid}</span>
                <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.textMuted }}>{net.secured ? '🔒' : 'OPEN'}</span>
              </button>
            ))}
          </div>
          {sel?.secured && (
            <div style={{ padding: '10px 14px', borderTop: `1px solid ${t.border}` }}>
              <input type="password" value={pw} onChange={e => { setPw(e.target.value); setErr('') }}
                onKeyDown={e => e.key === 'Enter' && doConnect()} placeholder={`Password for ${sel.ssid}`} autoFocus
                style={{ width: '100%', padding: '7px 10px', background: t.inputBg, border: `1px solid ${err ? '#ef4444' : t.borderStrong}`, borderRadius: 7, color: t.text, fontSize: 12, fontFamily: 'DM Mono, monospace', outline: 'none', boxSizing: 'border-box' }} />
              {err && <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: '#ef4444', marginTop: 4 }}>{err}</div>}
            </div>
          )}
          <div style={{ padding: '10px 14px', borderTop: `1px solid ${t.border}` }}>
            <button onClick={doConnect} disabled={!sel || busy}
              style={{ width: '100%', padding: '9px', background: sel && !busy ? t.accent : t.surface2, border: 'none', borderRadius: 8, cursor: sel && !busy ? 'pointer' : 'not-allowed', color: sel && !busy ? (theme === 'dark' ? '#0b0f1a' : '#fff') : t.textMuted, fontSize: 13, fontWeight: 600, display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 8 }}>
              {busy ? <><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" style={{ animation: 'spin 1s linear infinite' }}><path d="M12 2v4M12 18v4M4.93 4.93l2.83 2.83M16.24 16.24l2.83 2.83M2 12h4M18 12h4M4.93 19.07l2.83-2.83M16.24 7.76l2.83-2.83" /></svg>Connecting…</> : sel ? `Connect to ${sel.ssid}` : 'Select a network'}
            </button>
          </div>
        </div>
      </div>
      <style>{`@keyframes spin{from{transform:rotate(0deg)}to{transform:rotate(360deg)}}`}</style>
    </div>
  )
}

// ── Sensor Chip (2-col grid style) ────────────────────────────────────────────

function SensorChip({ sensor, active, onClick }: { sensor: SensorReading; active: boolean; onClick: () => void }) {
  const { theme } = useTheme()
  const t = tok(theme)
  const status = sensor.connected ? getStatus(sensor.value, sensor.goodMax, sensor.warnMax) : null
  const color = status ? SC[status] : t.textDim
  const pct = Math.min(100, ((sensor.value - sensor.min) / (sensor.max - sensor.min)) * 100)
  const dp = sensor.value < 1 ? 2 : sensor.unit === '°F' || sensor.unit === '%RH' ? 1 : 0

  return (
    <button onClick={onClick} style={{
      padding: '5px 7px', background: active ? t.accentDim : t.surface,
      border: `1px solid ${active ? t.accentBorder : status ? `${SC[status]}35` : t.border}`,
      borderRadius: 8, cursor: 'pointer', display: 'flex', flexDirection: 'column', gap: 2,
      textAlign: 'left', transition: 'all 0.15s', width: '100%', boxSizing: 'border-box', flex: 1,
    }}>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
        <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: active ? t.accent : t.textMuted, letterSpacing: '0.06em' }}>{sensor.label}</span>
        <div style={{ display: 'flex', alignItems: 'center', gap: 3 }}>
          <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color: t.textDim }}>{sensor.addr}</span>
          <div style={{ width: 5, height: 5, borderRadius: '50%', background: sensor.connected ? color : t.textDim, boxShadow: sensor.connected ? `0 0 4px ${color}70` : 'none' }} />
        </div>
      </div>
      <div style={{ display: 'flex', alignItems: 'baseline', gap: 4 }}>
        <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 16, fontWeight: 500, color, lineHeight: 1, letterSpacing: '-0.02em' }}>
          {sensor.connected ? sensor.value.toFixed(dp) : '—'}
        </span>
        <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted }}>{sensor.unit}</span>
      </div>
      {status && <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color, padding: '1px 4px', background: `${color}15`, borderRadius: 3, alignSelf: 'flex-start', letterSpacing: '0.06em' }}>{SL[status]}</div>}
      <div style={{ height: 2, background: t.chartGrid, borderRadius: 1, marginTop: 1 }}>
        {sensor.connected && <div style={{ height: '100%', width: `${pct}%`, background: color, borderRadius: 1, transition: 'width 0.5s' }} />}
      </div>
    </button>
  )
}

// ── AQI Widget ────────────────────────────────────────────────────────────────

function AQIWidget({ score, label, color }: { score: number; label: string; color: string }) {
  const { theme } = useTheme()
  const t = tok(theme)
  return (
    <div style={{ width: '100%', background: t.surface, border: `1px solid ${t.border}`, borderRadius: 10, padding: '8px', display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 2 }}>
      <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted, letterSpacing: '0.1em' }}>INDOOR AQI</div>
      <AQISunFace score={score} color={color} size={62} />
      <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 24, fontWeight: 500, color, lineHeight: 1, letterSpacing: '-0.02em' }}>{score}</div>
      <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color, letterSpacing: '0.12em', padding: '2px 8px', background: `${color}18`, borderRadius: 4 }}>{label}</div>
    </div>
  )
}

// ── System Widget ─────────────────────────────────────────────────────────────

function SystemWidget({ ssid }: { ssid: string }) {
  const { theme } = useTheme()
  const t = tok(theme)
  const cpus = [
    { name: 'DISPLAY CPU', temp: 112, ram: 180, ramTotal: 320, psram: 64, psramUsed: 12 },
    { name: 'DSTAMPS3', temp: 107, ram: 142, ramTotal: 512, psram: 8192, psramUsed: 2048 },
  ]
  return (
    <div style={{ width: '100%', background: t.surface, border: `1px solid ${t.border}`, borderRadius: 10, padding: '7px 8px', display: 'flex', flexDirection: 'column', gap: 4, flex: 1 }}>
      <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted, letterSpacing: '0.1em' }}>SYSTEM</div>
      {cpus.map(cpu => (
        <div key={cpu.name} style={{ background: t.surface2, borderRadius: 7, padding: '5px 7px' }}>
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color: t.accent, letterSpacing: '0.08em', marginBottom: 4 }}>{cpu.name}</div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
            {[['TEMP', `${cpu.temp} °F`], ['RAM', `${cpu.ram}/${cpu.ramTotal} K`], ['FREE', `${cpu.ramTotal - cpu.ram} K`], ['PSRAM', `${cpu.psramUsed}/${cpu.psram} K`]].map(([l, v]) => (
              <div key={l} style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', gap: 4 }}>
                <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color: t.textMuted, flexShrink: 0 }}>{l}</span>
                <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color: t.textSub, fontWeight: 600, textAlign: 'right', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap', minWidth: 0 }}>{v}</span>
              </div>
            ))}
          </div>
        </div>
      ))}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', padding: '2px 2px 0' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 5 }}>
          <WifiBar strength={85} color={t.accent} />
          <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted, maxWidth: 74, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{ssid}</span>
        </div>
        <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textSub }}>−52 dBm</span>
      </div>
    </div>
  )
}

// ── Motivational Quote Blowup ─────────────────────────────────────────────────

function QuoteBlowup({ quote, onSave, saved }: { quote: QuoteData | null; onSave: () => void; saved: boolean }) {
  const { theme } = useTheme()
  const t = tok(theme)
  const [flash, setFlash] = useState(false)

  const handleSave = () => {
    if (saved) return
    onSave()
    setFlash(true)
    setTimeout(() => setFlash(false), 1800)
  }

  if (!quote) return (
    <div style={{ width: '100%', height: '100%', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
      <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke={t.textMuted} strokeWidth="2" style={{ animation: 'spin 1s linear infinite' }}><path d="M12 2v4M12 18v4M4.93 4.93l2.83 2.83M16.24 16.24l2.83 2.83M2 12h4M18 12h4M4.93 19.07l2.83-2.83M16.24 7.76l2.83-2.83" /></svg>
    </div>
  )

  return (
    <button onClick={handleSave} style={{ width: '100%', height: '100%', background: 'none', border: 'none', cursor: 'pointer', display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', padding: '14px 16px', gap: 10, textAlign: 'center', position: 'relative' }}>
      {/* Quote marks */}
      <div style={{ fontFamily: 'Georgia, serif', fontSize: 48, color: t.accent, opacity: 0.25, lineHeight: 0.7, alignSelf: 'flex-start', marginLeft: 4 }}>"</div>
      <p style={{ margin: 0, fontSize: 13, color: t.text, lineHeight: 1.55, fontStyle: 'italic', flex: 1, display: 'flex', alignItems: 'center' }}>
        {quote.quote}
      </p>
      <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.accent, letterSpacing: '0.1em' }}>— {quote.author}</div>
      {/* Save hint / confirmation */}
      <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: flash ? '#22c55e' : t.textDim, letterSpacing: '0.1em', transition: 'color 0.3s' }}>
        {flash ? '✓ SAVED TO SD CARD' : saved ? '✓ ALREADY SAVED' : 'TAP TO SAVE TO SD'}
      </div>
    </button>
  )
}

// ── Ports Blowup ──────────────────────────────────────────────────────────────

function PortsBlowup({ sensors }: { sensors: SensorReading[] }) {
  const { theme } = useTheme()
  const t = tok(theme)
  const i2cDevices = [
    { addr: '0x59', chip: 'SGP40', channels: ['VOC Index'] },
    { addr: '0x44', chip: 'SHT31', channels: ['Humidity', 'Temp'] },
    { addr: '0x75', chip: 'MiCS-4514', channels: ['CO', 'NO₂', 'C₂H₅OH', 'H₂', 'NH₃', 'CH₄'] },
  ]
  const ports = [
    { name: 'UART 0', icon: '⌨', detail: 'Debug / Flash console', baud: '115200', online: true, color: '#22c55e' },
    { name: 'UART 1', icon: '📡', detail: 'GPS module (NEO-8M)', baud: '9600', online: false, color: '#6b7280' },
    { name: 'I2C', icon: '🔗', detail: '3 devices · 9 channels', baud: '400kHz', online: true, color: '#22d3ee' },
    { name: 'USB-C', icon: '⚡', detail: 'Power + data bridge', baud: 'USB 2.0', online: true, color: '#a78bfa' },
  ]
  return (
    <div style={{ width: '100%', height: '100%', display: 'flex', flexDirection: 'column', padding: '8px 10px', gap: 6, overflowY: 'auto' }}>
      <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.textMuted, letterSpacing: '0.12em', flexShrink: 0 }}>PORT STATUS</div>
      {ports.map(p => (
        <div key={p.name} style={{ background: t.surface2, border: `1px solid ${p.online ? `${p.color}35` : t.border}`, borderRadius: 8, padding: '7px 10px', flexShrink: 0 }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 7, marginBottom: p.name === 'I2C' ? 6 : 0 }}>
            <div style={{ width: 7, height: 7, borderRadius: '50%', background: p.color, boxShadow: p.online ? `0 0 5px ${p.color}80` : 'none', animation: p.online ? 'pulse 2s infinite' : 'none', flexShrink: 0 }} />
            <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 11, color: p.online ? t.text : t.textMuted, fontWeight: 600 }}>{p.name}</span>
            <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted, marginLeft: 'auto' }}>{p.baud}</span>
          </div>
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted, marginLeft: 14 }}>{p.detail}</div>
          {p.name === 'I2C' && (
            <div style={{ marginTop: 5, display: 'flex', flexDirection: 'column', gap: 3 }}>
              {i2cDevices.map(dev => {
                const devSensors = sensors.filter(s => s.addr === dev.addr)
                const ok = devSensors.some(s => s.connected)
                return (
                  <div key={dev.addr} style={{ display: 'flex', alignItems: 'center', gap: 6, padding: '3px 8px', background: t.surface, borderRadius: 5, border: `1px solid ${t.border}` }}>
                    <div style={{ width: 5, height: 5, borderRadius: '50%', background: ok ? '#22d3ee' : '#6b7280', flexShrink: 0 }} />
                    <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.accent }}>{dev.addr}</span>
                    <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textSub }}>{dev.chip}</span>
                    <div style={{ display: 'flex', gap: 3, marginLeft: 'auto', flexWrap: 'wrap', justifyContent: 'flex-end' }}>
                      {dev.channels.map(ch => <span key={ch} style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color: t.textMuted, background: t.surface2, borderRadius: 3, padding: '1px 3px' }}>{ch}</span>)}
                    </div>
                  </div>
                )
              })}
            </div>
          )}
        </div>
      ))}
    </div>
  )
}

// ── SD Card Blowup ────────────────────────────────────────────────────────────

function SDCardBlowup({ saved, onClear }: { saved: SavedQuote[]; onClear: () => void }) {
  const { theme } = useTheme()
  const t = tok(theme)
  const usedKB = saved.reduce((a, q) => a + q.quote.length * 2 / 1024, 0)
  return (
    <div style={{ width: '100%', height: '100%', display: 'flex', flexDirection: 'column', padding: '8px 10px', gap: 6 }}>
      {/* Header */}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexShrink: 0 }}>
        <div>
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.textMuted, letterSpacing: '0.12em' }}>SD CARD · SAVED QUOTES</div>
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textSub, marginTop: 1 }}>{saved.length} entries · {usedKB.toFixed(2)} KB used</div>
        </div>
        {saved.length > 0 && (
          <button onClick={onClear} style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: '#ef4444', background: 'rgba(239,68,68,0.1)', border: '1px solid rgba(239,68,68,0.25)', borderRadius: 5, padding: '3px 8px', cursor: 'pointer' }}>CLEAR</button>
        )}
      </div>
      {/* Storage bar */}
      <div style={{ height: 3, background: t.chartGrid, borderRadius: 2, flexShrink: 0 }}>
        <div style={{ height: '100%', width: `${Math.min(100, usedKB / 40.96 * 100)}%`, background: t.accent, borderRadius: 2, transition: 'width 0.5s' }} />
      </div>
      {/* List */}
      <div style={{ flex: 1, overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: 5 }}>
        {saved.length === 0 ? (
          <div style={{ flex: 1, display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 6 }}>
            <div style={{ fontSize: 28 }}>💾</div>
            <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.textMuted, textAlign: 'center', letterSpacing: '0.08em' }}>NO SAVED QUOTES<br />Tap a quote to save it</div>
          </div>
        ) : (
          saved.map((q, i) => (
            <div key={i} style={{ background: t.surface2, border: `1px solid ${t.border}`, borderRadius: 7, padding: '7px 9px', flexShrink: 0 }}>
              <div style={{ fontSize: 11, color: t.text, fontStyle: 'italic', lineHeight: 1.4, marginBottom: 4 }}>"{q.quote.length > 90 ? q.quote.slice(0, 90) + '…' : q.quote}"</div>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.accent }}>— {q.author}</span>
                <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted }}>{q.savedAt}</span>
              </div>
            </div>
          ))
        )}
      </div>
    </div>
  )
}

// ── FireTV Remote ─────────────────────────────────────────────────────────────

function FireTVRemote() {
  const { theme } = useTheme()
  const t = tok(theme)
  const [pressed, setPressed] = useState<string | null>(null)
  const [btDevice, setBtDevice] = useState('Fire TV Stick 4K · Living Room')
  const [btOpen, setBtOpen] = useState(false)
  const BT_DEVICES = [
    { name: 'Fire TV Stick 4K · Living Room', rssi: -42 },
    { name: 'Fire TV Cube · Bedroom', rssi: -61 },
    { name: 'Echo Show 10 · Kitchen', rssi: -68 },
    { name: 'Fire TV Stick Lite · Office', rssi: -75 },
  ]

  const Btn = ({ id, label, sz = 36, accent = false, wide = false }: { id: string; label: string; sz?: number; accent?: boolean; wide?: boolean }) => {
    const on = pressed === id
    return (
      <button onPointerDown={() => setPressed(id)} onPointerUp={() => setPressed(null)} onPointerLeave={() => setPressed(null)}
        style={{ width: wide ? 'auto' : sz, minWidth: wide ? 52 : undefined, height: sz, borderRadius: 7, border: `1px solid ${accent ? t.accentBorder : t.borderStrong}`, background: on ? (accent ? t.accent : t.surface2) : (accent ? t.accentDim : t.surface), cursor: 'pointer', display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 4, fontSize: 12, color: accent ? t.accent : t.textSub, fontFamily: 'DM Mono, monospace', transition: 'all 0.08s', transform: on ? 'scale(0.92)' : 'scale(1)', flexShrink: 0, letterSpacing: '0.04em', padding: wide ? '0 10px' : undefined }}>
        {label}
      </button>
    )
  }

  const ar = 44
  return (
    <div style={{ width: '100%', height: '100%', display: 'flex', flexDirection: 'column', padding: '8px 12px', gap: 6 }}>

      {/* BT device header + dropdown */}
      <div style={{ flexShrink: 0, position: 'relative' }}>
        <button onClick={() => setBtOpen(o => !o)} style={{ width: '100%', display: 'flex', alignItems: 'center', gap: 7, padding: '5px 8px', background: btOpen ? t.accentDim : t.surface2, borderRadius: btOpen ? '7px 7px 0 0' : 7, border: `1px solid ${btOpen ? t.accentBorder : t.border}`, cursor: 'pointer', textAlign: 'left' }}>
          <div style={{ width: 7, height: 7, borderRadius: '50%', background: '#60a5fa', boxShadow: '0 0 5px #60a5fa80', flexShrink: 0, animation: 'pulse 2s infinite' }} />
          <div style={{ flex: 1, minWidth: 0 }}>
            <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color: t.textMuted, letterSpacing: '0.1em' }}>BLUETOOTH CONNECTED</div>
            <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.text, fontWeight: 600, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{btDevice}</div>
          </div>
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted, transition: 'transform 0.2s', transform: btOpen ? 'rotate(180deg)' : 'none' }}>▾</div>
        </button>
        {btOpen && (
          <div style={{ position: 'absolute', top: '100%', left: 0, right: 0, background: t.surface, border: `1px solid ${t.accentBorder}`, borderTop: 'none', borderRadius: '0 0 7px 7px', zIndex: 10, overflow: 'hidden' }}>
            {BT_DEVICES.map(d => {
              const active = d.name === btDevice
              const bars = d.rssi > -55 ? 4 : d.rssi > -65 ? 3 : d.rssi > -72 ? 2 : 1
              return (
                <button key={d.name} onClick={() => { setBtDevice(d.name); setBtOpen(false) }}
                  style={{ width: '100%', padding: '6px 8px', background: active ? t.accentDim : 'transparent', border: 'none', borderBottom: `1px solid ${t.border}`, cursor: 'pointer', display: 'flex', alignItems: 'center', gap: 7, textAlign: 'left' }}>
                  <div style={{ display: 'flex', gap: 1, alignItems: 'flex-end', height: 10, flexShrink: 0 }}>
                    {[1,2,3,4].map(i => <div key={i} style={{ width: 2, height: 2 + i * 2, borderRadius: 1, background: i <= bars ? '#60a5fa' : t.textDim }} />)}
                  </div>
                  <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: active ? t.accent : t.text, flex: 1, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{d.name}</span>
                  {active && <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color: '#60a5fa' }}>●</span>}
                </button>
              )
            })}
          </div>
        )}
      </div>

      {/* D-pad — 3-col grid: ◀ | ▲/OK/▼ | ▶ with ⏻ at top-right */}
      <div style={{ display: 'grid', gridTemplateColumns: `${ar}px ${ar}px ${ar}px`, gridTemplateRows: `${ar}px ${ar}px ${ar}px`, gap: 5, justifyContent: 'center', flexShrink: 0 }}>
        {/* row 0 */}
        <div />
        <Btn id="up" label="▲" sz={ar} />
        <Btn id="pwr" label="⏻" sz={ar} />
        {/* row 1 */}
        <Btn id="left" label="◀" sz={ar} />
        <button onPointerDown={() => setPressed('ok')} onPointerUp={() => setPressed(null)} onPointerLeave={() => setPressed(null)}
          style={{ width: ar, height: ar, borderRadius: '50%', border: `2px solid ${t.accentBorder}`, background: pressed === 'ok' ? t.accent : t.accentDim, cursor: 'pointer', fontFamily: 'DM Mono, monospace', fontSize: 11, color: t.accent, fontWeight: 700, transform: pressed === 'ok' ? 'scale(0.9)' : 'scale(1)', transition: 'all 0.08s', flexShrink: 0 }}>
          OK
        </button>
        <Btn id="right" label="▶" sz={ar} />
        {/* row 2 */}
        <div />
        <Btn id="down" label="▼" sz={ar} />
        <div />
      </div>

      {/* Media row — col widths match d-pad cols so ⏯ center = ▼ center */}
      <div style={{ display: 'grid', gridTemplateColumns: `${ar}px ${ar}px ${ar}px`, gap: 5, justifyContent: 'center', flexShrink: 0 }}>
        <Btn id="rew" label="⏮" sz={ar} />
        <Btn id="play" label="⏯" sz={ar} accent />
        <Btn id="fwd" label="⏭" sz={ar} />
      </div>

      {/* Volume row with mute — same grid footprint */}
      <div style={{ display: 'grid', gridTemplateColumns: `${ar}px ${ar}px ${ar}px`, gap: 5, justifyContent: 'center', flexShrink: 0 }}>
        <Btn id="volD" label="🔉" sz={ar} />
        <Btn id="mute" label="🔇" sz={ar} />
        <Btn id="volU" label="🔊" sz={ar} />
      </div>

      {/* Nav row: Home · Back · Menu · Input */}
      {/* Nav + Apps pinned to bottom */}
      <div style={{ display: 'flex', flexDirection: 'column', gap: 6, marginTop: 'auto' }}>
        <div style={{ display: 'flex', gap: 5, justifyContent: 'center' }}>
          <Btn id="home" label="⌂ HOME" sz={22} accent wide />
          <Btn id="back" label="↩ BACK" sz={22} wide />
          <Btn id="menu" label="☰" sz={22} />
          <Btn id="input" label="INPUT" sz={22} wide />
        </div>
        <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color: t.textMuted, letterSpacing: '0.1em', textAlign: 'center' }}>APPS</div>
          <div style={{ display: 'flex', gap: 5, justifyContent: 'center' }}>
            <Btn id="prime" label="📦 Prime" sz={22} wide />
            <Btn id="yt" label="▶ YouTube" sz={22} wide />
            <Btn id="fox" label="🦊 FOX" sz={22} wide />
          </div>
        </div>
      </div>

    </div>
  )
}

// ── Blowup Widget (center panel) ──────────────────────────────────────────────

function buildWeatherCharts(w: WeatherData) {
  if (!w.hourly.length) return []
  return [
    { key: 'temp' as const, title: '🌡️ Temperature (°F)', color: '#f97316' },
    { key: 'precipProb' as const, title: '🌧️ Precip Probability (%)', color: '#60a5fa' },
    { key: 'radiation' as const, title: '☀️ Solar Irradiance (W/m²)', color: '#f59e0b' },
    { key: 'cloudCover' as const, title: '☁️ Cloud Cover (%)', color: '#94a3b8' },
    { key: 'wind' as const, title: '💨 Wind Speed (mph)', color: '#a78bfa' },
  ]
}

// ── Hourly Forecast Blowup ────────────────────────────────────────────────────

function HourlyBlowup({ hourly }: { hourly: HourlySlice[] }) {
  const { theme } = useTheme()
  const t = tok(theme)
  const moon = getMoonPhase()
  return (
    <div style={{ width: '100%', height: '100%', display: 'flex', flexDirection: 'column', padding: '8px 10px', gap: 4 }}>
      {/* Header row: title + moon phase */}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexShrink: 0 }}>
        <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.textMuted, letterSpacing: '0.12em' }}>TODAY · HOURLY FORECAST</div>
        <div style={{ display: 'flex', alignItems: 'center', gap: 6, background: t.surface2, border: `1px solid ${t.border}`, borderRadius: 7, padding: '3px 8px 3px 4px' }}>
          <MoonSVG phase={moon.phase} size={32} />
          <div>
            <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted, letterSpacing: '0.06em' }}>{moon.name.toUpperCase()}</div>
            <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: '#fde68a', fontWeight: 600 }}>{moon.illumination}% lit</div>
          </div>
        </div>
      </div>
      {/* Mini dual chart */}
      <div style={{ height: 80, flexShrink: 0 }}>
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={hourly} margin={{ top: 4, right: 4, bottom: 0, left: 0 }}>
            <defs>
              <linearGradient id="htgrad" x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor="#f97316" stopOpacity={0.35} />
                <stop offset="100%" stopColor="#f97316" stopOpacity={0} />
              </linearGradient>
              <linearGradient id="hpgrad" x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stopColor="#60a5fa" stopOpacity={0.35} />
                <stop offset="100%" stopColor="#60a5fa" stopOpacity={0} />
              </linearGradient>
            </defs>
            <XAxis dataKey="time" tick={{ fontFamily: 'DM Mono, monospace', fontSize: 7, fill: t.textMuted }} axisLine={false} tickLine={false} interval={3} />
            <YAxis yAxisId="t" tick={{ fontFamily: 'DM Mono, monospace', fontSize: 7, fill: t.textMuted }} axisLine={false} tickLine={false} width={26} />
            <YAxis yAxisId="p" orientation="right" tick={{ fontFamily: 'DM Mono, monospace', fontSize: 7, fill: '#60a5fa' }} axisLine={false} tickLine={false} width={24} domain={[0, 100]} />
            <CartesianGrid strokeDasharray="2 4" stroke={t.chartGrid} vertical={false} />
            <Tooltip contentStyle={{ background: t.surface, border: `1px solid ${t.borderStrong}`, borderRadius: 6, fontFamily: 'DM Mono, monospace', fontSize: 9 }} labelStyle={{ color: t.textMuted }} />
            <Area yAxisId="t" type="monotone" dataKey="temp" stroke="#f97316" strokeWidth={1.5} fill="url(#htgrad)" dot={false} name="°F" />
            <Area yAxisId="p" type="monotone" dataKey="precipProb" stroke="#60a5fa" strokeWidth={1.5} fill="url(#hpgrad)" dot={false} name="Precip%" />
          </AreaChart>
        </ResponsiveContainer>
      </div>
      {/* Hourly rows */}
      <div style={{ flex: 1, overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: 2 }}>
        {hourly.map((h, i) => {
          const barW = (h.precipProb / 100) * 100
          const wmo = wmoLabel(h.wmoCode)
          return (
            <div key={i} style={{ display: 'grid', gridTemplateColumns: '44px 14px 36px 36px 1fr 32px', alignItems: 'center', gap: 5, padding: '3px 4px', background: i % 2 === 0 ? t.surface2 : 'transparent', borderRadius: 5 }}>
              <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted }}>{h.time}</span>
              <span style={{ fontSize: 10 }}>{wmo.icon}</span>
              <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: '#f97316', textAlign: 'right' }}>{h.temp}°F</span>
              <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: '#60a5fa', textAlign: 'right' }}>{h.precipProb}%</span>
              <div style={{ height: 4, background: t.chartGrid, borderRadius: 2 }}>
                <div style={{ height: '100%', width: `${barW}%`, background: h.precipProb > 60 ? '#3b82f6' : h.precipProb > 30 ? '#93c5fd' : '#bfdbfe', borderRadius: 2 }} />
              </div>
              <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted, textAlign: 'right' }}>{h.precip}"</span>
            </div>
          )
        })}
      </div>
    </div>
  )
}

function BlowupWidget({ state, sensors, weather, weatherChartIdx, quote, savedQuotes, onSaveQuote, onClearSD }: {
  state: BlowupState; sensors: SensorReading[]; weather: WeatherData
  weatherChartIdx: number; quote: QuoteData | null
  savedQuotes: SavedQuote[]; onSaveQuote: () => void; onClearSD: () => void
}) {
  const { theme } = useTheme()
  const t = tok(theme)

  if (state.kind === 'quote') return <QuoteBlowup quote={quote} onSave={onSaveQuote} saved={quote ? savedQuotes.some(s => s.id === quote.id) : false} />
  if (state.kind === 'firetv') return <FireTVRemote />
  if (state.kind === 'ports') return <PortsBlowup sensors={sensors} />
  if (state.kind === 'sdcard') return <SDCardBlowup saved={savedQuotes} onClear={onClearSD} />
  if (state.kind === 'hourly') return <HourlyBlowup hourly={weather.hourly} />

  if (state.kind === 'sensor') {
    const sensor = sensors.find(s => s.id === state.id)
    if (!sensor) return null
    const status = sensor.connected ? getStatus(sensor.value, sensor.goodMax, sensor.warnMax) : null
    const color = status ? SC[status] : t.textMuted
    const dp = sensor.value < 1 ? 3 : sensor.unit === '°F' || sensor.unit === '%RH' ? 1 : 0
    const dMin = Math.min(...sensor.history.map(d => d.value)) * 0.88
    const dMax = Math.max(...sensor.history.map(d => d.value)) * 1.12
    const hourly = sensor.history.filter((_, i) => i % 2 === 0)
    return (
      <div style={{ width: '100%', height: '100%', display: 'flex', flexDirection: 'column', padding: '7px 10px', gap: 4 }}>
        <div style={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', flexShrink: 0 }}>
          <div style={{ display: 'flex', alignItems: 'baseline', gap: 5 }}>
            <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 36, fontWeight: 500, color, lineHeight: 1, letterSpacing: '-0.03em' }}>
              {sensor.connected ? sensor.value.toFixed(dp) : '—'}
            </span>
            <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 13, color: t.textMuted }}>{sensor.unit}</span>
          </div>
          <div style={{ textAlign: 'right' }}>
            <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 14, fontWeight: 600, color: t.text }}>{sensor.label}</div>
            <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted }}>{sensor.addr}</div>
            {status && <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color, padding: '1px 6px', background: `${color}18`, borderRadius: 4, display: 'inline-block', marginTop: 2, letterSpacing: '0.08em' }}>{SL[status]}</div>}
          </div>
        </div>
        <div style={{ flex: 1, minHeight: 0 }}>
          <ResponsiveContainer width="100%" height="100%">
            <AreaChart data={hourly} margin={{ top: 4, right: 6, bottom: 0, left: 0 }}>
              <defs>
                <linearGradient id={`bg-${sensor.id}`} x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor={color} stopOpacity={0.28} />
                  <stop offset="100%" stopColor={color} stopOpacity={0} />
                </linearGradient>
              </defs>
              <XAxis dataKey="time" tick={{ fontFamily: 'DM Mono, monospace', fontSize: 8, fill: t.textMuted }} axisLine={false} tickLine={false} interval={5} />
              <YAxis domain={[dMin, dMax]} tick={{ fontFamily: 'DM Mono, monospace', fontSize: 8, fill: t.textMuted }} axisLine={false} tickLine={false} width={38} tickFormatter={v => v.toFixed(sensor.value < 1 ? 2 : 0)} />
              <CartesianGrid strokeDasharray="3 3" stroke={t.chartGrid} vertical={false} />
              <ReferenceLine y={sensor.goodMax} stroke="#22c55e" strokeDasharray="3 3" strokeOpacity={0.5} />
              <ReferenceLine y={sensor.warnMax} stroke="#f59e0b" strokeDasharray="3 3" strokeOpacity={0.5} />
              <Tooltip contentStyle={{ background: t.surface, border: `1px solid ${t.borderStrong}`, borderRadius: 7, fontFamily: 'DM Mono, monospace', fontSize: 10 }} labelStyle={{ color: t.textMuted }} itemStyle={{ color }} />
              <Area type="monotone" dataKey="value" stroke={color} strokeWidth={2} fill={`url(#bg-${sensor.id})`} dot={false} activeDot={{ r: 4, fill: color, stroke: t.surface, strokeWidth: 2 }} />
            </AreaChart>
          </ResponsiveContainer>
        </div>
        <div style={{ display: 'flex', gap: 8, flexWrap: 'wrap', flexShrink: 0 }}>
          {[{ l: 'AVG', v: (sensor.history.reduce((a, d) => a + d.value, 0) / sensor.history.length).toFixed(dp) }, { l: 'MIN', v: Math.min(...sensor.history.map(d => d.value)).toFixed(dp) }, { l: 'MAX', v: Math.max(...sensor.history.map(d => d.value)).toFixed(dp) }].map(s => (
            <div key={s.l} style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, display: 'flex', gap: 4, color: t.textMuted }}>
              <span>{s.l}</span><span style={{ color: t.textSub, fontWeight: 600 }}>{s.v} {sensor.unit}</span>
            </div>
          ))}
        </div>
      </div>
    )
  }

  if (state.kind === 'weather') {
    const charts = buildWeatherCharts(weather)
    const chart = charts[weatherChartIdx % charts.length]
    if (!chart || !weather.hourly.length) return (
      <div style={{ width: '100%', height: '100%', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 11, color: t.textMuted }}>NO WEATHER DATA</span>
      </div>
    )
    return (
      <div style={{ width: '100%', height: '100%', display: 'flex', flexDirection: 'column', padding: '8px 10px 6px', gap: 4 }}>
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexShrink: 0 }}>
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 12, color: t.text, fontWeight: 600 }}>{chart.title}</div>
          <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted }}>24H · {weather.location}</div>
        </div>
        <div style={{ flex: 1, minHeight: 0 }}>
          <ResponsiveContainer width="100%" height="100%">
            <AreaChart data={weather.hourly} margin={{ top: 4, right: 6, bottom: 0, left: 0 }}>
              <defs>
                <linearGradient id="wxgrad" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stopColor={chart.color} stopOpacity={0.3} />
                  <stop offset="100%" stopColor={chart.color} stopOpacity={0} />
                </linearGradient>
              </defs>
              <XAxis dataKey="time" tick={{ fontFamily: 'DM Mono, monospace', fontSize: 8, fill: t.textMuted }} axisLine={false} tickLine={false} interval={3} />
              <YAxis tick={{ fontFamily: 'DM Mono, monospace', fontSize: 8, fill: t.textMuted }} axisLine={false} tickLine={false} width={34} />
              <CartesianGrid strokeDasharray="3 3" stroke={t.chartGrid} vertical={false} />
              <Tooltip contentStyle={{ background: t.surface, border: `1px solid ${t.borderStrong}`, borderRadius: 7, fontFamily: 'DM Mono, monospace', fontSize: 10 }} labelStyle={{ color: t.textMuted }} itemStyle={{ color: chart.color }} />
              <Area type="monotone" dataKey={chart.key} stroke={chart.color} strokeWidth={2} fill="url(#wxgrad)" dot={false} activeDot={{ r: 4, fill: chart.color, stroke: t.surface, strokeWidth: 2 }} />
            </AreaChart>
          </ResponsiveContainer>
        </div>
      </div>
    )
  }

  return null
}

// ── Weather Panel ─────────────────────────────────────────────────────────────

function WeatherPanel({ weather, onRefresh, onChartTouch, onTodayTouch }: {
  weather: WeatherData; onRefresh: () => void; onChartTouch: (idx: number) => void; onTodayTouch: () => void
}) {
  const { theme } = useTheme()
  const t = tok(theme)
  const [chartIdx, setChartIdx] = useState(0)
  const charts = buildWeatherCharts(weather)
  const timerRef = useRef<ReturnType<typeof setInterval> | null>(null)

  useEffect(() => {
    if (!charts.length) return
    timerRef.current = setInterval(() => setChartIdx(i => (i + 1) % charts.length), 5000)
    return () => { if (timerRef.current) clearInterval(timerRef.current) }
  }, [charts.length])

  const chart = charts[chartIdx]

  return (
    <div style={{ width: '100%', height: '100%', background: t.surface, border: `1px solid ${t.border}`, borderRadius: 10, display: 'flex', flexDirection: 'column', overflow: 'hidden' }}>
      <div style={{ padding: '5px 10px', borderBottom: `1px solid ${t.border}`, display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexShrink: 0 }}>
        <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.accent, letterSpacing: '0.1em' }}>WEATHER</span>
        <div style={{ display: 'flex', gap: 6, alignItems: 'center' }}>
          <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted }}>{weather.location}</span>
          <button onClick={onRefresh} style={{ background: 'none', border: 'none', cursor: 'pointer', color: t.textMuted, padding: 0, display: 'flex' }}>
            <svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.2" style={{ animation: weather.loading ? 'spin 1s linear infinite' : 'none' }}><path d="M23 4v6h-6"/><path d="M1 20v-6h6"/><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"/></svg>
          </button>
        </div>
      </div>
      {weather.error ? (
        <div style={{ flex: 1, display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 6 }}>
          <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: '#ef4444' }}>FETCH ERROR</span>
          <button onClick={onRefresh} style={{ padding: '4px 10px', background: t.accentDim, border: `1px solid ${t.accentBorder}`, borderRadius: 6, color: t.accent, fontSize: 10, cursor: 'pointer', fontFamily: 'DM Mono, monospace' }}>RETRY</button>
        </div>
      ) : (
        <>
          <div style={{ padding: '7px 10px 5px', borderBottom: `1px solid ${t.border}`, flexShrink: 0 }}>
            <div style={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between' }}>
              <div>
                <div style={{ display: 'flex', alignItems: 'baseline', gap: 2 }}>
                  <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 34, fontWeight: 300, lineHeight: 1, color: t.text, letterSpacing: '-0.04em' }}>{weather.temp}</span>
                  <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 13, color: t.textMuted }}>°F</span>
                </div>
                <div style={{ fontSize: 11, color: t.textSub }}>{weather.condition}</div>
                <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted }}>FEELS {weather.feelsLike}°F</div>
              </div>
              <div style={{ textAlign: 'right' }}>
                <div style={{ fontSize: 26, lineHeight: 1 }}>{weather.icon}</div>
                <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted, marginTop: 3 }}>{weather.cloudCover}% cloud</div>
              </div>
            </div>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: 4, marginTop: 5 }}>
              {[['HUM', `${weather.humidity}%`], ['WIND', `${weather.windSpeed}mph`], ['☀', `${weather.hourly[0]?.radiation ?? 0}W`]].map(([l, v]) => (
                <div key={l} style={{ background: t.surface2, borderRadius: 5, padding: '3px 4px', textAlign: 'center' }}>
                  <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted }}>{l}</div>
                  <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.text, fontWeight: 500 }}>{v}</div>
                </div>
              ))}
            </div>
            <div style={{ display: 'flex', gap: 4, marginTop: 4 }}>
              {[['🌅', weather.sunrise], ['🌇', weather.sunset]].map(([e, v]) => (
                <div key={String(e)} style={{ flex: 1, display: 'flex', gap: 4, alignItems: 'center', background: t.surface2, borderRadius: 5, padding: '3px 5px' }}>
                  <span style={{ fontSize: 9 }}>{e}</span>
                  <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textSub }}>{v}</span>
                </div>
              ))}
            </div>
          </div>
          {chart && (
            <button onClick={() => onChartTouch(chartIdx)} style={{ flex: 1, minHeight: 0, background: 'none', border: 'none', cursor: 'pointer', padding: '4px 6px 2px', display: 'flex', flexDirection: 'column', gap: 2, textAlign: 'left' }}>
              <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', flexShrink: 0 }}>
                <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted }}>{chart.title}</span>
                <div style={{ display: 'flex', gap: 3 }}>{charts.map((_, i) => <div key={i} style={{ width: i === chartIdx ? 10 : 4, height: 3, borderRadius: 2, background: i === chartIdx ? t.accent : t.textDim, transition: 'all 0.3s' }} />)}</div>
              </div>
              <div style={{ flex: 1, minHeight: 0 }}>
                <ResponsiveContainer width="100%" height="100%">
                  <AreaChart data={weather.hourly.slice(0, 24)} margin={{ top: 2, right: 4, bottom: 0, left: 0 }}>
                    <defs><linearGradient id={`wpg-${chartIdx}`} x1="0" y1="0" x2="0" y2="1"><stop offset="0%" stopColor={chart.color} stopOpacity={0.4} /><stop offset="100%" stopColor={chart.color} stopOpacity={0} /></linearGradient></defs>
                    <XAxis dataKey="time" hide />
                    <YAxis hide />
                    <Area type="monotone" dataKey={chart.key} stroke={chart.color} strokeWidth={1.5} fill={`url(#wpg-${chartIdx})`} dot={false} />
                  </AreaChart>
                </ResponsiveContainer>
              </div>
              <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textDim, textAlign: 'center', flexShrink: 0 }}>TAP TO EXPAND</div>
            </button>
          )}
          {weather.daily.length > 0 && (
            <div style={{ borderTop: `1px solid ${t.border}`, padding: '4px 8px', flexShrink: 0 }}>
              {weather.daily.map((day, i) => {
                const dw = wmoLabel(day.wmoCode)
                const isToday = i === 0
                const Row = isToday ? 'button' : 'div'
                return (
                  <Row key={i} onClick={isToday ? onTodayTouch : undefined}
                    style={{ display: 'flex', alignItems: 'center', gap: 5, padding: '3px 0', borderBottom: i < weather.daily.length - 1 ? `1px solid ${t.border}` : 'none', width: '100%', background: 'none', border: 'none', cursor: isToday ? 'pointer' : 'default', textAlign: 'left' } as React.CSSProperties}>
                    <span style={{ fontSize: 12 }}>{dw.icon}</span>
                    <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: i === 0 ? t.accent : t.textSub, flex: 1 }}>{i === 0 ? 'TODAY ›' : day.date}</span>
                    <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.text, fontWeight: 500 }}>{day.maxTemp}°</span>
                    <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: '#f59e0b' }}>{day.radiationSum}MJ</span>
                  </Row>
                )
              })}
            </div>
          )}
        </>
      )}
    </div>
  )
}

// ── Settings Panel ────────────────────────────────────────────────────────────

function SettingsPanel({ settings, onChange, onClose }: { settings: Settings; onChange: (s: Settings) => void; onClose: () => void }) {
  const { theme, toggle } = useTheme()
  const t = tok(theme)
  const Tog = ({ on, fn }: { on: boolean; fn: () => void }) => (
    <button onClick={fn} style={{ width: 36, height: 18, borderRadius: 9, border: 'none', cursor: 'pointer', background: on ? t.accent : (theme === 'dark' ? 'rgba(255,255,255,0.1)' : 'rgba(0,0,0,0.1)'), position: 'relative', flexShrink: 0, transition: 'background 0.2s' }}>
      <div style={{ position: 'absolute', top: 2, left: on ? 18 : 2, width: 14, height: 14, borderRadius: '50%', background: '#fff', transition: 'left 0.2s', boxShadow: '0 1px 3px rgba(0,0,0,0.3)' }} />
    </button>
  )
  return (
    <div style={{ position: 'absolute', inset: 0, zIndex: 50, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
      <div onClick={onClose} style={{ position: 'absolute', inset: 0, background: 'rgba(0,0,0,0.5)' }} />
      <div style={{ position: 'relative', zIndex: 1, width: 300, background: t.surface, borderRadius: 14, border: `1px solid ${t.border}`, padding: 14, display: 'flex', flexDirection: 'column', gap: 8 }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
          <span style={{ fontSize: 14, fontWeight: 600, color: t.text }}>Settings</span>
          <button onClick={onClose} style={{ background: 'none', border: 'none', cursor: 'pointer', color: t.textMuted, fontSize: 14 }}>✕</button>
        </div>
        {[{ label: 'Night Mode', sub: 'DARK / LIGHT THEME', ctrl: <Tog on={theme === 'dark'} fn={toggle} /> }, { label: 'Threshold Alerts', sub: 'WARN ON POOR AQI', ctrl: <Tog on={settings.alertsEnabled} fn={() => onChange({ ...settings, alertsEnabled: !settings.alertsEnabled })} /> }].map(r => (
          <div key={r.label} style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', padding: '6px 0', borderBottom: `1px solid ${t.border}` }}>
            <div>
              <div style={{ fontSize: 12, color: t.text }}>{r.label}</div>
              <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted, marginTop: 1 }}>{r.sub}</div>
            </div>
            {r.ctrl}
          </div>
        ))}
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
          <div>
            <div style={{ fontSize: 12, color: t.text }}>Refresh Rate</div>
            <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.textMuted, marginTop: 1 }}>SENSOR UPDATE INTERVAL</div>
          </div>
          <div style={{ display: 'flex', gap: 3 }}>
            {[1, 3, 5, 10].map(r => (
              <button key={r} onClick={() => onChange({ ...settings, refreshRate: r })}
                style={{ padding: '3px 7px', background: settings.refreshRate === r ? t.accent : 'transparent', border: `1px solid ${settings.refreshRate === r ? t.accent : t.border}`, borderRadius: 5, cursor: 'pointer', fontFamily: 'DM Mono, monospace', fontSize: 10, color: settings.refreshRate === r ? (theme === 'dark' ? '#0b0f1a' : '#fff') : t.textMuted }}>
                {r}s
              </button>
            ))}
          </div>
        </div>
      </div>
    </div>
  )
}

// ── Dashboard ─────────────────────────────────────────────────────────────────

const TIMEOUT_MS = 120_000
const QUOTE_REFRESH_MS = 300_000

function Dashboard({ ssid, onDisconnect }: { ssid: string; onDisconnect: () => void }) {
  const { theme, toggle } = useTheme()
  const t = tok(theme)
  const [sensors, setSensors] = useState<SensorReading[]>(INITIAL_SENSORS)
  const [settings, setSettings] = useState<Settings>(DEFAULT_SETTINGS)
  const [showSettings, setShowSettings] = useState(false)
  const [blowup, setBlowup] = useState<BlowupState>({ kind: 'quote' })
  const [weatherChartIdx, setWeatherChartIdx] = useState(0)
  const [now, setNow] = useState(Date.now())
  const [quote, setQuote] = useState<QuoteData | null>(null)
  const [savedQuotes, setSavedQuotes] = useState<SavedQuote[]>([])
  const timeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null)

  const [weather, setWeather] = useState<WeatherData>({
    temp: 72, feelsLike: 70, humidity: 58, windSpeed: 9, cloudCover: 30,
    condition: 'Partly Cloudy', icon: '⛅', location: '33.95°N 84.55°W',
    hourly: [], daily: [], sunrise: '6:42 AM', sunset: '8:14 PM', loading: true, error: null,
  })
  const weatherUrl = 'https://api.open-meteo.com/v1/forecast?latitude=33.9526&longitude=-84.5499&daily=weather_code,apparent_temperature_max,sunrise,sunset,daylight_duration,precipitation_hours,shortwave_radiation_sum&hourly=temperature_2m,shortwave_radiation,relative_humidity_2m,apparent_temperature,precipitation_probability,precipitation,weather_code,cloud_cover,wind_speed_10m&models=best_match&current=weather_code,apparent_temperature,relative_humidity_2m,temperature_2m,cloud_cover,wind_speed_10m&timezone=auto&forecast_days=3&wind_speed_unit=mph&temperature_unit=fahrenheit&precipitation_unit=inch&past_hours=1&forecast_hours=24'

  // sensor live update
  useEffect(() => {
    const id = setInterval(() => {
      setNow(Date.now())
      setSensors(prev => prev.map(s => {
        if (!s.connected) return s
        const jitter = (Math.random() - 0.5) * 0.04
        const nv = Math.max(s.min, Math.min(s.max, s.value * (1 + jitter)))
        const dp = s.value < 1 ? 3 : s.unit === '°F' || s.unit === '%RH' ? 1 : 0
        const rounded = parseFloat(nv.toFixed(dp))
        const nh = [...s.history.slice(-47), { time: new Date().toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', hour12: false }), value: rounded }]
        return { ...s, value: rounded, lastUpdated: Date.now(), history: nh }
      }))
    }, settings.refreshRate * 1000)
    return () => clearInterval(id)
  }, [settings.refreshRate])

  // weather fetch
  const fetchWeather = useCallback(async () => {
    setWeather(w => ({ ...w, loading: true, error: null }))
    try {
      const res = await fetch(weatherUrl)
      if (!res.ok) throw new Error(`HTTP ${res.status}`)
      const d = await res.json()
      const cur = d.current ?? {}
      const wmo = wmoLabel(cur.weather_code ?? 0)
      const hourly: HourlySlice[] = []
      if (d.hourly?.time) {
        const nowIso = new Date().toISOString().slice(0, 13)
        const si = Math.max(0, d.hourly.time.findIndex((x: string) => x >= nowIso))
        for (let i = si; i < Math.min(si + 24, d.hourly.time.length); i++) {
          hourly.push({ time: new Date(d.hourly.time[i]).toLocaleTimeString('en-US', { hour: 'numeric', hour12: true }), temp: Math.round(d.hourly.temperature_2m?.[i] ?? 0), feelsLike: Math.round(d.hourly.apparent_temperature?.[i] ?? 0), precipProb: d.hourly.precipitation_probability?.[i] ?? 0, precip: d.hourly.precipitation?.[i] ?? 0, cloudCover: d.hourly.cloud_cover?.[i] ?? 0, wind: Math.round(d.hourly.wind_speed_10m?.[i] ?? 0), radiation: Math.round(d.hourly.shortwave_radiation?.[i] ?? 0), wmoCode: d.hourly.weather_code?.[i] ?? 0 })
        }
      }
      const daily: DailySlice[] = []
      if (d.daily?.time) {
        for (let i = 0; i < d.daily.time.length; i++) {
          daily.push({ date: fmtDayLabel(d.daily.time[i]), maxTemp: Math.round(d.daily.apparent_temperature_max?.[i] ?? 0), sunrise: fmtTime(d.daily.sunrise?.[i] ?? ''), sunset: fmtTime(d.daily.sunset?.[i] ?? ''), daylightHrs: parseFloat(((d.daily.daylight_duration?.[i] ?? 0) / 3600).toFixed(1)), precipHours: d.daily.precipitation_hours?.[i] ?? 0, radiationSum: parseFloat((d.daily.shortwave_radiation_sum?.[i] ?? 0).toFixed(1)), wmoCode: d.daily.weather_code?.[i] ?? 0 })
        }
      }
      setWeather(w => ({ ...w, loading: false, error: null, temp: Math.round(cur.temperature_2m ?? w.temp), feelsLike: Math.round(cur.apparent_temperature ?? w.feelsLike), humidity: cur.relative_humidity_2m ?? w.humidity, windSpeed: Math.round(cur.wind_speed_10m ?? w.windSpeed), cloudCover: cur.cloud_cover ?? w.cloudCover, condition: wmo.label, icon: wmo.icon, sunrise: daily[0]?.sunrise ?? w.sunrise, sunset: daily[0]?.sunset ?? w.sunset, hourly, daily }))
    } catch (e: unknown) {
      setWeather(w => ({ ...w, loading: false, error: e instanceof Error ? e.message : 'Fetch failed' }))
    }
  }, [weatherUrl])

  useEffect(() => { fetchWeather() }, [])

  // quote fetch — random ID each time
  const fetchQuote = useCallback(async () => {
    try {
      const id = Math.floor(Math.random() * 100) + 1
      const res = await fetch(`https://dummyjson.com/quotes/${id}`)
      if (!res.ok) throw new Error('quote fetch failed')
      const data = await res.json()
      setQuote({ id: data.id, quote: data.quote, author: data.author })
    } catch { /* keep previous */ }
  }, [])

  useEffect(() => { fetchQuote() }, [])
  useEffect(() => {
    const id = setInterval(fetchQuote, QUOTE_REFRESH_MS)
    return () => clearInterval(id)
  }, [fetchQuote])

  // 2-minute timeout: revert to quote (except firetv)
  const startTimeout = useCallback(() => {
    if (timeoutRef.current) clearTimeout(timeoutRef.current)
    timeoutRef.current = setTimeout(() => setBlowup({ kind: 'quote' }), TIMEOUT_MS)
  }, [])

  const clearTimeoutRef = () => { if (timeoutRef.current) clearTimeout(timeoutRef.current) }

  const open = (state: BlowupState) => {
    setBlowup(state)
    if (state.kind === 'firetv' || state.kind === 'quote') clearTimeoutRef()
    else startTimeout()
  }

  const isActive = (kind: BlowupKind, id?: string) => {
    if (blowup.kind !== kind) return false
    if (kind === 'sensor') return (blowup as { kind: 'sensor'; id: string }).id === id
    return true
  }

  const aqi = computeAQI(sensors)
  const timeStr = new Date(now).toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false })

  const TabBtn = ({ kind, label, idVal }: { kind: BlowupKind; label: string; idVal?: string }) => {
    const active = isActive(kind, idVal)
    const toggle2 = () => {
      if (active) open({ kind: 'quote' })
      else if (kind === 'firetv') open({ kind: 'firetv' })
      else if (kind === 'ports') open({ kind: 'ports' })
      else if (kind === 'sdcard') open({ kind: 'sdcard' })
    }
    return (
      <button onClick={toggle2} style={{ display: 'flex', alignItems: 'center', gap: 3, padding: '2px 6px', background: active ? t.accentDim : 'transparent', border: `1px solid ${active ? t.accentBorder : t.border}`, borderRadius: 5, cursor: 'pointer', fontFamily: 'DM Mono, monospace', fontSize: 8, color: active ? t.accent : t.textMuted, transition: 'all 0.15s', whiteSpace: 'nowrap' }}>
        {label}
      </button>
    )
  }

  const saveQuote = () => {
    if (!quote) return
    if (savedQuotes.some(s => s.id === quote.id)) return
    const ts = new Date().toLocaleTimeString('en-US', { hour: 'numeric', minute: '2-digit', hour12: true })
    setSavedQuotes(prev => [{ ...quote, savedAt: ts }, ...prev])
  }

  return (
    <div style={{ width: 800, height: 400, background: t.bg, fontFamily: 'Outfit, sans-serif', display: 'flex', flexDirection: 'column', overflow: 'hidden', position: 'relative', transition: 'background 0.3s' }}>
      <div style={{ position: 'absolute', inset: 0, pointerEvents: 'none', backgroundImage: `linear-gradient(${t.grid} 1px,transparent 1px),linear-gradient(90deg,${t.grid} 1px,transparent 1px)`, backgroundSize: '40px 40px' }} />

      {/* Header 28px */}
      <div style={{ height: 28, borderBottom: `1px solid ${t.border}`, display: 'flex', alignItems: 'center', paddingInline: 6, gap: 4, flexShrink: 0, position: 'relative', zIndex: 2, background: t.surface, overflow: 'hidden' }}>
        <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.accent, letterSpacing: '0.14em', whiteSpace: 'nowrap' }}>AIRWATCH PRO</span>
        <div style={{ width: 1, height: 14, background: t.border, flexShrink: 0 }} />
        <WifiBar strength={85} color={t.accent} />
        <button onClick={onDisconnect} style={{ fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted, maxWidth: 80, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap', background: 'none', border: 'none', cursor: 'pointer', padding: 0 }}>{ssid}</button>
        <div style={{ flex: 1 }} />
        <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 10, color: t.textMuted, whiteSpace: 'nowrap' }}>{timeStr}</span>
        <div style={{ width: 1, height: 14, background: t.border, flexShrink: 0 }} />
        <TabBtn kind="ports" label="⬡ PORTS" />
        <TabBtn kind="sdcard" label={`💾 SD${savedQuotes.length > 0 ? ` (${savedQuotes.length})` : ''}`} />
        <TabBtn kind="firetv" label="🔥 FIRE TV" />
        <div style={{ width: 1, height: 14, background: t.border, flexShrink: 0 }} />
        <button onClick={toggle} style={{ display: 'flex', alignItems: 'center', gap: 3, padding: '2px 6px', background: t.accentDim, border: `1px solid ${t.accentBorder}`, borderRadius: 5, cursor: 'pointer', fontFamily: 'DM Mono, monospace', fontSize: 8, color: t.accent, whiteSpace: 'nowrap' }}>
          {theme === 'dark' ? '☀ DAY' : '🌙 NIGHT'}
        </button>
        <button onClick={() => setShowSettings(true)} style={{ padding: '2px 6px', background: 'transparent', border: `1px solid ${t.border}`, borderRadius: 5, cursor: 'pointer', fontFamily: 'DM Mono, monospace', fontSize: 9, color: t.textMuted }}>⚙</button>
      </div>

      {/* Body 372px */}
      <div style={{ flex: 1, display: 'flex', gap: 4, padding: '4px 5px 5px', position: 'relative', zIndex: 1, minHeight: 0, overflow: 'hidden' }}>

        {/* LEFT — 162px: AQI + System */}
        <div style={{ width: 162, flexShrink: 0, display: 'flex', flexDirection: 'column', gap: 5 }}>
          <AQIWidget score={aqi.score} label={aqi.label} color={aqi.color} />
          <SystemWidget ssid={ssid} />
        </div>

        {/* SENSORS LEFT — 5 sensors in a single column */}
        <div style={{ width: 72, flexShrink: 0, display: 'flex', flexDirection: 'column', gap: 4 }}>
          {sensors.slice(0, 5).map(s => (
            <SensorChip key={s.id} sensor={s} active={isActive('sensor', s.id)} onClick={() => open({ kind: 'sensor', id: s.id })} />
          ))}
        </div>

        {/* BLOWUP — flex 1, takes remaining space */}
        <div style={{ flex: 1, minWidth: 0, background: t.surface, border: `1px solid ${blowup.kind !== 'quote' ? t.accentBorder : t.border}`, borderRadius: 10, overflow: 'hidden', transition: 'border-color 0.25s' }}>
          <BlowupWidget state={blowup} sensors={sensors} weather={weather} weatherChartIdx={weatherChartIdx} quote={quote} savedQuotes={savedQuotes} onSaveQuote={saveQuote} onClearSD={() => setSavedQuotes([])} />
        </div>

        {/* SENSORS RIGHT — remaining sensors + quote chip */}
        <div style={{ width: 72, flexShrink: 0, display: 'flex', flexDirection: 'column', gap: 4 }}>
          {sensors.slice(5).map(s => (
            <SensorChip key={s.id} sensor={s} active={isActive('sensor', s.id)} onClick={() => open({ kind: 'sensor', id: s.id })} />
          ))}
          {/* Quote chip — no status dot, symmetric padding */}
          <button onClick={() => open({ kind: 'quote' })} style={{
            flex: 1, padding: '5px 7px',
            background: blowup.kind === 'quote' ? t.accentDim : t.surface,
            border: `1px solid ${blowup.kind === 'quote' ? t.accentBorder : t.border}`,
            borderRadius: 8, cursor: 'pointer', display: 'flex', flexDirection: 'column', gap: 2,
            textAlign: 'left', transition: 'all 0.15s', width: '100%', boxSizing: 'border-box',
          }}>
            <span style={{ fontFamily: 'DM Mono, monospace', fontSize: 8, color: blowup.kind === 'quote' ? t.accent : t.textMuted, letterSpacing: '0.06em' }}>INSPIRATION</span>
            <div style={{ fontSize: 16, lineHeight: 1 }}>💬</div>
            <div style={{ fontFamily: 'DM Mono, monospace', fontSize: 7, color: blowup.kind === 'quote' ? '#22c55e' : t.textDim, padding: '1px 4px', background: blowup.kind === 'quote' ? 'rgba(34,197,94,0.1)' : 'transparent', borderRadius: 3, alignSelf: 'flex-start', letterSpacing: '0.06em' }}>{blowup.kind === 'quote' ? 'ACTIVE' : 'TAP TO READ'}</div>
            <div style={{ height: 2, background: t.chartGrid, borderRadius: 1, marginTop: 'auto' }}>
              <div style={{ height: '100%', width: blowup.kind === 'quote' ? '100%' : '0%', background: '#22c55e', borderRadius: 1, transition: 'width 0.5s' }} />
            </div>
          </button>
        </div>

        {/* WEATHER — 162px: matches left sidebar */}
        <div style={{ width: 162, flexShrink: 0 }}>
          <WeatherPanel weather={weather} onRefresh={fetchWeather} onChartTouch={idx => { setWeatherChartIdx(idx); open({ kind: 'weather', chartIdx: idx }) }} onTodayTouch={() => open({ kind: 'hourly' })} />
        </div>
      </div>

      {showSettings && <SettingsPanel settings={settings} onChange={setSettings} onClose={() => setShowSettings(false)} />}

      <style>{`
        @keyframes spin{from{transform:rotate(0deg)}to{transform:rotate(360deg)}}
        @keyframes pulse{0%,100%{opacity:1}50%{opacity:0.4}}
        ::-webkit-scrollbar{width:3px}
        ::-webkit-scrollbar-thumb{background:rgba(128,128,128,0.2);border-radius:2px}
        ::-webkit-scrollbar:horizontal{height:3px}
      `}</style>
    </div>
  )
}

// ── Root ──────────────────────────────────────────────────────────────────────

export default function App() {
  const [theme, setTheme] = useState<Theme>('dark')
  const [ssid, setSsid] = useState('')
  const [screen, setScreen] = useState<'wifi' | 'dashboard'>('wifi')
  return (
    <ThemeCtx.Provider value={{ theme, toggle: () => setTheme(t => t === 'dark' ? 'light' : 'dark') }}>
      <div style={{ width: '100vw', minHeight: '100vh', display: 'flex', alignItems: 'center', justifyContent: 'center', background: '#000' }}>
        {screen === 'wifi'
          ? <WiFiScreen onConnect={s => { setSsid(s); setScreen('dashboard') }} />
          : <Dashboard ssid={ssid} onDisconnect={() => { setSsid(''); setScreen('wifi') }} />}
      </div>
    </ThemeCtx.Provider>
  )
}
