import React, { Fragment } from 'react';
import { Route } from 'react-router-dom';
import { CssBaseline } from '@material-ui/core';
import AppHeader from './components/AppHeader';
import Home from './pages/Home';
import Callback from './Auth/Callback';

const App = () => (
  <Fragment>
    <CssBaseline />
    <AppHeader />
    <Route exact path="/" component={Home} />
    <Route exact path='/callback' component={Callback}/>
  </Fragment>
);

export default App;
