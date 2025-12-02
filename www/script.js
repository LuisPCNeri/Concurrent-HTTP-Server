const BUTTON = document.getElementById("btn");

var obj;

function get_request(body_element){
    fetch("http://localhost:8080/index.html")
    .then(response => response)
    .then(data => {
        console.log(data);
        // Makes server's response body appear in html page
        data.text().then( text => {
            console.log(text);
            //document.getElementById("text").innerHTML = text;
            body_element.innerHTML = text;
        });
    })
    .catch(error => console.log(error));
}

BUTTON.onclick = () => {
    get_request(document.getElementById("text"));
};