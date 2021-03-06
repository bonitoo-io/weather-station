import React, { Component, RefObject } from 'react';
import { TextValidator, ValidatorForm } from 'react-material-ui-form-validator';

import { Checkbox,  LinearProgress, Typography } from '@material-ui/core';

import SaveIcon from '@material-ui/icons/Save';

import { RestFormProps, BlockFormControlLabel, FormActions, FormButton } from '../components';


import { RegionalSettings, ValidationStatus, ValidationStatusResponse } from './types';
import { NUM_POLLS, POLLING_FREQUENCY, REGIONAL_SETTINGS_VALIDATE_ENDPOINT, retryError503, retryErrorPolling, RETRY_EXCEPTION_TYPE } from '../api';
import { Theme, createStyles, withStyles, WithStyles } from '@material-ui/core/styles';
import { AppStateContext } from '../AppStateContext';

const styles = (theme: Theme) => createStyles({ 
   connectingSettings: {
    margin: theme.spacing(0.5),
  },
  connectingSettingsDetails: {
    margin: theme.spacing(4),
    textAlign: "center"
  },
  connectingProgress: {
    margin: theme.spacing(4),
    textAlign: "center"
  }
});


type RegionalSettingsFormProps = RestFormProps<RegionalSettings> & WithStyles<typeof styles>;

interface RegionalSettingsFormState {
  validatingParams: boolean;
  errorMessage?: string;
}

const UTCOffsetErrorText = 'Must be a number between -50400 (-14 hours) and 50400 (+14 hours)'
const LatitudeErrorText = 'Must be a number between -90 and 90'
const LongitudeErrorText = 'Must be a number between -180 and 180'

class RegionalSettingsForm extends Component<RegionalSettingsFormProps, RegionalSettingsFormState> {

  pollCount: number = 0;

  form: RefObject<ValidatorForm>;

  saveData: () => void 

  state: RegionalSettingsFormState = {
    validatingParams: false,
  };

  constructor(props: RegionalSettingsFormProps) {
    super(props);
    // create a ref to store the textInput DOM element
    this.form = React.createRef();
    const { saveData } = this.props;
    this.saveData = saveData;
  }

  

