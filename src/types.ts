export type ActiveTab = 'wiring' | 'daq' | 'protocols' | 'webserver' | 'code' | 'docs';

export type FilterType = 'BYPASS' | 'MAV' | 'EMA' | 'IIR_LPF' | 'MEDIAN';

export interface PinDefinition {
  id: string;
  header: 'J1' | 'J2' | 'EXTERNAL';
  pinNumber: number;
  mcuPin: string;
  signalName: string;
  voltageLevel: '3.3V' | '5.0V' | 'GND' | 'Analog';
  category: 'mux' | 'adc' | 'actuator' | 'comm' | 'power' | 'gpio';
  destination: string;
  descriptionFa: string;
  descriptionEn: string;
  levelShifterPinA?: string;
  levelShifterPinB?: string;
}

export interface ChannelSample {
  channel: number;
  raw: number;
  voltageMv: number;
  filteredVoltageMv: number;
  label: string;
  unit: string;
  noise: number;
}

export interface CyclicTestState {
  state: 'IDLE' | 'RUNNING' | 'PAUSED' | 'COMPLETED';
  frequencyHz: number;
  currentCycle: number;
  targetCycles: number;
  actuatorState: boolean;
  syncPulse: boolean;
  sampleRateHz: number;
}

export interface ModbusRegister {
  addressHex: string;
  addressDec: number;
  name: string;
  type: 'RO' | 'RW';
  value: number;
  formattedValue: string;
  description: string;
}

export interface FirmwareFile {
  path: string;
  filename: string;
  category: 'Driver' | 'Core' | 'Config' | 'Build' | 'Doc';
  language: 'c' | 'h' | 'makefile' | 'ld' | 'xml' | 'markdown';
  description: string;
  content: string;
}
