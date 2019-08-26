package main

import (
	"os"
	"sort"
	"strconv"
	"sync"
)

//DeviceCloud main device sctruct
type DeviceCloud struct {
	status   int
	login    string
	password string
	url      string
	cserver  string
	cport    string
	cid      string
	camip    string
	camport  string
	camuri   string
}

//SyncCloud map locket
type SyncCloud struct {
	mutex sync.RWMutex
	ljson string
	m     map[string]DeviceCloud
}

//NewCloudStorage create nev locked storage
func NewCloudStorage() *SyncCloud {
	return &SyncCloud{m: make(map[string]DeviceCloud)}
}

//Add new element to strorage
func (obj *SyncCloud) Add(key string, value DeviceCloud) {
	obj.mutex.Lock()
	obj.m[key] = value
	obj.mutex.Unlock()
	obj.SaveINFOFILE()
}

//SetStatus change cloud Status
func (obj *SyncCloud) SetStatus(name string, val int) {
	obj.mutex.Lock()
	if tmp, ok := obj.m[name]; ok {
		tmp.status = val
		obj.m[name] = tmp
	}
	obj.mutex.Unlock()
	obj.SaveINFOFILE()
}

//SetINFO Set Information
func (obj *SyncCloud) SetINFO(name string, Status int, CameraLogin string, CameraPassword string, URL string, CloudServer string, CloudPort string, CloudID string) {
	obj.mutex.Lock()
	if tmp, ok := obj.m[name]; ok {
		tmp.status = Status
		tmp.login = CameraLogin
		tmp.password = CameraPassword
		tmp.url = URL
		tmp.cserver = CloudServer
		tmp.cport = CloudPort
		tmp.cid = CloudID
		obj.m[name] = tmp
	}
	obj.mutex.Unlock()
	obj.SaveINFOFILE()
}

//GetINFOJSON get json responce
func (obj *SyncCloud) GetINFOJSON() string {
	return obj.MakeJSON()
}

//MakeJSON get json responce
func (obj *SyncCloud) MakeJSON() string {
	obj.mutex.RLock()
	defer obj.mutex.RUnlock()
	var json string
	json += "{\"chanels\": [\r\n"
	if len(obj.m) > 0 {
		alen := len(obj.m)
		for id, val := range SortChanel(obj.m) {
			if val == "10000" {
				continue
			}
			var lp string
			if obj.m[val].login != "" {
				lp = obj.m[val].login + ":" + obj.m[val].password + "@"
			}
			//	log.Println(alen, id)
			if alen != id+1 {
				json += "{\"chanel\":\"" + val + "\",\"real_chanel\":\"" + obj.m[val].camuri + "\",\"cloud_id\":\"" + obj.m[val].cid + "\",\"camera_login\":\"" + obj.m[val].login + "\",\"camera_password\":\"" + obj.m[val].password + "\",\"cloud_server\":\"" + obj.m[val].cserver + "\",\"cloud_port\":\"" + obj.m[val].cport + "\",\"cloud_url\":\"" + "rtsp://" + lp + obj.m[val].cserver + ":" + obj.m[val].cport + "/" + obj.m[val].cid + "\",\"status\":\"" + strconv.FormatInt(int64(obj.m[val].status), 10) + "\"},\r\n"
			} else {
				json += "{\"chanel\":\"" + val + "\",\"real_chanel\":\"" + obj.m[val].camuri + "\",\"cloud_id\":\"" + obj.m[val].cid + "\",\"camera_login\":\"" + obj.m[val].login + "\",\"camera_password\":\"" + obj.m[val].password + "\",\"cloud_server\":\"" + obj.m[val].cserver + "\",\"cloud_port\":\"" + obj.m[val].cport + "\",\"cloud_url\":\"" + "rtsp://" + lp + obj.m[val].cserver + ":" + obj.m[val].cport + "/" + obj.m[val].cid + "\",\"status\":\"" + strconv.FormatInt(int64(obj.m[val].status), 10) + "\"}\r\n"

			}
		}
	}
	json += "]}"
	return json
}

