import React, { Component, RefObject } from 'react';
import { SnackbarProvider } from 'notistack';

import { IconButton } from '@material-ui/core';
import CloseIcon from '@material-ui/icons/Close';

import AppRouting from './AppRouting';
import CustomMuiTheme from './CustomMuiTheme';
import { PROJECT_NAME, retryError503, RETRY_EXCEPTION_TYPE, WIFI_STATUS_ENDPOINT } from './api';
import { AppStateContext, AppStateContextValue } from './AppStateContext';
import FullScreenLoading from './components/FullScreenLoading';
import { WiFiConnectionStatus, WiFiStatus } from './wifi/types';

class App extends Component<{}, AppStateContextValue> {

  notistackRef: RefObject<any> = React.createRef();

  constructor(props: any) {
    super(props)
    this.state = {
      loading: true,
      wifiConfigured: false
    }
    this.loadData()
  }


  componentDidMount() {
    document.title = PROJECT_NAME;
  }

  onClickDismiss = (key: string | number | undefined) => () => {
    this.notistackRef.current.closeSnackbar(key);
  }

  render() {
    const {loading} = this.state
    return (
      <CustomMuiTheme>
        <AppStateContext.Provider value={this.state}>
          <SnackbarProvider maxSnack={3} anchorOrigin={{ vertical: 'bottom', horizontal: 'left' }}
            ref={this.notistackRef}
            action={(key) => (
              <IconButton onClick={this.onClickDismiss(key)} size="small">
                <CloseIcon />
              </IconButton>
            )}>
            {loading?
            <FullScreenLoading/>
            :
            <AppRouting/>
            }
          </SnackbarProvider>
        </AppStateContext.Provider>
      </CustomMuiTheme>
    );
  }

  loadData = () => {
     fetch(WIFI_STATUS_ENDPOINT).then(response => {
      if (response.status === 200) {
        return response.json();
      } else if (response.status === 503 || response.status ===  429) {
        const retryAfter = response.headers.get("Retry-After")
        let timeout = 1
        if (retryAfter) {
          timeout = parseInt(retryAfter, 10);
        }
        setTimeout(this.loadData, timeout*1000)
        throw retryError503()
      }
      throw Error("Invalid status code: " + response.status);
    }).then(json => {
      const status : WiFiStatus = json
      this.setState({loading: false, wifiConfigured: status.status === WiFiConnectionStatus.WIFI_STATUS_CONNECTED})
    }).catch(error => {
      if(error.name !== RETRY_EXCEPTION_TYPE) {
        const errorMessage = error.message || "Unknown error";
        this.notistackRef.current.enqueueSnackbar("Problem fetching: " + errorMessage, { variant: 'error' });
        this.setState({loading: false}) 
      }
    });
  }
}

export default App
