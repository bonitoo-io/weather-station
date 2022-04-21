import React, { Component, Fragment, RefObject } from 'react';
import { TextValidator, ValidatorForm } from 'react-material-ui-form-validator';

import SaveIcon from '@material-ui/icons/Save';
import SettingsBackupRestoreIcon from '@material-ui/icons/SettingsBackupRestore';

import { RestFormProps, FormActions, FormButton, BlockFormControlLabel, ErrorButton } from '../components';
import { MaterialUiPickersDate  } from '@material-ui/pickers/typings/date'
import { TimePicker, MuiPickersUtilsProvider  } from '@material-ui/pickers'
import DateFnsUtils from '@date-io/date-fns'


import { AdvancedSettings, ValidationStatus, ValidationStatusResponse } from './types';
import { Theme, createStyles, withStyles, WithStyles } from '@material-ui/core/styles';
import { Button, Checkbox, Dialog, DialogActions, DialogContent, DialogTitle, LinearProgress, Link, TextField, Typography } from '@material-ui/core';
import { ADVANCED_SETTINGS_VALIDATE_ENDPOINT, numberToTime, NUM_POLLS, POLLING_FREQUENCY, retryError503, retryErrorPolling, RETRY_EXCEPTION_TYPE } from '../api';


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
  },
  error: {
    margin: theme.spacing(0.5),
    color: "red"
  }
});

type AdvancedSettingsFormProps = RestFormProps<AdvancedSettings> & WithStyles<typeof styles>;

interface AdvancedSettingsFormState {
  validatingParams: boolean;
  errorMessage?: string;
  calibrateSensor: boolean;
}


const UpdateDataErrorText = 'Must be a number between 30 and 1440 (24 hours)'

class AdvancedSettingsForm extends Component<AdvancedSettingsFormProps, AdvancedSettingsFormState> {

  pollCount: number = 0;

  form: RefObject<ValidatorForm>;

  saveData: () => void 

  state: AdvancedSettingsFormState = {
    validatingParams: false,
    calibrateSensor: false
  };

