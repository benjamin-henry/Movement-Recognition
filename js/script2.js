
let bt_btn = document.getElementById("bt_btn");
let bt_btn_icon = document.getElementById("bt_icon");
let bt_com_enabled = false;

let lock_device_btn = document.getElementById("lock_device_btn");
let lock_device_btn_icon = document.getElementById("lock_device_btn_icon");

let btn_send_serial = document.getElementById("btn_send_serial");
let console_input = document.getElementById("console_input");
let console_text_area = document.getElementById("console");

let local_name_input = document.getElementById("local_name_input");
let uuid_input = document.getElementById("uuid_input");
let serial_number_input = document.getElementById("serial_number_input");

let device_password = "RFYVtLE2pbWe83TI";
let device_locked = true;


let recording_axl_enabled = false;
let active_label = "";
let recorded_axl_data =  [];

let tmp_axl_data_buff = [];

let selected_label={};

/*//////////////////////////////////////Bluetooth MANAGEMENT*/
let myDevice = null;

const imuService = '917649a0-d98e-11e5-9eec-0002a5d5c51b';
let characteristicCache = null;
uuid_input.value = imuService;

function log_to_console(value, color) {
  console_text_area.innerHTML += '<div class="console_span" style="text-shadow: 0 0 1px '+color+';color:'+color+'">'+value+('\n')+'</div>'+'<br>';
  console_text_area.scrollTop = console_text_area.scrollHeight;
}

function send_n_log(value, color) {
  log_to_console(">> "+value, color);
  send(value);
}

function unlock_device() {
  send_n_log("ulk:"+device_password+":", "rgb(0, 225, 255);");
}

function lock_device() {
  send_n_log("lck:", "rgb(0, 225, 255);")
}


if ("bluetooth" in navigator) {
    bt_btn.onclick = () => {
        if(!bt_com_enabled) {
            bt_com_enabled=true;
            bt_btn.className = "com-icon-btn bt_btn_active";
            document.getElementById('bt_icon').innerHTML = "&#xe1aa;";
            bt_connect();
                
        } else {
            bt_com_enabled=false;
            bt_btn.className = "com-icon-btn bt_btn";
            document.getElementById('bt_icon').innerHTML = "&#xe1a7;";
            bt_disconnect();
        }
    };
    console_input.onkeypress = () => {
      let key = window.event.keyCode;
      if(key == 13) {
        send_n_log(console_input.value, "rgb(0, 225, 255);");
        console_input.value = "";
      }
    }
    btn_send_serial.onclick = () => {
      send_n_log(console_input.value, "rgb(0, 225, 255);");
      console_input.value = "";
    }
    lock_device_btn.onclick = () => {
      if(device_locked===true) {
        unlock_device();
      }
      else{
        lock_device();
      }
 
    }  
    document.getElementById("device_lock_status").onclick = () => {
      if(device_locked===true) {
        unlock_device();
      }
      else{
        lock_device();
      }
    }  
  } else {
    status("browser not supported"); bigButton.style.backgroundColor = "red";
    alert("Error: This browser doesn't support Web Bluetooth. Try using Chrome.");
}

function bt_connect() {
    navigator.bluetooth.requestDevice({filters: [{ services: [imuService] }]
    }).then(function (device) {
      myDevice = device;
      status('Connecting to GATT server...'); return device.gatt.connect();
    }).then(function (server) {
      status('Getting service...'); return server.getPrimaryService(imuService);
    }).then(function (service) {
      status('Getting characteristics...'); return service.getCharacteristics();
    }).then(function (characteristics) {
      status('Subscribing...');
      characteristicCache = characteristics;
      characteristicCache[0].addEventListener('characteristicvaluechanged', handleData);
      characteristicCache[0].startNotifications();
      characteristicCache[2].addEventListener('characteristicvaluechanged', handleAxlStream);
      characteristicCache[2].startNotifications();
      document.getElementById('bt_icon').innerHTML = "&#xe1a8;";
      local_name_input.value=myDevice.name;
      status("Connected to "+myDevice["name"]);
      log_to_console("[SUCCESS] Connected to "+myDevice.name, "rgba(10,255,10,1);");
      send_n_log("sn:get:", "rgba(0, 225, 255,1);");   
    })
    .catch(function (error) {
      console.error('Connection failed!', error);
      bt_com_enabled=false;
      bt_btn.className = "com-icon-btn bt_btn";
      document.getElementById('bt_icon').innerHTML = "&#xe1a7;";
      status("Connection failed. Restart device and app.");
    });
}

