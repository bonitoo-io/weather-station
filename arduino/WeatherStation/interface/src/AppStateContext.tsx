import React from 'react';

export interface AppStateContextValue {
  loading: boolean
  wifiConfigured: boolean
}

export const AppStateContextDefaultValue = {} as AppStateContextValue
export const AppStateContext = React.createContext(
  AppStateContextDefaultValue
);
