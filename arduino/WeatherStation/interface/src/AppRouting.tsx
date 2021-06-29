import React, { Component } from 'react';
import { Switch, Route, Redirect } from 'react-router';


import WiFiConnection from './wifi/WiFiConnection';
import About from './about/About';
import InfluxDB from './influxdb/InfluxDB'

export const getDefaultRoute = () =>  "/about/";

class AppRouting extends Component {

  componentDidMount() {
  }

  render() {
    return (
        <Switch>
          <Route exact path="/about/*" component={About} />
          <Route exact path="/wifi/*" component={WiFiConnection} />
          <Route exact path="/influxdb/*" component={InfluxDB} />
          <Redirect to={getDefaultRoute()} />
        </Switch>
    )
  }
}

export default AppRouting;
