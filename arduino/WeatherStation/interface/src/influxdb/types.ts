
export interface InfluxDBSettings {
  server: string;
  org: string;
  bucket: string;
  token: string;
  writeInterval: number;
}

export enum ValidationStatus {
  Idle = 0,
  StartRequest,
  Running,
  Finished,
  Error
}

export interface ValidationStatusResponse {
  status: ValidationStatus;
  message?:string
}
