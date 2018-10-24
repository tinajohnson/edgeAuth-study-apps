import React, { Fragment } from 'react';
import { Route } from 'react-router-dom';
import { ImplicitCallback } from '@okta/okta-react';
import { CssBaseline } from '@material-ui/core';
import AppHeader from './components/AppHeader';
import Home from './pages/Home';

const App = () => (
  <Fragment>
    <CssBaseline />
    <AppHeader />
    <Route exact path="/" component={Home} />
    <Route path="/implicit/callback" component={ImplicitCallback} />
  </Fragment>
);

export default App;
