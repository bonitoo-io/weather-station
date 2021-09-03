import React, { Component } from 'react';
import { Switch, Route, Redirect } from 'react-router';


import WiFiConnection from './wifi/WiFiConnection';
import About from './about/About';
import InfluxDB from './influxdb/InfluxDB'
import { AppStateContext } from './AppStateContext';



class AppRouting extends Component {

  componentDidMount() {
  }

  render() {
    return (
      <AppStateContext.Consumer>
        {({wifiConfigured}) => (
          <Switch>
            <Route exact path="/about/*" component={About} />
            <Route exact path="/wifi/*" component={WiFiConnection} />
            <Route exact path="/influxdb/*" component={InfluxDB} />
            <Redirect to={wifiConfigured?"/about/about":"/wifi/scan"} />
          </Switch>
          )
        }
      </AppStateContext.Consumer>
    )
  }
}

export default AppRouting;
