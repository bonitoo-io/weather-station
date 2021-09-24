import React, { Component } from 'react';

import { restController, RestControllerProps, RestFormLoader, SectionContent } from '../components';
import RegionalSettingsForm from './RegionalSettingsForm';
import { REGIONAL_SETTINGS_ENDPOINT } from '../api';
import { RegionalSettings } from './types';

type RegionalSettingsControllerProps = RestControllerProps<RegionalSettings>;

class RegionalSettingsController extends Component<RegionalSettingsControllerProps> {

  componentDidMount() {
    this.props.loadData();
  }

  render() {
    return (
      <SectionContent title="Regional Settings">
        <RestFormLoader
          {...this.props}
          render={formProps => <RegionalSettingsForm {...formProps} />}
        />
      </SectionContent>
    );
  }

}

export default restController(REGIONAL_SETTINGS_ENDPOINT, RegionalSettingsController);
