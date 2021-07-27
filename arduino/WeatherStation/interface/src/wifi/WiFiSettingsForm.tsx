import React, { Fragment } from 'react';
import { TextValidator, ValidatorForm } from 'react-material-ui-form-validator';

import { Checkbox, List, ListItem, ListItemText, ListItemAvatar, ListItemSecondaryAction, CircularProgress } from '@material-ui/core';

import Avatar from '@material-ui/core/Avatar';
import IconButton from '@material-ui/core/IconButton';
import LockIcon from '@material-ui/icons/Lock';
import LockOpenIcon from '@material-ui/icons/LockOpen';
import DeleteIcon from '@material-ui/icons/Delete';
import SaveIcon from '@material-ui/icons/Save';

import { RestFormProps, PasswordValidator, BlockFormControlLabel, FormActions, FormButton } from '../components';
import { isIP, isHostname, optional } from '../validators';

import { WiFiConnectionContext, WiFiConnectionContextValue } from './WiFiConnectionContext';
import { isNetworkOpen, networkSecurityMode } from './WiFiSecurityModes';
import { WiFiConnectionStatus, WiFiSettings, WiFiStatus } from './types';
import { WIFI_SETTINGS_ENDPOINT, WIFI_STATUS_ENDPOINT } from '../api';
import { AppStateContext } from '../AppStateContext';

const NUM_POLLS = 20
const POLLING_FREQUENCY = 500
const RETRY_EXCEPTION_TYPE = "retry"

type WiFiStatusFormProps = RestFormProps<WiFiSettings>;

interface WiFiStatusFormState {
  validatingParams: boolean;
  errorMessage?: string;
}

class WiFiSettingsForm extends React.Component<WiFiStatusFormProps, WiFiStatusFormState> {

  static contextType = WiFiConnectionContext;
  context!: React.ContextType<typeof WiFiConnectionContext>;

  state: WiFiStatusFormState = {
    validatingParams: false,
  };

  pollCount: number = 0;

  saveData: () => void

  constructor(props: WiFiStatusFormProps, context: WiFiConnectionContextValue) {
    super(props);

    const { saveData } = this.props;
    this.saveData = saveData;

    const { selectedNetwork } = context;
    if (selectedNetwork) {
      const wifiSettings: WiFiSettings = {
        ssid: selectedNetwork.ssid,
        password: "",
        hostname: props.data.hostname,
        static_ip_config: false,
      }
      props.setData(wifiSettings);
    }
  }

  componentDidMount() {
    ValidatorForm.addValidationRule('isIP', isIP);
    ValidatorForm.addValidationRule('isHostname', isHostname);
    ValidatorForm.addValidationRule('isOptionalIP', optional(isIP));
  }

  componentWillUnmount() {
    this.context.deselectNetwork();
  }

