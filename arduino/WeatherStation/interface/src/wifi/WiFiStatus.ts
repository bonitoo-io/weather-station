import { Theme } from '@material-ui/core';
import { WiFiStatus, WiFiConnectionStatus, WiFiMode } from './types';

export const isConnected = ({ status }: WiFiStatus) => status === WiFiConnectionStatus.WIFI_STATUS_CONNECTED;

export const wifiStatusHighlight = ({ status }: WiFiStatus, theme: Theme) => {
  switch (status) {
    case WiFiConnectionStatus.WIFI_STATUS_IDLE:
    case WiFiConnectionStatus.WIFI_STATUS_DISCONNECTED:
    case WiFiConnectionStatus.WIFI_STATUS_NO_SHIELD:
      return theme.palette.info.main;
    case WiFiConnectionStatus.WIFI_STATUS_CONNECTED:
      return theme.palette.success.main;
    case WiFiConnectionStatus.WIFI_STATUS_CONNECT_FAILED:
    case WiFiConnectionStatus.WIFI_STATUS_CONNECTION_LOST:
      return theme.palette.error.main;
    default:
      return theme.palette.warning.main;
  }
}

export const wifiStatus = ({ status }: WiFiStatus) => {
  switch (status) {
    case WiFiConnectionStatus.WIFI_STATUS_NO_SHIELD:
      return "Inactive";
    case WiFiConnectionStatus.WIFI_STATUS_IDLE:
      return "Idle";
    case WiFiConnectionStatus.WIFI_STATUS_NO_SSID_AVAIL:
      return "No SSID Available";
    case WiFiConnectionStatus.WIFI_STATUS_CONNECTED:
      return "Connected";
    case WiFiConnectionStatus.WIFI_STATUS_CONNECT_FAILED:
      return "Connection Failed";
    case WiFiConnectionStatus.WIFI_STATUS_CONNECTION_LOST:
      return "Connection Lost";
    case WiFiConnectionStatus.WIFI_STATUS_DISCONNECTED:
      return "Disconnected";
    default:
      return "Unknown";
  }
}

export const wifiMode = ({ mode }: WiFiStatus) => {
  switch (mode) {
    case WiFiMode.WIFI_OFF:
      return "Off";
    case WiFiMode.WIFI_STA:
      return "Station";
    case WiFiMode.WIFI_AP:
      return "Access Point";
    case WiFiMode.WIFI_AP_STA:
      return "Access Point and Station";
    default:
      return "Unknown";
  }
}