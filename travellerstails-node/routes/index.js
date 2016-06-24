var express = require('express'),
	fs = require('fs'),
	path = require('path'),
	request = require('request'),
	// easiest to keep these here though they may be migrated to the db later
	collectionIDs = ['cook', 'grant', 'horniman', 'hunterian', 'nmm'], // keys in guaranteed alphabetical order
	collections = {
	    'cook': { 'collectionID': 'cook', 'name': 'cook', 'sources': ['COOK/Cook test images and data'] },
	    'grant': { 'collectionID': 'grant', 'name': 'grant', 'sources': ['GRANT/standardised-images'] },
	    'horniman': { 'collectionID': 'horniman', 'name': 'horniman', 'sources': ['Horniman/assets_Horniman02'] },
	    'hunterian': { 'collectionID': 'hunterian', 'name': 'hunterian', 'sources': ['Hunterian/travellers tales - hunterian'] },
	    'nmm': { 'collectionID': 'nmm', 'name': 'national maritime museum', 'sources': ['NMM/NMM 1080 tall images', 'NMM/TravellersTailsDigitalToolimages_15137'] }
	};


function collectionName(collectionID) {
	return collections[collectionID] || collectionID;
};


function errorResponse(res, status, failMessage, e) {
	console.log(failMessage);
	res.status(status);
	res.render('error', { 'message': failMessage, 'error': e });
};


// to add to the tags db collection
function addTag(tag, collectionID, fileID, db, cb) {
	db.get('tags').update( //### need to wait for func completion? ### use async here?
		{ 'tag': tag }, // query
		{ '$addToSet': { 'taggings': [collectionID, fileID] } }, // update operation
		{ 'upsert': true }, // options ( upsert creates if entry for tag doesn't already exist
		cb);
};

var router = express.Router();

router.get('/image/:collectionID/:fileID', function (req, res) {
    var size = req.params.size,
		collectionID = req.params.collectionID,
		fileID = unescape(req.params.fileID);

    req.db.get('resources').findOne({ 'id': fileID, 'source': collectionID }, function (e, item) {
        
        if (e) {
            errorResponse(res, 404, "can't find collections directory");
        } else {
            //streamDropboxImage(item, size, res);
            try {
                var image_path = process.env.OPENSHIFT_DATA_DIR + '/images/' + collectionID + '/' + item.image;
                res.sendFile(image_path, {}, function (err) {
                    if (err) {
                        console.log(err);
                        res.status(err.status).end();
                    } else {
                        console.log('sent file : ' + image_path);
                    }
                });
            } catch (err) {
                console.log(err);
                res.status(err.status).end();
            }
        }

    });
});


// show signin form
router.get('/signin', function(req, res) {
	var referrer = req.query['to'] || '/',
		unknown = req.query['unknown'] === 'true' || false;
	
	res.clearCookie('user'); // visit sign-in page to sign out (!)
	res.render('signin', {'title': 'taglet: sign in', 'unknown': unknown, 'referrer': referrer});
});


// handle actual signin as post
router.post('/signin', function(req, res) {
	var user = req.body['user'],
		referrer = req.body['to'] || '/';
	
	res.clearCookie('user');
	if(user) { //todo: check user for valid chars here
		// responding to form submit; attempt to signin as user
		//req.db.get('users').findOne({'user': user}, function(e, u) {
			//if (e || ! u) {
			if( ! (user in {
				'dosa': 'admin',
				'seadog': 'admin',
				'punk': 'admin',
				'extra': 'user'})) { //######### move to DB again ############
				// user not valid
				res.redirect('/signin/?unknown=true&to=' + referrer);
			}
			else {
				// user ok; set cookie & return to original page or root
				res.cookie('user', user, {'signed': true});
				res.redirect(referrer);
			}
		//});
	}
	else {
		// user not supplied
		res.redirect('/signin/?to=' + referrer);
	}
});


// index page: link to collections & tags list
router.get('/', function(req, res) {			
	res.render('index', {'title': 'taglet', 'user': req.signedCookies['user'], 'collectionIDs': collectionIDs, 'collections': collections});
});


// show all tags
router.get('/tags', function(req, res) {
	req.db.get('tags').find({}, {sort: {'tag': 1}}, function(e, tags) {
		if (e) {
			errorResponse(res, 500, "can't find full tag list", e);
		}
		else {
			res.render('tags', {'title': 'taglet: all tags', 'user': req.signedCookies['user'], 'tags': tags});
		}
	});
});


// show all unused tags
router.get('/unused', function(req, res) {
	req.db.get('tags').find({'taggings': {$size: 0}}, {sort: {'tag': 1}}, function(e, tags) {
		if (e) {
			errorResponse(res, 500, "can't find full tag list", e);
		}
		else {
			//####### reusing tags template ########## note heading is "all tags"
			res.render('tags', {'title': 'taglet: unused tags', 'user': req.signedCookies['user'], 'tags': tags}); 
		}
	});
});


