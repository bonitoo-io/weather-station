import React, { RefObject} from 'react';
import { Link, withRouter, RouteComponentProps } from 'react-router-dom';

import { Drawer, AppBar, Toolbar, Divider, Box, IconButton } from '@material-ui/core';
import { Hidden, Typography } from '@material-ui/core';
import { List, ListItem, ListItemIcon, ListItemText } from '@material-ui/core';

import { withStyles, createStyles, Theme, WithTheme, WithStyles, withTheme } from '@material-ui/core/styles';

import WifiIcon from '@material-ui/icons/Wifi';
import SettingsIcon from '@material-ui/icons/Settings';
import MenuIcon from '@material-ui/icons/Menu';

import { PROJECT_NAME } from '../api';

const drawerWidth = 290;

const styles = (theme: Theme) => createStyles({
  root: {
    display: 'flex',
  },
  drawer: {
    [theme.breakpoints.up('md')]: {
      width: drawerWidth,
      flexShrink: 0,
    },
  },
  title: {
    flexGrow: 1
  },
  appBar: {
    marginLeft: drawerWidth,
    [theme.breakpoints.up('md')]: {
      width: `calc(100% - ${drawerWidth}px)`,
    },
  },
  toolbarImage: {
    [theme.breakpoints.up('xs')]: {
      height: 24,
      marginRight: theme.spacing(2)
    },
    [theme.breakpoints.up('sm')]: {
      height: 36,
      marginRight: theme.spacing(3)
    },
  },
  menuButton: {
    marginRight: theme.spacing(2),
    [theme.breakpoints.up('md')]: {
      display: 'none',
    },
  },
  toolbar: theme.mixins.toolbar,
  drawerPaper: {
    width: drawerWidth,
  },
  content: {
    flexGrow: 1
  },
  authMenu: {
    zIndex: theme.zIndex.tooltip,
    maxWidth: 400,
  },
  authMenuActions: {
    padding: theme.spacing(2),
    "& > * + *": {
      marginLeft: theme.spacing(2),
    }
  },
});

interface MenuAppBarState {
  mobileOpen: boolean;
  authMenuOpen: boolean;
}

interface MenuAppBarProps extends WithTheme, WithStyles<typeof styles>, RouteComponentProps {
  sectionTitle: string;
}

class MenuAppBar extends React.Component<MenuAppBarProps, MenuAppBarState> {

  constructor(props: MenuAppBarProps) {
    super(props);
    this.state = {
      mobileOpen: false,
      authMenuOpen: false
    };
  }

  anchorRef: RefObject<HTMLButtonElement> = React.createRef();

  handleToggle = () => {
    this.setState({ authMenuOpen: !this.state.authMenuOpen });
  }

  handleClose = (event: React.MouseEvent<Document>) => {
    if (this.anchorRef.current && this.anchorRef.current.contains(event.currentTarget)) {
      return;
    }
    this.setState({ authMenuOpen: false });
  }

  handleDrawerToggle = () => {
    this.setState({ mobileOpen: !this.state.mobileOpen });
  };

  render() {
    const { classes, theme, children, sectionTitle } = this.props;
    const { mobileOpen } = this.state;
    const path = this.props.match.url;
    const drawer = (
      <div>
        <Toolbar>
          <Box display="flex">
            <img src="/app/icon.png" className={classes.toolbarImage} alt={PROJECT_NAME} />
          </Box>
          <Typography variant="h6" color="textPrimary">
            {PROJECT_NAME}
          </Typography>
          <Divider absolute />
        </Toolbar>
        <List>
          <ListItem to='/wifi/' selected={path.startsWith('/wifi/')} button component={Link}>
            <ListItemIcon>
              <WifiIcon />
            </ListItemIcon>
            <ListItemText primary="WiFi Connection" />
          </ListItem>
          <ListItem to='/system/' selected={path.startsWith('/system/')} button component={Link} >
            <ListItemIcon>
              <SettingsIcon />
            </ListItemIcon>
            <ListItemText primary="System" />
          </ListItem>
          <ListItem to='/influxdb/' selected={path.startsWith('/influxdb/')} button component={Link} >
            <ListItemIcon>
              <SettingsIcon />
            </ListItemIcon>
            <ListItemText primary="InfluxDB Settings" />
          </ListItem>
        </List>
      </div>
    );

    
    return (
      <div className={classes.root}>
        <AppBar position="fixed" className={classes.appBar} elevation={0}>
          <Toolbar>
            <IconButton
              color="inherit"
              aria-label="Open drawer"
              edge="start"
              onClick={this.handleDrawerToggle}
              className={classes.menuButton}
            >
              <MenuIcon />
            </IconButton>
            <Typography variant="h6" color="inherit" noWrap className={classes.title}>
              {sectionTitle}
            </Typography>
          </Toolbar>
        </AppBar>
        <nav className={classes.drawer}>
          <Hidden mdUp implementation="css">
            <Drawer
              variant="temporary"
              anchor={theme.direction === 'rtl' ? 'right' : 'left'}
              open={mobileOpen}
              onClose={this.handleDrawerToggle}
              classes={{
                paper: classes.drawerPaper,
              }}
              ModalProps={{
                keepMounted: true,
              }}
            >
              {drawer}
            </Drawer>
          </Hidden>
          <Hidden smDown implementation="css">
            <Drawer
              classes={{
                paper: classes.drawerPaper,
              }}
              variant="permanent"
              open
            >
              {drawer}
            </Drawer>
          </Hidden>
        </nav>
        <main className={classes.content}>
          <div className={classes.toolbar} />
          {children}
        </main>
      </div>
    );
  }
}

export default withRouter(
  withTheme(
    withStyles(styles)(MenuAppBar)
  )
);
