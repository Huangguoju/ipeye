/*
THIS PRIVATE SOFTWARE IPEYE COMPANY
Site:  		http://ipeye.ru
Director:	Michail Chernaev
Autor: 		Andrey Semochkin
Email: 		admin@wetel.ru
*/
package main

import (
	"flag"
	"io"
	"io/ioutil"
	"net"
	"net/url"
	"os"
	"strconv"
	"strings"
	"time"
)

//Storage - make new storage obj
var Storage = NewCloudStorage()

//ConfigDir - Full Patch cloud.status file
var ConfigDir = flag.String("config_dir", "", "Full Patch dir example /mnt/flash/productinfo/ need end /")

//EnableAPI - Enable Api Registration
var EnableAPI = flag.String("enable_api", "1", "EnableAPI - Enable Api Registration")

//EnableHTTP - Enable HTTP Server
var EnableHTTP = flag.String("enable_http", "1", "Enable HTTP Server")

//HTTPPort - HTTP Server Port
var HTTPPort = flag.String("http_port", "8282", "HTTP Server Port")

//HTTPAddURL - Custom ADD Server IP Address
var HTTPAddURL = flag.String("http_add_url", "http://ipeye.ru/addcamera.php", "Custom ADD Process Server IP Address")

//HTTPDisableAdd - HTTP Disable Add Page
var HTTPDisableAdd = flag.String("http_disable_add", "0", "HTTP Disable Add Page")

//HTTPCameraMode - HTTP Disable Add Page
var HTTPCameraMode = flag.String("http_camera_mode", "0", "HTTP Camera mode mrage stream1+stream2 to second")

//HTTPLogoText - HTTP Disable Add Page
var HTTPLogoText = flag.String("http_logo_text", "IPEYE", "HTTP Form Logo Text")

//HTTPRegSite - HTTP Disable Add Page
var HTTPRegSite = flag.String("http_reg_site", "https://ipeye.ru", "site to reg new client")

//APIServer - Custom API Server IP Address
var APIServer = flag.String("api_server", "171.25.232.2", "Custom API Server IP Address")

//APIPort - Custom API Server Port
var APIPort = flag.String("api_port", "8111", "Custom API Server Port")

//CloudServer - Custom Cloud Server IP Address
var CloudServer = flag.String("cloud_server", "171.25.232.11", "Custom Cloud Server IP Address")

//CloudPort - Custom Cloud Server Port
var CloudPort = flag.String("cloud_port", "5511", "Custom Cloud Server Port")

//EnableDubug - Enable Debug Out
var EnableDubug = flag.String("enable_debug", "0", "Enable Debug Out")

//EnableSpeek - Enable Speek chanel
var EnableSpeek = flag.String("enable_speek", "0", "Enable audio imput chanel port 90")

//Streams - Streams list split ,
var Streams = flag.String("streams", "rtsp://admin:admin@127.0.0.1:554/mpeg4,rtsp://admin:admin@127.0.0.1:554/mpeg4cif", "Streams list split (,) if use & list split (\"1\",\"2\") ")

//EnableOnvif - Onvif Request router
//var EnableOnvif = flag.String("enable_onvif", "0", "Make Chanel to Onvif Sample http://CloudServer:5522/UUID/0 only single chanel")

//Vendor - Vendor
var Vendor = flag.String("vendor", "ipeye", "Vendor")

//Model - Model
var Model = flag.String("model", "", "Model")

//Sleep Start Wait Timer
var Sleep = flag.Duration("sleep", 0, "Start Wait timer sapmle 10s")

//EnableMAC mac mechanic
var EnableMAC = flag.String("enable_mac", "0", "Enable MAC replace cloud request UUDI to use mac need options string or file")

//MACFile file patch
var MACFile = flag.String("mac_file", "", "Full Patch dir example /mnt/flash/productinfo/mac the file must contain only the address line MM-MM-MM-SS-SS-SS or MM:MM:MM:SS:SS:SS")

//MACString string
var MACString = flag.String("mac_string", "", "enter mac string MM:MM:MM:SS:SS:SS")

