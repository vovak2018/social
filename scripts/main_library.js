function log(str) {
    if (main_json.debug == "true") {
        console.log(str);
    }
}
function getFilename(url)
{
   return url.split('/').pop();
}
function get(lnk) 
{
    let req = new XMLHttpRequest();
    req.open("GET", lnk, false);
    req.send();
    if (req.status != 200) {
        return false;
    }
    return req.responseText
}
function get_url_var(key) {
    // history.replaceState({}, '', '/test?param=value&param2=42&keyonly');
    urlParams = new URLSearchParams(window.location.search);
    params = {};
    urlParams.forEach((p, key) => {
      params[key] = p;
    });
    log(params)
    return params[key]
};
function check_token() {
    
    if (token == undefined || token == "") 
    {
        window.location = "/login"
    }
    req = new XMLHttpRequest();
    req.open("GET", "/users/" + username + "/data.json?token=" + token, false);
    req.send();
    let user_config = req.responseText;
    if (false == user_config) window.location = "/login";
    if (user_config == "INVALID TOKEN" || user_config == "NOT AUTHORIZED" || req.status == 500) {
        window.location = "/login"
    }
    else {
        //window.location.search = ""
    }

}
function load_user_data() {
    user_array = JSON.parse(get("/users/" + username + "/data.json?token=" + token))
    if (user_array.user_icon != "undefined")
        document.querySelector(".user-icon").style = "background-image: url(" + user_array.user_icon + ");"
        /*
       var xhr = new XMLHttpRequest();

    xhr.open('GET', 'phones.json', true);
    xhr.send(); // (1)
    xhr.onreadystatechange = function() { // (3)
    if (xhr.readyState != 4) return;
    button.innerHTML = 'Готово!';
    if (xhr.status == 404) {
        alert(xhr.status + ': ' + xhr.statusText);
    } else {
        document.querySelector(".user-icon").style = "background-image: url(/media/img/" + user_array.nick + ".png);"
    }
    }
    */
}
function no_space(str) {
    while (str.indexOf(" ") != -1) {
        str.replace(" ", "");
    }
    return str
}
function add_friend(e) {
        e.preventDefault();
        e.target.blur();
        UIkit.modal.prompt('Введите ник друга:', '@').then(function (name) {
          let req = new XMLHttpRequest();
          if (name[0] == "@") {
            name = name.slice(1, name.length)
          }
          log(name)
          req.open("GET", `/users/${name}/avatar.png`, false);
          req.send();
          if (req.status == 404 || name == username) {
            log("not found")
            //Сообщение, что не нашли
            UIkit.modal.alert('Пользователь не был обнаружен или вы ввели свое же имя')
          }
          else {
            log("founded")
            friend_array = JSON.parse(req.responseText)
            log(friend_array)
            if (friend_array.user_icon == "undefined") {
              friend_array.user_icon = "/media/img/avatar.png"
            }
            UIkit.modal.confirm(`
            <div class="uk-card-header uk-dark uk-padding uk-panel" style="margin: 0;padding: 0;">
              <div class="uk-grid-small uk-margin-large" uk-grid>
                  <h2>Проверьте, это он?</h2>
                  <br>
                  <div class="uk-width-auto">
                      <div class="uk-border-circle user-icon" style="background-image: url(${friend_array.user_icon});"></div>
                  </div>
                  <div class="uk-width-expand">
                      <h3 class="uk-card-title uk-margin-remove-bottom uk-text-muted">${friend_array.off_name}</h3>
                      <p class="uk-text-meta uk-margin-remove-top"><time datetime="2018-04-01T19:00">@${friend_array.user_name}</time></p>
                  </div>
              </div>
              <!--div class="flex uk-margin-small">
                <button class="uk-button uk-button-default">Нет</button>
                <button class="uk-button uk-button-primary">Да, это он</button>
              </div>
            </div-->
            <div style="width:20px;height: 20px;"></div>
            `).then(function() {
              log("yes")
              //Тут нужно добавлять в друзья, проверяя, что его там нет
              let tempFriendArray = JSON.parse(get("/users/" + username + "/friends.json?token=" + token))
              log(tempFriendArray)
              if (tempFriendArray.indexOf(name) == -1) {
                tempFriendArray.push(name)
                log(tempFriendArray)
                get(`/edit?file=/users/${username}/friends.json&cont=${JSON.stringify(tempFriendArray)}`)
                
                tempFriendArray = JSON.parse(get("/users/" + name + "/friends.png"))
                tempFriendArray.push(username)
                log(tempFriendArray)
                get(`/edit?file=/users/${name}/friends.json&cont=${JSON.stringify(tempFriendArray)}`)
                location = location
                UIkit.modal.alert('Теперь этот пользователь у вас в друзьях!')
              }
              else {
                UIkit.modal.alert('Этот пользователь у вас в друзьях')
              }
            }, function() {log("no")});
          }
        });
    
}