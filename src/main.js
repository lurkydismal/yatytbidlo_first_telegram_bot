function GETRequestCallback( _response ) {
    print( `RESPONSE: ${_response}\n` );
}

function onReceiveRoutine( _response ) {
    const obj = JSON.parse( _response );

    print( "OK" );

    return new Array( "/bot" +
                          "token" +
                          "/getMe",
                      GETRequestCallback.name );
}
