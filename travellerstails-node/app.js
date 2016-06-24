var express = require('express'),
	path = require('path'),
	favicon = require('serve-favicon'),
	logger = require('morgan'),
	cookieParser = require('cookie-parser'),
	bodyParser = require('body-parser'),

	mongo = require('mongodb'),
	monk = require('monk'),
	
	db = monk(process.env.OPENSHIFT_MONGODB_DB_URL ?
		process.env.OPENSHIFT_MONGODB_DB_URL + 'travellerstails' : 'mongodb://localhost:27017/taglet'),
	
	stylus = require('stylus'),
	routes = require('./routes/index'),
	app = express();

function compile(str, path) {
  return stylus(str)
    .set('filename', path);
    //.use(nib());
}

// view engine setup
app.set('views', path.join(__dirname, 'views'));
app.set('view engine', 'jade');

/*
if (app.get('env') === 'development') {
  app.locals.pretty = true; // true gives better formatting but introduces extra unwanted spaces into html
}
*/

// uncomment after placing your favicon in /public
//app.use(favicon(__dirname + '/public/favicon.ico'));
app.use(logger('dev'));
app.use(bodyParser.json({ 'limit': '5mb' }));
app.use(bodyParser.urlencoded({'limit': '5mb', 'extended': false }));
app.use(cookieParser('nemotode k apocalypse'));

app.use(stylus.middleware({src: path.join(__dirname, '/public'), compile: compile}));

app.use(express.static(path.join(__dirname, 'public')));

// ensure we're signed in at all other locations except /signin itself
app.use(function(req, res, next) {
	res.locals.user = req.signedCookies['user'];
	
	// static files are handled already, signin form is accessed when not signed-in but all others require a user
	if (res.locals.user || req.path === '/signin/' || req.path === '/signin' || req.path === '/export/' || req.path === '/export') {
    	next();
    }
    else {
    	if(req.xhr) {
    		// ajax requests get a brief response
			res.status(401).send("no longer signed in");
    	}
    	else {
    		// all others are redirected to signin
			res.redirect('/signin/?to=' + (req.originalUrl || req.url));
		}
	}
});

// Make our db accessible to our router
app.use(function(req, res, next) {
    req.db = db;
    next();
});

app.use('/', routes);

// catch 404 and forward to error handler
app.use(function(req, res, next) {
  var err = new Error('not found');
  err.status = 404;
  next(err);
});

// error handlers

// development error handler
// will print stacktrace
if (app.get('env') === 'development') {
  app.use(function(err, req, res, next) {
    res.status(err.status || 500);
    res.render('error', {
      message: err.message,
      error: err
    });
  });
}

// production error handler
// no stacktraces leaked to user
app.use(function(err, req, res, next) {
  res.status(err.status || 500);
  res.render('error', {
    message: err.message,
    error: {}
  });
});


module.exports = app;
