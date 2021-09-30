import React, { Component } from 'react';

import {restController, RestControllerProps, RestFormLoader, SectionContent } from '../components';
import { ABOUT_INFO_ENDPOINT } from '../api';

import StatusPage from './StatusPage';
import { AboutInfo } from './types';

type StatusProps = RestControllerProps<AboutInfo>;

class StatusController extends Component<StatusProps> {

  componentDidMount() {
    this.props.loadData();
  }

  render() {
    return (
      <SectionContent title="Weather Station Status">
        <RestFormLoader
          {...this.props}
          render={formProps => <StatusPage {...formProps} />}
        />
      </SectionContent>
    );
  }

}

export default restController(ABOUT_INFO_ENDPOINT, StatusController);
