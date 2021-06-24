import React, { Component } from 'react';

import { restController, RestControllerProps, RestFormLoader, SectionContent } from '../components';
import InfluxDBSettingsForm from './InfluxDBSettingsForm';
import { INFLUXDB_SETTINGS_ENDPOINT } from '../api';
import { InfluxDBSettings } from './types';

type InfluxDBControllerProps = RestControllerProps<InfluxDBSettings>;

class InfluxDBSettingsController extends Component<InfluxDBControllerProps> {

  componentDidMount() {
    this.props.loadData();
  }

  render() {
    return (
      <SectionContent title="InfluxDB Settings">
        <RestFormLoader
          {...this.props}
          render={formProps => <InfluxDBSettingsForm {...formProps} />}
        />
      </SectionContent>
    );
  }

}

export default restController(INFLUXDB_SETTINGS_ENDPOINT, InfluxDBSettingsController);
