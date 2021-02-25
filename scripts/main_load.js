
let main_json = JSON.parse(get("/data/config.json"));
if (main_json.debug == "true") {
    let debug = true;
}
else {
    let debug = false;
};
log(main_json);