//GetBallancerCloudID получает id устройства
func GetBallancerCloudID(APIServer string, APIPort string) (string, bool) {
	if conn, err := net.DialTimeout("tcp", APIServer+":"+APIPort, 3*time.Second); err == nil {
		defer conn.Close()
		data := ""
		if _, werr := conn.Write([]byte("GET /balancer/uuid HTTP/1.1\r\nHost: " + APIServer + "\r\nContent-Type: text/xml\r\nContent-Length: " + strconv.FormatInt(int64(len(data)), 10) + "\r\n\r\n" + data)); werr == nil {
			byteSlice := make([]byte, 1024)
			if r, rerr := conn.Read(byteSlice); r > 0 && rerr == nil {
				if Array := strings.Split(string(byteSlice[:r]), "\r\n\r\n"); len(Array) == 2 {
					for _, element := range strings.Split(FixString(Array[1]), ",") {
						element = strings.Trim(element, "}")
						element = strings.Trim(element, "{")
						for _, element2 := range strings.Split(FixString(element), ",") {
							element3 := strings.Split(FixString(element2), ":")
							if len(element3) == 2 {
								key := strings.Trim(element3[0], "\"")
								val := strings.Trim(element3[1], "\"")
								if key == "message" && len(val) == 36 {
									return FixString(val), true
								}
							}
						}
					}
				}
			}
		}
	}
	return "", false
}

