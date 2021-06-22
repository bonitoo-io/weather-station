import React, { Fragment } from 'react';
import { TextValidator, ValidatorForm } from 'react-material-ui-form-validator';


import SaveIcon from '@material-ui/icons/Save';

import { RestFormProps, FormActions, FormButton } from '../components';
import { isURL } from '../validators';

import { InfluxDBSettings } from './types';

type InfluxDBSettingsFormProps = RestFormProps<InfluxDBSettings>;

class InfluxDBSettingsForm extends React.Component<InfluxDBSettingsFormProps> {


  constructor(props: InfluxDBSettingsFormProps) {
    super(props);
  }

  componentDidMount() {
    ValidatorForm.addValidationRule('isURL', isURL);
  }

  render() {
    const { data, handleValueChange, saveData } = this.props;
    return (
      <ValidatorForm onSubmit={saveData} ref="InfluxDBSettingsForm">
        <TextValidator
          validators={['required', 'isURL']}
          errorMessages={['Server URL is required', "Not a valid URL"]}
          name="server"
          label="Server URL"
          fullWidth
          variant="outlined"
          value={data.server}
          onChange={handleValueChange('server')}
          margin="normal"
        />
        <TextValidator
          validators={['required']}
          errorMessages={['Organization is required']}
          name="org"
          label="Organization"
          fullWidth
          variant="outlined"
          value={data.org}
          onChange={handleValueChange('org')}
          margin="normal"
        />
         <TextValidator
          validators={['required']}
          errorMessages={['Bucket is required']}
          name="bucket"
          label="Bucket"
          fullWidth
          variant="outlined"
          value={data.bucket}
          onChange={handleValueChange('bucket')}
          margin="normal"
        />
         <TextValidator
          validators={['required']}
          errorMessages={['Authorization token is required']}
          name="token"
          label="Authorization token"
          fullWidth
          variant="outlined"
          value={data.token}
          onChange={handleValueChange('token')}
          margin="normal"
        />
         <TextValidator
          validators={['required','isNumber','minNumber: 1']}
          errorMessages={['Write Interval is required']}
          name="writeInterval"
          label="Write Interval [seconds]"
          fullWidth
          variant="outlined"
          value={data.writeInterval}
          onChange={handleValueChange('writeInterval')}
          margin="normal"
        />
        <FormActions>
          <FormButton startIcon={<SaveIcon />} variant="contained" color="primary" type="submit">
            Save
          </FormButton>
        </FormActions>
      </ValidatorForm>
    );
  }
}

export default InfluxDBSettingsForm;
