import { Tab, Tabs } from '@material-ui/core';
import React, { Component } from 'react';
import { Redirect, Switch, Route, RouteComponentProps } from 'react-router-dom'


import { MenuAppBar } from '../components';

import AboutController from './AboutController';
import StatusController from './StatusController';
import UploadFirmwareController from './UploadFirmwareController';

type DeviceProps = RouteComponentProps;

class Device extends Component<DeviceProps> {

  handleTabChange = (event: React.ChangeEvent<{}>, path: string) => {
    this.props.history.push(path);
  };

  render() {
    return (
      <MenuAppBar sectionTitle="Device">
        <Tabs value={this.props.match.url} onChange={this.handleTabChange} variant="fullWidth">
          <Tab value="/device/about" label="About" />
          <Tab value="/device/status" label="Status"/>
          <Tab value="/device/upload" label="Upload Firmware"/>
        </Tabs>
        <Switch>
          <Route exact path="/device/about" component={AboutController} />
          <Route exact path="/device/status" component={StatusController} />
          <Route exact path="/device/upload" component={UploadFirmwareController} />
          <Redirect to="/device/about" />
        </Switch>
      </MenuAppBar>
    )
  }
}

export default Device;
