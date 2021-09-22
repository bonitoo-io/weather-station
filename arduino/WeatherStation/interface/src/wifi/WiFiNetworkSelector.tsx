import React, { Component } from 'react';

import { Avatar, Badge } from '@material-ui/core';
import { List, ListItem, ListItemIcon, ListItemText, ListItemAvatar } from '@material-ui/core';

import WifiIcon from '@material-ui/icons/Wifi';
import LockIcon from '@material-ui/icons/Lock';
import LockOpenIcon from '@material-ui/icons/LockOpen';

import { isNetworkOpen, networkSecurityMode } from './WiFiSecurityModes';
import { WiFiConnectionContext } from './WiFiConnectionContext';
import { SavedNetwork, SavedNetworkList, WiFiNetwork, WiFiNetworkList } from './types';

interface WiFiNetworkSelectorProps {
  networkList: WiFiNetworkList;
  savedNetworks?: SavedNetworkList;
  connectToSaved: (savedNetWork: SavedNetwork, network: WiFiNetwork) => void;
}

class WiFiNetworkSelector extends Component<WiFiNetworkSelectorProps> {

  static contextType = WiFiConnectionContext;
  context!: React.ContextType<typeof WiFiConnectionContext>;

  networkClicked = (network: WiFiNetwork) => () => {
    if(this.props.savedNetworks){
      let sn = this.props.savedNetworks.networks.find((value)=>{
        return value.ssid === network.ssid
      })
      if(sn) {
        this.props.connectToSaved(sn, network)
        return
      }
    }
    this.context.selectNetwork(network)
  }

  

  renderNetwork = (network: WiFiNetwork) => {
    return (
      <ListItem key={network.bssid} button onClick={ this.networkClicked(network) }>
        <ListItemAvatar>
          <Avatar>
            {isNetworkOpen(network) ? <LockOpenIcon /> : <LockIcon />}
          </Avatar>
        </ListItemAvatar>
        <ListItemText
          primary={network.ssid}
          secondary={"Security: " + networkSecurityMode(network) + ", Ch: " + network.channel}
        />
        <ListItemIcon>
          <Badge badgeContent={(network.rssi <= -100 ? 0 : (network.rssi >= -50 ? 100 : 2 * (network.rssi + 100))) + "%"}>
            <WifiIcon />
          </Badge>
        </ListItemIcon>
      </ListItem>
    );
  }

  render() {
    return (
      <List>
        {this.props.networkList.networks.map(this.renderNetwork)}
      </List>
    );
  }

}

export default WiFiNetworkSelector;
