import React, { Component } from 'react';
import { Redirect, Switch, Route, RouteComponentProps } from 'react-router-dom'

import { Tabs, Tab } from '@material-ui/core';


import { MenuAppBar } from '../components';

import SystemStatusController from './SystemStatusController';

type SystemProps = RouteComponentProps;

class System extends Component<SystemProps> {

  handleTabChange = (event: React.ChangeEvent<{}>, path: string) => {
    this.props.history.push(path);
  };

  render() {
    return (
      <MenuAppBar sectionTitle="System">
        <Tabs value={this.props.match.url} onChange={this.handleTabChange} variant="fullWidth">
          <Tab value="/system/status" label="System Status" />
        </Tabs>
        <Switch>
          <Route exact path="/system/status" component={SystemStatusController} />
          <Redirect to="/system/status" />
        </Switch>
      </MenuAppBar>
    )
  }
}

export default System;
