import React, { Component } from 'react';

import { restController, RestControllerProps, RestFormLoader, SectionContent } from '../components';
import AdvancedSettingsForm from './AdvancedSettingsForm';
import { ADVANCED_SETTINGS_ENDPOINT } from '../api';
import { AdvancedSettings } from './types';

type AdvancedSettingsControllerProps = RestControllerProps<AdvancedSettings>;

class AdvancedSettingsController extends Component<AdvancedSettingsControllerProps> {

  componentDidMount() {
    this.props.loadData();
  }

  render() {
    return (
      <SectionContent title="Advanced Settings">
        <RestFormLoader
          {...this.props}
          render={formProps => <AdvancedSettingsForm {...formProps} />}
        />
      </SectionContent>
    );
  }

}

export default restController(ADVANCED_SETTINGS_ENDPOINT, AdvancedSettingsController);
