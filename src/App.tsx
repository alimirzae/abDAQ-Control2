import React, { useState } from 'react';
import { Header } from './components/Header';
import { WiringView } from './components/WiringView';
import { DaqOscilloscope } from './components/DaqOscilloscope';
import { ProtocolConsole } from './components/ProtocolConsole';
import { WebServerDashboard } from './components/WebServerDashboard';
import { CodeExplorer } from './components/CodeExplorer';
import { DocReader } from './components/DocReader';
import { ActiveTab, CyclicTestState, FilterType } from './types';

export default function App() {
  const [activeTab, setActiveTab] = useState<ActiveTab>('wiring');
  const [filterType, setFilterType] = useState<FilterType>('IIR_LPF');
  const [noiseLevel, setNoiseLevel] = useState<number>(30);

  const [cyclicState, setCyclicState] = useState<CyclicTestState>({
    state: 'RUNNING',
    frequencyHz: 1.0,
    currentCycle: 1248,
    targetCycles: 20000,
    actuatorState: false,
    syncPulse: false,
    sampleRateHz: 1000
  });

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 flex flex-col font-sans selection:bg-emerald-500 selection:text-white">
      {/* Main App Navigation Bar */}
      <Header
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        cyclicState={cyclicState}
        sampleRate={cyclicState.sampleRateHz / 1000}
      />

      {/* Main Workspace Container */}
      <main className="flex-1 max-w-7xl w-full mx-auto px-4 sm:px-6 lg:px-8 py-6">
        {activeTab === 'wiring' && <WiringView />}

        {activeTab === 'daq' && (
          <DaqOscilloscope
            cyclicState={cyclicState}
            setCyclicState={setCyclicState}
            filterType={filterType}
            setFilterType={setFilterType}
            noiseLevel={noiseLevel}
            setNoiseLevel={setNoiseLevel}
          />
        )}

        {activeTab === 'protocols' && (
          <ProtocolConsole
            cyclicState={cyclicState}
            setCyclicState={setCyclicState}
          />
        )}

        {activeTab === 'webserver' && (
          <WebServerDashboard
            cyclicState={cyclicState}
            setCyclicState={setCyclicState}
            filterType={filterType}
            setFilterType={setFilterType}
          />
        )}

        {activeTab === 'code' && <CodeExplorer />}

        {activeTab === 'docs' && <DocReader />}
      </main>

      {/* Footer */}
      <footer className="border-t border-slate-900 bg-slate-950/80 py-4 text-center text-xs text-slate-500 font-mono">
        <div className="max-w-7xl mx-auto px-4 flex flex-col sm:flex-row items-center justify-between gap-2">
          <span>LabDAQ-Control — STM32F407VGT6 DAQ &amp; Cyclic Fatigue Testing Platform</span>
          <span>EWB-STM32F407V-LAN-V3.0 | CD74HC4067 | TXS0108E | LAN8720A</span>
        </div>
      </footer>
    </div>
  );
}
