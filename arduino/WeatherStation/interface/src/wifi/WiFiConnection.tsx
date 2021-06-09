import React, { Component } from 'react';
import { Redirect, Route, Switch, RouteComponentProps } from 'react-router-dom'

import { Tabs, Tab } from '@material-ui/core';

import { MenuAppBar } from '../components';

import WiFiStatusController from './WiFiStatusController';
import WiFiSettingsController from './WiFiSettingsController';
import WiFiNetworkScanner from './WiFiNetworkScanner';
import { WiFiConnectionContext, WiFiConnectionContextValue } from './WiFiConnectionContext';
import { WiFiNetwork } from './types';

type WiFiConnectionProps = RouteComponentProps;

class WiFiConnection extends Component<WiFiConnectionProps, WiFiConnectionContextValue> {

  constructor(props: WiFiConnectionProps) {
    super(props);
    this.state = {
      selectNetwork: this.selectNetwork,
      deselectNetwork: this.deselectNetwork
    };
  }

  selectNetwork = (network: WiFiNetwork) => {
    this.setState({ selectedNetwork: network });
    this.props.history.push('/wifi/settings');
  }

  deselectNetwork = () => {
    this.setState({ selectedNetwork: undefined });
  }

  handleTabChange = (event: React.ChangeEvent<{}>, path: string) => {
    this.props.history.push(path);
  };

  render() {
    return (
      <WiFiConnectionContext.Provider value={this.state}>
        <MenuAppBar sectionTitle="WiFi Connection">
          <Tabs value={this.props.match.url} onChange={this.handleTabChange} variant="fullWidth">
            <Tab value="/wifi/status" label="WiFi Status" />
            <Tab value="/wifi/scan" label="Scan Networks"/>
            <Tab value="/wifi/settings" label="WiFi Settings"/>
          </Tabs>
          <Switch>
            <Route exact path="/wifi/status" component={WiFiStatusController} />
            <Route exact path="/wifi/scan" component={WiFiNetworkScanner} />
            <Route exact path="/wifi/settings" component={WiFiSettingsController} />
            <Redirect to="/wifi/status" />
          </Switch>
        </MenuAppBar>
      </WiFiConnectionContext.Provider>
    )
  }
}

export default WiFiConnection;