function bt_disconnect() {
    if (myDevice) {
        status("Disconnecting...");
        myDevice.gatt.disconnect();
        local_name_input.value="";
        bt_com_enabled=false;
        bt_btn.className = "com-icon-btn bt_btn";
        document.getElementById('bt_icon').innerHTML = "&#xe1a7;";
        status("Disconnected");
        log_to_console("[SUCCESS] Disconnected from "+myDevice.name, "rgb(10,255,10);");
    }
}

// send serial data as string
function send(data) {
  if(local_name_input.value !== ""){
    data = String(data);
    if (!data || !characteristicCache) {
      return;
    }
    try {
      characteristicCache[1].writeValue(str2ab(data));
    } catch (error) {
      log_to_console(error, "rgb(255,10,10);");
    }
    
  }
  else {
    log_to_console("No connected device", "rgb(207, 24, 24);");
  }
}

/*//////////////////////////////////////Bluetooth MANAGEMENT END*/

function status(s) {
  let conn_status = document.getElementById("bt_connection_status");
  conn_status.innerHTML = s;
}

function handleData(event) {
  try {
    let tmp = ab2str(event.target.value.buffer);
    log_to_console("<< "+tmp, "#FF7007;")
    parse_data(tmp);
  } catch (error) {
    log_to_console(error);
  }
}

function ab2str(buf) {
  return String.fromCharCode.apply(null, new Uint8Array(buf));
}

function str2ab(str) {
  var buf = new ArrayBuffer(str.length*2); // 2 bytes for each char
  var bufView = new Uint8Array(buf);
  for (var i=0, strLen=str.length; i < strLen; i++) {
    bufView[i] = str.charCodeAt(i);
  }
  return buf;
}

function parse_data(d) {
  let _data = d.split(':');
  if(_data[0] === "sn" && _data[1] === "get") {
    serial_number_input.value = _data[2];
  }
  else if(_data[0] === "ulk") {
    device_locked=false;
    lock_device_btn_icon.innerHTML="&#xe898;";
    document.getElementById("device_lock_status").innerHTML="Device unlocked";
    document.getElementById("device_lock_status").style.color = "rgb(0,255,0)";
    document.getElementById("device_lock_status").style.textShadow = "0 0 5px rgba(50, 255, 50,1)";
    lock_device_btn_icon.style.color="rgb(0,255,0)";
    lock_device_btn_icon.style.textShadow="0 0 5px rgba(50, 255, 50,1)";
    log_to_console(serial_number_input.value+" UNLOCKED", "rgb(0,255,0);");
  }
  else if(_data[0] === "lck") {
    device_locked=true;
    lock_device_btn_icon.innerHTML="&#xe899;";
    document.getElementById("device_lock_status").style.color = "rgb(207, 24, 24)";
    document.getElementById("device_lock_status").style.textShadow = "0 0 5px rgba(207, 24, 24,1)";
    document.getElementById("device_lock_status").innerHTML="Device locked";
    lock_device_btn_icon.style.color="rgb(207, 24, 24)";
    lock_device_btn_icon.style.textShadow="0 0 5px rgba(207, 24, 24,1)";
    log_to_console(serial_number_input.value+" LOCKED", "rgb(0,255,0);");
  }
  else if(_data[0] === "axl") {
    if (_data[1] === "pos") {
      document.getElementById("position_btn-"+_data[2]).style.background = "radial-gradient(#45FF2A, #238014) ";
      document.getElementById("position_btn-"+_data[2]).border="solid 2px rgba(0,200,0,.5)"
      let measuredValue = _data[3].replace('\n','').split('|');
      for(let i in measuredValue) {
        document.getElementById("axl_measured-"+_data[2]).children[i].innerHTML = measuredValue[i];
      }
    }
    else if (_data[1]="params") {
      let parameters = _data[2].slice(0,-1).split('\n');
      console.log(parameters);
      for(let i in parameters){
        parameters[i] = parameters[i].split('|');
        let elem = document.getElementById("axl_calib_params-"+i);
        for(let j in parameters[i]){
          elem.children[j].innerHTML = parameters[i][j];
        }
      }
    }
  }
}

function position_btn_clicked(elem) {
  send_n_log("calib:axl:pos:"+elem.id.split('-')[1]+":", "rgb(0, 225, 255);");
}

