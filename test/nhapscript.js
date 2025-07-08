
  var ws;
  window.onload = function() {
    ws = new WebSocket('ws://' + location.host + '/ws');
    ws.onmessage = onMessage;
    ws.onopen = onOpen;
    ws.onclose = onClose;
    ws.onerror = onError;
  };
  function onMessage(event) {
    var resp = document.getElementById('server_response');
    if (resp.value.length > 0) resp.value += '\n';
    resp.value += event.data;
    resp.scrollTop = resp.scrollHeight;
  }
  function onOpen() {
    console.log('WebSocket connection opened');
  }
  function onClose() {
    console.log('WebSocket connection closed');
  }
  function onError(error) {
    console.error('WebSocket error:', error);
  }
  
  function sendCommand() {
    var cmd = document.getElementById('client_cmd').value;
    if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(cmd);
    } else {
    alert('WebSocket is not connected');
    }
  }
  function sendLogin() {
    var username = document.getElementById('username').value;
    var password = document.getElementById('password').value;
    var msg = JSON.stringify({username: username, password: password});
    document.getElementById('client_cmd').value = msg;
    ws.send(msg);
    return flas; // Không submit form truyền thống
  }
