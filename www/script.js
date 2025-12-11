// fetching the fs module to handle file retreival
const fs = require('fs');

const BUTTON = document.getElementById("btn");

var obj;

function get_request(body_element){
    fetch("http://localhost:8080/index.html")
    .then(response => response)
    .then(data => {
        console.log(data);
        // Makes server's response body appear in html page
        data.text().then( text => {
            fetch('statFile.txt')
            .then(res => res.text())
            .then(data => {
                const dataArray = data.split(",");

                const output =
                    "Active connections: " + dataArray[0] + "<br>" +
                    "Total bytes transferred: " + dataArray[1] + " B<br>" +
                    "Status 200 count: " + dataArray[2] + "<br>" +
                    "Status 404 count: " + dataArray[3] + "<br>" +
                    "Status 500 count: " + dataArray[4] + "<br>" +
                    "Total requests count: " + dataArray[5];

                document.getElementById("stats").innerHTML = output;
            }).catch(err => console.error(err));
        });
    })
    .catch(error => console.log(error));
}

function statsFileReading() {
    fs.readFile('statFile.txt', 'utf8', (err, data) => {
    if (err) throw err;
    
    const dataArray = data.split(",");
    
    return "Active connections: " + dataArray[0] + "\nTotal bytes transferred: " + dataArray[1] 
            + " B\nStatus 200 count: " + dataArray[2] + "\nStatus 404 count: " + dataArray[3] 
            + "\nStatus 500 count: " + dataArray[4] + "\nTotal requests count: " + dataArray[5];
});
}

BUTTON.onclick = () => {
    get_request(document.getElementById("stats"));
};