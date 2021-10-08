import React, { Component } from 'react';
import { Redirect, Route, Switch, RouteComponentProps } from 'react-router-dom'

import { Tabs, Tab } from '@material-ui/core';

import { MenuAppBar } from '../components';
import InfluxDBSettingsController from './InfluxDBSettingsController';
import UpdateSettingsController from './UpdateSettingsController';
import RegionalSettingsController from './RegionalSettingsController';
import AdvancedSettingsController from './AdvancedSettingsController';

type SettingsProps = RouteComponentProps


class Settings extends Component<SettingsProps> {

  handleTabChange = (event: React.ChangeEvent<{}>, path: string) => {
    this.props.history.push(path);
  };

  render() {
    return (
      <MenuAppBar sectionTitle="Settings">
        <Tabs value={this.props.match.url} onChange={this.handleTabChange} variant="fullWidth">
          <Tab value="/settings/influxdb" label="InfluxDB" />
          <Tab value="/settings/update" label="Update"/>
          <Tab value="/settings/regional" label="Regional"/>
          <Tab value="/settings/advanced" label="Advanced"/>
        </Tabs>
        <Switch>
          <Route exact path="/settings/influxdb" component={InfluxDBSettingsController} />
          <Route exact path="/settings/update" component={UpdateSettingsController} />
          <Route exact path="/settings/regional" component={RegionalSettingsController} />
          <Route exact path="/settings/advanced" component={AdvancedSettingsController} />
          <Redirect to="/settings/influxdb" />
        </Switch>
      </MenuAppBar>
    )
  }
}

export default Settings;
