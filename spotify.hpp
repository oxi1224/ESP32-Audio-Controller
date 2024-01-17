#ifndef SPOTIFY_HPP
#define SPOTIFY_HPP

#include <iostream>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <ctime>
#include "./util.hpp"

using namespace Util;

size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
	size_t totalSize = size * nmemb;
	output->append(static_cast<char*>(contents), totalSize);
	return totalSize;
}

namespace Spotify {

	class Client {
	public:
		std::string s_clientID;
		std::string s_clientSecret;
		std::string s_redirectURI;
		long int expireTime;
		long int startTime;

		Client(std::string clientID, std::string clientSecret, std::string redirectURI) {
			s_clientID = clientID;
			s_clientSecret = clientSecret;
			s_redirectURI = redirectURI;
		}

		std::string setToken(const std::string &authToken) {
			if (!authToken.empty()) {
				m_accessToken = getAccessToken(authToken);
				if (!m_accessToken.empty()) {
					return "Authorization successful!";
				} else {
					return "Failed to obtain access token.";
				}
			} else {
				return "Authorization code not found.";
			}
		}

		nlohmann::json getTrackInfo() {
			nlohmann::json data;
			data["trackName"] = "ERROR";
			data["artistName"] = "ERROR";
			data["trackProgress"] = 0;
			data["trackDuration"] = 600000;

			if (m_accessToken.empty()) {
				log("Access token is empty. Please authorize the application first.", true);
				data["trackName"] = "TOKEN EMPTY";
				return data;
			}
			std::string url = "https://api.spotify.com/v1/me/player/currently-playing";
			std::vector<std::string> headers{};
			headers.push_back("Authorization: Bearer " + m_accessToken);

			std::string response = performRequest(url, headers, "GET");

			if (response.empty()) {
				data["trackName"] = "RES EMPTY";
				return data;
			}

			size_t bodyPos = response.find('{');
			std::string responseBody = response.substr(bodyPos);
			nlohmann::json jsonResponse = nlohmann::json::parse(responseBody);
	
			std::string trackName = jsonResponse["item"]["name"];
			std::string artistName = jsonResponse["item"]["artists"][0]["name"];
			int progressMs = jsonResponse["progress_ms"];
			int durationMs = jsonResponse["item"]["duration_ms"];

			data["trackName"] = plToEn(trackName);
			data["artistName"] = plToEn(artistName);
			data["trackProgress"] = progressMs;
			data["trackDuration"] = durationMs;

			return data;
		}

		void refreshToken() {
			std::string url = "https://accounts.spotify.com/api/token";
			std::string postData = "grant_type=refresh_token&refresh_token=" + m_refreshToken;
			std::vector<std::string> headers{};
			std::string auth = s_clientID + ":" + s_clientSecret;
			std::string fullAuth = "Authorization: Basic " + base64encode(auth);
			headers.push_back(fullAuth);
			headers.push_back("Content-Type: application/x-www-form-urlencoded");

			std::string response = performRequest(url, headers, "POST", postData);
			if (response.empty()) {
				log("Failed to refresh token", true);
				return;
			}

			size_t bodyPos = response.find('{');
			std::string responseBody = response.substr(bodyPos);

			nlohmann::json jsonResponse = nlohmann::json::parse(responseBody);
			m_accessToken = jsonResponse["access_token"];
			expireTime = jsonResponse["expires_in"];
			startTime = static_cast<long int>(std::time(0));

			log("Token Refreshed");
			log("New start time: " + std::to_string(startTime));
		}

	private:
		std::string performRequest(
			std::string url,
			std::vector<std::string> stringHeaders,
			std::string method,
			std::string data = ""
		) {
			CURL* curl;
			CURLcode res;
			curl_global_init(CURL_GLOBAL_DEFAULT);
			curl = curl_easy_init();
			if (!curl) {
				log("Failed to initialize curl", true);
				return "";
			}
			struct curl_slist* headers = nullptr;
			for (std::string headerData : stringHeaders) {
				headers = curl_slist_append(headers, headerData.c_str());
			}
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			if (method == "POST") {
				curl_easy_setopt(curl, CURLOPT_POST, 1L);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
			}

			std::string response;
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

			res = curl_easy_perform(curl);
			if (res != CURLE_OK) {
				log("curl_easy_perform() failed (GetAccessToken):", true);
				log(curl_easy_strerror(res), true);
				curl_easy_cleanup(curl);
				curl_global_cleanup();
				return "";
			}

			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return response;
		}

		std::string getAccessToken(const std::string& authCode) {
			std::string url = "https://accounts.spotify.com/api/token";
			std::string postData = "grant_type=authorization_code&code=" + authCode + "&redirect_uri=" + s_redirectURI;

			std::string auth = s_clientID + ":" + s_clientSecret;
			std::string fullAuth = "Authorization: Basic " + base64encode(auth);
			std::vector<std::string> headers{};

			headers.push_back(fullAuth);
			headers.push_back("Content-Type: application/x-www-form-urlencoded");
			
			std::string response = performRequest(url, headers, "POST", postData);

			if (response.empty()) {
				return "";
			}

			size_t bodyPos = response.find('{');
			std::string responseBody = response.substr(bodyPos);

			nlohmann::json jsonResponse = nlohmann::json::parse(responseBody);
			std::string accessToken = jsonResponse["access_token"];

			m_refreshToken = jsonResponse["refresh_token"];;
			expireTime = static_cast<long int>(jsonResponse["expires_in"]);
			startTime = static_cast<long int>(std::time(0));

			return accessToken;
		}

		std::string m_accessToken;
		std::string m_refreshToken;
	};
}

#endif