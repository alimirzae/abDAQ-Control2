import React, { useState } from 'react';
import { 
  Layers, 
  Search, 
  Cpu, 
  Zap, 
  ArrowRight, 
  Info, 
  Image as ImageIcon, 
  Check, 
  ShieldAlert,
  Sliders
} from 'lucide-react';
import { PINOUT_DATA } from '../data/pinoutData';
import { PinDefinition } from '../types';

export const WiringView: React.FC = () => {
  const [searchTerm, setSearchTerm] = useState('');
  const [selectedCategory, setSelectedCategory] = useState<string>('all');
  const [selectedPin, setSelectedPin] = useState<PinDefinition | null>(PINOUT_DATA[3]); // PE8 MUX S0 default

  const filteredPins = PINOUT_DATA.filter(pin => {
    const matchesSearch = 
      pin.mcuPin.toLowerCase().includes(searchTerm.toLowerCase()) ||
      pin.signalName.toLowerCase().includes(searchTerm.toLowerCase()) ||
      pin.destination.toLowerCase().includes(searchTerm.toLowerCase()) ||
      pin.descriptionFa.includes(searchTerm) ||
      pin.descriptionEn.toLowerCase().includes(searchTerm.toLowerCase());

    const matchesCategory = 
      selectedCategory === 'all' || pin.category === selectedCategory;

    return matchesSearch && matchesCategory;
  });

  const getVoltageBadge = (level: string) => {
    switch (level) {
      case '3.3V':
        return <span className="px-2 py-0.5 text-xs font-mono font-semibold rounded bg-blue-500/20 text-blue-400 border border-blue-500/30">3.3V Logic</span>;
      case '5.0V':
        return <span className="px-2 py-0.5 text-xs font-mono font-semibold rounded bg-amber-500/20 text-amber-400 border border-amber-500/30">5.0V Power</span>;
      case 'Analog':
        return <span className="px-2 py-0.5 text-xs font-mono font-semibold rounded bg-purple-500/20 text-purple-400 border border-purple-500/30">Analog In</span>;
      case 'GND':
        return <span className="px-2 py-0.5 text-xs font-mono font-semibold rounded bg-slate-700 text-slate-300 border border-slate-600">GND</span>;
      default:
        return null;
    }
  };

  return (
    <div className="space-y-6">
      {/* Top Architecture Overview Banner */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm">
        <div className="flex flex-col lg:flex-row items-start lg:items-center justify-between gap-4">
          <div>
            <h2 className="text-lg font-bold text-white flex items-center gap-2">
              <Layers className="w-5 h-5 text-emerald-400" />
              سیم‌بندی و نقشه پین‌های سخت‌افزار LabDAQ
            </h2>
            <p className="text-sm text-slate-400 mt-1 max-w-3xl">
              اتصال هدرهای J1 و J2 میکروکنترلر STM32F407V به شیفتر سطح منطقی TXS0108E (تبدیل ۳.۳ ولت به ۵.۰ ولت)،
              مالتی‌پلکسر آنالوگ ۱۶ کاناله CD74HC4067، فرستنده RS-485 و کنترلر اترنت LAN8720A.
            </p>
          </div>

          {/* Quick Stats */}
          <div className="flex flex-wrap gap-2 text-xs font-mono">
            <div className="px-3 py-1.5 rounded-lg bg-slate-800/80 border border-slate-700 text-slate-300">
              <span className="text-slate-500">Board:</span> EWB-STM32F407V-LAN
            </div>
            <div className="px-3 py-1.5 rounded-lg bg-slate-800/80 border border-slate-700 text-slate-300">
              <span className="text-slate-500">Shifter:</span> TXS0108E (8-Ch)
            </div>
            <div className="px-3 py-1.5 rounded-lg bg-slate-800/80 border border-slate-700 text-slate-300">
              <span className="text-slate-500">MUX:</span> CD74HC4067 (16:1)
            </div>
          </div>
        </div>
      </div>

      {/* Interactive Signal Flow Diagram */}
      <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm">
        <h3 className="text-sm font-semibold uppercase tracking-wider text-slate-400 mb-4 flex items-center gap-2">
          <Zap className="w-4 h-4 text-amber-400" />
          توالی اتصال لایه‌های سیگنال (Signal Path & Level Shifting)
        </h3>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          {/* Stage 1: MCU Port E */}
          <div className="bg-slate-800/60 border border-blue-500/30 rounded-lg p-4 relative">
            <div className="flex items-center justify-between mb-2">
              <span className="font-bold text-blue-300 text-sm flex items-center gap-1.5">
                <Cpu className="w-4 h-4" />
                STM32F407 (Port E High)
              </span>
              <span className="text-xs font-mono text-blue-400 bg-blue-500/10 px-2 py-0.5 rounded border border-blue-500/30">
                3.3V Logic
              </span>
            </div>
            <ul className="text-xs space-y-1.5 text-slate-300 font-mono">
              <li className="flex justify-between border-b border-slate-700/50 pb-1">
                <span className="text-slate-400">PE8..PE11:</span>
                <span className="text-emerald-400">MUX S0..S3 (Channel Select)</span>
              </li>
              <li className="flex justify-between border-b border-slate-700/50 pb-1">
                <span className="text-slate-400">PE12:</span>
                <span className="text-emerald-400">MUX /EN (Output Enable)</span>
              </li>
              <li className="flex justify-between border-b border-slate-700/50 pb-1">
                <span className="text-slate-400">PE13:</span>
                <span className="text-amber-400">SYNC_OUT (Test Trigger)</span>
              </li>
              <li className="flex justify-between border-b border-slate-700/50 pb-1">
                <span className="text-slate-400">PE14:</span>
                <span className="text-amber-400">ACTUATOR_OUT (Valve/Relay)</span>
              </li>
              <li className="flex justify-between">
                <span className="text-slate-400">PE15:</span>
                <span className="text-slate-300">TEST_STATUS (Run Flag)</span>
              </li>
            </ul>
          </div>

          {/* Stage 2: TXS0108E Level Shifter */}
          <div className="bg-slate-800/60 border border-amber-500/30 rounded-lg p-4 relative">
            <div className="flex items-center justify-between mb-2">
              <span className="font-bold text-amber-300 text-sm flex items-center gap-1.5">
                <Zap className="w-4 h-4" />
                TXS0108E Level Shifter
              </span>
              <span className="text-xs font-mono text-amber-400 bg-amber-500/10 px-2 py-0.5 rounded border border-amber-500/30">
                3.3V ⇄ 5.0V
              </span>
            </div>
            <p className="text-xs text-slate-300 mb-2 leading-relaxed">
              مبدل دوطرفه ۸ کاناله جهت تامین سطح ولتاژ مطمئن ۵ ولت برای سوئیچ‌های آنالوگ و درایور رله‌ها.
            </p>
            <div className="text-xs font-mono bg-slate-900/80 p-2 rounded border border-slate-700 space-y-1">
              <div className="flex justify-between text-slate-400">
                <span>VCCA (3.3V):</span>
                <span className="text-blue-300">J1 Pin 1 (Board 3V3)</span>
              </div>
              <div className="flex justify-between text-slate-400">
                <span>VCCB (5.0V):</span>
                <span className="text-amber-300">J1 Pin 16 (Board 5V)</span>
              </div>
              <div className="flex justify-between text-slate-400">
                <span>OE (Enable):</span>
                <span className="text-emerald-400">Tied to +3.3V</span>
              </div>
            </div>
          </div>

          {/* Stage 3: MUX & Analog Return */}
          <div className="bg-slate-800/60 border border-purple-500/30 rounded-lg p-4 relative">
            <div className="flex items-center justify-between mb-2">
              <span className="font-bold text-purple-300 text-sm flex items-center gap-1.5">
                <Sliders className="w-4 h-4" />
                CD74HC4067 & ADC Return
              </span>
              <span className="text-xs font-mono text-purple-400 bg-purple-500/10 px-2 py-0.5 rounded border border-purple-500/30">
                16:1 Analog
              </span>
            </div>
            <ul className="text-xs space-y-1.5 text-slate-300 font-mono">
              <li className="flex justify-between border-b border-slate-700/50 pb-1">
                <span className="text-slate-400">C0..C15 Inputs:</span>
                <span className="text-slate-200">16 Sensor Channels</span>
              </li>
              <li className="flex justify-between border-b border-slate-700/50 pb-1">
                <span className="text-slate-400">SIG Pin 1:</span>
                <span className="text-purple-300 font-bold">PA4 (ADC1_IN4)</span>
              </li>
              <li className="flex justify-between border-b border-slate-700/50 pb-1">
                <span className="text-slate-400">Ron Resistance:</span>
                <span className="text-slate-300">~70 Ω @ 5.0V</span>
              </li>
              <li className="flex justify-between">
                <span className="text-slate-400">Settling Delay:</span>
                <span className="text-emerald-400">5 µs Software Wait</span>
              </li>
            </ul>
          </div>
        </div>
      </div>

      {/* Main Pinout Table and Detail Inspector */}
      <div className="grid grid-cols-1 lg:grid-cols-12 gap-6">
        {/* Pinout Table (8 cols) */}
        <div className="lg:col-span-8 bg-slate-900 border border-slate-800 rounded-xl overflow-hidden shadow-sm">
          {/* Controls: Search and Filter */}
          <div className="p-4 border-b border-slate-800 flex flex-col sm:flex-row gap-3 items-center justify-between bg-slate-900/50">
            {/* Search */}
            <div className="relative w-full sm:w-64">
              <Search className="w-4 h-4 text-slate-400 absolute left-3 top-2.5" />
              <input
                id="pin-search"
                type="text"
                value={searchTerm}
                onChange={e => setSearchTerm(e.target.value)}
                placeholder="جستجوی پین یا سیگنال..."
                className="w-full bg-slate-800 border border-slate-700 rounded-lg pl-9 pr-3 py-1.5 text-xs text-white placeholder-slate-400 focus:outline-none focus:border-emerald-500"
              />
            </div>

            {/* Category Filters */}
            <div className="flex flex-wrap gap-1.5 w-full sm:w-auto">
              {[
                { id: 'all', label: 'همه' },
                { id: 'mux', label: 'مالتی‌پلکسر' },
                { id: 'adc', label: 'ورودی ADC' },
                { id: 'actuator', label: 'کنترل چرخه‌ای' },
                { id: 'comm', label: 'ارتباطات' },
                { id: 'power', label: 'تغذیه/زمین' }
              ].map(cat => (
                <button
                  key={cat.id}
                  id={`filter-${cat.id}`}
                  onClick={() => setSelectedCategory(cat.id)}
                  className={`px-2.5 py-1 text-xs font-medium rounded-md transition-colors ${
                    selectedCategory === cat.id
                      ? 'bg-emerald-600 text-white'
                      : 'bg-slate-800 text-slate-400 hover:text-slate-200 hover:bg-slate-700'
                  }`}
                >
                  {cat.label}
                </button>
              ))}
            </div>
          </div>

          {/* Table */}
          <div className="overflow-x-auto max-h-[520px] scrollbar-thin scrollbar-thumb-slate-700">
            <table className="w-full text-left border-collapse text-xs">
              <thead className="bg-slate-800/80 text-slate-400 sticky top-0 uppercase tracking-wider font-mono text-[11px] z-10">
                <tr>
                  <th className="py-2.5 px-3">هدر / پین</th>
                  <th className="py-2.5 px-3">پین MCU</th>
                  <th className="py-2.5 px-3">سیگنال</th>
                  <th className="py-2.5 px-3">ولتاژ</th>
                  <th className="py-2.5 px-3">شیفتر TXS</th>
                  <th className="py-2.5 px-3">مقصد و کاربرد</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800/60 font-mono">
                {filteredPins.map(pin => {
                  const isSelected = selectedPin?.id === pin.id;
                  return (
                    <tr
                      key={pin.id}
                      onClick={() => setSelectedPin(pin)}
                      className={`cursor-pointer transition-colors ${
                        isSelected 
                          ? 'bg-emerald-500/10 hover:bg-emerald-500/15' 
                          : 'hover:bg-slate-800/40'
                      }`}
                    >
                      <td className="py-2.5 px-3 font-semibold text-slate-300">
                        {pin.header}-{pin.pinNumber}
                      </td>
                      <td className="py-2.5 px-3 text-emerald-400 font-bold">
                        {pin.mcuPin}
                      </td>
                      <td className="py-2.5 px-3 text-white font-medium">
                        {pin.signalName}
                      </td>
                      <td className="py-2.5 px-3">
                        {getVoltageBadge(pin.voltageLevel)}
                      </td>
                      <td className="py-2.5 px-3 text-slate-400">
                        {pin.levelShifterPinA ? (
                          <span className="text-amber-400">{pin.levelShifterPinA} → {pin.levelShifterPinB}</span>
                        ) : (
                          <span className="text-slate-600">—</span>
                        )}
                      </td>
                      <td className="py-2.5 px-3 text-slate-300 truncate max-w-xs font-sans">
                        {pin.descriptionFa}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </div>

        {/* Selected Pin Details Inspector (4 cols) */}
        <div className="lg:col-span-4 space-y-4">
          {selectedPin ? (
            <div className="bg-slate-900 border border-slate-800 rounded-xl p-5 shadow-sm space-y-4">
              <div className="flex items-center justify-between border-b border-slate-800 pb-3">
                <div>
                  <span className="text-xs text-slate-400 font-mono">Header {selectedPin.header} / Pin {selectedPin.pinNumber}</span>
                  <h4 className="text-base font-bold text-white font-mono flex items-center gap-2">
                    <span className="text-emerald-400">{selectedPin.mcuPin}</span>
                    <span className="text-slate-500">|</span>
                    <span>{selectedPin.signalName}</span>
                  </h4>
                </div>
                {getVoltageBadge(selectedPin.voltageLevel)}
              </div>

              {/* Description */}
              <div>
                <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider block mb-1">
                  توضیحات و عملکرد:
                </span>
                <p className="text-xs text-slate-200 leading-relaxed bg-slate-800/40 p-3 rounded-lg border border-slate-800">
                  {selectedPin.descriptionFa}
                </p>
              </div>

              {/* Destination */}
              <div>
                <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider block mb-1">
                  مقصد اتصال سخت‌افزاری:
                </span>
                <p className="text-xs font-mono text-emerald-300 bg-slate-800/40 p-2.5 rounded-lg border border-slate-800">
                  {selectedPin.destination}
                </p>
              </div>

              {/* Level Shifter Details */}
              {selectedPin.levelShifterPinA && (
                <div className="bg-amber-500/5 border border-amber-500/20 rounded-lg p-3">
                  <span className="text-xs font-bold text-amber-400 flex items-center gap-1.5 mb-1.5">
                    <Zap className="w-3.5 h-3.5" />
                    مسیر شیفتر TXS0108E:
                  </span>
                  <div className="flex items-center justify-between text-xs font-mono text-slate-300">
                    <div>
                      <span className="text-slate-500 block text-[10px]">Side A (STM32)</span>
                      <span className="font-bold text-blue-400">{selectedPin.levelShifterPinA}</span>
                    </div>
                    <ArrowRight className="w-4 h-4 text-slate-500" />
                    <div className="text-right">
                      <span className="text-slate-500 block text-[10px]">Side B (Sensors)</span>
                      <span className="font-bold text-amber-400">{selectedPin.levelShifterPinB}</span>
                    </div>
                  </div>
                </div>
              )}

              {/* Firmware Mapping Macro */}
              <div>
                <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider block mb-1 font-mono">
                  تعریف در labdaq_config.h:
                </span>
                <code className="block bg-slate-950 p-2.5 rounded text-[11px] font-mono text-emerald-400 border border-slate-800 overflow-x-auto">
                  #define LABDAQ_{selectedPin.signalName}_PIN GPIO_PIN_{selectedPin.mcuPin.replace(/\D/g, '') || '0'}
                </code>
              </div>
            </div>
          ) : (
            <div className="bg-slate-900 border border-slate-800 rounded-xl p-8 text-center text-slate-500 text-xs">
              یک پین را از جدول انتخاب کنید تا جزییات اتصال نمایش داده شود.
            </div>
          )}

          {/* Hardware Photos Reference Card */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-4 space-y-3">
            <h4 className="text-xs font-bold uppercase tracking-wider text-slate-300 flex items-center gap-2">
              <ImageIcon className="w-4 h-4 text-blue-400" />
              شواهد سخت‌افزاری و تصاویر برد (Hardware Docs)
            </h4>
            <p className="text-xs text-slate-400">
              تصاویر مدار سیم‌بندی و بورد در شاخه مستندات ثبت شده‌اند:
            </p>
            <div className="space-y-2 text-xs font-mono">
              <div className="bg-slate-800/60 p-2.5 rounded border border-slate-700/60 flex items-center justify-between">
                <div>
                  <span className="text-slate-200 block font-semibold">IMG_20260918_121022.jpg</span>
                  <span className="text-[10px] text-slate-400">اتصال هدر J1 به ماژول TXS0108E و MUX</span>
                </div>
                <span className="text-[10px] px-2 py-0.5 rounded bg-blue-900/50 text-blue-300 border border-blue-500/30">
                  docs/images/
                </span>
              </div>
              <div className="bg-slate-800/60 p-2.5 rounded border border-slate-700/60 flex items-center justify-between">
                <div>
                  <span className="text-slate-200 block font-semibold">IMG_20260918_121030.jpg</span>
                  <span className="text-[10px] text-slate-400">مدار آنالوگ PA4، تغذیه و ورودی‌های سنسورها</span>
                </div>
                <span className="text-[10px] px-2 py-0.5 rounded bg-blue-900/50 text-blue-300 border border-blue-500/30">
                  docs/images/
                </span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
