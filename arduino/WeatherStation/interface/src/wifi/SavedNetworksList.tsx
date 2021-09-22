import { Avatar, Button, createStyles, Dialog, DialogActions, DialogContent, DialogContentText, DialogTitle, IconButton, LinearProgress, 
  List, ListItem, ListItemAvatar, ListItemSecondaryAction, ListItemText, Theme, Typography, withStyles, WithStyles } from '@material-ui/core';
import { withSnackbar, WithSnackbarProps } from 'notistack';
import WifiIcon from '@material-ui/icons/Wifi';
import DeleteIcon from '@material-ui/icons/Delete';
import React, { Component } from 'react';
import { SavedNetwork, SavedNetworkList } from './types';
import { LIST_SAVED_NETWORKS_ENDPOINT } from '../api';
import { SectionContent } from '../components';

interface SavedNetworksListState {
  loading: boolean;
  confirmRemove: boolean
  errorMessage?: string
  networkList?: SavedNetworkList
  networkToRemove?: number
}

const styles = (theme: Theme) => createStyles({
  loadingSettings: {
    margin: theme.spacing(0.5),
  },
  loadingSettingsDetails: {
    margin: theme.spacing(4),
    textAlign: "center"
  },
  loadingProgress: {
    margin: theme.spacing(4),
    textAlign: "center"
  }
});


type SavedNetworksListProps = WithSnackbarProps & WithStyles<typeof styles>;

class SavedNetworksList extends Component<SavedNetworksListProps, SavedNetworksListState> {
  state: SavedNetworksListState = {
    loading: false,
    confirmRemove: false
  };

  componentDidMount() {
    this.loadNetworks();
  }

  renderList() {
    const { classes } = this.props;
    const { loading, errorMessage, networkList } = this.state;
    if (loading || !networkList) {
      return (
        <div className={classes.loadingSettings}>
          <LinearProgress className={classes.loadingSettingsDetails} />
          <Typography variant="h6" className={classes.loadingProgress}>
            Loading&hellip;
          </Typography>
        </div>
      );
    }
    if (errorMessage) {
      return (
        <div className={classes.loadingSettings}>
          <Typography variant="h6" className={classes.loadingSettingsDetails}>
            {errorMessage}
          </Typography>
        </div>
      );
    }
    if(networkList.networks.length > 0) {
      return (
        <List>
          {networkList.networks.map(this.renderNetwork)}
        </List>
      );
    } else {
      return (
        <Typography variant="subtitle1" className={classes.loadingSettingsDetails}>
            No saved networks
        </Typography>
      );
    }
  }

  render() {
    return (
      <SectionContent title="Saved Networks">
          {this.renderList()}
          {this.renderDeleteDialog()}
      </SectionContent>
    )
  }


  renderNetwork = (network: SavedNetwork) => {
    return (
      <ListItem key={network.id}>
        <ListItemAvatar>
          <Avatar>
            <WifiIcon/>
          </Avatar>
        </ListItemAvatar>
        <ListItemText
          primary={network.ssid}
          secondary = {network.connected?"Connected":""}
        />
        <ListItemSecondaryAction>
          <IconButton aria-label="Forget network" onClick={this.onRemoveNetwork(network.id)}>
            <DeleteIcon />
          </IconButton>
        </ListItemSecondaryAction>
      </ListItem>
    );
  }

  renderDeleteDialog = () => {
    const { confirmRemove, networkList, networkToRemove} = this.state
    let network : SavedNetwork | undefined
    if(networkList && networkToRemove) {
      network = networkList.networks.find((value)=>{
        return value.id === networkToRemove
      })
    }
    return (
      <Dialog
        open={confirmRemove}
        onClose={this.onRemoveRejected}
      >
        <DialogTitle>Confirm Forget</DialogTitle>
        <DialogContent dividers>
          Are you sure you want to remove network '{network?.ssid}'? 
          {network?.connected?
          <DialogContentText>Device will disconnect from WiFi</DialogContentText>:""} 
        </DialogContent>
        <DialogActions>
          <Button variant="contained" onClick={this.onRemoveRejected} color="secondary">
            Cancel
          </Button>
          <Button startIcon={<DeleteIcon />} variant="contained" onClick={this.removeNetwork} disabled={this.state.loading} color="primary" autoFocus>
            Forget
          </Button>
        </DialogActions>
      </Dialog>
    )
  }

  compareNetworks(network1: SavedNetwork, network2: SavedNetwork) {
    if (network1.ssid > network2.ssid)
      return 1;
    if (network1.ssid < network2.ssid)
      return -1;
    return 0;
  }

  loadList = (fetch: Promise<Response>) => {
    fetch.then(response => {
      if (response.status === 200) {
        return response.json();
      }
      throw Error("Device returned unexpected response code: " + response.status);
    })
    .then(json => {
      json.networks.sort(this.compareNetworks)
      this.setState({ loading: false, networkList: json, errorMessage: undefined })
    }).catch(error => {
      this.props.enqueueSnackbar("Problem loading: " + error.message, {
        variant: 'error',
      });
      this.setState({ loading: false, networkList: undefined, errorMessage: error.message });
    });
  } 

  loadNetworks = () => {
    this.setState({ loading: true, networkList: undefined, errorMessage: undefined });
    this.loadList(fetch(LIST_SAVED_NETWORKS_ENDPOINT))
  }

  onRemoveNetwork = (id: number) => ()  => {
    this.setState({ confirmRemove: true, networkToRemove: id });
  }

  removeNetwork = () => {
    this.setState({ loading: true, confirmRemove: false })
    this.loadList(fetch(LIST_SAVED_NETWORKS_ENDPOINT + "?id=" + this.state.networkToRemove, {
      method: 'DELETE',
    }))
    this.setState({networkToRemove: undefined})
  }

  onRemoveRejected = () => {
    this.setState({ confirmRemove: false, networkToRemove: undefined });
  }


}

export default withSnackbar(withStyles(styles)(SavedNetworksList));
