import React, { Component } from 'react';
import { Redirect, Switch, Route, RouteComponentProps } from 'react-router-dom'


import { MenuAppBar } from '../components';

import AboutController from './AboutController';

type AboutProps = RouteComponentProps;

class About extends Component<AboutProps> {

  handleTabChange = (event: React.ChangeEvent<{}>, path: string) => {
    this.props.history.push(path);
  };

  render() {
    return (
      <MenuAppBar sectionTitle="Status">
        <Switch>
          <Route exact path="/about/about" component={AboutController} />
          <Redirect to="/about/about" />
        </Switch>
      </MenuAppBar>
    )
  }
}

export default About;