//GetBallancerServer получает id устройства
func GetBallancerServer(APIServer string, APIPort string, CloudID string) (string, string, bool) {
	if conn, err := net.DialTimeout("tcp", APIServer+":"+APIPort, 3*time.Second); err == nil {
		defer conn.Close()
		data := ""
		if _, werr := conn.Write([]byte("GET /balancer/server/" + CloudID + "?vendor=" + *Vendor + "&model=" + *Model + " HTTP/1.1\r\nHost: " + APIServer + "\r\nContent-Type: text/xml\r\nContent-Length: " + strconv.FormatInt(int64(len(data)), 10) + "\r\n\r\n" + data)); werr == nil {
			byteSlice := make([]byte, 2048)
			if r, rerr := conn.Read(byteSlice); r > 0 && rerr == nil {
				if Array := strings.Split(string(byteSlice[:r]), "\r\n\r\n"); len(Array) == 2 {
					for _, element := range strings.Split(FixString(Array[1]), ",") {
						element = strings.Trim(element, "}")
						element = strings.Trim(element, "{")
						for _, element2 := range strings.Split(FixString(element), ",") {
							element3 := strings.Split(FixString(element2), ":")
							if len(element3) == 2 {
								key := strings.Trim(element3[0], "\"")
								val := strings.Trim(element3[1], "\"")
								if key == "message" {
									sp := strings.Split(FixString(val), "|")
									if len(sp) == 2 && (len(sp[0]) > 6 && len(sp[0]) < 17) && (len(sp[1]) > 1 && len(sp[1]) < 6) {
										return sp[0], sp[1], true
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return "", "", false
}

//GetCloudID Get CloudID
func GetCloudID(APIServer string, APIPort string) string {
	for {
		if EnableMAC != nil && *EnableMAC == "1" {
			if MACString != nil && *MACString != "" {
				return *MACString
			} else if MACFile != nil && *MACFile != "" {
				if f, err := os.Open(*MACFile); err == nil {
					if data, err := ioutil.ReadAll(f); err == nil && len(data) > 5 {
						f.Close()
						return FixString(string(data))
					}
					f.Close()
				}
			} else if MacAddr := iface(); MacAddr != "" {
				return MacAddr
			}
		} else {
			if len(*ConfigDir) > 0 {
				if stat, err := os.Stat(*ConfigDir); err == nil && stat.IsDir() {
					Logging("[Cloud Client]", "Patch Config OK", "::", *ConfigDir, "::")
				} else {
					Logging("[Cloud Client]", "Patch Config Error", "::", *ConfigDir, "::", err)
					time.Sleep(5 * time.Second)
					continue
				}
			}
			if stat, err := os.Stat(*ConfigDir + "cloud.id"); err == nil && !stat.IsDir() {
				Logging("[Cloud Client]", "File cloud.id Found")
				if f, err := os.Open(*ConfigDir + "cloud.id"); err == nil {
					if data, err := ioutil.ReadAll(f); err == nil && len(data) > 5 {
						f.Close()
						return FixString(string(data))
					}
					f.Close()
				}
			} else {
				Logging("[Cloud Client]", "File cloud.id Not Found")
				if CloudID, ok := GetBallancerCloudID(APIServer, APIPort); ok && len(CloudID) > 5 {
					if f, err := os.OpenFile(*ConfigDir+"cloud.id", os.O_WRONLY|os.O_TRUNC|os.O_CREATE, 0660); err == nil {
						if _, err := f.Write([]byte(CloudID)); err == nil {
							f.Close()
							return CloudID
						}
						f.Close()
					}
				}
				Logging("[Cloud Client]", "Get CloudID Error")
			}
		}
		time.Sleep(5 * time.Second)
	}
}

//GetServerAndPort GetServerAndPort loop
func GetServerAndPort(APIServer string, APIPort string, CloudID string) (string, string) {
	for {
		if Server, Port, ok := GetBallancerServer(APIServer, APIPort, CloudID); ok && len(Server) > 5 && len(Port) > 2 {
			return Server, Port
		}
		Logging("[Cloud Client]", "Get Server And Port Error")
		time.Sleep(5 * time.Second)
	}
}

//CloudStart Start Loop
func CloudStart(conn net.Conn, buffer []byte, CameraIP string, CameraPort string, id int, audio bool) {
	CameraTimeout := 10 * time.Second
	if audio {
		CameraTimeout = 600 * time.Second
	}
	MediaTiemeout := 60 * time.Second
	if audio {
		MediaTiemeout = 5 * time.Second
	}
	Connector := &net.Dialer{Timeout: CameraTimeout}
	if conn2, err := Connector.Dial("tcp", CameraIP+":"+CameraPort); err == nil {
		if _, err = conn2.Write(buffer); err != nil {
			return
		}
		if err = conn2.SetDeadline(time.Now().Add(CameraTimeout)); err != nil {
			return
		}
		if err = conn.SetDeadline(time.Now().Add(MediaTiemeout)); err != nil {
			return
		}
		Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Start Working Chanel Online")
		Storage.SetStatus(strconv.FormatInt(int64(id), 10), 2)
		go CloudLoop(conn, conn2, MediaTiemeout, audio, 1)
		CloudLoop(conn2, conn, CameraTimeout, audio, 0)
		Storage.SetStatus(strconv.FormatInt(int64(id), 10), 0)
		Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Stop Working Chanel Offile")
	} else {
		Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Connect to Camera Error", err)
	}
}

//CloudLoop Sock to Sock
func CloudLoop(in net.Conn, out net.Conn, Timeout time.Duration, audio bool, inout int) {
	defer in.Close()
	defer out.Close()
	if audio {
		tmp := make([]byte, 5)
		payload := make([]byte, 292)
		for {
			if err := in.SetDeadline(time.Now().Add(Timeout)); err != nil {
				return
			}
			if n, err := io.ReadFull(in, tmp); err != nil || n != 5 {
				return
			}
			if inout == 1 {
				if tmp[0] != 11 {
					return
				}
				plen := Uint32(tmp[1:])
				if plen <= 292 && plen > 0 {
					if n, err := io.ReadFull(in, payload[:plen]); err != nil || n != int(plen) {
						return
					}
					if _, err1 := out.Write(payload[:plen]); err1 != nil {
						return
					}
				}
			}
		}
	} else {
		tmp := make([]byte, 8096)
		for {
			if err := in.SetDeadline(time.Now().Add(Timeout)); err != nil {
				return
			}
			if n, err2 := in.Read(tmp); err2 == nil {
				if n > 0 {
					if _, err1 := out.Write(tmp[:n]); err1 != nil {
						return
					}
				}
			} else {
				return
			}
		}
	}
}

//Uint32 convert
func Uint32(b []byte) uint32 {
	return uint32(b[0]) | uint32(b[1])<<8 | uint32(b[2])<<16 | uint32(b[3])<<24
}

//main cloud main func
func main() {
	flag.Parse()
	if *Sleep != 0 {
		Logging("[Cloud Client]", "Wait ::", *Sleep)
		time.Sleep(*Sleep)
	}
	if *EnableHTTP == "1" {
		go SocketServerStart()
	}
	CloudID := GetCloudID(*APIServer, *APIPort)
	if *EnableAPI == "1" {
		*CloudServer, *CloudPort = GetServerAndPort(*APIServer, *APIPort, CloudID)
	}
	Logging("[Cloud Client]", "Start Main CloudID ::", CloudID, ":: CloudServer ::", *CloudServer, ":: CloudServerPort ::", *CloudPort, ":: ConfigDir ::", *ConfigDir)
	SpeekStart := false
	for index, element := range strings.Split(*Streams, ",") {
		u, err := url.Parse(element)
		if err != nil {
			Logging("[Cloud Client]", "Parse Chanel Error", element, err)
			continue
		}
		CameraIP := "127.0.0.1"
		CameraPort := "554"
		CameraLogin := ""
		CameraPassword := ""
		RUri := ""
		if u.Port() != "" {
			CameraPort = u.Port()
		}
		if u.Hostname() != "" {
			CameraIP = u.Hostname()
		}
		if u.RequestURI() != "" {
			RUri = u.RequestURI()
		}
		if u.User != nil {
			CameraLogin = u.User.Username()
			CameraPassword, _ = u.User.Password()
		}
		Storage.Add(strconv.FormatInt(int64(index+1), 10), DeviceCloud{camip: CameraIP, camport: CameraPort, camuri: RUri, status: 0, url: element, login: CameraLogin, password: CameraPassword, cserver: *CloudServer, cport: *CloudPort, cid: CloudID + "/" + strconv.FormatInt(int64(index+1), 10)})
		go ChanelStart(index+1, *CloudServer, *CloudPort, CloudID+"/"+strconv.FormatInt(int64(index+1), 10), CameraIP, CameraPort, CameraLogin, CameraPassword, RUri, element)
		if *EnableSpeek == "1" && !SpeekStart {
			SpeekStart = true
			CameraPort = "90"
			RUri = ""
			Storage.Add("10000", DeviceCloud{camip: CameraIP, camport: "90", camuri: "", status: 0, url: "10000", login: CameraLogin, password: CameraPassword, cserver: *CloudServer, cport: *CloudPort, cid: CloudID + "/" + "10000"})
			go ChanelStart(10000, *CloudServer, *CloudPort, CloudID+"/10000", CameraIP, CameraPort, CameraLogin, CameraPassword, RUri, "10000")

		}
	}
	select {}
}

//HTTPRoute http client heandle
func HTTPRoute(c net.Conn) {
	defer c.Close()
	buffer := make([]byte, 4096)
	n, err := c.Read(buffer)
	if err != nil || n == 0 {
		return
	}
	head := strings.Split(string(buffer), "\r\n")
	if len(head) == 0 {
		return
	}
	uri := strings.Split(head[0], " ")
	if len(uri) < 1 {
		return
	}
	switch uri[1] {
	case "/status":
		data := Storage.GetINFOHTML()
		c.Write([]byte("HTTP/1.1 200 OK\r\nServer: Golang\r\nCache-Control: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " + strconv.FormatInt(int64(len(data)), 10) + "\r\nConnection: Closed\r\n\r\n" + data))
	case "/status/json":
		data := Storage.GetINFOJSON()
		c.Write([]byte("HTTP/1.1 200 OK\r\nServer: Golang\r\nCache-Control: no-cache\r\nContent-Type: text/json; charset=utf-8\r\nContent-Length: " + strconv.FormatInt(int64(len(data)), 10) + "\r\nConnection: Closed\r\n\r\n" + data))
	case "/status/html":
		data := Storage.GetINFOHTML()
		c.Write([]byte("HTTP/1.1 200 OK\r\nServer: Golang\r\nCache-Control: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " + strconv.FormatInt(int64(len(data)), 10) + "\r\nConnection: Closed\r\n\r\n" + data))
	default:
		if *HTTPDisableAdd == "0" {
			data := Storage.GetINFOHTMLADD()
			c.Write([]byte("HTTP/1.1 200 OK\r\nServer: Golang\r\nCache-Control: no-cache\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " + strconv.FormatInt(int64(len(data)), 10) + "\r\nConnection: Closed\r\n\r\n" + data))
		}
	}
}

//SocketServerStart Start Http Server
func SocketServerStart() {
	ln, err := net.Listen("tcp", ":"+*HTTPPort)
	if err != nil {
		Logging("[Cloud Client]", "Start HTTP Error port", err)
	}
	Logging("[Cloud Client]", "Start HTTP server port", *HTTPPort, "========================== Open Browser http://127.0.0.1:"+*HTTPPort+" =========================")
	for {
		conn, err := ln.Accept()
		if err != nil {
			continue
		}
		go HTTPRoute(conn)
	}
}

//ChanelStart Start Chanel
func ChanelStart(id int, CloudServer, CloudPort, CloudID, CameraIP, CameraPort, CameraLogin, CameraPassword, RUri, URL string) {
	Logging("[Cloud Client]", "Start Stream Chanel ::", id, ":: CloudServer ::", CloudServer, ":: CloudPort ::", CloudPort, ":: CloudID ::", CloudID, ":: CameraIP ::", CameraIP, ":: CameraPort ::", CameraPort, " :: RUri ::", RUri)
	for {
		Connector := &net.Dialer{Timeout: 2 * time.Second}
		if conn, err := Connector.Dial("tcp", CloudServer+":"+CloudPort); err == nil {
			if err = conn.SetDeadline(time.Now().Add(5 * time.Second)); err != nil {
				Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Set SetDeadline Error", err)
				continue
			}
			if _, err = conn.Write([]byte("REGISTER={\"cloudid\":\"" + CloudID + "\",\"login\":\"" + CameraLogin + "\",\"password\":\"" + CameraPassword + "\",\"uri\":\"" + RUri + "\",\"model\":\"" + *Model + "\",\"vendor\":\"" + *Vendor + "\"}")); err != nil {
				Logging("[Cloud Client]", "Stream Chanel ::", id, ":: REGISTER Error", err)
				continue
			}
			//if _, err = conn.Write([]byte("{\"cloudid\":\"" + CloudID + "\",\"login\":\"" + CameraLogin + "\",\"password\":\"" + CameraPassword + "\",\"uri\":\"" + RUri + "\",\"vendor\":\"" + *Vendor + "\"}")); err != nil {
			//	Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Meta Write Error", err)
			//	continue
			//}
			if err = conn.SetDeadline(time.Now().Add(120 * time.Second)); err != nil {
				Logging("[Cloud Client]", "Stream Chanel ::", id, ":: REGISTER Error", err)
				continue
			}
			lp := ""
			if CameraLogin != "" {
				lp = CameraLogin + ":" + CameraPassword + "@"
			}
			Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Registred ON Cloud Server Stream Avalible :: rtsp://"+lp+CloudServer+":"+CloudPort+"/"+CloudID, " :: Maybe Need Auth Add to Loging Password to Url")
			Storage.SetINFO(strconv.FormatInt(int64(id), 10), 1, CameraLogin, CameraPassword, URL, CloudServer, CloudPort, CloudID)
			tmp := make([]byte, 256)
			for {
				if n, err7 := conn.Read(tmp); err7 == nil && n > 0 {
					if strings.Contains(string(tmp[:n]), "RTSP") {
						CloudStart(conn, tmp[:n], CameraIP, CameraPort, id, false)
						break
					} else if len(tmp) > 3 && tmp[0] == 73 && tmp[1] == 78 && tmp[2] == 70 {
						CloudStart(conn, tmp[:n], CameraIP, CameraPort, id, true)
						break
					} else if strings.Contains(string(tmp[:n]), "\r\n\r\n") {
						Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Recive ACK Packet")
						if _, err1 := conn.Write([]byte("ok")); err1 != nil {
							Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Send OK Error", err1)
							break
						}
						if err2 := conn.SetDeadline(time.Now().Add(120 * time.Second)); err2 != nil {
							Logging("[Cloud Client]", "Stream Chanel ::", id, ":: SetDeadline Error", err2)
							break
						}
					} else {
						Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Read Bad Hello", string(tmp[:n]), tmp[:n])
						break
					}
				} else {
					Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Read Task Error", err7, n)
					break
				}
				time.Sleep(1 * time.Second)
			}
			conn.Close()
			Storage.SetStatus(strconv.FormatInt(int64(id), 10), 0)
		} else {
			Logging("[Cloud Client]", "Stream Chanel ::", id, ":: Connection Cloud Server Error Offline", err)
		}
		time.Sleep(1 * time.Second)
	}
}

//FixString clean string special char
func FixString(str string) string {
	return strings.Trim(strings.Trim(str, "\n"), "\r")
}

//Logging Message
func Logging(message ...interface{}) {
	if *EnableDubug == "1" {
		//	log.Println(message...)
	}
}
func iface() string {
	str := ""
	ifaces, err := net.Interfaces()
	if err != nil {
		return str
	}
	for _, i := range ifaces {
		addrs, err := i.Addrs()
		if err != nil {
			return str
		}
		for _, addr := range addrs {
			if ipnet, ok := addr.(*net.IPNet); ok && !ipnet.IP.IsLoopback() {
				if ipnet.IP.To4() != nil {
					str = i.HardwareAddr.String()
					return str
				}
			}
		}
	}
	return str
}
