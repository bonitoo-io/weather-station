import React, { Component, RefObject } from 'react';
import { SnackbarProvider } from 'notistack';

import { IconButton, Typography } from '@material-ui/core';
import CloseIcon from '@material-ui/icons/Close';

import AppRouting from './AppRouting';
import CustomMuiTheme from './CustomMuiTheme';
import { ABOUT_INFO_ENDPOINT, PROJECT_NAME } from './api';
import { AppStateContext, AppStateContextValue } from './AppStateContext';
import { AboutInfo, AppState } from './about/types';

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
            <Typography variant="subtitle1">
              Loading..
            </Typography>
            :
            <AppRouting/>
            }
          </SnackbarProvider>
        </AppStateContext.Provider>
      </CustomMuiTheme>
    );
  }

  //            <Switch>
//  <Route component={AppRouting} />
  //</Switch>

  loadData = () => {
     fetch(ABOUT_INFO_ENDPOINT).then(response => {
      if (response.status === 200) {
        return response.json();
      }
      throw Error("Invalid status code: " + response.status);
    }).then(json => {
      const info : AboutInfo = json
      console.log({wifiConfigured: info.appState !== AppState.WifiConfigNeeded})
      this.setState({loading: false, wifiConfigured: info.appState !== AppState.WifiConfigNeeded})
    }).catch(error => {
      const errorMessage = error.message || "Unknown error";
      this.notistackRef.current.enqueueSnackbar("Problem fetching: " + errorMessage, { variant: 'error' });
      this.setState({loading: false}) 
    });
  }
}

export default App