function axl_calib_btn_clicked() {
  send_n_log("calib:axl:go:", "rgb(0, 225, 255);");
}

function handleAxlStream(event) {
  let axlStreamValues = [
    event.target.value.getFloat32(0,true),
    event.target.value.getFloat32(4,true),
    event.target.value.getFloat32(8,true)
  ];
  for(let i in axlStreamValues) {
    document.getElementById("stream_axl-"+i).innerHTML = axlStreamValues[i].toFixed(3);
    document.getElementById("record-"+i).innerHTML = axlStreamValues[i].toFixed(3);
  }
  if (on_recording) {
    axlStreamValues.push(selected_label["name"])
    recorded_axl_data.push(axlStreamValues);
  }
}

let editing_sn = false;
let btn_calibration = document.getElementById("calibration_btn")
let btn_labelizer = document.getElementById("labelizer_btn");
let btn_help = document.getElementById("help_btn");

btn_calibration.onclick = () => {
  document.getElementById("calibration_panel").style.display = "block";
  document.getElementById("labelizer_panel").style.display = "none";
  btn_calibration.classList.add("nav_btn_selected");
  btn_labelizer.classList.remove("nav_btn_selected");
}

btn_labelizer.onclick = () => {
  document.getElementById("calibration_panel").style.display = "none";
  document.getElementById("labelizer_panel").style.display = "block";
  btn_calibration.classList.remove("nav_btn_selected");
  btn_labelizer.classList.add("nav_btn_selected");
}


//////////////////////////////////////// LABELIZER
let buff_size = 128;
let axl_x_data_buff = [];
let axl_y_data_buff = [];
let axl_z_data_buff = [];

let labels = {};
let labels_counter = 0;

let btn_add_label = document.getElementById("btn-add_label");

let labelizer_mode = "edit";
let btn_lab_edit_mode=document.getElementById("btn-lab_edit_mode");
let btn_lab_check_mode=document.getElementById("btn-lab_check_mode");
let btn_lab_delete_mode=document.getElementById("btn-lab_delete_mode");

function set_labelizer_mode(mode) {
  labelizer_mode=mode;
}

btn_lab_edit_mode.onclick=()=>{
  set_labelizer_mode("edit");
  deselect_all_labels();
  if(document.getElementById("container-btn-add_label").classList.contains("no-pointer-event"))
  document.getElementById("container-btn-add_label").classList.remove("no-pointer-event");
  for(let i in labels){
    document.getElementById(i).children[0].readOnly = false;
    document.getElementById(i).children[1].readOnly = false;
  }
}
btn_lab_check_mode.onclick=()=>{
  set_labelizer_mode("check");
  set_selected_label(selected_label["id"]);
  check_selected_label();
  if(!document.getElementById("container-btn-add_label").classList.contains("no-pointer-event"))
    document.getElementById("container-btn-add_label").classList.add("no-pointer-event");
  for(let i in labels){
    document.getElementById(i).children[0].readOnly = true;
    document.getElementById(i).children[1].readOnly = true;
  }
}
btn_lab_delete_mode.onclick=()=>{
  set_labelizer_mode("delete");
  deselect_all_labels();
  if(document.getElementById("container-btn-add_label").classList.contains("no-pointer-event"))
  document.getElementById("container-btn-add_label").classList.remove("no-pointer-event");
}