// show images for a single tag
router.get('/tag/:tagText', function(req, res) {
	var tagText = req.params.tagText;
	
	req.db.get('tags').findOne({'tag': decodeURIComponent(tagText)}, function(e, tag) {
		if (e) {
			errorResponse(res, 404, "can't find tag " + tagText);
		}
		else {
			res.render('tag', {'title': 'taglet: single tag', 'user': req.signedCookies['user'], 'tag': tag});
		}
	});
});


// collection image list page
router.get('/collection/:collectionID', function(req, res) {
	var collectionID = req.params.collectionID,
		collection = collectionIDs.indexOf(collectionID) >= 0 ? collections[collectionID] : null;
		
	if(collection) {
		// find filename & tags list for every image in this collection
		req.db.get('resources').find(
			{'source': collectionID},
			{ 'sort': { 'id': 1 } },
			function (e, resources) {
				if(e) {
					errorResponse(res, 500, "couldn't find images in collection: " + collectionID, e);
				}
				else {
					res.render('collection', {
						'title': 'taglet: ' + collection.name,
						'user': req.signedCookies['user'],
						'collection': collection,
						'images': resources}); //############## resources
						//'message': JSON.stringify(resources, null, 4)});
				}
			}
		);
	}
	else {
		errorResponse(res, 404, "can't find collection: " + collectionID);
	}
});


// add-tags page for single image
router.get('/tagger/:collectionID/:fileID', function(req, res) {
	var collectionID = req.params.collectionID,
		fileID = decodeURIComponent(req.params.fileID),
		resource;
		
	req.db.get('resources').find(
		{ 'source': collectionID, 'id': { '$gte': fileID } },
		{ 'limit': 2, 'sort': { 'id': 1 } },
		function(e, resources) {
		    if (resources && resources[0] && resources[0]['id'] === fileID) {
		            resource = resources[0];
				req.db.get('tags').find({}, {sort: {'tag': 1}, fields: {'tag': 1, '_id': 0}}, function(e, tags) {
					if(e) {
						errorResponse(res, 500, "can't find tag list", e);
					}
					else {
						// remove tags from pick list if they are already tagging image
						for(var index = tags.length - 1; index >= 0; index--) {
							if(resource.tags.indexOf(tags[index].tag) >= 0) { //## case-sensitive comparison
								tags.splice(index, 1); // remove tag
							}
						}
				
						res.render('tagger', {
							'title': 'taglet: ' + fileID + " in " + collections[collectionID]['name'] + " collection",
							'user': req.signedCookies['user'], 
							'image': resource, //###########
							'picktags': tags,
							'next': resources[1]
						    //, 'message': JSON.stringify(tags, null, 4)
						});
							
					}
				});
			}
			else {
				errorResponse(res, 404, "can't find " + fileID + " result is " + resources[0]['id'] + " and " + resources[1]['id'] , e);
			}
		}
	);
});


// receive json {"add": newTagToAdd} OR {"remove": tagToRemoveFromThisImage}
// add or remove it & supply rendered updated tags list dom element for this image
router.post('/tagger/:collectionID/:fileID', function(req, res) {
	var json = req.body,
		collectionID = req.params.collectionID,
		fileID = decodeURIComponent(req.params.fileID),
		description = fileID + " in " + collectionID + " collection", //#####
		tag,
		
		error = function(status, message, e) {
			res.status(status)
			res.send(message);
			if(e) {
				res.send(JSON.stringify(e, null, 4));
			}
		};

	if(json && (json.hasOwnProperty('add') || json.hasOwnProperty('remove') || json.hasOwnProperty('name'))) {
		if ( json['name'] ) {
			var name = json['name'];
			if ( name && name.length > 0 ) {
				req.db.get('resources').findAndModify(
					{'id': fileID, 'source': collectionID},
					{'$set': {'name': name}},
					{'new': false},
					function(e, resource) {
						if(e) {
							res.json( { status: "fail", message: "unable to change name", error: JSON.stringify(e) } );
						}
						else {
							res.json( { status: "ok", message: "name changed sucessfully" } );
						}
					}
				);
			} else {
				error(400, "empty name");
			}
		} else {
			tag = json['add'] || json['remove'];
			if(tag) {
				//tag = tag.toLowerCase();
			
				if(json.hasOwnProperty('add')) {
					// add tag to resource
					req.db.get('resources').findAndModify(
						{'id': fileID, 'source': collectionID},
						{'$addToSet': {'tags': tag}},
						{'new': true},
						function(e, resource) {
							if(e) {
								error(404, "can't add tag (" + tag + ") to " + description, e);
							}
							else {
								// add tag to general tags list
								addTag(tag, collectionID, fileID, req.db, function(e, ignored) {
									if(e) {
										error(500, "can't add tag (" + tag + ") to " + description, e);
									}
									else {
										res.render('tagslistelement', {image: resource}); //#######
									}
								});
							}
						}
					);
				}
				else {
					// remove tag from resource
					req.db.get('resources').findAndModify(
						{'id': fileID, 'source': collectionID},
						tag=='alltags' ? {'$set' : { 'tags':[] }} : {'$pull': {'tags': tag}},
						{'new': true},
						function(e, resource) {
							if(e) {
								error(404, "can't remove tag (" + tag + ") from " + description, e);
							}
							else {
								res.render('tagslistelement', {'image': resource});
								/*
								// remove tagging [collection filename] from tags.'tag text'
								req.db.get('tags').update( //### need to wait for func completion? ### use async here?
									{'tag': tag},
									{'$pull': {'taggings': [collectionID, fileID]}},
									{'upsert': true},
									function(e, ignored) {
										if(e) {
											error(500, "can't remove tag (" + tag + ") from " + description, e);
										}
										else {
											res.render('tagslistelement', {'image': resource});
										}
									}
								);
								*/
							}
						}
					);
				}
			}
			else {
				error(400, "empty tag");
			}
		}
	}
	else {
		error(400, "unexpected json");
	}
});

