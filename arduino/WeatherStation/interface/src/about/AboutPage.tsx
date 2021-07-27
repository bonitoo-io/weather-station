  import React, { Component, Fragment } from 'react';
import { Link as RouterLink } from 'react-router-dom';
import { Avatar, Button, Divider, Dialog, DialogTitle, DialogContent, DialogActions, Box, Typography, Link } from '@material-ui/core';
import { List, ListItem, ListItemAvatar, ListItemText } from '@material-ui/core';
import { Theme } from '@material-ui/core';
import { WithTheme, withTheme } from '@material-ui/core/styles';

import ThermostatIcon from './ThermostatIcon';
import ShowChartIcon from '@material-ui/icons/ShowChart';
import DataUsageIcon from '@material-ui/icons/DataUsage';
import MemoryIcon from '@material-ui/icons/Memory';
import SdStorageIcon from '@material-ui/icons/SdStorage';
import FolderIcon from '@material-ui/icons/Folder';
import DevicesIcon from '@material-ui/icons/Devices';
import PowerSettingsNewIcon from '@material-ui/icons/PowerSettingsNew';
import RefreshIcon from '@material-ui/icons/Refresh';
import SettingsBackupRestoreIcon from '@material-ui/icons/SettingsBackupRestore';
import FilterIcon from '@material-ui/icons/Filter';
import AlignHorizontalCenterIcon from '@material-ui/icons/FormatAlignCenter';

import { RestFormProps, FormButton, ErrorButton, HighlightAvatar } from '../components';
import { FACTORY_RESET_ENDPOINT, RESTART_ENDPOINT } from '../api';
import { AppStateContext } from '../AppStateContext';
import { AboutInfo, AppState } from './types';

interface AboutPageState {
  confirmRestart: boolean;
  confirmFactoryReset: boolean;
  processing: boolean;
}

type AboutPageProps = RestFormProps<AboutInfo> & WithTheme;

function formatNumber(num: number) {
  return new Intl.NumberFormat().format(num);
}

const day = 24*3600 
const hour = 3600 

function formatUptime(num: number) {
  let uptime: string = ''
  num = Math.floor(num/ 1000)
  if(num >= day) {
    uptime += formatNumber(Math.floor(num /day)) + 'days '
    num %= day
  }
  if(num >= hour) {
    uptime += formatNumber(Math.floor(num / hour)) + 'hours '
    num %= hour
  }
  if(num >= 60) {
    uptime += formatNumber(Math.floor(num / 60)) + 'minutes '
    num %= 60
  }
  if(num > 0) {
    uptime += formatNumber(num)  + 'seconds'
  }
  return uptime;
}

export const appStatusHighlight = ({ appState }: AboutInfo, theme: Theme) => {
  switch (appState) {
    case AppState.Ok:
      return theme.palette.primary.main;
    case AppState.WifiConfigNeeded:
    case AppState.InfluxDBConfigNeeded:
      return theme.palette.warning.main;
    case AppState.Error:
      return theme.palette.error.main;
    default:
      return theme.palette.secondary.main;
  }
}

function appStatus(data: AboutInfo) {
  switch(data.appState) {
    case AppState.WifiConfigNeeded: 
      return <Typography variant="subtitle1">
        Weather Station requires <Link component={RouterLink}  to="/wifi/scan">WiFi configuration</Link>
        </Typography>
    case AppState.InfluxDBConfigNeeded:
      return <Typography variant="subtitle1">
        Weather Station requires <Link component={RouterLink}  to="/influxdb/settings">InfluxDB connection configuration</Link>
        </Typography>
    case AppState.Error:
      return <Typography variant="subtitle1">
        {data.error}
      </Typography>
    default:
      return <Typography variant="subtitle1">
        Working
      </Typography>
  }
}


class AboutPage extends Component<AboutPageProps, AboutPageState> {

  static contextType = AppStateContext;
  context!: React.ContextType<typeof AppStateContext>;


  state: AboutPageState = {
    confirmRestart: false,
    confirmFactoryReset: false,
    processing: false
  }