  render() {
    const { data, handleValueChange, classes}  = this.props;
    const { validatingParams } = this.state

    return (
      <AppStateContext.Consumer>
        {({wifiConfigured}) => (
          <ValidatorForm onSubmit={wifiConfigured?this.validateAndSaveParams:this.saveData} ref={this.form}>
            {validatingParams &&
              <div className={classes.connectingSettings}>
                <LinearProgress className={classes.connectingSettingsDetails} />
                <Typography variant="h6" className={classes.connectingProgress}>
                  Validating&hellip;
                </Typography>
            </div>}
            <BlockFormControlLabel
              control={
                <Checkbox
                  value="detectAutomatically"
                  checked={data.detectAutomatically}
                  onChange={handleValueChange("detectAutomatically")}
                />
              }
              label="Detect Automatically"
            />
              <TextValidator
                validators={['required']}
                errorMessages={['City is required']}
                name="location"
                label="City"
                fullWidth
                variant="outlined"
                value={data.location}
                onChange={handleValueChange('location')}
                margin="normal"
                disabled = {data.detectAutomatically}
                helperText="City name with optional country code"
              />
              <TextValidator
                validators={['required']}
                errorMessages={['Language is required']}
                name="language"
                label="Language"
                fullWidth
                variant="outlined"
                value={data.language}
                onChange={handleValueChange('language')}
                margin="normal"
                disabled = {data.detectAutomatically}
                inputProps={{
                  maxLength: 2,
                  minLength: 2,
                  style: { textTransform: "lowercase" }
                }}
                helperText="Two letters language code"
                >
              </TextValidator>
              <TextValidator
                validators={['required', 'isNumber', 'minNumber:-50400', 'maxNumber:50400']}
                errorMessages={['UTC Offset is required', UTCOffsetErrorText, UTCOffsetErrorText, UTCOffsetErrorText]}
                name="utcOffset"
                label="UTC Offset (in seconds)"
                fullWidth
                variant="outlined"
                value={data.utcOffset}
                onChange={handleValueChange('utcOffset')}
                margin="normal"
                disabled = {data.detectAutomatically}
              />
              <TextValidator
                validators={['required', 'isFloat', 'minFloat:-90', 'maxFloat:90']}
                errorMessages={['Latitude is required', LatitudeErrorText, LatitudeErrorText, LatitudeErrorText]}
                name="latitude"
                label="Latitude"
                fullWidth
                variant="outlined"
                value={data.latitude}
                onChange={handleValueChange('latitude')}
                margin="normal"
                disabled = {data.detectAutomatically}
              />
              <TextValidator
                validators={['required', 'isFloat', 'minFloat:-180.0', 'maxFloat:180.0']}
                errorMessages={['Longitude is required', LongitudeErrorText, LongitudeErrorText, LongitudeErrorText ]}
                name="longitude"
                label="Longitude"
                fullWidth
                variant="outlined"
                value={data.longitude}
                onChange={handleValueChange('longitude')}
                margin="normal"
                disabled = {data.detectAutomatically}
              />
              <BlockFormControlLabel
                value="useMetricUnits"
                control={
                  <Checkbox
                  value="useMetricUnits"
                  checked={data.useMetricUnits}
                  onChange={handleValueChange("useMetricUnits")}
                  disabled = {data.detectAutomatically}
                />
                }
                label="Use Metrics Units"
                labelPlacement="end"
              />
              <BlockFormControlLabel
                value="use24Hours"
                control={
                  <Checkbox
                  value="use24Hours"
                  checked={data.use24Hours}
                  onChange={handleValueChange("use24Hours")}
                  disabled = {data.detectAutomatically}
                />
                }
                label="Use 24 hours time"
                labelPlacement="end"
              />
              <BlockFormControlLabel
                value="useYMDFormat"
                control={
                  <Checkbox
                  value="useYMDFormat"
                  checked={data.useYMDFormat}
                  onChange={handleValueChange("useYMDFormat")}
                  disabled = {data.detectAutomatically}
                />
                }
                label="Use YMD (Year-Month-Day) date format"
                labelPlacement="end"
              />
            <FormActions>
              <FormButton startIcon={<SaveIcon />} variant="contained" color="primary" type="submit" disabled={this.state.validatingParams}>
                Save
              </FormButton>
            </FormActions>
          </ValidatorForm>
           )
          }
        </AppStateContext.Consumer>
      )
  }

  validateAndSaveParams = () => {
    if(this.form.current) {
      this.form.current.isFormValid(false)
      .then(valid => {
        if(valid) {
          if(this.props.data.detectAutomatically) {
            this.saveData();
          } else {
            this.postValidation()
          }
        }
        return
      })
      .catch(error => {
        this.props.enqueueSnackbar(error.message || "Problem validating params", { variant: 'error' });
      });
    }
  }

  postValidation = () => {
    this.setState({ validatingParams: true });
    fetch(REGIONAL_SETTINGS_VALIDATE_ENDPOINT, {
        method: 'POST',
        body: JSON.stringify(this.props.data),
        headers: {
          'Content-Type': 'application/json'
        }
      })
    .then(response => {
      if (response.status === 200) {
        this.pollCount = 0
        this.schedulePollValidation()
        return
      } else if (response.status === 503 || response.status ===  429) {
        const retryAfter = response.headers.get("Retry-After")
        let timeout = 1
        if (retryAfter) {
          timeout = parseInt(retryAfter, 10);
        }
        setTimeout(this.postValidation, timeout*1000)
        throw retryError503()
      } else {
        throw Error("Invalid status code: " + response.status);
      }
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
    fetch(REGIONAL_SETTINGS_VALIDATE_ENDPOINT, {method: 'GET'})
      .then(response => {
        if(response.status === 200) {
          return response.json();
        } else if(response.status === 202) {
          if (++this.pollCount < NUM_POLLS) {
            this.schedulePollValidation();
            throw retryErrorPolling()
          } else {
            throw Error("Validation has not completed in timely manner.");
          }
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
          this.setState({ validatingParams: false });
          this.saveData()
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

export default withStyles(styles)(RegionalSettingsForm);
