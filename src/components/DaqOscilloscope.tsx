import React, { useState, useEffect, useRef } from 'react';
import { 
  Play, 
  Pause, 
  Square, 
  RotateCcw, 
  Activity, 
  Sliders, 
  Zap, 
  Radio, 
  Eye, 
  Layers,
  Sparkles,
  BarChart2
} from 'lucide-react';
import { CyclicTestState, FilterType } from '../types';

interface DaqOscilloscopeProps {
  cyclicState: CyclicTestState;
  setCyclicState: React.Dispatch<React.SetStateAction<CyclicTestState>>;
  filterType: FilterType;
  setFilterType: (type: FilterType) => void;
  noiseLevel: number;
  setNoiseLevel: (level: number) => void;
}

export const DaqOscilloscope: React.FC<DaqOscilloscopeProps> = ({
  cyclicState,
  setCyclicState,
  filterType,
  setFilterType,
  noiseLevel,
  setNoiseLevel
}) => {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const [selectedChannel, setSelectedChannel] = useState<number>(0);
  const [timebase, setTimebase] = useState<number>(50); // milliseconds per division
  const [showRawCompare, setShowRawCompare] = useState<boolean>(true);

  // History buffer for oscilloscope
  const historyRef = useRef<{ raw: number; filtered: number; time: number }[]>([]);
  const lastStateToggle = useRef<number>(Date.now());

  // Cyclic test simulation loop & signal generator
  useEffect(() => {
    let animationFrameId: number;
    let t = 0;

    const render = () => {
      t += 0.05;
      const now = Date.now();

      // Handle Cyclic Actuator and Cycle Counting
      if (cyclicState.state === 'RUNNING') {
        const halfPeriodMs = (1000 / cyclicState.frequencyHz) / 2;
        if (now - lastStateToggle.current >= halfPeriodMs) {
          lastStateToggle.current = now;
          setCyclicState(prev => {
            const nextActuator = !prev.actuatorState;
            const newCycle = nextActuator ? prev.currentCycle + 1 : prev.currentCycle;
            const isCompleted = prev.targetCycles > 0 && newCycle >= prev.targetCycles;

            return {
              ...prev,
              actuatorState: nextActuator,
              syncPulse: nextActuator,
              currentCycle: newCycle,
              state: isCompleted ? 'COMPLETED' : prev.state
            };
          });
        }
      }

      // Generate signal for 16 channels
      // Base waveform is a cyclic test response (sine + actuator square wave harmonic + noise)
      const baseSignal = 1650 + 900 * Math.sin(t * (cyclicState.frequencyHz * 2 * Math.PI) * 0.1);
      const actuatorCoupling = cyclicState.actuatorState ? 300 : -200;
      const gaussianNoise = (Math.random() - 0.5) * noiseLevel * 40;
      // Occasional spike/impulse to demonstrate median filter
      const spike = Math.random() < 0.03 ? (Math.random() < 0.5 ? 600 : -600) : 0;

      const rawVal = Math.min(3300, Math.max(0, baseSignal + actuatorCoupling + gaussianNoise + spike));

      // Real-time software filter emulation matching STM32 driver
      let filteredVal = rawVal;
      const hist = historyRef.current;

      if (filterType === 'MAV') {
        const windowSize = 8;
        const slice = hist.slice(-windowSize);
        const sum = slice.reduce((acc, curr) => acc + curr.raw, rawVal);
        filteredVal = sum / (slice.length + 1);
      } else if (filterType === 'EMA') {
        const alpha = 0.25;
        const prev = hist.length > 0 ? hist[hist.length - 1].filtered : rawVal;
        filteredVal = alpha * rawVal + (1 - alpha) * prev;
      } else if (filterType === 'IIR_LPF') {
        // 2nd-order Butterworth simulation
        const prev1 = hist.length > 0 ? hist[hist.length - 1].filtered : rawVal;
        const prev2 = hist.length > 1 ? hist[hist.length - 2].filtered : rawVal;
        filteredVal = 0.09 * rawVal + 0.18 * (hist.length > 0 ? hist[hist.length - 1].raw : rawVal) + 0.09 * (hist.length > 1 ? hist[hist.length - 2].raw : rawVal) + 1.14 * prev1 - 0.41 * prev2;
      } else if (filterType === 'MEDIAN') {
        const p1 = hist.length > 0 ? hist[hist.length - 1].raw : rawVal;
        const p2 = hist.length > 1 ? hist[hist.length - 2].raw : rawVal;
        const sorted = [rawVal, p1, p2].sort((a, b) => a - b);
        filteredVal = sorted[1];
      }

      hist.push({ raw: rawVal, filtered: filteredVal, time: t });
      if (hist.length > 300) hist.shift();

      // Draw on canvas
      const canvas = canvasRef.current;
      if (canvas) {
        const ctx = canvas.getContext('2d');
        if (ctx) {
          const w = canvas.width;
          const h = canvas.height;

          // Clear
          ctx.fillStyle = '#0f172a';
          ctx.fillRect(0, 0, w, h);

          // Grid lines
          ctx.strokeStyle = '#1e293b';
          ctx.lineWidth = 1;
          for (let x = 0; x < w; x += 40) {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, h);
            ctx.stroke();
          }
          for (let y = 0; y < h; y += 30) {
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(w, y);
            ctx.stroke();
          }

          // Center 1.65V line
          ctx.strokeStyle = '#334155';
          ctx.setLineDash([4, 4]);
          ctx.beginPath();
          ctx.moveTo(0, h / 2);
          ctx.lineTo(w, h / 2);
          ctx.stroke();
          ctx.setLineDash([]);

          // Draw Raw Waveform (Amber / Red dotted)
          if (showRawCompare && hist.length > 1) {
            ctx.strokeStyle = '#f59e0b';
            ctx.lineWidth = 1.2;
            ctx.globalAlpha = 0.5;
            ctx.beginPath();
            hist.forEach((pt, idx) => {
              const x = (idx / 300) * w;
              const y = h - (pt.raw / 3300) * h;
              if (idx === 0) ctx.moveTo(x, y);
              else ctx.lineTo(x, y);
            });
            ctx.stroke();
            ctx.globalAlpha = 1.0;
          }

          // Draw Filtered Waveform (Emerald Neon Green)
          if (hist.length > 1) {
            ctx.strokeStyle = '#10b981';
            ctx.lineWidth = 2.2;
            ctx.beginPath();
            hist.forEach((pt, idx) => {
              const x = (idx / 300) * w;
              const y = h - (pt.filtered / 3300) * h;
              if (idx === 0) ctx.moveTo(x, y);
              else ctx.lineTo(x, y);
            });
            ctx.stroke();
          }

          // Actuator digital pulse overlay in bottom 15%
          ctx.fillStyle = cyclicState.actuatorState ? 'rgba(245, 158, 11, 0.25)' : 'rgba(30, 41, 59, 0.5)';
          ctx.fillRect(0, h - 24, w, 24);
          ctx.fillStyle = '#94a3b8';
          ctx.font = '10px monospace';
          ctx.fillText(`ACTUATOR (PE14): ${cyclicState.actuatorState ? 'ENERGIZED (ON)' : 'RELEASED (OFF)'}`, 12, h - 8);
        }
      }

      animationFrameId = requestAnimationFrame(render);
    };

    animationFrameId = requestAnimationFrame(render);
    return () => cancelAnimationFrame(animationFrameId);
  }, [cyclicState.state, cyclicState.frequencyHz, cyclicState.actuatorState, filterType, noiseLevel, showRawCompare]);

  // Cyclic control buttons
  const handleStart = () => {
    setCyclicState(prev => ({
      ...prev,
      state: 'RUNNING',
      currentCycle: prev.state === 'COMPLETED' ? 0 : prev.currentCycle
    }));
  };

  const handlePause = () => {
    setCyclicState(prev => ({ ...prev, state: 'PAUSED', actuatorState: false }));
  };

  const handleStop = () => {
    setCyclicState(prev => ({
      ...prev,
      state: 'IDLE',
      currentCycle: 0,
      actuatorState: false,
      syncPulse: false
    }));
  };

  // Channel voltages preview
  const currentVolt = historyRef.current.length > 0 
    ? historyRef.current[historyRef.current.length - 1].filtered 
    : 1650;

  return (
    <div className="space-y-6">
      {/* Top Banner & Cyclic Actuator Status */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Left 8 cols: Live Oscilloscope Canvas */}
        <div className="lg:col-span-8 bg-slate-900 border border-slate-800 rounded-xl p-4 shadow-sm space-y-3">
          <div className="flex flex-wrap items-center justify-between gap-3 border-b border-slate-800 pb-3">
            <div className="flex items-center gap-3">
              <div className="w-8 h-8 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center text-emerald-400">
                <Activity className="w-4 h-4" />
              </div>
              <div>
                <h3 className="font-bold text-white text-sm">
                  اسیلوسکوپ کانال {selectedChannel} (ADC1_IN4 / MUX Pin {selectedChannel})
                </h3>
                <span className="text-xs text-slate-400 font-mono">
                  دامنه ورودی: 0.0 الی 3300.0 میلی‌ولت | Vref = 3.30V
                </span>
              </div>
            </div>

            {/* Filter Toggle & Compare */}
            <div className="flex items-center gap-2">
              <button
                id="toggle-raw"
                onClick={() => setShowRawCompare(!showRawCompare)}
                className={`px-2.5 py-1 text-xs font-mono rounded-md border transition-colors flex items-center gap-1.5 ${
                  showRawCompare 
                    ? 'bg-amber-500/20 text-amber-300 border-amber-500/40' 
                    : 'bg-slate-800 text-slate-400 border-slate-700'
                }`}
              >
                <Eye className="w-3.5 h-3.5" />
                <span>نمایش سیگنال خام (Raw)</span>
              </button>

              <div className="text-xs font-mono px-2.5 py-1 rounded bg-slate-800 border border-slate-700 text-emerald-400 font-bold">
                {currentVolt.toFixed(1)} mV
              </div>
            </div>
          </div>

          {/* Canvas */}
          <div className="relative rounded-lg overflow-hidden border border-slate-800">
            <canvas
              ref={canvasRef}
              width={760}
              height={320}
              className="w-full h-72 block bg-slate-950"
            />

            {/* Overlay indicators */}
            <div className="absolute top-2 left-3 flex items-center gap-3 text-[11px] font-mono pointer-events-none">
              <span className="text-emerald-400 flex items-center gap-1">
                <span className="w-2 h-2 rounded-full bg-emerald-400" />
                فیلتر شده ({filterType})
              </span>
              {showRawCompare && (
                <span className="text-amber-400 flex items-center gap-1">
                  <span className="w-2 h-2 rounded-full bg-amber-400" />
                  سیگنال خام با نویز (Raw + Spikes)
                </span>
              )}
            </div>

            {/* SYNC Pulse indicator */}
            {cyclicState.syncPulse && (
              <div className="absolute top-2 right-3 text-[11px] font-mono bg-blue-500/20 text-blue-300 border border-blue-500/40 px-2 py-0.5 rounded animate-pulse">
                SYNC_OUT (PE13) PULSE
              </div>
            )}
          </div>

          {/* 16 Channels Selector Buttons */}
          <div className="pt-2">
            <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider block mb-2 font-mono">
              انتخاب کانال مالتی‌پلکسر (PE8..PE11 Select Lines):
            </span>
            <div className="grid grid-cols-8 sm:grid-cols-16 gap-1">
              {Array.from({ length: 16 }).map((_, idx) => (
                <button
                  key={idx}
                  id={`ch-btn-${idx}`}
                  onClick={() => setSelectedChannel(idx)}
                  className={`py-1.5 text-xs font-mono font-bold rounded border transition-all ${
                    selectedChannel === idx
                      ? 'bg-emerald-600 text-white border-emerald-400 shadow-sm'
                      : 'bg-slate-800 text-slate-400 border-slate-700 hover:text-slate-200 hover:bg-slate-750'
                  }`}
                >
                  C{idx}
                </button>
              ))}
            </div>
          </div>
        </div>

        {/* Right 4 cols: Cyclic Test Control Panel */}
        <div className="lg:col-span-4 bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm space-y-5">
          <div className="flex items-center justify-between border-b border-slate-800 pb-3">
            <h3 className="font-bold text-white text-sm flex items-center gap-2">
              <Zap className="w-4 h-4 text-amber-400" />
              کنترلر آزمون خستگی چرخه‌ای
            </h3>
            <span className={`px-2 py-0.5 text-xs font-mono rounded font-bold border ${
              cyclicState.state === 'RUNNING' ? 'bg-amber-500/20 text-amber-300 border-amber-500/30' :
              cyclicState.state === 'PAUSED' ? 'bg-blue-500/20 text-blue-300 border-blue-500/30' :
              cyclicState.state === 'COMPLETED' ? 'bg-emerald-500/20 text-emerald-300 border-emerald-500/30' :
              'bg-slate-800 text-slate-400 border-slate-700'
            }`}>
              {cyclicState.state}
            </span>
          </div>

          {/* Cycle Counter Display */}
          <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 text-center space-y-1">
            <span className="text-xs text-slate-400 font-mono uppercase tracking-wider block">
              شمارنده سیکل (Cycle Counter)
            </span>
            <div className="text-3xl font-extrabold font-mono text-emerald-400 tracking-tight">
              {cyclicState.currentCycle.toLocaleString()}
            </div>
            <span className="text-xs text-slate-500 font-mono block">
              از مجموع {cyclicState.targetCycles.toLocaleString()} سیکل هدف
            </span>

            {/* Progress Bar */}
            <div className="w-full bg-slate-800 h-1.5 rounded-full overflow-hidden mt-2">
              <div 
                className="bg-emerald-500 h-full transition-all duration-300"
                style={{ width: `${Math.min(100, (cyclicState.currentCycle / cyclicState.targetCycles) * 100)}%` }}
              />
            </div>
          </div>

          {/* Main Action Buttons */}
          <div className="grid grid-cols-3 gap-2">
            <button
              id="btn-cyclic-start"
              onClick={handleStart}
              disabled={cyclicState.state === 'RUNNING'}
              className="flex items-center justify-center gap-1.5 py-2 px-3 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-bold transition-colors disabled:opacity-50"
            >
              <Play className="w-3.5 h-3.5" />
              شروع
            </button>
            <button
              id="btn-cyclic-pause"
              onClick={handlePause}
              disabled={cyclicState.state !== 'RUNNING'}
              className="flex items-center justify-center gap-1.5 py-2 px-3 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-bold border border-slate-700 transition-colors disabled:opacity-50"
            >
              <Pause className="w-3.5 h-3.5" />
              مکث
            </button>
            <button
              id="btn-cyclic-stop"
              onClick={handleStop}
              className="flex items-center justify-center gap-1.5 py-2 px-3 rounded-lg bg-rose-600/80 hover:bg-rose-500 text-white text-xs font-bold transition-colors"
            >
              <Square className="w-3.5 h-3.5" />
              توقف
            </button>
          </div>

          {/* Controls: Frequency & Target Cycles */}
          <div className="space-y-4 pt-2 border-t border-slate-800/80">
            {/* Frequency Slider */}
            <div>
              <div className="flex justify-between text-xs font-mono mb-1">
                <span className="text-slate-400">فرکانس تحریک (Frequency):</span>
                <span className="text-amber-400 font-bold">{cyclicState.frequencyHz.toFixed(1)} Hz</span>
              </div>
              <input
                id="freq-slider"
                type="range"
                min="0.1"
                max="10.0"
                step="0.1"
                value={cyclicState.frequencyHz}
                onChange={e => setCyclicState(prev => ({ ...prev, frequencyHz: parseFloat(e.target.value) }))}
                className="w-full accent-amber-500 cursor-pointer"
              />
            </div>

            {/* Target Cycles */}
            <div>
              <label className="text-xs font-mono text-slate-400 block mb-1">
                تعداد سیکل هدف (Target Cycles):
              </label>
              <input
                id="target-cycles-input"
                type="number"
                value={cyclicState.targetCycles}
                onChange={e => setCyclicState(prev => ({ ...prev, targetCycles: Math.max(10, parseInt(e.target.value) || 1000) }))}
                className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-1.5 text-xs font-mono text-white focus:outline-none focus:border-emerald-500"
              />
            </div>

            {/* Actuator & Output Flags status */}
            <div className="bg-slate-800/60 p-3 rounded-lg border border-slate-700/60 space-y-2 text-xs font-mono">
              <div className="flex items-center justify-between">
                <span className="text-slate-400">خروجی اکچویتور (PE14):</span>
                <span className={`px-2 py-0.5 rounded text-[10px] font-bold ${
                  cyclicState.actuatorState 
                    ? 'bg-amber-500/20 text-amber-300 border border-amber-500/40' 
                    : 'bg-slate-700 text-slate-400'
                }`}>
                  {cyclicState.actuatorState ? 'ENERGIZED (1)' : 'DE-ENERGIZED (0)'}
                </span>
              </div>

              <div className="flex items-center justify-between">
                <span className="text-slate-400">پالس همگام‌ساز (PE13):</span>
                <span className="text-blue-400 font-bold">10 µs SYNC Pulse</span>
              </div>
            </div>
          </div>

          {/* DSP Filter Engine Selector */}
          <div className="pt-2 border-t border-slate-800/80 space-y-3">
            <h4 className="text-xs font-bold text-slate-300 uppercase tracking-wider flex items-center gap-1.5 font-mono">
              <Sliders className="w-3.5 h-3.5 text-emerald-400" />
              فیلترهای پردازش سیگنال دیجیتال
            </h4>

            <div className="grid grid-cols-2 gap-1.5">
              {[
                { id: 'BYPASS', label: 'بدون فیلتر (Raw)' },
                { id: 'MAV', label: 'میانگین متحرک (MAV)' },
                { id: 'EMA', label: 'فیلتر نمایی (EMA)' },
                { id: 'IIR_LPF', label: 'باترورث (Butterworth)' },
                { id: 'MEDIAN', label: 'حذف اسپایک (Median)' }
              ].map(f => (
                <button
                  key={f.id}
                  id={`filter-${f.id}`}
                  onClick={() => setFilterType(f.id as FilterType)}
                  className={`px-2 py-1.5 text-[11px] font-medium rounded text-left transition-colors border ${
                    filterType === f.id
                      ? 'bg-emerald-600 text-white border-emerald-400'
                      : 'bg-slate-800 text-slate-300 border-slate-700 hover:bg-slate-750'
                  }`}
                >
                  {f.label}
                </button>
              ))}
            </div>

            {/* Noise Injection Slider for Demo */}
            <div>
              <div className="flex justify-between text-[11px] font-mono mb-1">
                <span className="text-slate-400">تزریق نویز به ورودی سنسور:</span>
                <span className="text-slate-300">{noiseLevel}%</span>
              </div>
              <input
                id="noise-slider"
                type="range"
                min="0"
                max="100"
                value={noiseLevel}
                onChange={e => setNoiseLevel(parseInt(e.target.value))}
                className="w-full accent-emerald-500 cursor-pointer"
              />
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