function add_label(name, hotkey){
  let _container = document.createElement('div'); //new row
  _container.style.flexDirection = "column";
  _container.style.justifyContent="center";
  _container.style.alignItems = "center";
  _container.style.flex = "1 0 21%;";
  _container.style.height = "100px";
  _container.style.maxWidth = "23%";  
  _container.style.marginTop = "1%";
  _container.style.border="dotted 2px rgb(50,50,50)";
  _container.style.borderRadius = "10px";
  _container.style.transition=".3s";
  _container.style.boxShadow="0 0 4px 0px";
  _container.id = "label_cell-"+labels_counter;
  _container.style.zIndex="1";
  
  let _label_name = document.createElement('input');
  _label_name.style.flex = "1";
  _label_name.style.outline = "none";
  _label_name.style.textAlign = "center";
  _label_name.style.borderTopLeftRadius ="10px";
  _label_name.style.borderTopRightRadius ="10px";
  _label_name.style.paddingLeft = "10px";
  _label_name.style.border = "none";
  _label_name.style.boxSizing="border-box";
  _label_name.style.width="100%";
  _label_name.style.height="50%";
  _label_name.style.background = "black";
  _label_name.style.color = "white";
  _label_name.style.fontSize = "1.3em";
  _label_name.type = "text";
  name!="" ? _label_name.value = name : _label_name.value = "label-"+labels_counter;
  _label_name.style.zIndex="0";

  let _hk_input = document.createElement('input');
  _hk_input.style.display = "flex";
  _hk_input.style.outline = "none";
  _hk_input.style.borderBottomRightRadius = "10px";
  _hk_input.style.borderBottomLeftRadius = "10px";
  _hk_input.style.width = "100%"
  _hk_input.style.backgroundColor = "rgba(0,0,0,.5)"
  _hk_input.style.border = "none";
  _hk_input.style.height = "50%";
  _hk_input.style.textAlign = "center";
  _hk_input.style.color = "rgb(231, 57, 44)";
  _hk_input.style.fontSize = "1.75em";
  _hk_input.style.boxSizing = "border-box";
  _hk_input.style.zIndex = "0";
  _hk_input.value = hotkey;

  _container.appendChild(_label_name);
  _container.appendChild(_hk_input);

  _label_name.onfocus = ()=>{
    switch (labelizer_mode) {
      case "edit":
        _container.classList = []
        if(!(_container.classList.contains("editing"))) _container.classList.add("editing");
        break;

      case "check":
        
        // _container.classList.add("checking");
        // set_selected_label(_container.id);
        break;

      case "delete":
        break;

      default:
        break;
    }
  }

  _label_name.onblur = ()=>{
    switch (labelizer_mode) {
      case "edit":
        if(_container.classList.contains("editing")) _container.classList.remove("editing");
        break;

      case "check":   
        _container.style.zIndex="0";     
        break;

      case "delete":
        _container.classList = []
        break;

      default:
        break;
    }
  }

  _hk_input.onfocus = ()=>{
    switch (labelizer_mode) {
      case "edit":
        _container.classList = []
        if(!(_container.classList.contains("editing"))) _container.classList.add("editing");
        break;

      case "check":
        
        // _container.classList.add("checking");
        // set_selected_label(_container.id);
        break;

      case "delete":
        break;

      default:
        break;
    }
  }

  _hk_input.onblur = ()=>{
    _container.style.zIndex="0";
    switch (labelizer_mode) {
      case "edit":
        if(_container.classList.contains("editing")) _container.classList.remove("editing");
        break;

      case "check":        
        break;

      case "delete":
        _container.classList = []
        break;

      default:
        break;
    }
  }

  _label_name.oninput = (event) => {
    let _p_id = _label_name.parentNode.id;
    labels[_p_id]["name"] = event.target.value;
    if(selected_label["id"]===labels[_p_id]["id"]){
      selected_label["name"]=event.target.value;
      display_label();
    }
  }
  _hk_input.oninput = (event) => {
    let _p_id = _label_name.parentNode.id;
    labels[_p_id]["hotkey"] = event.target.value;
    if(selected_label["id"]===labels[_p_id]["id"]){
      selected_label["hotkey"]=event.target.value;
    }
  }

  labels["label_cell-"+labels_counter] = {"id":"label_cell-"+labels_counter,"name":"label-"+labels_counter, "hotkey":""};
  
  btn_add_label.parentNode.parentNode.insertBefore(_container,btn_add_label.parentNode);
  btn_add_label.parentNode.parentNode.scrollTop = btn_add_label.parentNode.parentNode.scrollHeight;
  // set_selected_label("label_cell-"+labels_counter)
  labels_counter++;

  _container.onclick = () => {
    // set_selected_label(_container.id)
    switch (labelizer_mode) {
      case "edit":
        _container.style.zIndex="0";
        // _container.classList = []
        // _container.classList.add("editing");
        break;

      case "check":
        // _container.classList = []
        set_selected_label(_container.id)
        
        break;

      case "delete":
        if(_container.parentNode.children.length > 2){
          delete labels[_container.id]
          if(_container.id==selected_label["id"]){
            set_selected_label(Object.keys(labels)[0]);
            
          }  
          _container.parentNode.removeChild(_container);
        }
        break;

      default:
        break;
    }
  }

  _container.onmouseenter = () => {
    _container.style.zIndex="10";
    _container.classList.add("hovered")
    switch (labelizer_mode) {
      case "edit":
        _container.style.zIndex="5";
        _container.style.filter= "drop-shadow(0px 5px 15px rgb(0, 225, 255))";
        break;

      case "check":
          if(!_container.classList.contains("checking"))
          _container.style.filter= "drop-shadow(0px 5px 15px rgb(100, 255, 100))";
          else
          _container.style.filter="";
        break;

      case "delete":
        _container.style.filter= "drop-shadow(0px 5px 15px rgb(255, 50, 50))";
        break;

      default:
        break;
    }
  }
  _container.onmouseleave=()=>{
    _container.classList.remove("hovered")
    switch (labelizer_mode) {
      case "edit":
        _container.style.zIndex="0";
        _container.style.filter="";
        break;

      case "check":
        _container.style.zIndex="0";
        _container.style.filter="";
        break;

      case "delete":
        _container.style.zIndex="0";
        _container.style.filter="";
        break;

      default:
        break;
    } 
    check_selected_label();
  }
  
}