//MakeJSONSafe get json responce
func (obj *SyncCloud) MakeJSONSafe() string {
	obj.mutex.RLock()
	defer obj.mutex.RUnlock()
	var json string
	json += "{\"chanels\": [\r\n"
	if len(obj.m) > 0 {
		alen := len(obj.m)
		for id, val := range SortChanel(obj.m) {
			if val == "10000" {
				continue
			}
			var lp string
			if obj.m[val].login != "" {
				lp = obj.m[val].login + ":" + obj.m[val].password + "@"
			}
			//	log.Println(alen, id)
			if alen != id+1 {
				json += "{\"chanel\":\"" + val + "\",\"real_chanel\":\"" + obj.m[val].camuri + "\",\"cloud_id\":\"" + obj.m[val].cid + "\",\"cloud_server\":\"" + obj.m[val].cserver + "\",\"cloud_port\":\"" + obj.m[val].cport + "\",\"cloud_url\":\"" + "rtsp://" + lp + obj.m[val].cserver + ":" + obj.m[val].cport + "/" + obj.m[val].cid + "\",\"status\":\"" + strconv.FormatInt(int64(obj.m[val].status), 10) + "\"},\r\n"
			} else {
				json += "{\"chanel\":\"" + val + "\",\"real_chanel\":\"" + obj.m[val].camuri + "\",\"cloud_id\":\"" + obj.m[val].cid + "\",\"cloud_server\":\"" + obj.m[val].cserver + "\",\"cloud_port\":\"" + obj.m[val].cport + "\",\"cloud_url\":\"" + "rtsp://" + lp + obj.m[val].cserver + ":" + obj.m[val].cport + "/" + obj.m[val].cid + "\",\"status\":\"" + strconv.FormatInt(int64(obj.m[val].status), 10) + "\"}\r\n"

			}
		}
	}
	json += "]}"
	return json
}

//GetINFOHTML return html code status
func (obj *SyncCloud) GetINFOHTML() string {
	obj.mutex.RLock()
	defer obj.mutex.RUnlock()
	ret := `<html lang="en">
  <head>
    <link href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u" crossorigin="anonymous">
    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js" integrity="sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa" crossorigin="anonymous"></script>
    <script src="https://cdn.rawgit.com/google/code-prettify/master/loader/run_prettify.js?skin=sunburst"></script>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="description" content="">
    <meta name="author" content="">
    <title>Cloud Client</title><table  class="table table-striped" width="50%">`
	ret += "<tr><th>ID</th><th>Local URL</th><th>Cloud ID</th><th>Cloud Server</th><th>Cloud Port</th><th>Cloud URL</th><th>Status</th>"
	for _, key := range SortChanel(obj.m) {
		if key == "10000" {
			continue
		}
		var lp string
		if obj.m[key].login != "" {
			lp = obj.m[key].login + ":" + obj.m[key].password + "@"
		}
		var status string
		if obj.m[key].status == 1 {
			status = "<td width=\"20%\" bgcolor=\"#00FFFF\" align=center>Wait</td>"
		} else if obj.m[key].status == 2 {
			status = "<td bgcolor=\"#00FA9A\" align=center>Online</td>"
		} else {
			status = "<td bgcolor=\"#FF4500\" align=center>Offline</td>"
		}
		ret += "<tr><td>" + key + "</td><td>rtsp://" + obj.m[key].camip + ":" + obj.m[key].camport + obj.m[key].camuri + "</td><td>" + obj.m[key].cid + "</td><td>" + obj.m[key].cserver + "</td><td>" + obj.m[key].cport + "</td><td>" + "rtsp://" + lp + obj.m[key].cserver + ":" + obj.m[key].cport + "/" + obj.m[key].cid + "</td>" + status
	}
	ret += "</table>"
	return ret
}

