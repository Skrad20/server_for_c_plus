// JavaScript source code
socket = new WebSocket("ws://localhost:9001/"); socket.onmessage = ({ data }) => console.log("From server: ", data);
socket.send("dgsgs")

socket.send('{"command": "public_msg", "text": "Hello, people!"}');

socket.send('{"command": "private_msg", "text": "Hello, people!", "user_to":13}');

socket.send(JSON.stringify(
    {
        "command": "private_msg",
        "user_to": 12,
        "text": "Hello, 12"
    }
));


socket.send('{"command": "set_name", "name": "Jon"}');