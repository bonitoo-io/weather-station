import React, { Component } from 'react';

import {restController, RestControllerProps, RestFormLoader, SectionContent } from '../components';
import { ABOUT_INFO_ENDPOINT } from '../api';

import AboutPage from './AboutPage';
import { AboutInfo } from './types';

type AboutProps = RestControllerProps<AboutInfo>;

class AboutController extends Component<AboutProps> {

  componentDidMount() {
    this.props.loadData();
  }

  render() {
    return (
      <SectionContent title="Weather Station Status">
        <RestFormLoader
          {...this.props}
          render={formProps => <AboutPage {...formProps} />}
        />
      </SectionContent>
    );
  }

}

export default restController(ABOUT_INFO_ENDPOINT, AboutController);
