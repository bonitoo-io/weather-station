import React, { Component } from 'react';
import { withSnackbar, WithSnackbarProps } from 'notistack';
import { Link as RouterLink } from 'react-router-dom';
import { createStyles, WithStyles, Theme, withStyles, Typography, LinearProgress, Link } from '@material-ui/core';
import PermScanWifiIcon from '@material-ui/icons/PermScanWifi';

import { FormActions, FormButton, SectionContent } from '../components';
import { SCAN_NETWORKS_ENDPOINT, LIST_NETWORKS_ENDPOINT, LIST_SAVED_NETWORKS_ENDPOINT, CONNECT_TO_SAVED_ENDPOINT_PATH, CONNECT_STATUS_ENDPOINT_PATH, POLLING_FREQUENCY, NUM_POLLS, retryErrorPolling, RETRY_EXCEPTION_TYPE, retryError503 } from '../api';
import { AppStateContext } from '../AppStateContext';
import WiFiNetworkSelector from './WiFiNetworkSelector';
import { WiFiNetworkList, WiFiNetwork, SavedNetworkList, SavedNetwork,  ConnectingStatus } from './types';
import { WiFiConnectionContext } from './WiFiConnectionContext';

interface WiFiNetworkScannerState {
  scanningForNetworks: boolean;
  errorMessage?: string;
  networkList?: WiFiNetworkList;
  savedNetworks?: SavedNetworkList;
  connectingToSaved: boolean;
  reconnectionInfo: boolean;
}

const styles = (theme: Theme) => createStyles({
  scanningSettings: {
    margin: theme.spacing(0.5),
  },
  scanningSettingsDetails: {
    margin: theme.spacing(4),
    textAlign: "center"
  },
  scanningProgress: {
    margin: theme.spacing(4),
    textAlign: "center"
  }
});

type WiFiNetworkScannerProps = WithSnackbarProps & WithStyles<typeof styles>;

class WiFiNetworkScanner extends Component<WiFiNetworkScannerProps, WiFiNetworkScannerState> {

  static contextType = WiFiConnectionContext;
  context!: React.ContextType<typeof WiFiConnectionContext>;

  networkToSelect: WiFiNetwork | undefined;
  networkToConnect: SavedNetwork | undefined;

  pollCount: number = 0;

  state: WiFiNetworkScannerState = {
    scanningForNetworks: false,
    connectingToSaved: false,
    reconnectionInfo: false,
  };

  componentDidMount() {
    this.scanNetworks();
    this.loadSavedNetworks();
  }

  requestNetworkScan = () => {
    const { scanningForNetworks } = this.state;
    if (!scanningForNetworks) {
      this.scanNetworks();
    }
  }

  scanNetworks() {
    this.pollCount = 0;
    this.setState({ scanningForNetworks: true, networkList: undefined, errorMessage: undefined });
    fetch(SCAN_NETWORKS_ENDPOINT).then(response => {
      if (response.status === 202) {
        this.schedulePollTimeout();
        return;
      } else if (response.status === 503 || response.status ===  429) {
        const retryAfter = response.headers.get("Retry-After")
        let timeout = 1
        if (retryAfter) {
          timeout = parseInt(retryAfter, 10);
        }
        setTimeout(this.scanNetworks, timeout*1000)
        throw retryError503()
      }
      throw Error("Scanning for networks returned unexpected response code: " + response.status);
    }).catch(error => {
      if(error.name !== RETRY_EXCEPTION_TYPE) {
        this.props.enqueueSnackbar("Problem scanning: " + error.message, {
          variant: 'error',
        });
        this.setState({ scanningForNetworks: false, networkList: undefined, errorMessage: error.message });
      }
    });
  }

  schedulePollTimeout() {
    setTimeout(this.pollNetworkList, POLLING_FREQUENCY);
  }

  compareNetworks(network1: WiFiNetwork, network2: WiFiNetwork) {
    if (network1.rssi < network2.rssi)
      return 1;
    if (network1.rssi > network2.rssi)
      return -1;
    return 0;
  }

  pollNetworkList = () => {
    fetch(LIST_NETWORKS_ENDPOINT)
      .then(response => {
        if (response.status === 200) {
          return response.json();
        } else if (response.status === 202) {
          if (++this.pollCount < NUM_POLLS) {
            this.schedulePollTimeout();
            throw retryErrorPolling();
          } else {
            throw Error("Device did not return network list in timely manner.");
          }
        } else if (response.status === 503 || response.status ===  429) {
          const retryAfter = response.headers.get("Retry-After")
          let timeout = 1
          if (retryAfter) {
            timeout = parseInt(retryAfter, 10);
          }
          setTimeout(this.pollNetworkList, timeout*1000)
          throw retryError503()
        }
        throw Error("Device returned unexpected response code: " + response.status);
      })
      .then(json => {
        json.networks.sort(this.compareNetworks)
        this.setState({ scanningForNetworks: false, networkList: json, errorMessage: undefined })
      })
      .catch(error => {
        if (error.name !== RETRY_EXCEPTION_TYPE) {
          this.props.enqueueSnackbar("Problem scanning: " + error.message, {
            variant: 'error',
          });
          this.setState({ scanningForNetworks: false, networkList: undefined, errorMessage: error.message });
        }
      });
  }

