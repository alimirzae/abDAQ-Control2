import React, { useState } from 'react';
import { 
  Terminal, 
  Send, 
  Database, 
  Cpu, 
  Binary, 
  Copy, 
  Check, 
  RotateCcw,
  Sparkles,
  Zap,
  Wifi
} from 'lucide-react';
import { CyclicTestState, ModbusRegister } from '../types';

interface ProtocolConsoleProps {
  cyclicState: CyclicTestState;
  setCyclicState: React.Dispatch<React.SetStateAction<CyclicTestState>>;
}

export const ProtocolConsole: React.FC<ProtocolConsoleProps> = ({
  cyclicState,
  setCyclicState
}) => {
  const [activeSubTab, setActiveSubTab] = useState<'scpi' | 'modbus' | 'binary' | 'udp'>('scpi');
  const [udpCmdInput, setUdpCmdInput] = useState(':RATE 1000');
  const [udpTargetIp, setUdpTargetIp] = useState('239.255.0.100');
  const [udpMode, setUdpMode] = useState<'MULTICAST' | 'UNICAST'>('MULTICAST');
  const [udpLogs, setUdpLogs] = useState<Array<{ id: number; time: string; source: string; text: string; dir: 'tx' | 'rx' }>>([
    { id: 1, time: '00:00:01.000125', source: '239.255.0.100:5001', text: '[STREAM] Seq: #1024, Timestamp: 1420500125 µs, Rate: 1000 Hz, Ch0: 1650 mV, Ch1: 1720 mV', dir: 'rx' },
    { id: 2, time: '00:00:01.001125', source: '239.255.0.100:5001', text: '[STREAM] Seq: #1025, Timestamp: 1420501125 µs, Rate: 1000 Hz, Ch0: 1652 mV, Ch1: 1721 mV', dir: 'rx' },
    { id: 3, time: '00:00:01.045000', source: 'CLIENT -> UDP:5001', text: ':RATE 2000', dir: 'tx' },
    { id: 4, time: '00:00:01.045280', source: 'STM32 -> CLIENT', text: 'OK: RATE=2000 Hz (Source: UDP_SOCKET:5001)\r\n', dir: 'rx' }
  ]);

  // SCPI Console State
  const [scpiInput, setScpiInput] = useState('*IDN?');
  const [scpiLogs, setScpiLogs] = useState<Array<{ id: number; time: string; type: 'cmd' | 'resp' | 'err'; text: string }>>([
    { id: 1, time: '00:00:01', type: 'cmd', text: '*IDN?' },
    { id: 2, time: '00:00:01', type: 'resp', text: 'LabDAQ Instrumentation,LabDAQ-STM32F407-16CH,SN-407V3,v1.0.0\\r\\n' },
    { id: 3, time: '00:00:02', type: 'cmd', text: ':RATE 1000' },
    { id: 4, time: '00:00:02', type: 'resp', text: 'OK: RATE=1000 Hz\\r\\n' }
  ]);

  // Execute SCPI
  const executeScpi = (cmd: string) => {
    const trimmed = cmd.trim();
    if (!trimmed) return;

    const timeStr = new Date().toTimeString().split(' ')[0];
    const newLogs = [...scpiLogs, { id: Date.now(), time: timeStr, type: 'cmd' as const, text: trimmed }];

    let response = '';
    const upper = trimmed.toUpperCase();

    if (upper === '*IDN?') {
      response = 'LabDAQ Instrumentation,LabDAQ-STM32F407-16CH,SN-407V3,v1.0.0\\r\\n';
    } else if (upper === '*RST') {
      response = 'OK: RESET\\r\\n';
      setCyclicState(prev => ({ ...prev, state: 'IDLE', currentCycle: 0 }));
    } else if (upper === ':MEAS:VOLT:ALL?') {
      const volts = Array.from({ length: 16 }, (_, i) => (1200 + i * 85 + Math.random() * 20).toFixed(2));
      response = `:MEAS:VOLT ${volts.join(',')}\\r\\n`;
    } else if (upper.startsWith(':MEAS:VOLT:CHAN?')) {
      const ch = parseInt(upper.replace(':MEAS:VOLT:CHAN?', '').trim()) || 0;
      const v = (1200 + ch * 85 + Math.random() * 10).toFixed(2);
      response = `${v} mV\\r\\n`;
    } else if (upper.startsWith(':RATE')) {
      const rate = parseInt(upper.replace(':RATE', '').trim()) || 1000;
      response = `OK: RATE=${rate} Hz\\r\\n`;
      setCyclicState(prev => ({ ...prev, sampleRateHz: rate }));
    } else if (upper.startsWith(':CYCLIC:START')) {
      const params = upper.replace(':CYCLIC:START', '').trim().split(',');
      const freq = parseFloat(params[0]) || 1.0;
      const target = parseInt(params[1]) || 10000;
      setCyclicState(prev => ({ ...prev, state: 'RUNNING', frequencyHz: freq, targetCycles: target }));
      response = `OK: CYCLIC TEST STARTED F=${freq.toFixed(2)}Hz, TARGET=${target}\\r\\n`;
    } else if (upper === ':CYCLIC:STOP') {
      setCyclicState(prev => ({ ...prev, state: 'IDLE', actuatorState: false }));
      response = 'OK: CYCLIC TEST STOPPED\\r\\n';
    } else if (upper === ':CYCLIC:STATUS?') {
      response = `STATE=${cyclicState.state},CYCLES=${cyclicState.currentCycle},TARGET=${cyclicState.targetCycles},FREQ=${cyclicState.frequencyHz.toFixed(2)}\\r\\n`;
    } else {
      response = 'ERR: UNKNOWN COMMAND\\r\\n';
    }

    newLogs.push({ id: Date.now() + 1, time: timeStr, type: response.startsWith('ERR') ? 'err' : 'resp', text: response });
    setScpiLogs(newLogs);
    setScpiInput('');
  };

  // Modbus Register Map
  const modbusRegisters: ModbusRegister[] = [
    { addressHex: '0x0000', addressDec: 0, name: 'DEVICE_ID', type: 'RO', value: 0x4C44, formattedValue: '0x4C44 ("LD")', description: 'شناسه سخت‌افزاری LabDAQ' },
    { addressHex: '0x0001', addressDec: 1, name: 'FW_VERSION', type: 'RO', value: 0x0100, formattedValue: '0x0100 (v1.0)', description: 'نسخه فریم‌ور سیستم' },
    { addressHex: '0x0002', addressDec: 2, name: 'TEST_STATE', type: 'RO', value: cyclicState.state === 'RUNNING' ? 2 : cyclicState.state === 'PAUSED' ? 3 : cyclicState.state === 'COMPLETED' ? 4 : 0, formattedValue: `${cyclicState.state}`, description: 'وضعیت اجرای آزمون چرخه‌ای' },
    { addressHex: '0x0003', addressDec: 3, name: 'CYCLE_COUNT_LO', type: 'RO', value: cyclicState.currentCycle & 0xFFFF, formattedValue: `${cyclicState.currentCycle & 0xFFFF}`, description: '۱۶ بیت پایین شمارنده سیکل' },
    { addressHex: '0x0004', addressDec: 4, name: 'CYCLE_COUNT_HI', type: 'RO', value: (cyclicState.currentCycle >> 16) & 0xFFFF, formattedValue: `${(cyclicState.currentCycle >> 16) & 0xFFFF}`, description: '۱۶ بیت بالای شمارنده سیکل' },
    { addressHex: '0x0010', addressDec: 16, name: 'CH0_VOLTAGE', type: 'RO', value: 1652, formattedValue: '1652 mV', description: 'ولتاژ کانال صفر (آنالوگ ورودی)' },
    { addressHex: '0x0011', addressDec: 17, name: 'CH1_VOLTAGE', type: 'RO', value: 1740, formattedValue: '1740 mV', description: 'ولتاژ کانال یک' },
    { addressHex: '0x001F', addressDec: 31, name: 'CH15_VOLTAGE', type: 'RO', value: 2480, formattedValue: '2480 mV', description: 'ولتاژ کانال ۱۵ (انتهای مالتی‌پلکسر)' },
    { addressHex: '0x0020', addressDec: 32, name: 'CMD_CONTROL', type: 'RW', value: cyclicState.state === 'RUNNING' ? 1 : 0, formattedValue: cyclicState.state === 'RUNNING' ? '1 (START)' : '0 (STOP)', description: 'فرمان شروع (1)، توقف (0) یا مکث (2)' },
    { addressHex: '0x0021', addressDec: 33, name: 'SAMPLE_RATE', type: 'RW', value: cyclicState.sampleRateHz, formattedValue: `${cyclicState.sampleRateHz} Hz`, description: 'فرکانس نمونه‌برداری به هرتز' },
    { addressHex: '0x0022', addressDec: 34, name: 'TARGET_CYCLES_LO', type: 'RW', value: cyclicState.targetCycles & 0xFFFF, formattedValue: `${cyclicState.targetCycles & 0xFFFF}`, description: '۱۶ بیت پایین سیکل هدف' },
    { addressHex: '0x0024', addressDec: 36, name: 'EXCITE_FREQ_X100', type: 'RW', value: Math.round(cyclicState.frequencyHz * 100), formattedValue: `${Math.round(cyclicState.frequencyHz * 100)} (${cyclicState.frequencyHz.toFixed(1)} Hz)`, description: 'فرکانس تحریک چرخه‌ای ضرب در ۱۰۰' }
  ];

  return (
    <div className="space-y-6">
      {/* Top Banner & Sub-Tabs */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-4 shadow-sm flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
        <div>
          <h2 className="text-base font-bold text-white flex items-center gap-2">
            <Terminal className="w-5 h-5 text-emerald-400" />
            ترمینال پروتکل‌های ارتباطی (SCPI / Modbus RTU / TCP Binary)
          </h2>
          <p className="text-xs text-slate-400 mt-0.5">
            آزمایش و استعلام بلادرنگ سیستم LabDAQ با رابط خط فرمان استاندارد، رجیسترهای مدباس و استریم باینری.
          </p>
        </div>

        {/* Sub-tab pills */}
        <div className="flex bg-slate-800 p-1 rounded-lg border border-slate-700 text-xs font-mono">
          <button
            id="subtab-scpi"
            onClick={() => setActiveSubTab('scpi')}
            className={`px-3 py-1.5 rounded-md font-medium transition-colors ${
              activeSubTab === 'scpi' ? 'bg-emerald-600 text-white' : 'text-slate-400 hover:text-white'
            }`}
          >
            SCPI (USART1)
          </button>
          <button
            id="subtab-modbus"
            onClick={() => setActiveSubTab('modbus')}
            className={`px-3 py-1.5 rounded-md font-medium transition-colors ${
              activeSubTab === 'modbus' ? 'bg-emerald-600 text-white' : 'text-slate-400 hover:text-white'
            }`}
          >
            Modbus RTU (RS485)
          </button>
          <button
            id="subtab-binary"
            onClick={() => setActiveSubTab('binary')}
            className={`px-3 py-1.5 rounded-md font-medium transition-colors ${
              activeSubTab === 'binary' ? 'bg-emerald-600 text-white' : 'text-slate-400 hover:text-white'
            }`}
          >
            TCP Binary Frame
          </button>
          <button
            id="subtab-udp"
            onClick={() => setActiveSubTab('udp')}
            className={`px-3 py-1.5 rounded-md font-medium transition-colors ${
              activeSubTab === 'udp' ? 'bg-emerald-600 text-white' : 'text-slate-400 hover:text-white'
            }`}
          >
            UDP Multicast / Unicast (Port 5001)
          </button>
        </div>
      </div>

      {/* Subtab 1: SCPI Console */}
      {activeSubTab === 'scpi' && (
        <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
          {/* Console Window (8 cols) */}
          <div className="lg:col-span-8 bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-sm flex flex-col h-[480px]">
            {/* Terminal Header */}
            <div className="bg-slate-800/80 px-4 py-2 border-b border-slate-700/60 flex items-center justify-between text-xs font-mono">
              <div className="flex items-center gap-2">
                <span className="w-2.5 h-2.5 rounded-full bg-rose-500" />
                <span className="w-2.5 h-2.5 rounded-full bg-amber-500" />
                <span className="w-2.5 h-2.5 rounded-full bg-emerald-500" />
                <span className="text-slate-300 ml-2 font-semibold">USART1 @ 115200 8-N-1 (CP2104 USB VCP)</span>
              </div>
              <button
                id="btn-clear-terminal"
                onClick={() => setScpiLogs([])}
                className="text-slate-400 hover:text-white flex items-center gap-1"
              >
                <RotateCcw className="w-3.5 h-3.5" />
                پاک کردن
              </button>
            </div>

            {/* Log Terminal Output */}
            <div className="flex-1 p-4 overflow-y-auto font-mono text-xs space-y-2 bg-slate-950 scrollbar-thin scrollbar-thumb-slate-800">
              {scpiLogs.map(log => (
                <div key={log.id} className="flex items-start gap-2">
                  <span className="text-slate-600 select-none">[{log.time}]</span>
                  {log.type === 'cmd' ? (
                    <span className="text-emerald-400 font-bold">&gt; {log.text}</span>
                  ) : log.type === 'err' ? (
                    <span className="text-rose-400">{log.text}</span>
                  ) : (
                    <span className="text-slate-200">{log.text}</span>
                  )}
                </div>
              ))}
            </div>

            {/* Command Input Field */}
            <form
              onSubmit={e => {
                e.preventDefault();
                executeScpi(scpiInput);
              }}
              className="p-3 bg-slate-900 border-t border-slate-800 flex items-center gap-2"
            >
              <span className="text-emerald-400 font-mono font-bold pl-2">&gt;</span>
              <input
                id="scpi-cmd-input"
                type="text"
                value={scpiInput}
                onChange={e => setScpiInput(e.target.value)}
                placeholder="دستور SCPI را وارد کنید (مثلا *IDN? یا :MEAS:VOLT:ALL?)..."
                className="flex-1 bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-xs font-mono text-white placeholder-slate-500 focus:outline-none focus:border-emerald-500"
              />
              <button
                id="btn-send-scpi"
                type="submit"
                className="px-4 py-2 bg-emerald-600 hover:bg-emerald-500 text-white rounded-lg text-xs font-bold font-mono flex items-center gap-1.5 transition-colors"
              >
                <Send className="w-3.5 h-3.5" />
                ارسال
              </button>
            </form>
          </div>

          {/* Quick Commands Palette (4 cols) */}
          <div className="lg:col-span-4 bg-slate-900 border border-slate-800 rounded-xl p-4 shadow-sm space-y-4">
            <h3 className="text-xs font-bold uppercase tracking-wider text-slate-300 font-mono flex items-center gap-2">
              <Zap className="w-4 h-4 text-amber-400" />
              دستورات سریع SCPI (Quick Commands)
            </h3>
            <p className="text-xs text-slate-400">
              روی هر دستور کلیک کنید تا بلافاصله به پردازنده فریم‌ور ارسال شود:
            </p>

            <div className="space-y-2">
              {[
                { cmd: '*IDN?', label: 'استعلام مدل و نسخه فریم‌ور' },
                { cmd: ':MEAS:VOLT:ALL?', label: 'خواندن همزمان ولتاژ ۱۶ کانال' },
                { cmd: ':MEAS:VOLT:CHAN? 0', label: 'خواندن ولتاژ کانال صفر' },
                { cmd: ':RATE 1000', label: 'تنظیم فرکانس روی ۱۰۰۰ هرتز' },
                { cmd: ':CYCLIC:STATUS?', label: 'استعلام وضعیت آزمون چرخه‌ای' },
                { cmd: ':CYCLIC:START 2.0,10000', label: 'شروع تست با ۲ هرتز و ده‌هزار سیکل' },
                { cmd: ':CYCLIC:STOP', label: 'توقف فوری تست خستگی' },
                { cmd: '*RST', label: 'ریست تنظیمات و وضعیت' }
              ].map(item => (
                <button
                  key={item.cmd}
                  id={`quick-cmd-${item.cmd.replace(/[^a-zA-Z0-9]/g, '_')}`}
                  onClick={() => executeScpi(item.cmd)}
                  className="w-full text-left p-2 rounded-lg bg-slate-800/80 hover:bg-slate-700/80 border border-slate-700 transition-colors group"
                >
                  <div className="flex items-center justify-between">
                    <code className="text-xs font-bold font-mono text-emerald-400 group-hover:text-emerald-300">
                      {item.cmd}
                    </code>
                    <span className="text-[10px] text-slate-500 font-mono">Send</span>
                  </div>
                  <span className="text-[11px] text-slate-400 block mt-0.5">{item.label}</span>
                </button>
              ))}
            </div>
          </div>
        </div>
      )}

      {/* Subtab 2: Modbus RTU Register Inspector */}
      {activeSubTab === 'modbus' && (
        <div className="bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-sm">
          <div className="p-4 bg-slate-800/60 border-b border-slate-700/60 flex flex-wrap items-center justify-between gap-3 text-xs font-mono">
            <div className="flex items-center gap-3">
              <Database className="w-4 h-4 text-amber-400" />
              <span className="text-white font-bold">Modbus RTU Slave Map (Port: USART2 / RS-485 / SP3485)</span>
            </div>
            <div className="flex items-center gap-3 text-slate-400">
              <span>Slave ID: <b className="text-emerald-400">1</b></span>
              <span>Baudrate: <b className="text-emerald-400">115200</b></span>
              <span>DIR Pin: <b className="text-blue-400">PD7 (DE/~RE)</b></span>
            </div>
          </div>

          <div className="overflow-x-auto">
            <table className="w-full text-left border-collapse text-xs">
              <thead className="bg-slate-800 text-slate-400 uppercase tracking-wider font-mono text-[11px]">
                <tr>
                  <th className="py-2.5 px-4">آدرس هگز</th>
                  <th className="py-2.5 px-4">دسیمال</th>
                  <th className="py-2.5 px-4">نام رجیستر</th>
                  <th className="py-2.5 px-4">نوع</th>
                  <th className="py-2.5 px-4">مقدار زنده (Value)</th>
                  <th className="py-2.5 px-4">شرح عملکرد</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800 font-mono">
                {modbusRegisters.map(reg => (
                  <tr key={reg.addressHex} className="hover:bg-slate-800/40 transition-colors">
                    <td className="py-2.5 px-4 font-bold text-amber-400">{reg.addressHex}</td>
                    <td className="py-2.5 px-4 text-slate-400">{reg.addressDec}</td>
                    <td className="py-2.5 px-4 text-white font-semibold">{reg.name}</td>
                    <td className="py-2.5 px-4">
                      <span className={`px-2 py-0.5 text-[10px] rounded font-bold ${
                        reg.type === 'RO' ? 'bg-blue-900/40 text-blue-300 border border-blue-500/30' : 'bg-emerald-900/40 text-emerald-300 border border-emerald-500/30'
                      }`}>
                        {reg.type}
                      </span>
                    </td>
                    <td className="py-2.5 px-4 text-emerald-400 font-bold bg-slate-950/40">
                      {reg.formattedValue}
                    </td>
                    <td className="py-2.5 px-4 text-slate-300 font-sans">{reg.description}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* Subtab 3: TCP Binary Stream Inspector */}
      {activeSubTab === 'binary' && (
        <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm space-y-5">
          <div className="flex items-center justify-between border-b border-slate-800 pb-3">
            <div>
              <h3 className="text-sm font-bold text-white flex items-center gap-2">
                <Binary className="w-4 h-4 text-blue-400" />
                پکت داده باینری استریم پرسرعت اترنت (LAN8720A 100M TCP Frame)
              </h3>
              <p className="text-xs text-slate-400 mt-1">
                فرمت فریم ۵۱ بایتی فشرده با چک‌سام سخت‌افزاری CRC16-CCITT برای ارسال تا ۱۰,۰۰۰ فریم در ثانیه بدون تاخیر متن.
              </p>
            </div>
            <span className="text-xs font-mono bg-blue-500/10 text-blue-300 border border-blue-500/30 px-2.5 py-1 rounded">
              Fixed Frame Size: 51 Bytes
            </span>
          </div>

          {/* Byte Structure Visualizer */}
          <div className="grid grid-cols-1 md:grid-cols-4 gap-3 text-xs font-mono">
            <div className="bg-slate-950 p-3 rounded-lg border border-slate-800">
              <span className="text-slate-500 block text-[10px]">Bytes 0..1 (Header)</span>
              <span className="text-emerald-400 font-bold text-sm block">0xAA 0x55</span>
              <span className="text-slate-400 text-[11px]">بایت‌های جادویی تشخیص فریم</span>
            </div>

            <div className="bg-slate-950 p-3 rounded-lg border border-slate-800">
              <span className="text-slate-500 block text-[10px]">Byte 2..3 (Type & Ch Count)</span>
              <span className="text-blue-400 font-bold text-sm block">0x01 | 16 Channels</span>
              <span className="text-slate-400 text-[11px]">نوع بسته و تعداد کانال‌ها</span>
            </div>

            <div className="bg-slate-950 p-3 rounded-lg border border-slate-800">
              <span className="text-slate-500 block text-[10px]">Bytes 4..7 (Sequence Num)</span>
              <span className="text-amber-400 font-bold text-sm block"># {cyclicState.currentCycle * 16 + 120}</span>
              <span className="text-slate-400 text-[11px]">شمارنده فریم ارسالی (32-bit)</span>
            </div>

            <div className="bg-slate-950 p-3 rounded-lg border border-slate-800">
              <span className="text-slate-500 block text-[10px]">Bytes 49..50 (CRC Checksum)</span>
              <span className="text-purple-400 font-bold text-sm block">0x7F2A (Valid)</span>
              <span className="text-slate-400 text-[11px]">CRC16-CCITT (Poly 0x1021)</span>
            </div>
          </div>

          {/* Raw Hex Stream Preview */}
          <div>
            <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider block mb-2 font-mono">
              نمایش هگزادسیمال بسته نمونه در خط ارسال:
            </span>
            <div className="bg-slate-950 p-3.5 rounded-lg border border-slate-800 font-mono text-xs text-slate-300 leading-relaxed overflow-x-auto">
              <span className="text-emerald-400 font-bold">AA 55 </span>
              <span className="text-blue-400">01 10 </span>
              <span className="text-amber-400">00 00 04 D2 </span>
              <span className="text-slate-500">00 12 34 56 </span>
              <span className="text-white">
                06 74 06 72 06 70 06 75 06 78 06 80 06 82 06 85 
                06 88 06 90 06 92 06 95 06 98 07 00 07 05 07 10 
              </span>
              <span className="text-amber-400">02 00 00 04 D2 </span>
              <span className="text-purple-400 font-bold">7F 2A</span>
            </div>
          </div>
        </div>
      )}

      {/* Subtab 4: UDP Multicast & Symmetrical Command Engine */}
      {activeSubTab === 'udp' && (
        <div className="space-y-6">
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm space-y-4">
            <div className="flex flex-col md:flex-row md:items-center justify-between gap-3 border-b border-slate-800 pb-3">
              <div>
                <h3 className="text-sm font-bold text-white flex items-center gap-2">
                  <span className="w-2.5 h-2.5 rounded-full bg-emerald-400 animate-ping" />
                  استریم خودکار چندپخشی (UDP Multicast Auto-Stream) & پورت متقارن دستورات
                </h3>
                <p className="text-xs text-slate-400 mt-1">
                  ارسال ۱۰۰۰ نمونه در ثانیه با تگ زمان سخت‌افزاری میکروثانیه به آدرس گروهی <strong>239.255.0.100:5001</strong>. کلاینت نیازی به دانستن IP دستگاه ندارد و می‌تواند روی همان سوکت دستورات (:RATE، :FILTER و ...) را ارسال نماید.
                </p>
              </div>

              <div className="flex items-center gap-2 text-xs font-mono">
                <span className="px-2.5 py-1 rounded bg-slate-800 border border-slate-700 text-slate-300">
                  Rate: <strong className="text-emerald-400">1000 Hz</strong>
                </span>
                <span className="px-2.5 py-1 rounded bg-slate-800 border border-slate-700 text-slate-300">
                  Socket: <strong className="text-blue-400">Port 5001</strong>
                </span>
              </div>
            </div>

            {/* Live UDP Packet Console */}
            <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
              {/* Terminal feed (8 cols) */}
              <div className="lg:col-span-8 bg-slate-950 border border-slate-800 rounded-xl overflow-hidden flex flex-col h-[380px]">
                <div className="bg-slate-900 px-4 py-2 border-b border-slate-800 flex items-center justify-between text-xs font-mono">
                  <span className="text-slate-300 font-bold">
                    UDP Socket Monitor (Inbound & Outbound on Port 5001)
                  </span>
                  <button
                    onClick={() => setUdpLogs([])}
                    className="text-slate-500 hover:text-slate-300 text-[11px]"
                  >
                    پاکسازی لاگ
                  </button>
                </div>

                <div className="flex-1 p-3 overflow-y-auto space-y-1.5 font-mono text-xs">
                  {udpLogs.map(log => (
                    <div 
                      key={log.id} 
                      className={`p-1.5 rounded flex items-start gap-2 ${
                        log.dir === 'tx' 
                          ? 'bg-amber-950/20 text-amber-300 border border-amber-900/30' 
                          : 'bg-slate-900/40 text-slate-300'
                      }`}
                    >
                      <span className="text-slate-500 text-[10px] shrink-0">{log.time}</span>
                      <span className={`text-[10px] font-bold shrink-0 ${log.dir === 'tx' ? 'text-amber-400' : 'text-blue-400'}`}>
                        [{log.source}]
                      </span>
                      <span className="break-all">{log.text}</span>
                    </div>
                  ))}
                </div>

                {/* UDP Command Input */}
                <div className="p-3 bg-slate-900 border-t border-slate-800 flex gap-2">
                  <input
                    type="text"
                    value={udpCmdInput}
                    onChange={(e) => setUdpCmdInput(e.target.value)}
                    onKeyDown={(e) => {
                      if (e.key === 'Enter' && udpCmdInput.trim()) {
                        const cmd = udpCmdInput.trim();
                        const timeStr = new Date().toTimeString().split(' ')[0] + '.' + String(Math.floor(Math.random()*900 + 100));
                        const newLogs = [...udpLogs, {
                          id: Date.now(),
                          time: timeStr,
                          source: 'CLIENT -> UDP:5001',
                          text: cmd,
                          dir: 'tx' as const
                        }];

                        let resp = '';
                        if (cmd.startsWith(':RATE')) {
                          const r = parseInt(cmd.replace(':RATE', '').trim()) || 1000;
                          setCyclicState(prev => ({ ...prev, sampleRateHz: r }));
                          resp = `OK: RATE=${r} Hz (Source: UDP_SOCKET:5001)\r\n`;
                        } else if (cmd.startsWith(':FILTER')) {
                          resp = `OK: FILTER CONFIGURED VIA UDP (Source: UDP_SOCKET:5001)\r\n`;
                        } else if (cmd === '*IDN?') {
                          resp = `LabDAQ Instrumentation,LabDAQ-STM32F407-16CH,SN-407V3,v1.0.0 [SRC:UDP_SOCKET:5001]\r\n`;
                        } else {
                          resp = `OK: COMMAND PROCESSED IN UNIFIED ENGINE: ${cmd}\r\n`;
                        }

                        newLogs.push({
                          id: Date.now() + 1,
                          time: timeStr,
                          source: 'STM32 -> CLIENT',
                          text: resp,
                          dir: 'rx' as const
                        });

                        setUdpLogs(newLogs);
                        setUdpCmdInput('');
                      }
                    }}
                    placeholder="دستور متنی مانند :RATE 2000 یا :FILTER IIR 20 یا *IDN? را وارد کنید..."
                    className="flex-1 bg-slate-950 border border-slate-700 rounded-lg px-3 py-2 text-xs font-mono text-white focus:outline-none focus:border-emerald-500"
                  />
                  <button
                    onClick={() => {
                      if (!udpCmdInput.trim()) return;
                      const cmd = udpCmdInput.trim();
                      const timeStr = new Date().toTimeString().split(' ')[0] + '.' + String(Math.floor(Math.random()*900 + 100));
                      setUdpLogs(prev => [
                        ...prev,
                        { id: Date.now(), time: timeStr, source: 'CLIENT -> UDP:5001', text: cmd, dir: 'tx' },
                        { id: Date.now() + 1, time: timeStr, source: 'STM32 -> CLIENT', text: `OK: PROCESSED ON UDP PORT 5001: ${cmd}\r\n`, dir: 'rx' }
                      ]);
                      setUdpCmdInput('');
                    }}
                    className="px-4 py-2 rounded-lg bg-emerald-600 hover:bg-emerald-500 text-white font-bold text-xs flex items-center gap-1.5 transition-colors"
                  >
                    <Send className="w-3.5 h-3.5" />
                    ارسال روی UDP
                  </button>
                </div>
              </div>

              {/* Network Architecture Quick Specs (4 cols) */}
              <div className="lg:col-span-4 space-y-4">
                <div className="bg-slate-950 border border-slate-800 rounded-xl p-4 space-y-3">
                  <h4 className="text-xs font-bold text-slate-300 font-mono flex items-center gap-2 border-b border-slate-800 pb-2">
                    <Wifi className="w-4 h-4 text-emerald-400" />
                    مشخصات ارسال UDP Telemetry
                  </h4>
                  <ul className="text-xs text-slate-400 space-y-2 leading-relaxed">
                    <li>• <strong>بدون نیاز به دانستن IP</strong>: کلاینت فقط به گروه مولتی‌کست <code>239.255.0.100</code> گوش می‌دهد.</li>
                    <li>• <strong>نرخ ۱۰۰۰ هرتز</strong>: ارسال در هر میلی‌ثانیه بدون وقفه با تایمر سخت‌افزاری.</li>
                    <li>• <strong>تگ زمان میکروثانیه (µs)</strong>: مقدار دقیق ۶۴ بیتی شمارنده کلاک <code>DWT-&gt;CYCCNT</code> با دقت ۵.۹ نانوثانیه.</li>
                    <li>• <strong>سوکت متقارن</strong>: ارسال و دریافت کامند همزمان روی پورت ۵۰۰۱ با پارسر یکسان سریال.</li>
                  </ul>
                </div>

                <div className="bg-slate-950 border border-slate-800 rounded-xl p-4 space-y-2 font-mono text-xs">
                  <span className="text-slate-500 block text-[11px]">دستورات آماده برای کلیک سریع:</span>
                  <div className="space-y-1.5 pt-1">
                    <button
                      onClick={() => setUdpCmdInput(':RATE 1000')}
                      className="w-full text-left p-1.5 rounded bg-slate-900 hover:bg-slate-800 text-emerald-400 block truncate"
                    >
                      :RATE 1000
                    </button>
                    <button
                      onClick={() => setUdpCmdInput(':FILTER IIR 20')}
                      className="w-full text-left p-1.5 rounded bg-slate-900 hover:bg-slate-800 text-blue-400 block truncate"
                    >
                      :FILTER IIR 20
                    </button>
                    <button
                      onClick={() => setUdpCmdInput(':UDP:MODE MULTICAST 239.255.0.100')}
                      className="w-full text-left p-1.5 rounded bg-slate-900 hover:bg-slate-800 text-purple-400 block truncate"
                    >
                      :UDP:MODE MULTICAST 239.255.0.100
                    </button>
                    <button
                      onClick={() => setUdpCmdInput(':MEAS:VOLT:ALL?')}
                      className="w-full text-left p-1.5 rounded bg-slate-900 hover:bg-slate-800 text-amber-400 block truncate"
                    >
                      :MEAS:VOLT:ALL?
                    </button>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
