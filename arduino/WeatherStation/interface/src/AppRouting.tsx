import React, { Component } from 'react';
import { Switch, Route, Redirect } from 'react-router';


import WiFiConnection from './wifi/WiFiConnection';
import System from './system/System';
import InfluxDB from './influxdb/InfluxDB'

export const getDefaultRoute = () =>  "/wifi/";

class AppRouting extends Component {

  componentDidMount() {
  }

  render() {
    return (
        <Switch>
          <Route exact path="/wifi/*" component={WiFiConnection} />
          <Route exact path="/system/*" component={System} />
          <Route exact path="/influxdb/*" component={InfluxDB} />
          <Redirect to={getDefaultRoute()} />
        </Switch>
    )
  }
}

export default AppRouting;
