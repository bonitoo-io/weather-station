import React, { Component, Fragment } from 'react';
import { TextValidator, ValidatorForm } from 'react-material-ui-form-validator';

import { Checkbox, List, ListItem, ListItemText, ListItemAvatar, ListItemSecondaryAction, LinearProgress, Typography} from '@material-ui/core';

import Avatar from '@material-ui/core/Avatar';
import IconButton from '@material-ui/core/IconButton';
import LockIcon from '@material-ui/icons/Lock';
import LockOpenIcon from '@material-ui/icons/LockOpen';
import DeleteIcon from '@material-ui/icons/Delete';
import SaveIcon from '@material-ui/icons/Save';

import { RestFormProps, PasswordValidator, BlockFormControlLabel, FormActions, FormButton } from '../components';
import { isIP, isHostname, optional } from '../validators';

import { WiFiConnectionContext } from './WiFiConnectionContext';
import { isNetworkOpen, networkSecurityMode } from './WiFiSecurityModes';
import { ConnectingStatus, WiFiSettings } from './types';
import { CONNECT_STATUS_ENDPOINT_PATH, NUM_POLLS, POLLING_FREQUENCY, retryError503, retryErrorPolling, RETRY_EXCEPTION_TYPE, WIFI_SETTINGS_ENDPOINT } from '../api';
//import { withStyles, WithStyles } from '@material-ui/styles';
import { Theme, createStyles, withStyles, WithStyles } from '@material-ui/core/styles';

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


type WiFiSettingsFormProps = RestFormProps<WiFiSettings> & WithStyles<typeof styles>;
//type WiFiSettingsFormProps = RestFormProps<WiFiSettings> & WithTheme;

interface WiFiSettingsFormState {
  validatingParams: boolean;
  reconnectionInfo: boolean;
  errorMessage?: string;
}

class WiFiSettingsForm extends Component<WiFiSettingsFormProps, WiFiSettingsFormState> {

  static contextType = WiFiConnectionContext;
  context!: React.ContextType<typeof WiFiConnectionContext>;


  state: WiFiSettingsFormState = {
    validatingParams: false,
    reconnectionInfo: false,
  };

  pollCount: number = 0;

  saveData: () => void = () => {}

  componentDidMount() {
    ValidatorForm.addValidationRule('isIP', isIP);
    ValidatorForm.addValidationRule('isHostname', isHostname);
    ValidatorForm.addValidationRule('isOptionalIP', optional(isIP));
    const { saveData } = this.props;
    this.saveData = saveData;

    const { selectedNetwork } = this.context;
    if (selectedNetwork) {
      const wifiSettings: WiFiSettings = {
        ssid: selectedNetwork.ssid,
        password: "",
        hostname: this.props.data.hostname,
        static_ip_config: false,
      }
      this.props.setData(wifiSettings);
    }

  }

  componentWillUnmount() {
    this.context.deselectNetwork();
  }

  render() {
    const { data, handleValueChange, classes}  = this.props;
    const { validatingParams, reconnectionInfo } = this.state
    if(reconnectionInfo) {
      return (
        <div className={classes.connectingSettings}>
          <Typography variant="subtitle1">
            Device was reconnected. Check display for new IP and point browser to it.
          </Typography>
        </div>
      )
    }
    return (
      <WiFiConnectionContext.Consumer>
        {({selectedNetwork, deselectNetwork}) => (
        <ValidatorForm onSubmit={this.validateAndSaveParams}>
          <div>Selected network: {selectedNetwork?selectedNetwork.ssid:"n/a"}</div>
          {validatingParams &&
            <div className={classes.connectingSettings}>
              <LinearProgress className={classes.connectingSettingsDetails} />
              <Typography variant="h6" className={classes.connectingProgress}>
                Connecting&hellip;
              </Typography>
          </div>}
         {selectedNetwork ?
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
              Save
            </FormButton>
          </FormActions>
        </ValidatorForm>
        )
        }
      </WiFiConnectionContext.Consumer>
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
      } else if (response.status === 503 || response.status ===  429) {
        const retryAfter = response.headers.get("Retry-After")
        let timeout = 1
        if (retryAfter) {
          timeout = parseInt(retryAfter, 10);
        }
        setTimeout(this.validateAndSaveParams, timeout*1000)
        throw retryError503()
      } else {
        this.setState({ validatingParams: false });
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
    fetch(CONNECT_STATUS_ENDPOINT_PATH, {method: 'GET'})
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
        const status : ConnectingStatus = json
        if (status.success) {
          this.props.enqueueSnackbar("Update successful.", {
            variant: 'success',
          });
          this.setState({ validatingParams: false });
          return;
        } else if (status.disconnect_reason === 202) {
          throw Error("Invalid password")
        } else {
          throw Error("WiFi connection failed")
        }
      })
      .catch(error => {
        if (error.name !== RETRY_EXCEPTION_TYPE) {
          if(error instanceof  TypeError) {
            this.setState({ validatingParams: false, reconnectionInfo: true });
          } else {
            this.props.enqueueSnackbar("Validation error: " + error.message, {
              variant: 'error',
            });
            this.setState({ validatingParams: false });
          }
        }
      });
  }
}

export default withStyles(styles)(WiFiSettingsForm);
