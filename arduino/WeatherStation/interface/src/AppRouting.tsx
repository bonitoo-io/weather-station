import React, { Component } from 'react';
import { Switch, Route, Redirect } from 'react-router';


import WiFiConnection from './wifi/WiFiConnection';
import Device from './device/Device';
import Settings from './settings/Settings'
import { AppStateContext } from './AppStateContext';



class AppRouting extends Component {

  componentDidMount() {
  }

  render() {
    return (
      <AppStateContext.Consumer>
        {({wifiConfigured}) => (
          <Switch>
            <Route exact path="/device/*" component={Device} />
            <Route exact path="/wifi/*" component={WiFiConnection} />
            <Route exact path="/settings/*" component={Settings } />
            <Redirect to={wifiConfigured?"/device/about":"/wifi/scan"} />
          </Switch>
          )
        }
      </AppStateContext.Consumer>
    )
  }
}

export default AppRouting;
