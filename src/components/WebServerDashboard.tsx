import React, { useState, useEffect, useRef } from 'react';
import { 
  Globe, 
  Wifi, 
  Activity, 
  Sliders, 
  RefreshCw, 
  Cpu, 
  Gauge, 
  Send, 
  CheckCircle2, 
  Eye, 
  ArrowRight, 
  Database,
  Layers,
  Sparkles,
  Play,
  Square
} from 'lucide-react';
import { CyclicTestState, FilterType } from '../types';

interface WebServerDashboardProps {
  cyclicState: CyclicTestState;
  setCyclicState: React.Dispatch<React.SetStateAction<CyclicTestState>>;
  filterType: FilterType;
  setFilterType: (filter: FilterType) => void;
}

export const WebServerDashboard: React.FC<WebServerDashboardProps> = ({
  cyclicState,
  setCyclicState,
  filterType,
  setFilterType
}) => {
  const [selectedChannel, setSelectedChannel] = useState<number>(0);
  const [webSampleRate, setWebSampleRate] = useState<number>(cyclicState.sampleRateHz);
  const [isAutoStreaming, setIsAutoStreaming] = useState<boolean>(true);
  const [refreshIntervalMs, setRefreshIntervalMs] = useState<number>(50); // 20 Hz visual refresh

  // Oscilloscope canvas state
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const chartHistoryRef = useRef<number[]>([]);
  const [currentValMv, setCurrentValMv] = useState<number>(1650);
  const [currentRawAdc, setCurrentRawAdc] = useState<number>(2048);

  // Simulated UDP packets statistics
  const [udpPacketsSent, setUdpPacketsSent] = useState<number>(128450);
  const [udpMode, setUdpMode] = useState<'MULTICAST' | 'UNICAST'>('MULTICAST');
  const [multicastGroup, setMulticastGroup] = useState<string>('239.255.0.100');
  const [unicastTarget, setUnicastTarget] = useState<string>('192.168.1.255');
  const [lastUdpCmdMsg, setLastUdpCmdMsg] = useState<string>('OK: UDP Port 5001 Symmetrical Listener Active');

  // Simulated signal generation for chosen channel
  useEffect(() => {
    const timer = setInterval(() => {
      // Calculate realistic sensor waveform for selected channel
      const now = Date.now() / 1000;
      const baseFreq = 0.5 + selectedChannel * 0.2;
      const nominalMv = 1500 + selectedChannel * 75;
      
      // Add cyclic actuator influence if running
      let cyclicBonus = 0;
      if (cyclicState.state === 'RUNNING') {
        cyclicBonus = Math.sin(now * 2 * Math.PI * cyclicState.frequencyHz) * 450;
      }

      // Add harmonic and slight noise
      const wave = nominalMv + cyclicBonus + Math.sin(now * 2 * Math.PI * baseFreq) * 200 + (Math.random() - 0.5) * 25;
      const clampedMv = Math.max(0, Math.min(3300, wave));
      const raw = Math.round((clampedMv / 3300) * 4095);

      setCurrentValMv(clampedMv);
      setCurrentRawAdc(raw);

      // Add to scope buffer
      chartHistoryRef.current.push(clampedMv);
      if (chartHistoryRef.current.length > 200) {
        chartHistoryRef.current.shift();
      }

      // Increment UDP counter (1000 packets per second equivalent)
      if (isAutoStreaming) {
        setUdpPacketsSent(prev => prev + 10);
      }

      // Draw canvas
      drawChart();
    }, refreshIntervalMs);

    return () => clearInterval(timer);
  }, [selectedChannel, cyclicState.state, cyclicState.frequencyHz, refreshIntervalMs, isAutoStreaming]);

  // Canvas Drawing
  const drawChart = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const width = canvas.width;
    const height = canvas.height;

    // Clear background
    ctx.fillStyle = '#050811';
    ctx.fillRect(0, 0, width, height);

    // Draw Grid Lines (Voltages: 0V, 1V, 2V, 3V, 3.3V)
    ctx.strokeStyle = '#1e293b';
    ctx.lineWidth = 1;
    ctx.font = '10px monospace';
    ctx.fillStyle = '#64748b';

    for (let v = 0; v <= 3300; v += 660) {
      const y = height - (v / 3300) * height;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
      ctx.stroke();
      ctx.fillText(`${(v / 1000).toFixed(1)}V`, 8, y - 4);
    }

    // Vertical time grid lines
    for (let x = 0; x < width; x += 60) {
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, height);
      ctx.stroke();
    }

    // Draw Signal Trace
    const history = chartHistoryRef.current;
    if (history.length > 1) {
      ctx.lineWidth = 2.5;
      ctx.strokeStyle = '#10b981'; // Emerald Green
      ctx.beginPath();

      for (let i = 0; i < history.length; i++) {
        const x = (i / (history.length - 1)) * width;
        const y = height - (history[i] / 3300) * height;

        if (i === 0) {
          ctx.moveTo(x, y);
        } else {
          ctx.lineTo(x, y);
        }
      }
      ctx.stroke();

      // Glow effect on trace
      ctx.lineWidth = 6;
      ctx.strokeStyle = 'rgba(16, 185, 129, 0.15)';
      ctx.stroke();
    }
  };

  // Synchronize sample rate when changed via web UI
  const handleRateChange = (rate: number) => {
    setWebSampleRate(rate);
    setCyclicState(prev => ({ ...prev, sampleRateHz: rate }));
    setLastUdpCmdMsg(`OK: HTTP Config Updated Sample Rate to ${rate} Hz`);
  };

  return (
    <div className="space-y-6">
      {/* Top Banner */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div className="flex items-center gap-3">
            <div className="w-12 h-12 rounded-xl bg-gradient-to-br from-emerald-500/20 to-blue-500/20 border border-emerald-500/40 flex items-center justify-center text-emerald-400 shadow-inner">
              <Globe className="w-6 h-6" />
            </div>
            <div>
              <div className="flex items-center gap-2">
                <h2 className="text-lg font-bold text-white tracking-tight">
                  وب سرور تعبیه‌شده میکروکنترلر (STM32 Embedded Web Dashboard)
                </h2>
                <span className="px-2 py-0.5 rounded text-[11px] font-mono font-bold bg-emerald-950/80 border border-emerald-500/40 text-emerald-300">
                  HTTP Port 80
                </span>
                <span className="px-2 py-0.5 rounded text-[11px] font-mono font-bold bg-blue-950/80 border border-blue-500/40 text-blue-300">
                  UDP Port 5001
                </span>
              </div>
              <p className="text-xs text-slate-400 mt-1">
                صفحه وب زنده میزبانی‌شده روی تراشه STM32F407 (LwIP Stack): تنظیم نرخ نمونه‌برداری، انتخاب کانال چارت و مانیتورینگ استریم چندپخشی (Multicast/Unicast).
              </p>
            </div>
          </div>

          <div className="flex items-center gap-3">
            <a 
              href="#embedded-preview"
              className="flex items-center gap-2 px-3 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold shadow-md transition-colors"
            >
              <Eye className="w-4 h-4" />
              مشاهده پیش‌نمایش در مرورگر
            </a>
          </div>
        </div>
      </div>

      {/* Main Interactive Grid */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Left Column: Web Configuration Controls (4 cols) */}
        <div className="lg:col-span-4 space-y-5">
          {/* Web Config Card */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 space-y-4 shadow-sm">
            <h3 className="text-sm font-bold text-white flex items-center gap-2 border-b border-slate-800 pb-3">
              <Sliders className="w-4 h-4 text-emerald-400" />
              تنظیمات وب‌سرور (HTTP Web Controls)
            </h3>

            {/* Sampling Rate Dropdown */}
            <div className="space-y-1.5">
              <label className="text-xs font-semibold text-slate-300 block">
                تعداد نمونه در ثانیه (Sampling Rate - Hz)
              </label>
              <select
                id="select-web-sample-rate"
                value={webSampleRate}
                onChange={(e) => handleRateChange(Number(e.target.value))}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg px-3 py-2 text-sm text-white font-mono focus:border-emerald-500 focus:outline-none transition-colors"
              >
                <option value={100}>100 Hz (کند - ثبت طولانی مدت)</option>
                <option value={500}>500 Hz</option>
                <option value={1000}>1,000 Hz (پیش‌فرض استاندارد ۱ کیلوهرتز)</option>
                <option value={2000}>2,000 Hz (۲ کیلوهرتز)</option>
                <option value={5000}>5,000 Hz (۵ کیلوهرتز)</option>
                <option value={10000}>10,000 Hz (حداکثر ۱۰ کیلوهرتز بر کانال)</option>
              </select>
              <span className="text-[11px] text-slate-500 block">
                تغییر این گزینه همزمان در رجیستر ADC و تایمر سخت‌افزاری TIM2 اعمال می‌شود.
              </span>
            </div>

            {/* Select Channel to Display on Chart */}
            <div className="space-y-1.5">
              <label className="text-xs font-semibold text-slate-300 block">
                انتخاب ورودی جهت نمایش روی چارت (Selected Channel)
              </label>
              <select
                id="select-web-channel"
                value={selectedChannel}
                onChange={(e) => setSelectedChannel(Number(e.target.value))}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg px-3 py-2 text-sm text-emerald-400 font-mono font-bold focus:border-emerald-500 focus:outline-none transition-colors"
              >
                {Array.from({ length: 16 }, (_, i) => {
                  let label = `کانال CH${i}`;
                  if (i === 0) label += ' - لودسل بارگذاری (Load Cell)';
                  else if (i === 1) label += ' - سنسور جابه‌جایی LVDT';
                  else if (i === 2) label += ' - ترانسمیتر فشار هیدرولیک';
                  else if (i === 3) label += ' - دمای محفظه آزمون';
                  return (
                    <option key={i} value={i}>
                      {label}
                    </option>
                  );
                })}
              </select>
              <span className="text-[11px] text-slate-500 block">
                مالتی‌پلکسر CD74HC4067 خروجی این ورودی را روی پین PA4 میکرو سوییچ می‌کند.
              </span>
            </div>

            {/* Filter Selection */}
            <div className="space-y-1.5">
              <label className="text-xs font-semibold text-slate-300 block">
                پیکربندی فیلتر فعال (Digital Filter)
              </label>
              <select
                id="select-web-filter"
                value={filterType}
                onChange={(e) => setFilterType(e.target.value as FilterType)}
                className="w-full bg-slate-950 border border-slate-700 rounded-lg px-3 py-2 text-sm text-white font-mono focus:border-emerald-500 focus:outline-none transition-colors"
              >
                <option value="BYPASS">خام بدون فیلتر (Raw Bypass)</option>
                <option value="MAV">میانگین متحرک (MAV Window 8)</option>
                <option value="EMA">فیلتر نمایی (EMA Alpha 0.25)</option>
                <option value="IIR_LPF">پایین‌گذر باترورث (2nd IIR Butterworth)</option>
                <option value="MEDIAN">فیلتر میانه حذف اسپایک (3-Pt Median)</option>
              </select>
            </div>

            {/* Quick Actions */}
            <div className="pt-2 border-t border-slate-800 flex gap-2">
              <button
                id="btn-web-toggle-stream"
                onClick={() => setIsAutoStreaming(!isAutoStreaming)}
                className={`flex-1 flex items-center justify-center gap-2 py-2 px-3 rounded-lg text-xs font-bold transition-colors ${
                  isAutoStreaming
                    ? 'bg-rose-500/20 text-rose-300 border border-rose-500/30 hover:bg-rose-500/30'
                    : 'bg-emerald-600 text-white hover:bg-emerald-500'
                }`}
              >
                {isAutoStreaming ? <Square className="w-3.5 h-3.5" /> : <Play className="w-3.5 h-3.5" />}
                {isAutoStreaming ? 'توقف استریم' : 'شروع استریم UDP'}
              </button>

              <button
                id="btn-web-reset-scope"
                onClick={() => {
                  chartHistoryRef.current = [];
                  drawChart();
                }}
                className="p-2 rounded-lg bg-slate-800 text-slate-300 hover:text-white border border-slate-700 text-xs"
                title="پاکسازی چارت"
              >
                <RefreshCw className="w-4 h-4" />
              </button>
            </div>
          </div>

          {/* UDP Multicast / Unicast Network Status Card */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 space-y-3 shadow-sm">
            <div className="flex items-center justify-between border-b border-slate-800 pb-2">
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <Wifi className="w-4 h-4 text-blue-400" />
                استریم خودکار UDP (بدون نیاز به دانستن IP)
              </h3>
              <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-ping" />
            </div>

            <p className="text-xs text-slate-400 leading-relaxed">
              سیستم به صورت خودکار ۱۰۰۰ بسته در ثانیه شامل تگ زمان میکروثانیه (µs) را روی آدرس Multicast ارسال می‌کند؛ برنامه کلاینت بدون جستجو یا نیاز به دانستن IP سیستم، داده‌ها را فوراً تحویل می‌گیرد.
            </p>

            <div className="space-y-2 text-xs font-mono pt-1">
              <div className="flex items-center justify-between bg-slate-950 p-2.5 rounded-lg border border-slate-800">
                <span className="text-slate-400">حالت ارسال:</span>
                <div className="flex gap-1">
                  <button
                    id="btn-mode-multicast"
                    onClick={() => setUdpMode('MULTICAST')}
                    className={`px-2 py-0.5 rounded text-[11px] font-bold ${
                      udpMode === 'MULTICAST' ? 'bg-emerald-600 text-white' : 'bg-slate-800 text-slate-400'
                    }`}
                  >
                    Multicast
                  </button>
                  <button
                    id="btn-mode-unicast"
                    onClick={() => setUdpMode('UNICAST')}
                    className={`px-2 py-0.5 rounded text-[11px] font-bold ${
                      udpMode === 'UNICAST' ? 'bg-blue-600 text-white' : 'bg-slate-800 text-slate-400'
                    }`}
                  >
                    Unicast
                  </button>
                </div>
              </div>

              <div className="flex items-center justify-between bg-slate-950 p-2.5 rounded-lg border border-slate-800">
                <span className="text-slate-400">آدرس مقصد:</span>
                <span className="text-emerald-400 font-bold">
                  {udpMode === 'MULTICAST' ? `${multicastGroup}:5001` : `${unicastTarget}:5001`}
                </span>
              </div>

              <div className="flex items-center justify-between bg-slate-950 p-2.5 rounded-lg border border-slate-800">
                <span className="text-slate-400">تگ زمان (Hardware Timestamp):</span>
                <span className="text-amber-400 font-bold">
                  {(Date.now() * 1000).toLocaleString()} µs
                </span>
              </div>

              <div className="flex items-center justify-between bg-slate-950 p-2.5 rounded-lg border border-slate-800">
                <span className="text-slate-400">تعداد بسته‌های ارسالی:</span>
                <span className="text-blue-300 font-bold">{udpPacketsSent.toLocaleString()}</span>
              </div>
            </div>
          </div>
        </div>

        {/* Right Column: Live Web Chart & Symmetrical Command Console (8 cols) */}
        <div className="lg:col-span-8 space-y-5">
          {/* Embedded Web Canvas Chart */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm space-y-4">
            <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-2 border-b border-slate-800 pb-3">
              <div>
                <div className="flex items-center gap-2">
                  <span className="text-sm font-bold text-white">
                    نمایشگر اسیلوسکوپ کانال CH{selectedChannel}
                  </span>
                  <span className="px-2 py-0.5 rounded text-[11px] font-mono bg-slate-800 text-emerald-400 font-semibold border border-slate-700">
                    Live HTML5 Canvas
                  </span>
                </div>
                <span className="text-xs text-slate-400">
                  نمایش بلادرنگ ولتاژ ورودی آنالوگ تقویت‌شده از ۰ الی ۳۳۰۰ میلی‌ولت
                </span>
              </div>

              <div className="flex items-center gap-3 font-mono text-xs">
                <div className="text-right">
                  <span className="text-[10px] text-slate-500 block">Live Voltage</span>
                  <span className="text-emerald-400 font-bold text-base">{currentValMv.toFixed(1)} mV</span>
                </div>
                <div className="text-right pl-3 border-l border-slate-800">
                  <span className="text-[10px] text-slate-500 block">Raw ADC</span>
                  <span className="text-purple-400 font-bold text-base">{currentRawAdc}</span>
                </div>
              </div>
            </div>

            {/* Canvas Scope */}
            <div className="relative rounded-lg overflow-hidden border border-slate-800 bg-[#050811]">
              <canvas
                ref={canvasRef}
                width={800}
                height={260}
                className="w-full h-[260px] block"
              />
              <div className="absolute top-2 right-3 font-mono text-[10px] text-slate-500 bg-slate-900/80 px-2 py-1 rounded border border-slate-800">
                Scale: 0 - 3300 mV | Rate: {webSampleRate} Hz | Ch: {selectedChannel}
              </div>
            </div>

            {/* Quick Channel Buttons Bar */}
            <div>
              <span className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider block mb-2 font-mono">
                سوئیچ سریع کانال‌ها (Channel Selector Quick-Switch):
              </span>
              <div className="grid grid-cols-8 sm:grid-cols-16 gap-1">
                {Array.from({ length: 16 }, (_, i) => (
                  <button
                    key={i}
                    id={`btn-quick-ch-${i}`}
                    onClick={() => setSelectedChannel(i)}
                    className={`py-1.5 text-center text-xs font-mono font-bold rounded transition-colors ${
                      selectedChannel === i
                        ? 'bg-emerald-600 text-white shadow-md'
                        : 'bg-slate-950 text-slate-400 hover:bg-slate-800 hover:text-white border border-slate-800'
                    }`}
                  >
                    {i}
                  </button>
                ))}
              </div>
            </div>
          </div>

          {/* Symmetrical Dual-Port Command Execution Panel */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm space-y-4">
            <div className="flex items-center justify-between border-b border-slate-800 pb-3">
              <div>
                <h3 className="text-sm font-bold text-white flex items-center gap-2">
                  <Cpu className="w-4 h-4 text-amber-400" />
                  پردازشگر متقارن دستورات (Unified Serial + UDP Command Processor)
                </h3>
                <p className="text-xs text-slate-400 mt-0.5">
                  تمام دستورات (:RATE, :FILTER, :START, :STOP, :MEAS) به طور کاملاً یکسان هم از پورت سریال و هم از پورت شبکه UDP دریافت و اجرا می‌شوند.
                </p>
              </div>
              <span className="text-[11px] font-mono px-2 py-0.5 rounded bg-amber-500/10 text-amber-300 border border-amber-500/30">
                Symmetrical Socket
              </span>
            </div>

            {/* Interactive Test Commands Buttons */}
            <div className="grid grid-cols-2 sm:grid-cols-4 gap-2">
              <button
                id="btn-cmd-rate-1000"
                onClick={() => handleRateChange(1000)}
                className="p-2 rounded-lg bg-slate-950 border border-slate-800 hover:border-emerald-500/50 text-left text-xs font-mono transition-colors"
              >
                <span className="text-emerald-400 font-bold block">:RATE 1000</span>
                <span className="text-slate-500 text-[10px]">تنظیم ۱۰۰۰ نمونه/ثانیه</span>
              </button>

              <button
                id="btn-cmd-filter-iir"
                onClick={() => {
                  setFilterType('IIR_LPF');
                  setLastUdpCmdMsg('OK: FILTER=IIR_BUTTERWORTH (CUTOFF=20.0 Hz) via UDP:5001');
                }}
                className="p-2 rounded-lg bg-slate-950 border border-slate-800 hover:border-blue-500/50 text-left text-xs font-mono transition-colors"
              >
                <span className="text-blue-400 font-bold block">:FILTER IIR 20</span>
                <span className="text-slate-500 text-[10px]">فیلتر باترورث ۲۰ هرتز</span>
              </button>

              <button
                id="btn-cmd-filter-mav"
                onClick={() => {
                  setFilterType('MAV');
                  setLastUdpCmdMsg('OK: FILTER=MAV (WINDOW=8) via UDP:5001');
                }}
                className="p-2 rounded-lg bg-slate-950 border border-slate-800 hover:border-purple-500/50 text-left text-xs font-mono transition-colors"
              >
                <span className="text-purple-400 font-bold block">:FILTER MAV 8</span>
                <span className="text-slate-500 text-[10px]">میانگین متحرک ۸ نمونه</span>
              </button>

              <button
                id="btn-cmd-udp-mode"
                onClick={() => {
                  const newMode = udpMode === 'MULTICAST' ? 'UNICAST' : 'MULTICAST';
                  setUdpMode(newMode);
                  setLastUdpCmdMsg(`OK: UDP MODE=${newMode} PORT=5001 via UDP:5001`);
                }}
                className="p-2 rounded-lg bg-slate-950 border border-slate-800 hover:border-amber-500/50 text-left text-xs font-mono transition-colors"
              >
                <span className="text-amber-400 font-bold block">:UDP:MODE TOGGLE</span>
                <span className="text-slate-500 text-[10px]">تغییر مولتی‌کست / یونیکست</span>
              </button>
            </div>

            {/* Execution Feedback Box */}
            <div className="bg-slate-950 p-3 rounded-lg border border-slate-800 font-mono text-xs text-slate-300 flex items-center justify-between">
              <div className="flex items-center gap-2">
                <CheckCircle2 className="w-4 h-4 text-emerald-400 shrink-0" />
                <span className="text-emerald-300 font-semibold">{lastUdpCmdMsg}</span>
              </div>
              <span className="text-[10px] text-slate-500">Latency: 0.28 ms</span>
            </div>
          </div>
        </div>
      </div>

      {/* Raw Web Page Preview Frame */}
      <div id="embedded-preview" className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm space-y-4">
        <div className="flex items-center justify-between border-b border-slate-800 pb-3">
          <div className="flex items-center gap-2">
            <Globe className="w-4 h-4 text-emerald-400" />
            <h3 className="text-sm font-bold text-white">
              پیش‌نمایش زنده صفحه HTML خام تولید شده توسط میکروکنترلر (Raw Web Server Preview)
            </h3>
          </div>
          <span className="text-xs font-mono text-slate-400">
            URL: <strong className="text-white">http://192.168.1.150/</strong>
          </span>
        </div>

        <div className="rounded-xl border border-slate-700 bg-[#090d16] p-6 shadow-inner text-slate-200">
          <div className="flex items-center justify-between pb-4 border-b border-slate-800">
            <h4 className="text-base font-bold text-emerald-400 flex items-center gap-2">
              <span>⚡ LabDAQ-Control</span>
              <span className="text-xs px-2 py-0.5 rounded bg-emerald-950 text-emerald-300 border border-emerald-800 font-mono">
                STM32F407 Web Server
              </span>
            </h4>
            <span className="text-xs font-mono text-slate-500">HTTP/1.1 200 OK</span>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-4 my-4">
            <div>
              <label className="text-xs text-slate-400 block mb-1">انتخاب نرخ نمونه‌برداری:</label>
              <div className="p-2.5 rounded-lg bg-slate-900 border border-slate-700 font-mono text-xs text-white">
                {webSampleRate} Hz (فعال و همگام)
              </div>
            </div>
            <div>
              <label className="text-xs text-slate-400 block mb-1">کانال نمایشگر:</label>
              <div className="p-2.5 rounded-lg bg-slate-900 border border-slate-700 font-mono text-xs text-emerald-300">
                Channel {selectedChannel} (ورودی مشترک PA4)
              </div>
            </div>
          </div>

          <div className="grid grid-cols-3 gap-3 p-3 bg-slate-900/60 rounded-lg border border-slate-800 font-mono text-center">
            <div>
              <span className="text-[10px] text-slate-500 block">ولتاژ زنده</span>
              <span className="text-lg font-bold text-emerald-400">{currentValMv.toFixed(1)} mV</span>
            </div>
            <div>
              <span className="text-[10px] text-slate-500 block">کد دیجیتال ۱۲ بیتی</span>
              <span className="text-lg font-bold text-purple-400">{currentRawAdc}</span>
            </div>
            <div>
              <span className="text-[10px] text-slate-500 block">مقصد UDP چندپخشی</span>
              <span className="text-xs font-bold text-blue-400 block mt-1">239.255.0.100:5001</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
