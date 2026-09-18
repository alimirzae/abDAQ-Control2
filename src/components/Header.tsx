import React from 'react';
import { 
  Activity, 
  Cpu, 
  Layers, 
  Terminal, 
  FileCode, 
  BookOpen, 
  Radio, 
  CheckCircle2, 
  Zap,
  Gauge,
  Globe
} from 'lucide-react';
import { ActiveTab, CyclicTestState } from '../types';

interface HeaderProps {
  activeTab: ActiveTab;
  setActiveTab: (tab: ActiveTab) => void;
  cyclicState: CyclicTestState;
  sampleRate: number;
}

export const Header: React.FC<HeaderProps> = ({
  activeTab,
  setActiveTab,
  cyclicState,
  sampleRate
}) => {
  return (
    <header className="bg-slate-900 border-b border-slate-800 text-slate-100 sticky top-0 z-50">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          {/* Logo & Hardware Identity */}
          <div className="flex items-center gap-3">
            <div className="w-10 h-10 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center text-emerald-400">
              <Activity className="w-5 h-5" />
            </div>
            <div>
              <div className="flex items-center gap-2">
                <span className="font-bold text-lg tracking-tight text-white">LabDAQ-Control</span>
                <span className="px-2 py-0.5 text-xs font-mono font-semibold bg-blue-900/60 border border-blue-500/30 text-blue-300 rounded">
                  STM32F407
                </span>
                <span className="hidden sm:inline-block px-2 py-0.5 text-xs font-mono bg-emerald-900/40 border border-emerald-500/30 text-emerald-300 rounded">
                  16-CH ADC + TXS0108E
                </span>
              </div>
              <p className="text-xs text-slate-400 hidden sm:block">
                سیستم داده‌برداری پرسرعت، آزمون چرخه‌ای و فیلترهای بلادرنگ
              </p>
            </div>
          </div>

          {/* Live System Telemetry Badges */}
          <div className="flex items-center gap-3">
            {/* Actuator / Cyclic status */}
            <div className={`flex items-center gap-1.5 px-2.5 py-1 rounded-md text-xs font-mono border ${
              cyclicState.state === 'RUNNING'
                ? 'bg-amber-500/10 border-amber-500/30 text-amber-300 animate-pulse'
                : 'bg-slate-800 border-slate-700 text-slate-400'
            }`}>
              <Zap className="w-3.5 h-3.5" />
              <span>TEST: {cyclicState.state}</span>
              {cyclicState.state === 'RUNNING' && (
                <span className="font-bold text-amber-200">({cyclicState.currentCycle}/{cyclicState.targetCycles})</span>
              )}
            </div>

            {/* Sampling Rate */}
            <div className="hidden md:flex items-center gap-1.5 px-2.5 py-1 rounded-md text-xs font-mono bg-slate-800 border border-slate-700 text-slate-300">
              <Gauge className="w-3.5 h-3.5 text-blue-400" />
              <span>{sampleRate} kSPS/ch</span>
            </div>

            {/* Online Status */}
            <div className="flex items-center gap-1.5 px-2 py-1 rounded-md text-xs font-mono bg-emerald-500/10 border border-emerald-500/30 text-emerald-400">
              <span className="w-2 h-2 rounded-full bg-emerald-400 animate-ping" />
              <span className="hidden sm:inline">ONLINE</span>
            </div>
          </div>
        </div>

        {/* Tab Navigation */}
        <nav className="flex space-x-1 sm:space-x-4 border-t border-slate-800/80 pt-1 overflow-x-auto scrollbar-none">
          <button
            id="tab-wiring"
            onClick={() => setActiveTab('wiring')}
            className={`flex items-center gap-2 py-2.5 px-3 border-b-2 text-sm font-medium transition-colors whitespace-nowrap ${
              activeTab === 'wiring'
                ? 'border-emerald-500 text-emerald-400'
                : 'border-transparent text-slate-400 hover:text-slate-200 hover:border-slate-700'
            }`}
          >
            <Layers className="w-4 h-4" />
            <span>سیم‌بندی و پین‌اوت (Wiring)</span>
          </button>

          <button
            id="tab-daq"
            onClick={() => setActiveTab('daq')}
            className={`flex items-center gap-2 py-2.5 px-3 border-b-2 text-sm font-medium transition-colors whitespace-nowrap ${
              activeTab === 'daq'
                ? 'border-emerald-500 text-emerald-400'
                : 'border-transparent text-slate-400 hover:text-slate-200 hover:border-slate-700'
            }`}
          >
            <Activity className="w-4 h-4" />
            <span>اسیلوسکوپ و آزمون چرخه‌ای (Live DAQ)</span>
          </button>

          <button
            id="tab-protocols"
            onClick={() => setActiveTab('protocols')}
            className={`flex items-center gap-2 py-2.5 px-3 border-b-2 text-sm font-medium transition-colors whitespace-nowrap ${
              activeTab === 'protocols'
                ? 'border-emerald-500 text-emerald-400'
                : 'border-transparent text-slate-400 hover:text-slate-200 hover:border-slate-700'
            }`}
          >
            <Terminal className="w-4 h-4" />
            <span>پروتکل‌ها (SCPI / Modbus / UDP)</span>
          </button>

          <button
            id="tab-webserver"
            onClick={() => setActiveTab('webserver')}
            className={`flex items-center gap-2 py-2.5 px-3 border-b-2 text-sm font-medium transition-colors whitespace-nowrap ${
              activeTab === 'webserver'
                ? 'border-emerald-500 text-emerald-400'
                : 'border-transparent text-slate-400 hover:text-slate-200 hover:border-slate-700'
            }`}
          >
            <Globe className="w-4 h-4" />
            <span>وب‌سرور چارت زنده (HTML Server)</span>
          </button>

          <button
            id="tab-code"
            onClick={() => setActiveTab('code')}
            className={`flex items-center gap-2 py-2.5 px-3 border-b-2 text-sm font-medium transition-colors whitespace-nowrap ${
              activeTab === 'code'
                ? 'border-emerald-500 text-emerald-400'
                : 'border-transparent text-slate-400 hover:text-slate-200 hover:border-slate-700'
            }`}
          >
            <FileCode className="w-4 h-4" />
            <span>کدهای سورس فریم‌ور (STM32 Code)</span>
          </button>

          <button
            id="tab-docs"
            onClick={() => setActiveTab('docs')}
            className={`flex items-center gap-2 py-2.5 px-3 border-b-2 text-sm font-medium transition-colors whitespace-nowrap ${
              activeTab === 'docs'
                ? 'border-emerald-500 text-emerald-400'
                : 'border-transparent text-slate-400 hover:text-slate-200 hover:border-slate-700'
            }`}
          >
            <BookOpen className="w-4 h-4" />
            <span>مستندات مخزن (Docs & Memory)</span>
          </button>
        </nav>
      </div>
    </header>
  );
};
