import React, { RefObject }  from 'react';
import { CircularProgress, Link, Typography } from '@material-ui/core';
import { TextValidator, ValidatorForm } from 'react-material-ui-form-validator';


import SaveIcon from '@material-ui/icons/Save';

import { RestFormProps, FormActions, FormButton,  } from '../components';
import { isURL } from '../validators';
import CheckCircle from '@material-ui/icons/CheckCircle';
import { InfluxDBSettings, ValidationStatus, ValidationStatusResponse } from './types';
import { INFLUXDB_VALIDATE_ENDPOINT, NUM_POLLS, POLLING_FREQUENCY, retryError503, retryErrorPolling, RETRY_EXCEPTION_TYPE} from '../api';

type InfluxDBSettingsFormProps = RestFormProps<InfluxDBSettings>;


interface InfluxDBSettingsFormState {
  validatingParams: boolean;
  errorMessage?: string;
}

const WriteIntervalErrorText = 'Must be a number with value at least 1 (minute)'

class InfluxDBSettingsForm extends React.Component<InfluxDBSettingsFormProps, InfluxDBSettingsFormState> {

  state: InfluxDBSettingsFormState = {
    validatingParams: false,
  };

  pollCount: number = 0;

  form: RefObject<ValidatorForm>;

  constructor(props: InfluxDBSettingsFormProps) {
    super(props);
    // create a ref to store the textInput DOM element
    this.form = React.createRef();
  }


  componentDidMount() {
    ValidatorForm.addValidationRule('isURL', isURL);
  }

  render() {
    const { data, handleValueChange, saveData } = this.props;
    return (
      <>
       <Typography variant="body2">
       If you don't have access to an InfluxDB 2 server, &nbsp;
        <Link href="https://docs.influxdata.com/influxdb/cloud/sign-up/" target="_blank" rel="noreferrer" variant="body2">
          get started with InfluxDB Cloud
        </Link>
        .
        </Typography>
      <ValidatorForm onSubmit={saveData} ref={this.form}>
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
          errorMessages={['Write Interval is required', WriteIntervalErrorText, WriteIntervalErrorText]}
          name="writeInterval"
          label="Write Interval [minutes]"
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
          <FormButton startIcon={<CheckCircle />} variant="contained" color="secondary" onClick={this.onValidateParams} disabled={this.state.validatingParams}>
              {this.state.validatingParams?<CircularProgress />:<div>Test Connection</div>}
          </FormButton>
        </FormActions>
      </ValidatorForm>
      </>
    );
  }

  onValidateParams = () => {
    if(this.form.current) {
      this.form.current.isFormValid(false)
      .then(valid => {
        if(valid) {
          this.validateParams()
        }
        return
      })
      .catch(error => {
        this.props.enqueueSnackbar(error.message || "Problem validating params", { variant: 'error' });
      });
    }
  }

  validateParams = () => {
    this.setState({ validatingParams: true });
    fetch(INFLUXDB_VALIDATE_ENDPOINT, {
        method: 'POST',
        body: JSON.stringify(this.props.data),
        headers: {
          'Content-Type': 'application/json'
        }
      })
    .then(response => {
      if(response.status === 200) {
        return response.json()
      } else if (response.status === 503 || response.status ===  429) {
        const retryAfter = response.headers.get("Retry-After")
        let timeout = 1
        if (retryAfter) {
          timeout = parseInt(retryAfter, 10);
        }
        setTimeout(this.validateParams, timeout*1000)
        throw retryError503()
      } else {
        throw new Error("Invalid status code " + response.status)
      }
    })
    .then(json => {
      const status : ValidationStatusResponse = json
      if (status.status === ValidationStatus.StartRequest) {
        this.pollCount = 0
        this.schedulePollValidation()
      } else {
        this.setState({ validatingParams: false });
      }
      return
    })
    .catch(error => {
      if (error.name !== RETRY_EXCEPTION_TYPE) {
        this.props.enqueueSnackbar(error.message || "Problem validating params", { variant: 'error' });
        this.setState({ validatingParams: false });
      }
    });
  }
  schedulePollValidation() {
    setTimeout(this.pollValidation, POLLING_FREQUENCY);
  }

  pollValidation = () => {
    fetch(INFLUXDB_VALIDATE_ENDPOINT, {method: 'GET'})
      .then(response => {
        if(response.status === 200) {
          return response.json()
        } else if (response.status === 503 || response.status ===  429) {
          const retryAfter = response.headers.get("Retry-After")
          let timeout = 1
          if (retryAfter) {
            timeout = parseInt(retryAfter, 10);
          }
          setTimeout(this.pollValidation, timeout*1000)
          throw retryError503()
        } else {
          throw new Error("Invalid status code " + response.status)
        }
      })
      .then(json => {
        const status : ValidationStatusResponse = json
        if (status.status === ValidationStatus.Finished) {
          this.props.enqueueSnackbar("Connection successful", {
            variant: 'success',
          });
          this.setState({ validatingParams: false });
          return;
        } else if (status.status === ValidationStatus.Error) {
           throw Error(status.message)
        } else {
          if (++this.pollCount < NUM_POLLS) {
            this.schedulePollValidation();
            throw retryErrorPolling()
          } else {
            throw Error("Validation has not completed in timely manner.");
          }
        }
      })
      .catch(error => {
        if (error.name !== RETRY_EXCEPTION_TYPE) {
          this.props.enqueueSnackbar(error.message, {
            variant: 'error',
          });
          this.setState({ validatingParams: false });
        }
      });
  }
}

export default InfluxDBSettingsForm;
