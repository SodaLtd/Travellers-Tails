$(document).ready(function () {
	var post = function(url, dataOut, successFunction) {
			$.ajax({
				type: 'POST',
				contentType: 'application/json',
				url: url,
				data: JSON.stringify(dataOut),
				dataType: 'html', //response type
				success: successFunction,
			
				error: function (xhr, status) {
					var stuff = JSON.stringify(xhr, null, 4)
					console.log(stuff);
					$('#message').text(stuff);
				}
			});
		},
	
		addTag = function(url, tag) {
			post(url, {"add": tag}, function(data, status, xhr) {
				// server supplies new tags list
				$('#listtags').html(data);
				
				// remove tag from local pick list
				$('.pick').filter(function(){return tag === $(this).text();}).each(function() {
					$(this).next('.comma').remove();
					$(this).remove();
				});
			});
		},
		
		filterPicks = function(text) {
			var new_tags = text.split(',');
			if ( new_tags.length > 0 ) {
				//
				// hide all
				//
				$('.pick').each(function() {
					var tag = $(this);
					tag.addClass('hide');
				});
				//
				// show only those in list
				//
				new_tags.forEach( function( new_tag ) {
					// filter pick tags
					$('.pick').each(function() {
						var tag = $(this);
						if(new_tag === "" || tag.text().indexOf(new_tag.trim()) === 0) {
							tag.removeClass('hide');
						}
					});
				});
			} else {
				//
				// show all
				//
				$('.pick').each(function() {
					tag.removeClass('hide');
				});			
			}
			$('.comma').each(function() {
				var comma = $(this);
				if(comma.prev().hasClass('hide')) {
					comma.addClass('hide');
				}
				else {
					comma.removeClass('hide');
				}
			});
		},
		
		setName = function(url,name) {
			post(url, {"name": name}, function(data, status, xhr) {
				$('#message').text(data);
			});
		};
		
	$('#addtags').on('keypress', function(event) {
		if(event.which == 13) {
			var text = $(this).text().trim();
			event.preventDefault();
			// add tags
			if(text) {
				var new_tags = text.split(',');
				new_tags.forEach( function( new_tag ) {
					//
					// TODO: verify tag is in global list
					//
					addTag(window.location.href, new_tag.trim());
				});
			}
			$(this).text('');
			filterPicks('');
		}
	});
	
	$('#addtags').on('input', function(event){
		filterPicks($(this).text().trim());
	});

	$('#picktags').on('click', '.pick', function(event) {
		addTag(window.location.href, $(this).text());
	});
	
	$('#listtags').on('click', '.unpick', function(event) {
		var tag = $(this).text();
		
		post(window.location.href, {"remove": tag}, function(data, status, xhr) {
			// server supplies new tags list
			$('#listtags').html(data);
			
			// add tag to local pick list			
			$('#picktags ul').append(
				$('<span/>', {'class': 'comma', 'text': ', '}),
				$('<li/>', {'class': 'pick removed', 'text': tag}));
			
		});
	});

	$('#removealltags').on('click', function(event) {
		var tags = $('#listtags ul').children( '.unpick' );
		var removed = [];
		for ( var i = 0; i < tags.length; i++ ) {
			removed.push( $(tags[ i ]).text() );
		}
		post(window.location.href, {"remove": 'alltags'}, function(data, status, xhr) {
			// server supplies new tags list
			$('#listtags').html(data);
			
			// add tags to local pick list	
			for ( var i = 0; i < removed.length; i++ ) {		
				$('#picktags ul').append(
					$('<span/>', {'class': 'comma', 'text': ', '}),
					$('<li/>', {'class': 'pick removed', 'text': removed[ i ]}));
			}
			
		});
	});

	$("#name").keypress(function (e) {
		 if (e.which == 13) {
			e.preventDefault();
			setName(window.location.href, $(this).text());
		 } 
	});
	$('#import').on('change', function (event) {
	    var types = ['text.*'];
	    sda.filereader.readfile(this, types, {
	        onload: function (evt) {
	            var json = JSON.parse(evt.target.result);
	            if (json) {
	                post('/import', json, function () {
	                    alert('data imported');
	                });
	            } else {
	                alert('unable to parse');
	            }
	        },
	        onerror: function (error) {
	            alert(JSON.stringify(error));
	        }
	    });
	});
	$('#globaltags').on('keypress', function (event) {
		if ( event.which == 13 ) {
			var global_tags = $( this ).val().split(',');
			if ( global_tags.length > 0 ) {
				post('/updatetags', global_tags, function() {
					alert( 'tags updated' );
				} );
			}
		}
	});
	
});