  constructor(props: AdvancedSettingsFormProps) {
    super(props);
    // create a ref to store the textInput DOM element
    this.form = React.createRef();
    const { saveData } = this.props;
    this.saveData = saveData;
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

  changeTime = (name: string, valueSetter: (name: string, val: any)=>void ) =>(date: MaterialUiPickersDate) => {
    if(!date) {
      return
    }
    const val = date.getHours()*100+date.getMinutes()
    valueSetter(name, val)
  } 


  render() {
    const { data, handleValueChange, handleDirectValueChange, classes}  = this.props;
    const updateTime = numberToTime(data.updateTime)
    const { validatingParams } = this.state

    return (
      <Fragment>
        <ValidatorForm onSubmit={this.validateAndSaveParams} ref={this.form}>
          {validatingParams &&
            <div className={classes.connectingSettings}>
              <LinearProgress className={classes.connectingSettingsDetails} />
              <Typography variant="h6" className={classes.connectingProgress}>
                Validating&hellip;
              </Typography>
          </div>}
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
            <MuiPickersUtilsProvider utils={DateFnsUtils}>
              <TimePicker
                label="Firmware Auto Upgrade Time"
                value={updateTime}
                ampm={!data.use24Hours}
                onChange={this.changeTime('updateTime', handleDirectValueChange)}
                margin = "normal"
                fullWidth
              />
            </MuiPickersUtilsProvider>
            <BlockFormControlLabel
              value="start"
              control={
                <Checkbox
                value="checkBeta"
                checked={data.checkBeta}
                onChange={handleValueChange("checkBeta")}
              />
              }
              label="Upgrade to beta releases"
              labelPlacement="end"
            />
            <BlockFormControlLabel
              value="start"
              control={
                <Checkbox
                value="verifyCert"
                checked={data.verifyCert}
                onChange={handleValueChange("verifyCert")}
              />
              }
              label="Verify Github certificate"
              labelPlacement="end"
            />
          <FormActions>
            <FormButton startIcon={<SaveIcon />} variant="contained" color="primary" type="submit">
              Save
            </FormButton>
            <FormButton startIcon={<SettingsBackupRestoreIcon />} variant="contained" color="secondary" onClick={this.onCalibrationOpen}>
              Calibrate
            </FormButton>
          </FormActions>
        </ValidatorForm>
        {this.renderCalibrationDialog()}
      </Fragment>
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
          this.props.enqueueSnackbar(error.message, {
            variant: 'error',
          });
          this.setState({ validatingParams: false });
        }
      });
  }

  renderCalibrationDialog = () => {
    const { data, handleValueChange, classes } = this.props
    var sensors = ''
    if(data.actualTemp) {
      sensors = data.actualTemp.toFixed(1) + '°' + (data.useMetric?'C':'F')
    }
    if(data.actualHum) {
      if(sensors.length) {
        sensors += ' '
      }
      sensors += data.actualHum.toFixed(0) + '%'
    }
    if(sensors.length === 0) {
      sensors = 'n/a'
    }
    return (
      <Dialog
        open={this.state.calibrateSensor}
        onClose={this.onCalibrationClose}
      >
        <DialogTitle>Calibrate Sensor</DialogTitle>
        <DialogContent dividers>
          <Typography variant="subtitle1" className={classes.connectingSettings}>
            Temperature and humidity offset must be zero before calibration<br/>
            Sensor values: {sensors}
          </Typography>
          {this.state.errorMessage &&
            <Typography variant="subtitle1" className={classes.error}>
              {this.state.errorMessage}
            </Typography>
          }
          <TextField
              name="realTemp"
              label="Temperature"
              fullWidth
              variant="outlined"
              value={data.realTemp}
              onChange={handleValueChange('realTemp')}
              margin="normal"
              helperText="A floating point number for temperature calibration"
              focused
            />
            <TextField
              name="realHum"
              label="Humidity"
              fullWidth
              variant="outlined"
              value={data.realHum}
              onChange={handleValueChange('realHum')}
              margin="normal"
              helperText="A floating point number for humidity calibration"
            />
        </DialogContent>
        <DialogActions>
          <Button variant="contained" onClick={this.onCalibrationClose} color="secondary">
            Cancel
          </Button>
          <ErrorButton  variant="contained" onClick={this.onCalibrationSet}  autoFocus>
            Calibrate
          </ErrorButton>
        </DialogActions>
      </Dialog>
    )
  }

  onCalibrationClose = () => {
    this.setState({ calibrateSensor: false, errorMessage: "" });
  }
  onCalibrationOpen = () => {
    this.setState({ calibrateSensor: true });
  }

  onCalibrationSet = () => {
    const {data, setData } = this.props
    if(data.realTemp && data.realTemp.length > 0 ) {
      const i = data.realTemp.indexOf(',')
      if(i > 0) {
          data.realHum = data.realTemp.substring(i+1)
          data.realTemp = data.realTemp.substring(0,i)
      } 
    } else {
      this.setState({ errorMessage: "Temperature is required" }); 
      return
    }
    const temp = parseFloat(data.realTemp)
    if(isNaN(temp)) {
      this.setState({ errorMessage: "Temperature is not a number" }); 
      return
    }
    if(!data.realHum || !data.realHum.length) {
      this.setState({ errorMessage: "Humidity is required" }); 
      return
    }
    const hum = parseFloat(data.realHum)
    if(isNaN(hum)) {
      this.setState({ errorMessage: "Humidity is not a number" }); 
      return
    }
    data.tempOffset = temp-data.actualTemp
    data.humOffset = hum-data.actualHum
    this.setState({ calibrateSensor: false, errorMessage: "" });
    setData(data)
  }

}

export default withStyles(styles)(AdvancedSettingsForm);