  loadSavedNetworks = () => {
    this.setState({savedNetworks: undefined})
    fetch(LIST_SAVED_NETWORKS_ENDPOINT).then(response => {
      if (response.status === 200) {
        return response.json();
      } else if (response.status === 503 || response.status ===  429) {
        const retryAfter = response.headers.get("Retry-After")
        let timeout = 1
        if (retryAfter) {
          timeout = parseInt(retryAfter, 10);
        }
        setTimeout(this.loadSavedNetworks, timeout*1000)
        throw retryError503()
      }
      throw Error("Device returned unexpected response code: " + response.status);
    })
    .then(json => {
      this.setState({savedNetworks: json})
    }).catch(error => {
      if(error.name !== RETRY_EXCEPTION_TYPE) {
        this.props.enqueueSnackbar("Problem loading saved networks: " + error.message, {
          variant: 'error',
        });
      }
    });
  } 

  connectToSavedClicked = (savedNetwork: SavedNetwork, network: WiFiNetwork) => {
    if(savedNetwork.connected) {
      this.props.enqueueSnackbar("Already connected to " + savedNetwork.ssid, {
        variant: 'info',
      });
      return
    }
    this.networkToSelect = network;
    this.networkToConnect = savedNetwork;
    this.setState({ connectingToSaved: true });
    this.connectToSaved();
  }

  connectToSaved = () => {
    fetch(CONNECT_TO_SAVED_ENDPOINT_PATH, {
      method: 'POST',
      body: JSON.stringify(this.networkToConnect),
      headers: {
        'Content-Type': 'application/json'
      }
    }).then(response => { 
      if (response.status === 202) {
        this.pollCount = 0
        this.schedulePollValidation();
        return;
      } else if (response.status === 503 || response.status ===  429) {
        const retryAfter = response.headers.get("Retry-After")
        let timeout = 1
        if (retryAfter) {
          timeout = parseInt(retryAfter, 10);
        }
        setTimeout(this.connectToSaved, timeout*1000)
        throw retryError503()
      }
      throw Error("Device returned unexpected response code: " + response.status);
    })
   .catch(error => {
      if(error.name !== RETRY_EXCEPTION_TYPE) {
        this.props.enqueueSnackbar("Problem connecting to network: " + error.message, {
          variant: 'error',
        });
        this.setState({ connectingToSaved: false });
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
          this.props.enqueueSnackbar("Connection successful.", {
            variant: 'success',
          });
          this.setState({ connectingToSaved: false });
        } else if(this.networkToSelect) {
          this.context.selectNetwork(this.networkToSelect)
          return;
        } else {
          throw new Error("Selected network not found")
        }
      })
      .catch(error => {
        if (error.name !== RETRY_EXCEPTION_TYPE) {
          if(error instanceof  TypeError) {
            // Device was most probably reconnected to different network and is unreachable
            this.setState({ connectingToSaved: false, reconnectionInfo: true, networkList: undefined });
          } else {
            this.props.enqueueSnackbar("Problem connecting to network: " + error.message, {
              variant: 'error',
            });
            this.setState({ connectingToSaved: false });
          }
        }
      });
  }

  renderNetworkScanner() {
    const { classes } = this.props;
    const { scanningForNetworks, networkList, savedNetworks, errorMessage, reconnectionInfo, connectingToSaved } = this.state;
    if (scanningForNetworks) {
      return (
        <div className={classes.scanningSettings}>
          <LinearProgress className={classes.scanningSettingsDetails} />
          <Typography variant="h6" className={classes.scanningProgress}>
            Scanning&hellip;
          </Typography>
        </div>
      );
    }
    if (errorMessage) {
      return (
        <div className={classes.scanningSettings}>
          <Typography variant="h6" className={classes.scanningSettingsDetails}>
            {errorMessage}
          </Typography>
        </div>
      );
    }
    if (reconnectionInfo) {
      return (
        <div className={classes.scanningSettings}>
          <Typography variant="subtitle1">
            Device was reconnected. Check display for new IP and point browser to it.
          </Typography>
        </div>
      );
    }
    return (
      <>
      {connectingToSaved &&
      <div className={classes.scanningSettings}>
          <LinearProgress className={classes.scanningSettingsDetails} />
          <Typography variant="h6" className={classes.scanningProgress}>
            Connecting&hellip;
          </Typography>
      </div>
      }
      {networkList &&
        <WiFiNetworkSelector networkList={networkList} savedNetworks={savedNetworks} connectToSaved={this.connectToSavedClicked} />
      }
      </>
    );
  }

  render() {
    const { scanningForNetworks, reconnectionInfo } = this.state;
    return (
      <AppStateContext.Consumer>
        {({wifiConfigured}) => (
          <SectionContent title={wifiConfigured?"Network Scanner":"Select WiFi network"}>
            {!reconnectionInfo &&
              <Typography variant="subtitle1">
                Weather station requires active Wifi connection. Please select a suitable WiFi network or <Link component={RouterLink}  to="/wifi/settings">manually enter SSID</Link> (only 2.4GHz WiFi networks are supported) The WiFi networks are sorted from the strongest one.
              </Typography>
            }
            {this.renderNetworkScanner()}
            {!reconnectionInfo &&
            <FormActions>
              <FormButton startIcon={<PermScanWifiIcon />} variant="contained" color="secondary" onClick={this.requestNetworkScan} disabled={scanningForNetworks}>
                Scan again&hellip;
              </FormButton>
            </FormActions>
          }
          </SectionContent>
        )
      }
    </AppStateContext.Consumer>
    );
  }

}

export default withSnackbar(withStyles(styles)(WiFiNetworkScanner));