  render() {
    const { data, handleValueChange } = this.props;
    return (
      <AppStateContext.Consumer>
        {({wifiConfigured}) => (
          <WiFiConnectionContext.Consumer>
            {({selectedNetwork, deselectNetwork}) => (
            <ValidatorForm onSubmit={wifiConfigured?this.saveData:this.validateAndSaveParams}>
              <div>Selected network: {selectedNetwork?selectedNetwork.ssid:"n/a"}</div>
              {
                selectedNetwork ?
                  <List>
                    <ListItem>
                      <ListItemAvatar>
                        <Avatar>
                          {isNetworkOpen(selectedNetwork) ? <LockOpenIcon /> : <LockIcon />}
                        </Avatar>
                      </ListItemAvatar>
                      <ListItemText
                        primary={selectedNetwork.ssid}
                        secondary={"Security: " + networkSecurityMode(selectedNetwork) + ", Ch: " + selectedNetwork.channel}
                      />
                      <ListItemSecondaryAction>
                        <IconButton aria-label="Manual Config" onClick={deselectNetwork}>
                          <DeleteIcon />
                        </IconButton>
                      </ListItemSecondaryAction>
                    </ListItem>
                  </List>
                  :
                  <TextValidator
                    validators={['matchRegexp:^.{0,32}$']}
                    errorMessages={['SSID must be 32 characters or less']}
                    name="ssid"
                    label="SSID"
                    fullWidth
                    variant="outlined"
                    value={data.ssid}
                    onChange={handleValueChange('ssid')}
                    margin="normal"
                  />
              }
              {
                (!selectedNetwork || !isNetworkOpen(selectedNetwork)) &&
                <PasswordValidator
                  validators={['matchRegexp:^.{0,64}$']}
                  errorMessages={['Password must be 64 characters or less']}
                  name="password"
                  label="Password"
                  fullWidth
                  variant="outlined"
                  value={data.password}
                  onChange={handleValueChange('password')}
                  margin="normal"
                />
              }
              <TextValidator
                validators={['required', 'isHostname']}
                errorMessages={['Hostname is required', "Not a valid hostname"]}
                name="hostname"
                label="Hostname"
                fullWidth
                variant="outlined"
                value={data.hostname}
                onChange={handleValueChange('hostname')}
                margin="normal"
              />
              <BlockFormControlLabel
                control={
                  <Checkbox
                    value="static_ip_config"
                    checked={data.static_ip_config}
                    onChange={handleValueChange("static_ip_config")}
                  />
                }
                label="Static IP Config?"
              />
              {
                data.static_ip_config &&
                <Fragment>
                  <TextValidator
                    validators={['required', 'isIP']}
                    errorMessages={['Local IP is required', 'Must be an IP address']}
                    name="local_ip"
                    label="Local IP"
                    fullWidth
                    variant="outlined"
                    value={data.local_ip}
                    onChange={handleValueChange('local_ip')}
                    margin="normal"
                  />
                  <TextValidator
                    validators={['required', 'isIP']}
                    errorMessages={['Gateway IP is required', 'Must be an IP address']}
                    name="gateway_ip"
                    label="Gateway"
                    fullWidth
                    variant="outlined"
                    value={data.gateway_ip}
                    onChange={handleValueChange('gateway_ip')}
                    margin="normal"
                  />
                  <TextValidator
                    validators={['required', 'isIP']}
                    errorMessages={['Subnet mask is required', 'Must be an IP address']}
                    name="subnet_mask"
                    label="Subnet"
                    fullWidth
                    variant="outlined"
                    value={data.subnet_mask}
                    onChange={handleValueChange('subnet_mask')}
                    margin="normal"
                  />
                  <TextValidator
                    validators={['isOptionalIP']}
                    errorMessages={['Must be an IP address']}
                    name="dns_ip_1"
                    label="DNS IP #1"
                    fullWidth
                    variant="outlined"
                    value={data.dns_ip_1}
                    onChange={handleValueChange('dns_ip_1')}
                    margin="normal"
                  />
                  <TextValidator
                    validators={['isOptionalIP']}
                    errorMessages={['Must be an IP address']}
                    name="dns_ip_2"
                    label="DNS IP #2"
                    fullWidth
                    variant="outlined"
                    value={data.dns_ip_2}
                    onChange={handleValueChange('dns_ip_2')}
                    margin="normal"
                  />
                </Fragment>
              }
              <FormActions>
                <FormButton startIcon={<SaveIcon />} variant="contained" color="primary" type="submit" disabled={this.state.validatingParams}>
                  {this.state.validatingParams?<CircularProgress />:<>Save</>}
                </FormButton>
              </FormActions>
            </ValidatorForm>
            )
            }
          </WiFiConnectionContext.Consumer>
         )
        }
      </AppStateContext.Consumer>
    );
  }

  validateAndSaveParams = () => {
    this.setState({ validatingParams: true });
    fetch(WIFI_SETTINGS_ENDPOINT, {
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
      } else {
        this.setState({ validatingParams: false });
        throw Error("Invalid status code: " + response.status);
      }
    })
    .catch(error => {
      this.props.enqueueSnackbar(error.message || "Problem validating params", { variant: 'error' });
      this.setState({ validatingParams: false });
    });
  }
  schedulePollValidation() {
    setTimeout(this.pollValidation, POLLING_FREQUENCY);
  }

  retryError() {
    return {
      name: RETRY_EXCEPTION_TYPE,
      message: "Validation not ready, will retry in " + POLLING_FREQUENCY + "ms."
    };
  }
  pollValidation = () => {
    fetch(WIFI_STATUS_ENDPOINT, {method: 'GET'})
      .then(response => {
        if(response.status === 200) {
          return response.json();
        } else {
          throw new Error("Invalid status code " + response.status)
        }
      })
      .then(json => {
        const status : WiFiStatus = json
        if (status.status === WiFiConnectionStatus.WIFI_STATUS_CONNECTED) {
          this.props.enqueueSnackbar("Update successful.", {
            variant: 'success',
          });
          this.setState({ validatingParams: false });
          return;
        } else if (!status.disconnect_reason) {
          if (++this.pollCount < NUM_POLLS) {
            this.schedulePollValidation();
            throw this.retryError()
          } else {
            throw Error("Validation has not completed in timely manner.");
          }
        } else if (status.disconnect_reason === 202) {
          throw Error("Invalid password")
        } else {
          throw Error("WiFi connection failed")
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

export default WiFiSettingsForm;