document.getElementById("container-btn-add_label").onclick = () => {
  add_label("","");
  labelizer_mode="edit";
}

// document.getElementById("labels_editor").onkeypress = () => {
//   let key = window.event.keyCode;
//   if(key == 13) {
//     add_label();
//   }
// }
//////////////////////////////////////// LABELIZER END


function download_record(filename, record_str) {
  let elem = document.createElement('a');
  elem.setAttribute('href', 'data:text/plain;charset=utf-8,'+encodeURIComponent(record_str));
  elem.setAttribute('download', filename);
  elem.style.display = 'none';
  document.body.appendChild(elem);
  elem.click();
  document.body.removeChild(elem);
}

document.getElementById("download_btn").onclick = () => {
  let str = "";
  for(let i in recorded_axl_data) {
    recorded_axl_data[i] = recorded_axl_data[i].join('\t')
  }
  recorded_axl_data = recorded_axl_data.join('\n')
  console.log(recorded_axl_data.length)
  download_record(new Date()+'.txt', recorded_axl_data);
}

function display_label(){
  document.getElementById("record_label").innerHTML=selected_label["name"]
}

function set_selected_label(elem_id) {
  deselect_all_labels();
  selected_label = labels[elem_id];
  let tmp = document.getElementById(elem_id);
  tmp.classList.add("selected_label");
  check_selected_label();
  display_label();
}


function unselect_label(elem_id){
  let tmp = document.getElementById(elem_id);
  tmp.classList.remove("selected_label");
}

add_label("","")
set_selected_label(labels_editor.children[0].id)
display_label();

function clickHandler(event){
  check_selected_label();
}

document.addEventListener('mousedown', clickHandler)

labels_editor = document.getElementById("labels_editor");
function check_selected_label() {
  for(child in labels_editor.children){
    if(labelizer_mode=="check"){
    try {
      if (labels_editor.children[child].id.split('-')[0]=="label_cell") {
        if (labels_editor.children[child].classList.contains("selected_label")) {
          labels_editor.children[child].style.filter = "drop-shadow(0px 5px 15px rgb(30, 255, 30))"
          labels_editor.children[child].style.zIndex="10";
        }
        else if(labels_editor.children[child].classList.contains("hovered")){
          labels_editor.children[child].style.filter = "drop-shadow(0px 5px 15px rgb(120, 255, 120))"
          labels_editor.children[child].style.zIndex="10";
        }
        else {
          labels_editor.children[child].style.filter = ""
          labels_editor.children[child].style.zIndex="0";
        }
      }
    } catch (error) {
      
    }
    }
  }
}

function deselect_all_labels() {
  for(child in labels_editor.children){
    try {
      if (labels_editor.children[child].id.split('-')[0]=="label_cell") {
        labels_editor.children[child].classList.remove("selected_label");
      }
    } catch (error) {
      
    }
  }
}

setInterval(
  check_selected_label,1000/20
)

let on_recording = false;
document.getElementById("btn-launch_record").onclick = () => {
  on_recording = !on_recording;
  btn_lab_check_mode.click();
  if(on_recording==true&&labelizer_mode=="check") {
    recorded_axl_data=[]
    document.getElementById("record_btn_icon").style.animation = "record_btn_animation .5s ease infinite"
    document.getElementById("btn-launch_record").style.background="black";
    document.getElementById("btn-launch_record").style.color="white";
  }
  else {
    document.getElementById("record_btn_icon").style.animation = ".5";
    document.getElementById("btn-launch_record").style.background="white";
    document.getElementById("btn-launch_record").style.color="black";
  }  
}