//GetINFOHTMLADD return html code status
func (obj *SyncCloud) GetINFOHTMLADD() string {
	json := obj.MakeJSON()
	sjson := obj.MakeJSONSafe()
	obj.mutex.RLock()
	defer obj.mutex.RUnlock()
	var ret string
	var nowait bool
	for _, key := range SortChanel(obj.m) {
		if key == "10000" {
			continue
		}
		if obj.m[key].status != 1 || obj.m[key].status != 2 {
			nowait = true
		}
	}
	if *HTTPCameraMode == "1" {
		ret += `<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="description" content="">
    <meta name="author" content="">
    <title>Облачная IP Camera ` + *HTTPLogoText + ` </title>
		<script src="https://code.jquery.com/jquery-1.12.4.min.js" integrity="sha256-ZosEbRLbNQzLpnKIkEdrPv7lOy9C27hHQ+Xp8a4MxAQ=" crossorigin="anonymous"></script>
    <link href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u" crossorigin="anonymous">
  </head>
 <style>
body {
  padding-top: 20px;
  padding-bottom: 20px;
}
.header,
.marketing,
.footer {
  padding-right: 15px;
  padding-left: 15px;
}
.header {
  padding-bottom: 20px;
  border-bottom: 1px solid #e5e5e5;
}
.header h3 {
  margin-top: 0;
  margin-bottom: 0;
  line-height: 40px;
}
.footer {
  padding-top: 19px;
  color: #777;
  border-top: 1px solid #e5e5e5;
}
@media (min-width: 768px) {
  .container {
    max-width: 730px;
  }
}
.container-narrow > hr {
  margin: 30px 0;
}
.jumbotron {
  border-bottom: 1px solid #e5e5e5;
}
.jumbotron .btn {
  padding: 14px 24px;
  font-size: 21px;
}
.marketing {
  margin: 40px 0;
}
.marketing p + h4 {
  margin-top: 28px;
}
@media screen and (min-width: 768px) {
  .header,
  .marketing,
  .footer {
    padding-right: 0;
    padding-left: 0;
  }
  .header {
    margin-bottom: 30px;
  }
  .jumbotron {
    border-bottom: 0;
  }
}
</style>
<script>
function add_cloud() {
			$("#result").show();
	    $("#result").empty();
			$('#addbutton').attr("disabled", 'true');
			var jdata = ` + sjson + `
			var dataToSend = {
					'action':'addcamera',
					'login':$("#login").val(),
					'password':$("#password").val(),
					'login_camera':$("#clogin").val(),
					'password_camera':$("#cpassword").val(),
					'data': JSON.stringify(jdata)
			};
			$.ajax({
				type: 'POST',
				url: '` + *HTTPAddURL + `',
				data: dataToSend,
				success: function(data) {
					rjdata = JSON.parse(data)
					if (rjdata.status == 1) {
						$("#result").html("<font color='#00cc66'><b>"+rjdata.message+"</b></font>").hide(10000);
					}else{
						$("#result").html("<font color='#ff0066'><b>Ошибка Добавления</b></font>:" + rjdata.message).hide(5000);
					}
				},
				error: function(data) {
					$("#result").html("<font color='ff0066'><b>Ошибка Добавления</b></font>:" + data).hide(5000);;
				}
		});
		$("#addbutton").attr("disabled", false);
}
</script>
  <body>
    <div class="container">
      <div class="header clearfix">
        <h3 class="text-muted">Cloud Camera ` + *HTTPLogoText + `</h3>
      </div>
      <div class="jumbotron">`

		if !nowait {
			ret += `<script type="text/javascript">
									setTimeout(function(){
										location = ''
									},5000)
								</script>
								 <h2>Попытка переподключения к облаку..</h2>`
		} else {
			ret += `
				 <h2>Добавлениe камеры в облако</h2>
				 <form>
				  <div class="form-group">
				    <label for="login">Логин от облака</label>
				    <input type="text" class="form-control" id="login" aria-describedby="emailHelp" placeholder="Логин от облака">
				    <small id="loginHelp" class="form-text text-muted">Для регистрации в облаке перейдите по ссылке <a href="` + *HTTPRegSite + `">` + *HTTPRegSite + `</a>.</small>
				  </div>
				  <div class="form-group">
				    <label for="password">Пароль от облака</label>
				    <input type="password" class="form-control" id="password" placeholder="Пароль от облака">
				  </div>
					<div class="form-group">
						<label for="clogin">Логин от камеры</label>
						<input type="text" class="form-control" id="clogin" aria-describedby="emailHelp" placeholder="Логин от камеры">
					</div>
					<div class="form-group">
						<label for="cpassword">Пароль от камеры</label>
						<input type="password" class="form-control" id="cpassword" placeholder="Пароль от камеры">
					</div>
					<p><button id="addbutton" type="button" class="btn btn-lg btn-success" onclick="add_cloud();">Добавить в Облако</button></p>
				</form>`
		}
		ret += `<div id="result">
				</div>
      </div>
      <footer class="footer">
        <p>&copy; 2016 <a href="` + *HTTPRegSite + `">` + *HTTPLogoText + `</a> Company, Inc.</p>
      </footer>
    </div> <!-- /container -->
  </body>
</html>


`
	} else {
		ret = `<html lang="en">
  <head>
    <link href="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u" crossorigin="anonymous">
		<script src="https://code.jquery.com/jquery-2.2.4.min.js" integrity="sha256-BbhdlvQf/xTY9gja0Dq3HiwQF8LaCRTXxZKRutelT44="crossorigin="anonymous"></script>
    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js" integrity="sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa" crossorigin="anonymous"></script>
    <script src="https://cdn.rawgit.com/google/code-prettify/master/loader/run_prettify.js?skin=sunburst"></script>

    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta name="description" content="">
    <meta name="author" content="">
    <title>Добавление потоков в облако</title>
		<script>
		function add(actionid){
		  $("#b"+actionid).attr("disabled", true);
			jdata = ` + json + `
			var dataToSend = {
					'action':'add',
					'login':$("#login").val(),
					'password':$("#password").val(),
			    'data': JSON.stringify(jdata.chanels[actionid-1])
			};
			$.ajax({
		    type: 'POST',
		    url: '` + *HTTPAddURL + `',
		    data: dataToSend,
		    success: function(data) {
					jdata = JSON.parse(data)
					if (jdata.status == 1) {
						$("#d"+actionid).html(jdata.message)
					}else{
						$("#b"+actionid).attr("disabled", false);
						alert("Ошибка Добавления " + jdata.message)
					}
				},
				error: function(data) {
					$("#b"+actionid).attr("disabled", false);
					alert("Ошибка Добавления " + data)
				}
			});

		}
		</script>
		<div align=center>
		<h2>Добавление потоков в облако</h2>
		<form class="form-inline">
		  <div class="form-group">
		    <label for="login">Login</label>
		    <input id=login type="text" class="form-control" id="login" placeholder="Login">
		  </div>
		  <div class="form-group">
		    <label for="password">Password</label>
		    <input id=password type="password" class="form-control" id="password" placeholder="Password">
		  </div>
		</form>
		<table  class="table table-striped" style="width: 80%">`
		ret += "<tr><th>CloudID</th><th>Поток</th><th class=\"text-center\">Статус</th><th class=\"text-center\">Облако</th></tr>"
		for _, key := range SortChanel(obj.m) {
			if key == "10000" {
				continue
			}
			var status string
			if obj.m[key].status == 1 {
				status = "<td width=\"20%\" bgcolor=\"#00FFFF\"  class=\"text-center\">Wait</td>"
			} else if obj.m[key].status == 2 {
				status = "<td bgcolor=\"#00FA9A\"  class=\"text-center\">Online</td>"
			} else {
				status = "<td bgcolor=\"#FF4500\" class=\"text-center\">Offline</td>"
			}
			ret += "<tr><td>" + obj.m[key].cid + "</td><td>rtsp://" + obj.m[key].camip + ":" + obj.m[key].camport + obj.m[key].camuri + "</td>" + status + "<td class=\"text-center\"><div id=d" + key + "><button id=b" + key + " type=\"button\" class=\"btn btn-primary\" onclick=add(" + key + ")>Добавить</button></div></td></tr>"
		}
		ret += "</table></div>"
	}
	return ret
}

