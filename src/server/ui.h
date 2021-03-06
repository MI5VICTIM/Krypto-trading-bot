#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  string A();
  uWS::Hub hub(0, true);
  uWS::Group<uWS::SERVER> *uiGroup = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
  bool uiVisibleOpt = true;
  unsigned int uiOSR_1m = 0;
  double delayUI = 0;
  string uiNOTE = "";
  string uiNK64 = "";
  struct uiSess {
    map<char, function<json()>*> cbSnap;
    map<char, function<void(json)>*> cbMsg;
    map<uiTXT, vector<json>> D;
    int u = 0;
  };
  class UI: public Klass {
    protected:
      void load() {
        if (argHeadless) return;
        uiGroup->setUserData(new uiSess);
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (argUser != "NULL" && argPass != "NULL" && argUser.length() > 0 && argPass.length() > 0) {
          B64::Encode(argUser + ':' + argPass, &uiNK64);
          uiNK64 = string("Basic ") + uiNK64;
        }
        uiGroup->onConnection([sess](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          sess->u++;
          string addr = webSocket->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          FN::logUIsess(sess->u, addr);
        });
        uiGroup->onDisconnection([sess](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          sess->u--;
          string addr = webSocket->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          FN::logUIsess(sess->u, addr);
        });
        uiGroup->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          string document;
          stringstream content;
          string auth = req.getHeader("authorization").toString();
          string addr = res->getHttpSocket()->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          lock_guard<mutex> lock(wsMutex);
          if (argWhitelist != "" and argWhitelist.find(addr) == string::npos) {
            FN::log("UI", "dropping gzip bomb on", addr);
            content << ifstream("etc/K-bomb.gzip").rdbuf();
            document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
            document += "Content-Encoding: gzip\r\nContent-Length: " + to_string(content.str().length()) + "\r\n\r\n" + content.str();
            res->write(document.data(), document.length());
          } else if (uiNK64 != "" && auth == "") {
            FN::log("UI", "authorization attempt from", addr);
            document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (uiNK64 != "" && auth != uiNK64) {
            FN::log("UI", "authorization failed from", addr);
            document = "HTTP/1.1 403 Forbidden\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (req.getMethod() == uWS::HttpMethod::METHOD_GET) {
            string url = "";
            document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
            string path = req.getUrl().toString();
            string::size_type n = 0;
            while ((n = path.find("..", n)) != string::npos) path.replace(n, 2, "");
            const string leaf = path.substr(path.find_last_of('.')+1);
            if (leaf == "/") {
              FN::log("UI", "authorization success from", addr);
              document += "Content-Type: text/html; charset=UTF-8\r\n";
              url = "/index.html";
            } else if (leaf == "js") {
              document += "Content-Type: application/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n";
              url = path;
            } else if (leaf == "css") {
              document += "Content-Type: text/css; charset=UTF-8\r\n";
              url = path;
            } else if (leaf == "png") {
              document += "Content-Type: image/png\r\n";
              url = path;
            } else if (leaf == "mp3") {
              document += "Content-Type: audio/mpeg\r\n";
              url = path;
            }
            if (url.length() > 0) content << ifstream(FN::readlink("app/client").substr(3) + url).rdbuf();
            else {
              struct timespec txxs;
              clock_gettime(CLOCK_MONOTONIC, &txxs);
              srand((time_t)txxs.tv_nsec);
              if (rand() % 21) {
                document = "HTTP/1.1 404 Not Found\r\n";
                content << "Today, is a beautiful day.";
              } else { // Humans! go to any random url to check your luck
                document = "HTTP/1.1 418 I'm a teapot\r\n";
                content << "Today, is your lucky day!";
              }
            }
            document += "Content-Length: " + to_string(content.str().length()) + "\r\n\r\n" + content.str();
            res->write(document.data(), document.length());
          }
        });
        uiGroup->onMessage([sess](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
          string addr = webSocket->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          if ((argWhitelist != "" and argWhitelist.find(addr) == string::npos) or length < 2)
            return;
          if (uiBIT::SNAP == (uiBIT)message[0] and sess->cbSnap.find(message[1]) != sess->cbSnap.end()) {
            json reply = (*sess->cbSnap[message[1]])();
            lock_guard<mutex> lock(wsMutex);
            if (!reply.is_null()) webSocket->send(string(message, 2).append(reply.dump()).data(), uWS::OpCode::TEXT);
          } else if (uiBIT::MSG == (uiBIT)message[0] and sess->cbMsg.find(message[1]) != sess->cbMsg.end())
            (*sess->cbMsg[message[1]])(json::parse((length > 2 and (message[2] == '[' or message[2] == '{'))
              ? string(message, length).substr(2, length-2) : "{}"
            ));
        });
      };
      void waitTime() {
        if (argHeadless) return;
        uv_timer_init(hub.getLoop(), &tDelay);
        uv_timer_start(&tDelay, [](uv_timer_t *handle) {
          if (argDebugEvents) FN::log("DEBUG", "EV GW tDelay timer");
          uiSend(delayUI > 0);
        }, 0, 0);
      };
      void waitUser() {
        UI::uiSnap(uiTXT::ApplicationState, &onSnapApp);
        UI::uiSnap(uiTXT::Notepad, &onSnapNote);
        UI::uiHand(uiTXT::Notepad, &onHandNote);
        UI::uiSnap(uiTXT::ToggleConfigs, &onSnapOpt);
        UI::uiHand(uiTXT::ToggleConfigs, &onHandOpt);
      };
      void run() {
        if ((access("etc/sslcert/server.crt", F_OK) != -1)
          and (access("etc/sslcert/server.key", F_OK) != -1)
          and hub.listen(argPort, uS::TLS::createContext("etc/sslcert/server.crt", "etc/sslcert/server.key", ""), 0, uiGroup)
        ) uiPrtcl = "HTTPS";
        else if (hub.listen(argPort, nullptr, 0, uiGroup))
          uiPrtcl = "HTTP";
        else FN::logExit("IU", string("Use another UI port number, ")
          + to_string(argPort) + " seems already in use by:\n"
          + FN::output(string("netstat -anp 2>/dev/null | grep ") + to_string(argPort)),
          EXIT_SUCCESS);
        FN::logUI(uiPrtcl, argPort);
      };
    public:
      static void uiSnap(uiTXT k, function<json()> *cb) {
        if (argHeadless) return;
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->cbSnap.find((char)k) == sess->cbSnap.end())
          sess->cbSnap[(char)k] = cb;
        else FN::logExit("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" event", EXIT_SUCCESS);
      };
      static void uiHand(uiTXT k, function<void(json)> *cb) {
        if (argHeadless) return;
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->cbMsg.find((char)k) == sess->cbMsg.end())
          sess->cbMsg[(char)k] = cb;
        else FN::logExit("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" event", EXIT_SUCCESS);
      };
      static void uiSend(uiTXT k, json o, bool hold = false) {
        if (argHeadless) return;
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->u == 0) return;
        if (delayUI and hold) uiHold(k, o);
        else uiUp(k, o);
      };
      static void delay(double delayUI_) {
        if (argHeadless) return;
        delayUI = delayUI_;
        wsMutex.lock();
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        sess->D.clear();
        wsMutex.unlock();
        uv_timer_set_repeat(&tDelay, delayUI ? (int)(delayUI*1e+3) : 6e+4);
      };
    private:
      function<json()> onSnapApp = []() {
        return (json){ serverState() };
      };
      function<json()> onSnapNote = []() {
        return (json){ uiNOTE };
      };
      function<void(json)> onHandNote = [](json k) {
        if (!k.is_null() and k.size())
          uiNOTE = k.at(0);
      };
      function<json()> onSnapOpt = []() {
        return (json){ uiVisibleOpt };
      };
      function<void(json)> onHandOpt = [](json k) {
        if (!k.is_null() and k.size())
          uiVisibleOpt = k.at(0);
      };
      static void uiUp(uiTXT k, json o) {
        string m(1, (char)uiBIT::MSG);
        m += string(1, (char)k);
        m += o.is_null() ? "" : o.dump();
        lock_guard<mutex> lock(wsMutex);
        uiGroup->broadcast(m.data(), m.length(), uWS::OpCode::TEXT);
      };
      static void uiHold(uiTXT k, json o) {
        lock_guard<mutex> lock(wsMutex);
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->D.find(k) != sess->D.end() and sess->D[k].size() > 0) {
          if (k != uiTXT::OrderStatusReports) sess->D[k].clear();
          else for (vector<json>::iterator it = sess->D[k].begin(); it != sess->D[k].end();)
            if (it->value("orderId", "") == o.value("orderId", ""))
              it = sess->D[k].erase(it);
            else ++it;
        }
        sess->D[k].push_back(o);
      };
      static bool uiSend() {
        static unsigned long uiT_1m = 0;
        wsMutex.lock();
        map<uiTXT, vector<json>> msgs;
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        for (map<uiTXT, vector<json>>::iterator it_ = sess->D.begin(); it_ != sess->D.end();) {
          if (it_->first != uiTXT::OrderStatusReports) {
            msgs[it_->first] = it_->second;
            it_ = sess->D.erase(it_);
          } else ++it_;
        }
        for (vector<json>::iterator it = sess->D[uiTXT::OrderStatusReports].begin(); it != sess->D[uiTXT::OrderStatusReports].end();) {
          msgs[uiTXT::OrderStatusReports].push_back(*it);
          if (mORS::Working != (mORS)it->value("orderStatus", 0))
            it = sess->D[uiTXT::OrderStatusReports].erase(it);
          else ++it;
        }
        sess->D.erase(uiTXT::OrderStatusReports);
        wsMutex.unlock();
        for (map<uiTXT, vector<json>>::iterator it_ = msgs.begin(); it_ != msgs.end(); ++it_)
          for (vector<json>::iterator it = it_->second.begin(); it != it_->second.end(); ++it)
            uiUp(it_->first, *it);
        if (uiT_1m+6e+4 > FN::T()) return false;
        uiT_1m = FN::T();
        return true;
      };
      static void uiSend(bool delayed) {
        bool sec60 = true;
        if (delayed) sec60 = uiSend();
        if (!sec60) return;
        uiSend(uiTXT::ApplicationState, serverState());
        uiOSR_1m = 0;
      };
      static json serverState() {
        time_t rawtime;
        time(&rawtime);
        return {
          {"memory", FN::memory()},
          {"hour", localtime(&rawtime)->tm_hour},
          {"freq", uiOSR_1m},
          {"dbsize", DB::size()},
          {"a", A()}
        };
      };
  };
}

#endif
