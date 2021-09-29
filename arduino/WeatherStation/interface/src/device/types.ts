export enum AppState {
  Ok = 0,
  Error,
  WifiConfigNeeded,
  InfluxDBConfigNeeded
};

export interface AboutInfo {
  version : string;
  deviceId: string;
  useMetric: boolean
  temp: number;
  hum: number;
  uptime: number;
  appState: AppState;
  error?: string;
  espPlatform: string;
  freeHeap: number;
  maxAllocHeap: number;
  heapFragmentation: number;
  cpuFreq: number;
  sketchSize: number;
  freeSketchSpace: number;
  sdkVersion: string;
  flashChipSize: number;
  flashChipSpeed: number;
  fsUsed: number;
  fsTotal: number;
}