//SaveINFOFILE Save Info File
func (obj *SyncCloud) SaveINFOFILE() {
	if EnableMAC != nil && *EnableMAC == "1" {
		return
	}
	json := obj.MakeJSON()
	obj.mutex.Lock()
	defer obj.mutex.Unlock()
	if json == obj.ljson {
		return
	}
	obj.ljson = json
	if f, err := os.OpenFile(*ConfigDir+"status.cloud", os.O_WRONLY|os.O_TRUNC|os.O_CREATE, 0660); err == nil {
		defer f.Close()
		f.Write([]byte(json))
	}
}

//SortChanel Sort Chanel Array
func SortChanel(data map[string]DeviceCloud) []string {
	mk := make([]string, len(data))
	i := 0
	for k := range data {
		mk[i] = k
		i++
	}
	sort.Strings(mk)
	mk2 := make([]int, len(mk))
	for k2, v := range mk {
		mk2[k2], _ = strconv.Atoi(v)
	}
	sort.Ints(mk2)
	mk3 := make([]string, len(mk2))
	for k3, v3 := range mk2 {
		mk3[k3] = strconv.Itoa(v3)
	}
	return mk3
}

//SortChanel Sort Chanel Array
func SortChanel222(data map[string]DeviceCloud) []string {
	mk := make([]int, len(data))
	//i := 0
	for k := range data {
		v, _ := strconv.Atoi(k)
		//	log.Println(i, "=", v)
		mk[v] = v
		//i++
	}
	sort.Ints(mk)

	mk2 := make([]string, len(mk))
	i2 := 0
	for k2 := range mk {
		mk2[k2] = strconv.Itoa(k2)
		i2++
	}
	//log.Println(mk2)
	return mk2
}
