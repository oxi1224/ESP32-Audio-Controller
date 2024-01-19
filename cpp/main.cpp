#define NOMINMAX
#include <Winsock2.h>

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "./util.hpp"
#include "./spotify.hpp"
#include "./audiodata.hpp"

using namespace Util;

std::map<std::string, std::string> ENV{};

static void LoadEnv() {
	std::ifstream env(".env");
	if (!env.is_open()) {
		log("Failed to open .env file", true);
		return;
	}
	std::string line;
	while (std::getline(env, line)) {
		std::istringstream lineStream(line);
		std::string key, value;
		if (std::getline(lineStream, key, '=') && std::getline(lineStream, value)) {
			key.erase(0, key.find_first_not_of(" \t\r\n"));
			key.erase(key.find_last_not_of(" \t\r\n") + 1);
			value.erase(0, value.find_first_not_of(" \t\r\n"));
			value.erase(value.find_last_not_of(" \t\r\n") + 1);
			value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
			ENV[key] = value;
		}
	}
	env.close();
}

int main() {
	LoadEnv();
	httplib::Server srv;
	Spotify::Client spot = Spotify::Client(ENV["CLIENT_ID"], ENV["CLIENT_SECRET"], ENV["REDIRECT_URI"]);
	WindowsAudio::AudioClient audioClient;

	srv.Get("/get-processes",
		[&spot, &audioClient](const httplib::Request& req, httplib::Response& res) {
			audioClient.getActiveSessions();
			nlohmann::json jsonData;
			for (const auto& entry : audioClient.AudioDataMap) {
				nlohmann::json sessionData;
				sessionData["name"] = entry.second.name;
				sessionData["vol"] = entry.second.vol;
				sessionData["paused"] = entry.second.paused;

				jsonData[std::to_string(entry.first)] = sessionData;
			}
			res.set_content(jsonData.dump(), "text/plain");

			long int now = static_cast<long int>(std::time(0));
			if ((spot.startTime + spot.expireTime - 300) <= now) { // if expires in the next 5 minutes, refresh
				log("Refreshing Spotify Token");
				spot.refreshToken();
			}
		}
	);

	srv.Get("/set-vol",
		[&audioClient](const httplib::Request& req, httplib::Response& res) {
			auto pID = req.get_param_value("pID");
			float vol = std::stof(req.get_param_value("vol"));
			audioClient.setVolume(pID, vol);
		}
	);

	srv.Get("/exec-action",
		[&audioClient](const httplib::Request& req, httplib::Response& res) {
			auto action = req.get_param_value("action");
			int pID = std::stoi(req.get_param_value("pID"));
			HWND window = (pID == 0) ? HWND_DESKTOP : audioClient.findWindowByProcessId(pID);

			if (action == "pause") {
				auto processData = audioClient.AudioDataMap.find(pID);
				if (processData->second.name == "Spotify.exe") {
					audioClient.sendMediaKey(VK_MEDIA_PLAY_PAUSE, window);
				} else {
					audioClient.sendMediaKey(VK_SPACE, window);
				}
				audioClient.AudioDataMap[pID].paused = !audioClient.AudioDataMap[pID].paused;
			} else if (action == "prev") {
				audioClient.sendMediaKey(VK_MEDIA_PREV_TRACK, window);
			} else if (action == "next") {
				audioClient.sendMediaKey(VK_MEDIA_NEXT_TRACK, window);
			}
		}
	);

	srv.Get("/spotify-status",
		[&spot](const httplib::Request& req, httplib::Response& res) {
			nlohmann::json jsonData = spot.getTrackInfo();
			res.set_content(jsonData.dump(), "text/plain");
		}
	);

	srv.Get("/callback",
		[&spot](const httplib::Request& req, httplib::Response& res) {
			std::string authorizationCode = req.get_param_value("code");
			std::string msg = spot.setToken(authorizationCode);
			res.set_content(msg, "text/plain");
		}
	);
	
	srv.listen(ENV["ADDRESS"], std::stoi(ENV["PORT"]));
}