// show import form
router.get('/import', function (req, res) {
    var referrer = req.query['to'] || '/',
		unknown = req.query['unknown'] === 'true' || false;
    res.render('import', { 'unknown': unknown, 'referrer': referrer } );
});

// handle actual import as post
router.post('/import', function (req, res) {
    var json = req.body;
    var db = req.db;
    db.get('resources').drop();
    db.get('tags').drop();
    var resources = db.get('resources');
    var tags = db.get('tags');
    for (var entry_id in json) {

        (function (resource, id) {
            console.log('inserting resource : ' + id);
            resource.id = id;
            resources.insert(
                resource,
                function (e, ignored) {
                    if (e) {
                        console.log('Error: ' + e);
                    }
                    else {
                        //console.log(resource.collectionID + " " + resource.fileID);
                        try {
                            for (var tagIx = 0; tagIx < resource.tags.length; tagIx++) { //#### as in grant parser
                                console.log('adding tag:' + resource.tags[tagIx]);
                                addTag(resource.tags[tagIx], resource.source, resource.id, db, function (e, ignored) {
                                    if (e) {
                                        console.log("can't add tag due to :" + JSON.stringify(e, null, 4));
                                    }
                                });
                            }
                        } catch (err) {
                            console.log("can't extract tags from : " + JSON.stringify(resource));
                        }
                    }
                }
            );
        })(json[entry_id], entry_id);

    }
    res.redirect('/');
});
// export resources as JSON
router.get('/export', function (req, res) {
    var json = req.body;
    var db = req.db;
    db.get('resources').find({},function(err,items) {
        res.json(items);
    });
});
// replace global tag set
router.get('/updatetags', function (req, res) {
    var referrer = req.query['to'] || '/',
		unknown = req.query['unknown'] === 'true' || false;
    res.render('updatetags', { 'unknown': unknown, 'referrer': referrer } );
});
router.post('/updatetags', function (req, res) {
    var global_tags 	= req.body; // this should be an array [tag1,tag2,tag3 ... ]
    var db 				= req.db;
	//
	// drop existing tags
	//
	console.log("dropping existing tags");
    db.get('tags').drop();
    //
    // add new
    //
    console.log("inserting new tags");
    var tags = db.get('tags');
    global_tags.forEach( function( tag ) {
    	tags.insert( { tag: tag, taggings: [] } );
    } );
    //
    // match new global tags to existing resource tags
    //
    console.log("processing existing resource tags");
    var resource_tags = {};
    var resources = db.get('resources');
    resources.find({},function( e, items ) {
		try {
			items.forEach( function( resource ) {
				console.log("processing tags for " + resource.id );
				//
				// find tags in global tag list
				//
				resource_tags[resource.id] = [];
				resource.tags.forEach( function( tag ) {
					if ( global_tags.indexOf( tag ) != -1 ) {
						//
						// store valid tag
						//
						resource_tags[resource.id].push( tag );
						//
						// add tagging entry for this resource
						//
						console.log("adding tag " + tag + " for " + resource.id );
						addTag(tag, resource.source, resource.id, db, function (e, ignored) {
							if (e) {
								console.log("can't add tag due to :" + JSON.stringify(e, null, 4));
							}
						});
					}
				} );
			} );
			//
			// reset resource tags
			//
			console.log("updating existing resource tags");
			for ( var id in resource_tags ) {
				console.log("updating existing resource " + id + " with tags [" + resource_tags[ id ].toString() + "]" );
				resources.update( { id: id }, { $set: { tags: resource_tags[ id ] } } );
			}
		} catch( err ) {
			console.log("error processing existing resource tags : '" + err + "'");
		}
    });
    res.redirect('/');
});

module.exports = router;
