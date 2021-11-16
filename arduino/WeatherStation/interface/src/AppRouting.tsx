import React, { Component, lazy, Suspense } from 'react';
import { Switch, Route, Redirect } from 'react-router';

import Device from './device/Device';
import { AppStateContext } from './AppStateContext';

const Settings = lazy(() => import('./settings/Settings'));
const WiFiConnection  = lazy(() => import('./wifi/WiFiConnection'));

class AppRouting extends Component {

  componentDidMount() {
  }

  render() {
    return (
      <AppStateContext.Consumer>
        {({wifiConfigured}) => (
          <div>
             <Suspense fallback={<div>Page is Loading...</div>}>
              <Switch>
                <Route exact path="/device/*" component={Device} />
                <Route exact path="/wifi/*" component={WiFiConnection} />
                <Route exact path="/settings/*" component={Settings } />
                <Redirect to={wifiConfigured?"/device/about":"/wifi/scan"} />
              </Switch>
          </Suspense>
          </div>
          )
        }
      </AppStateContext.Consumer>
    )
  }
}

export default AppRouting;
