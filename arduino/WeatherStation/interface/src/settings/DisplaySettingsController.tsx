import React, { Component } from 'react';

import { restController, RestControllerProps, RestFormLoader, SectionContent } from '../components';
import DisplaySettingsForm from './DisplaySettingsForm';
import { DISPLAY_SETTINGS_ENDPOINT } from '../api';
import { DisplaySettings } from './types';

type DisplaySettingsControllerProps = RestControllerProps<DisplaySettings>;

class DisplaySettingsController extends Component<DisplaySettingsControllerProps> {

  componentDidMount() {
    this.props.loadData();
  }

  render() {
    return (
      <SectionContent title="Display Settings">
        <RestFormLoader
          {...this.props}
          render={formProps => <DisplaySettingsForm {...formProps} />}
        />
      </SectionContent>
    );
  }

}

export default restController(DISPLAY_SETTINGS_ENDPOINT, DisplaySettingsController);