  createListItems() {
    const { data, theme } = this.props
    this.context.wifiConfigured = data.appState !== AppState.WifiConfigNeeded
    return (
      <Fragment>
        <Typography variant="h5">
        Weather Station
        </Typography>
        <Typography variant="h6">
        Created for <Link href="https://www.influxdata.com" target="_blank" rel="noreferrer">InfluxData</Link> by <Link href="https://bonitoo.io" target="_blank" rel="noreferrer">Bonitoo</Link>
        </Typography>
        <ListItem >
          <ListItemAvatar>
            <HighlightAvatar color={appStatusHighlight(data, theme)}>
              <ShowChartIcon />
            </HighlightAvatar>
          </ListItemAvatar>
          <ListItemText primary="Status" secondary={appStatus(data)} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <ThermostatIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Sensor values" secondary={data.temp + '°' + (data.useMetric?'C':'F') + ' ' + data.hum + '%'} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <MemoryIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Device ID" secondary={data.deviceId} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <AlignHorizontalCenterIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Uptime" secondary={formatUptime(data.uptime)} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <FilterIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Version" secondary={data.version} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <DevicesIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Device (Platform / SDK)" secondary={data.espPlatform + ' / ' + data.sdkVersion} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <ShowChartIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="CPU Frequency" secondary={data.cpuFreq + ' MHz'} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <MemoryIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Heap (Free / Max Alloc)" secondary={formatNumber(data.freeHeap) + ' / ' + formatNumber(data.maxAllocHeap) + ' bytes (' + data.heapFragmentation + '% fragmentation)'} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <DataUsageIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Sketch (Size / Free)" secondary={formatNumber(data.sketchSize) + ' / ' + formatNumber(data.freeSketchSpace) + ' bytes'} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <SdStorageIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Flash Chip (Size / Speed)" secondary={formatNumber(data.flashChipSize) + ' bytes / ' + (data.flashChipSpeed / 1000000).toFixed(0) + ' MHz'} />
        </ListItem>
        <Divider variant="inset" component="li" />
        <ListItem >
          <ListItemAvatar>
            <Avatar>
              <FolderIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="File System (Used / Total)" secondary={formatNumber(data.fsUsed) + ' / ' + formatNumber(data.fsTotal) + ' bytes (' + formatNumber(data.fsTotal - data.fsUsed) + '\xa0bytes free)'} />
        </ListItem>
        <Divider variant="inset" component="li" />

      </Fragment>

    );
  }

  renderRestartDialog() {
    return (
      <Dialog
        open={this.state.confirmRestart}
        onClose={this.onRestartRejected}
      >
        <DialogTitle>Confirm Restart</DialogTitle>
        <DialogContent dividers>
          Are you sure you want to restart the device?
        </DialogContent>
        <DialogActions>
          <Button variant="contained" onClick={this.onRestartRejected} color="secondary">
            Cancel
          </Button>
          <Button startIcon={<PowerSettingsNewIcon />} variant="contained" onClick={this.onRestartConfirmed} disabled={this.state.processing} color="primary" autoFocus>
            Restart
          </Button>
        </DialogActions>
      </Dialog>
    )
  }

  onRestart = () => {
    this.setState({ confirmRestart: true });
  }

  onRestartRejected = () => {
    this.setState({ confirmRestart: false });
  }

  onRestartConfirmed = () => {
    this.setState({ processing: true });
    fetch(RESTART_ENDPOINT, { method: 'POST' })
      .then(response => {
        if (response.status === 200) {
          this.props.enqueueSnackbar("Device is restarting", { variant: 'info' });
          this.setState({ processing: false, confirmRestart: false });
        } else {
          throw Error("Invalid status code: " + response.status);
        }
      })
      .catch(error => {
        this.props.enqueueSnackbar(error.message || "Problem restarting device", { variant: 'error' });
        this.setState({ processing: false, confirmRestart: false });
      });
  }

  renderFactoryResetDialog() {
    return (
      <Dialog
        open={this.state.confirmFactoryReset}
        onClose={this.onFactoryResetRejected}
      >
        <DialogTitle>Confirm Factory Reset</DialogTitle>
        <DialogContent dividers>
          Are you sure you want to reset the device to its factory defaults?
        </DialogContent>
        <DialogActions>
          <Button variant="contained" onClick={this.onFactoryResetRejected} color="secondary">
            Cancel
          </Button>
          <ErrorButton startIcon={<SettingsBackupRestoreIcon />} variant="contained" onClick={this.onFactoryResetConfirmed} disabled={this.state.processing} autoFocus>
            Factory Reset
          </ErrorButton>
        </DialogActions>
      </Dialog>
    )
  }

  onFactoryReset = () => {
    this.setState({ confirmFactoryReset: true });
  }

  onFactoryResetRejected = () => {
    this.setState({ confirmFactoryReset: false });
  }

  onFactoryResetConfirmed = () => {
    this.setState({ processing: true });
    fetch(FACTORY_RESET_ENDPOINT, { method: 'POST' })
      .then(response => {
        if (response.status === 200) {
          this.props.enqueueSnackbar("Factory reset in progress.", { variant: 'error' });
          this.setState({ processing: false, confirmFactoryReset: false });
        } else {
          throw Error("Invalid status code: " + response.status);
        }
      })
      .catch(error => {
        this.props.enqueueSnackbar(error.message || "Problem factory resetting device", { variant: 'error' });
        this.setState({ processing: false, confirmRestart: false });
      });
  }

  render() {
    return (
      <Fragment>
        <List>
          {this.createListItems()}
        </List>
        <Box display="flex" flexWrap="wrap">
          <Box flexGrow={1} padding={1}>
            <FormButton startIcon={<RefreshIcon />} variant="contained" color="secondary" onClick={this.props.loadData}>
              Refresh
            </FormButton>
          </Box>
          <Box flexWrap="none" padding={1} whiteSpace="nowrap">
            <FormButton startIcon={<PowerSettingsNewIcon />} variant="contained" color="primary" onClick={this.onRestart}>
              Restart
            </FormButton>
            <ErrorButton startIcon={<SettingsBackupRestoreIcon />} variant="contained" onClick={this.onFactoryReset}>
              Factory reset
            </ErrorButton>
          </Box>
        </Box>
        {this.renderRestartDialog()}
        {this.renderFactoryResetDialog()}
      </Fragment>
    );
  }

}

export default withTheme(AboutPage);
