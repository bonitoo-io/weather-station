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
  freeRam: number;
  appState: AppState;
  error?: string;
}
