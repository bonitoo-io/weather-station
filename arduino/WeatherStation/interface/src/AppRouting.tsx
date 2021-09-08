import React, { Component } from 'react';
import { Switch, Route, Redirect } from 'react-router';


import WiFiConnection from './wifi/WiFiConnection';
import About from './about/About';
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
            <Route exact path="/about/*" component={About} />
            <Route exact path="/wifi/*" component={WiFiConnection} />
            <Route exact path="/settings/*" component={Settings } />
            <Redirect to={wifiConfigured?"/about/about":"/wifi/scan"} />
          </Switch>
          )
        }
      </AppStateContext.Consumer>
    )
  }
}

export default AppRouting;
