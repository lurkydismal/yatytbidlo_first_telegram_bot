function sendGETRequest( _callback ) {
    let l_xmlHttp = new XMLHttpRequest();

    l_xmlHttp.onreadystatechange = function() {
        if ( ( l_xmlHttp.readyState == XMLHttpRequest.DONE ) &&
             ( l_xmlHttp.status == 200 ) ) {
            _callback( l_xmlHttp.responseText );
        }
    };

    l_xmlHttp.open( "GET", _path, true );
    l_xmlHttp.send( null );
}

function receiveRoutine( _responce ) {
    print( _responce );
}
