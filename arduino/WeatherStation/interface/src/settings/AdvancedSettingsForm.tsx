import React, { Component, RefObject } from 'react';
import { TextValidator, ValidatorForm } from 'react-material-ui-form-validator';

import { isValidScreens } from '../validators';
import SaveIcon from '@material-ui/icons/Save';

import { RestFormProps, FormActions, FormButton } from '../components';

import { AdvancedSettings, ValidationStatus, ValidationStatusResponse } from './types';
import { Theme, createStyles, withStyles, WithStyles } from '@material-ui/core/styles';
import { LinearProgress, Link, Typography } from '@material-ui/core';
import { ADVANCED_SETTINGS_VALIDATE_ENDPOINT, NUM_POLLS, POLLING_FREQUENCY, retryError503, retryErrorPolling, RETRY_EXCEPTION_TYPE } from '../api';


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

type AdvancedSettingsFormProps = RestFormProps<AdvancedSettings> & WithStyles<typeof styles>;

interface AdvancedSettingsFormState {
  validatingParams: boolean;
  errorMessage?: string;
}

const UpdateDataErrorText = 'Must be a number between 30 and 1440 (24 hours)'

class AdvancedSettingsForm extends Component<AdvancedSettingsFormProps, AdvancedSettingsFormState> {

  pollCount: number = 0;

  form: RefObject<ValidatorForm>;

  saveData: () => void 

  state: AdvancedSettingsFormState = {
    validatingParams: false,
  };

  constructor(props: AdvancedSettingsFormProps) {
    super(props);
    // create a ref to store the textInput DOM element
    this.form = React.createRef();
    const { saveData } = this.props;
    this.saveData = saveData;
  }

  componentDidMount = () => {
    ValidatorForm.addValidationRule('isValidScreens', isValidScreens);
  }

  toUpperCase = (name: keyof AdvancedSettings, valueSetter: (name: string, val: any)=>void ) => (event: React.ChangeEvent<HTMLInputElement>) => {
    let val = event.target.value as string
    valueSetter(name, val.toUpperCase())
  }

  renderAPIKeyHelperText = () => {
    return (
      <>
        Can be obtained at <Link href="https://openweathermap.org/price" target="_blank" rel="noreferrer">
           https://openweathermap.org/price
        </Link>
      </>
    )
  }

  renderNTPServersHelperText = () => {
    return (
      <>
        Comma separated list of up to 3 NTP servers. For the fastest time sync find NTP servers in your area:  <Link href="https://www.pool.ntp.org/zone/" target="_blank" rel="noreferrer">
        https://www.pool.ntp.org/zone/
        </Link>
      </>
    )
  }

  render() {
    const { data, handleValueChange, handleDirectValueChange, classes}  = this.props;
    const { validatingParams } = this.state

    return (
        <ValidatorForm onSubmit={this.validateAndSaveParams} ref={this.form}>
          {validatingParams &&
            <div className={classes.connectingSettings}>
              <LinearProgress className={classes.connectingSettingsDetails} />
              <Typography variant="h6" className={classes.connectingProgress}>
                Validating&hellip;
              </Typography>
          </div>}
          <TextValidator
              validators={['required','isValidScreens']}
              errorMessages={['Display Screens is required','Invalid screen code']}
              name="screens"
              label="Display Screens"
              fullWidth
              variant="outlined"
              value={data.screens}
              onChange={this.toUpperCase('screens', handleDirectValueChange)}
              margin="normal"
              inputProps={{ style: { textTransform: "uppercase" } }}  
              helperText="Screens (letter is in brackets): (A)nalog time; (D)igital time; (I)ndoor temperature/humidity; Covid-19 (S)pread risk; (T)emperature chart; (O)utdoor weather; weather (F)orecast; (W)ind forecast; (M)oon phase rise/set; (C)onfiguration info"
            />
          <TextValidator
              validators={['required', 'isNumber', 'minNumber:1']}
              errorMessages={['Screen Rotate Interval is required', 'Must be a number at least 1', 'Must be a number at least 1']}
              name="screenRotateInterval"
              label="Screen Rotate Interval (in seconds)"
              fullWidth
              variant="outlined"
              value={data.screenRotateInterval}
              onChange={handleValueChange('screenRotateInterval')}
              margin="normal"
              helperText="How long a screen on display stays before changes"
            />
          <TextValidator
              validators={['required', 'isFloat']}
              errorMessages={['Temperature Offset is required', 'Must be a real number']}
              name="tempOffset"
              label="Temperature Offset"
              fullWidth
              variant="outlined"
              value={data.tempOffset}
              onChange={handleValueChange('tempOffset')}
              margin="normal"
              helperText="A floating point number for temperature value compensation"
            />
            <TextValidator
              validators={['required', 'isFloat']}
              errorMessages={['Humidity Offset is required', 'Must be a real number']}
              name="humOffset"
              label="Humidity Offset"
              fullWidth
              variant="outlined"
              value={data.humOffset}
              onChange={handleValueChange('humOffset')}
              margin="normal"
              helperText="A floating point number for humidity value compensation"
            />
            <TextValidator
              validators={['required']}
              errorMessages={['At least 1 NTP Server is required']}
              name="ntpServers"
              label="NTP Servers"
              fullWidth
              variant="outlined"
              value={data.ntpServers}
              onChange={handleValueChange('ntpServers')}
              margin="normal"
              helperText={this.renderNTPServersHelperText()}
            />
             <TextValidator
              validators={['required', 'isNumber', 'minNumber: 30', 'maxNumber:1440']}
              errorMessages={['Update Data Interval is required', UpdateDataErrorText, UpdateDataErrorText, UpdateDataErrorText]}
              name="updateDataInterval"
              label="Update Data Interval (in minutes)"
              fullWidth
              variant="outlined"
              value={data.updateDataInterval}
              onChange={handleValueChange('updateDataInterval')}
              margin="normal"
              helperText="How often is updated weather info, forecasts, synchronized time, etc."
            />
            <TextValidator
              validators={['required']}
              errorMessages={['API key is required']}
              name="openWeatherAPIKey"
              label="OpenWeatherMap API Key"
              fullWidth
              variant="outlined"
              value={data.openWeatherAPIKey}
              onChange={handleValueChange('openWeatherAPIKey')}
              margin="normal"
              helperText={this.renderAPIKeyHelperText()}
            />
          <FormActions>
            <FormButton startIcon={<SaveIcon />} variant="contained" color="primary" type="submit">
              Save
            </FormButton>
          </FormActions>
        </ValidatorForm>
      )
  }

  validateAndSaveParams = () => {
    if(this.form.current) {
      this.form.current.isFormValid(false)
      .then(valid => {
        if(valid) {
          this.postValidation()
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
    fetch(ADVANCED_SETTINGS_VALIDATE_ENDPOINT, {
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
        this.setState({ validatingParams: false });
        throw Error("Invalid status code: " + response.status);
      }
    })
    .catch(error => {
      if(error.name !== RETRY_EXCEPTION_TYPE) {
        this.props.enqueueSnackbar(error.message || "Problem validating params", { variant: 'error' });
        this.setState({ validatingParams: false });
      }
    });
  }

  schedulePollValidation() {
    setTimeout(this.pollValidation, POLLING_FREQUENCY);
  }

  

  pollValidation = () => {
    fetch(ADVANCED_SETTINGS_VALIDATE_ENDPOINT, {method: 'GET'})
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
          this.props.enqueueSnackbar("Validation error: " + error.message, {
            variant: 'error',
          });
          this.setState({ validatingParams: false });
        }
      });
  }
}

export default withStyles(styles)(AdvancedSettingsForm);
