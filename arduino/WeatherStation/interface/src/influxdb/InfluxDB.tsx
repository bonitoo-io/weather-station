import React, { Component } from 'react';
import { Redirect, Switch, Route, RouteComponentProps } from 'react-router-dom'

import { MenuAppBar } from '../components';

import InfluxDBSettingsController from './InfluxDBSettingsController';

type SystemProps = RouteComponentProps;

class InfluxDB extends Component<SystemProps> {

  handleTabChange = (event: React.ChangeEvent<{}>, path: string) => {
    this.props.history.push(path);
  };

  render() {
    return (
      <MenuAppBar sectionTitle="InfluxDB Settings">
        <Switch>
          <Route exact path="/influxdb/settings" component={InfluxDBSettingsController} />
          <Redirect to="/influxdb/settings" />
        </Switch>
      </MenuAppBar>
    )
  }
}

export default InfluxDB;
