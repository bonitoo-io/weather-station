import React, { Component, RefObject } from 'react';
import { Route, Switch } from 'react-router';
import { SnackbarProvider } from 'notistack';

import { IconButton } from '@material-ui/core';
import CloseIcon from '@material-ui/icons/Close';

import AppRouting from './AppRouting';
import CustomMuiTheme from './CustomMuiTheme';
import { PROJECT_NAME } from './api';
import { AppStateContext, AppStateContextDefaultValue } from './AppStateContext';

class App extends Component<{}> {

  notistackRef: RefObject<any> = React.createRef();

  componentDidMount() {
    document.title = PROJECT_NAME;
  }

  onClickDismiss = (key: string | number | undefined) => () => {
    this.notistackRef.current.closeSnackbar(key);
  }

  render() {
    return (
      <CustomMuiTheme>
        <AppStateContext.Provider value={AppStateContextDefaultValue}>
          <SnackbarProvider maxSnack={3} anchorOrigin={{ vertical: 'bottom', horizontal: 'left' }}
            ref={this.notistackRef}
            action={(key) => (
              <IconButton onClick={this.onClickDismiss(key)} size="small">
                <CloseIcon />
              </IconButton>
            )}>
            <Switch>
              <Route component={AppRouting} />
            </Switch>
          </SnackbarProvider>
        </AppStateContext.Provider>
      </CustomMuiTheme>
    );
  }
}

export default App
