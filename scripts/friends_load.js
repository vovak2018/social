
let friends_json = JSON.parse(get("/users/" + username + "/friends.json?token=" + token));
log(friends_json);
temp_string = ""
for (let i = 0; i < friends_json.length; i++) {
    temp_string += '<li><a href="#">' + friends_json[i] + '</a></li>'
}
friends_to_messages.innerHTML = temp_string
delete temp_string
