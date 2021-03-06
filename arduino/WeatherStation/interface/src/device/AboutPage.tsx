import React, { Component } from 'react';
import { Avatar, Divider, Typography, Link, List } from '@material-ui/core';
import { ListItem, ListItemAvatar, ListItemText } from '@material-ui/core';

import ShowChartIcon from '@material-ui/icons/ShowChart';
import MemoryIcon from '@material-ui/icons/Memory';
import DevicesIcon from '@material-ui/icons/Devices';
import FilterIcon from '@material-ui/icons/Filter';
import NoteIcon from '@material-ui/icons/Note';
import ThermostatIcon from './ThermostatIcon';

import { RestFormProps } from '../components';
import { AboutInfo } from './types';


type AboutPageProps = RestFormProps<AboutInfo>;

class AboutPage extends Component<AboutPageProps> {

  render() {
    const { data } = this.props
    return (
      <List>
        <Typography variant="subtitle1">
        Created for <Link href="https://www.influxdata.com" target="_blank" rel="noreferrer">InfluxData</Link> company by <Link href="https://bonitoo.io" target="_blank" rel="noreferrer">Bonitoo.io</Link>
        </Typography>
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
              <ThermostatIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Sensor Type" secondary={data.sensorName} />
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
              <NoteIcon />
            </Avatar>
          </ListItemAvatar>
          <ListItemText primary="Licenses" secondary={this.renderLicenses()} />
        </ListItem>
        <Divider variant="inset" component="li" />
      </List>
    );
  }

  renderLicenses = () => {
    return (
      <span>
        Programmed using various <Link href="https://github.com/bonitoo-io/weather-station/blob/main/arduino/WeatherStation/LICENSES.md" target="_blank" rel="noreferrer">open source software</Link>
      </span>
    )
  }

}

export default AboutPage;
