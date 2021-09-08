import React, { Component } from 'react';

import { restController, RestControllerProps, RestFormLoader, SectionContent } from '../components';
import UpdateSettingsForm from './UpdateSettingsForm';
import { UPDATE_SETTINGS_ENDPOINT } from '../api';
import { UpdateSettings } from './types';

type UpdateControllerProps = RestControllerProps<UpdateSettings>;

class UpdateSettingsController extends Component<UpdateControllerProps> {

  componentDidMount() {
    this.props.loadData();
  }

  render() {
    return (
      <SectionContent title="Update Settings">
        <RestFormLoader
          {...this.props}
          render={formProps => <UpdateSettingsForm {...formProps} />}
        />
      </SectionContent>
    );
  }

}

export default restController(UPDATE_SETTINGS_ENDPOINT, UpdateSettingsController);
