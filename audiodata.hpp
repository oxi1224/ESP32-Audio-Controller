#ifndef AUDIODATA_HPP
#define AUDIODATA_HPP

#include <Audiopolicy.h>
#include <Mmdeviceapi.h>
#include <Psapi.h>
#include <Shlwapi.h>
#include <Windows.h>
#include <endpointvolume.h>
#include <tlhelp32.h>
#include <iostream>
#include <map>
#include "./util.hpp";

using namespace Util;

namespace WindowsAudio {

	struct AudioSessionData {
		DWORD pID{ 0 };
		int vol{ 0 };
		std::string name;
		bool paused{ false };
	};

	class AudioClient {
	public:
		std::map<DWORD, AudioSessionData> AudioDataMap{};

		void getActiveSessions() {
			initInterfaces();

			std::map<DWORD, AudioSessionData> newMap{};
			float systemVol;
			m_endpointVolume->GetMasterVolumeLevelScalar(&systemVol);

			AudioSessionData systemData;
			systemData.name = "System";
			systemData.vol = static_cast<int>(systemVol * 100);
			systemData.paused = false;
			systemData.pID = 0;
			AudioDataMap[0] = systemData;

			int sessionCount;
			m_sessionEnumerator->GetCount(&sessionCount);

			for (int i = 0; i < sessionCount; ++i) {
				IAudioSessionControl* sessionControl;
				m_sessionEnumerator->GetSession(i, &sessionControl);
				IAudioSessionControl2* sessionControl2 = nullptr;
				sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);

				AudioSessionState state;
				sessionControl->GetState(&state);

				if (state != AudioSessionStateActive) {
					continue;
				}
				AudioSessionData data;
				DWORD pID;
				sessionControl2->GetProcessId(&pID);
				data.pID = pID;
				data.name = "Unknown";
				data.paused = false;

				HANDLE handle = nullptr;
				handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, pID);
				if (handle == INVALID_HANDLE_VALUE) {
					continue;
				}
				DWORD exeNameSize = MAX_PATH;

				PROCESSENTRY32W processEntry{};
				processEntry.dwSize = sizeof(PROCESSENTRY32W);
				// Get process name
				if (Process32FirstW(handle, &processEntry)) {
					do {
						if (processEntry.th32ProcessID == pID) {
							CloseHandle(handle);
							char ch[260];
							char DefChar = ' ';
							WideCharToMultiByte(CP_ACP, 0, processEntry.szExeFile, -1, ch, 260, &DefChar, NULL);
							data.name = std::string(ch);
							break;
						}
					} while (Process32NextW(handle, &processEntry));
				}

				if (handle != INVALID_HANDLE_VALUE) {
					CloseHandle(handle);
				}

				ISimpleAudioVolume* simpleAudioVolume = nullptr;
				sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleAudioVolume);
				float volume;
				simpleAudioVolume->GetMasterVolume(&volume);
				data.vol = static_cast<int>(volume * 100);
				simpleAudioVolume->Release();

				newMap[data.pID] = data;

				sessionControl->Release();
				sessionControl2->Release();
			}
			freeInterfaces();

			for (const auto& pair : AudioDataMap) {
				auto newMapData = newMap.find(pair.first);
				if (pair.second.paused) {
					if (newMapData != newMap.end()) {
						newMapData->second = pair.second;
					} else {
						newMap[pair.first] = pair.second;
					}
				}
			}

			newMap[0] = AudioDataMap[0];
			AudioDataMap = newMap;
		}

		void setVolume(std::string pID, float vol) {
			initInterfaces();
			if (pID == "0") {
				m_endpointVolume->SetMasterVolumeLevelScalar(vol, nullptr);
				freeInterfaces();
				return;
			}

			int sessionCount;
			m_sessionEnumerator->GetCount(&sessionCount);

			for (int i = 0; i < sessionCount; ++i) {
				IAudioSessionControl* sessionControl;
				m_sessionEnumerator->GetSession(i, &sessionControl);

				IAudioSessionControl2* sessionControl2 = nullptr;
				sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);

				DWORD curProcessID;
				sessionControl2->GetProcessId(&curProcessID);
				if (pID == std::to_string(curProcessID)) {
					ISimpleAudioVolume* simpleAudioVolume = nullptr;
					sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleAudioVolume);
					simpleAudioVolume->SetMasterVolume(vol, nullptr);
					simpleAudioVolume->Release();
				}

				sessionControl->Release();
				sessionControl2->Release();
			}
			freeInterfaces();
		}

		void sendMediaKey(UINT keyCode, HWND hwnd) {
			INPUT input = {};
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = keyCode;
			input.ki.dwFlags = 0;

			SendMessage(hwnd, WM_KEYDOWN, keyCode, 0);
			SendInput(1, &input, sizeof(INPUT));

			Sleep(200);

			input.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &input, sizeof(INPUT));

			SendMessage(hwnd, WM_KEYUP, keyCode, 0);
		}

		HWND findWindowByProcessId(DWORD processId) {
			HWND hwnd = nullptr;
			do {
				hwnd = FindWindowEx(nullptr, hwnd, nullptr, nullptr);
				DWORD windowProcessId;
				GetWindowThreadProcessId(hwnd, &windowProcessId);
				if (windowProcessId == processId) {
					return hwnd;
				}
			} while (hwnd != nullptr);
			return nullptr;
		}

	private:
		void initInterfaces() {
			HRESULT hr = CoInitialize(nullptr);
			if (FAILED(hr)) {
				DWORD code = GetLastError();
				log("CoInitialize Failed: " + code, true);
				freeInterfaces();
				return;
			}

			hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&m_pEnumerator);
			if (FAILED(hr)) {
				DWORD code = GetLastError();
				log("CoCreateInstance Failed: " + code, true);
				freeInterfaces();
				return;
			}

			m_pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_pDevice);
			m_pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&m_endpointVolume);
			m_pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&m_audioSessionManager);
			m_audioSessionManager->GetSessionEnumerator(&m_sessionEnumerator);
		}

		void freeInterfaces() {
			m_sessionEnumerator->Release();
			m_audioSessionManager->Release();
			m_endpointVolume->Release();
			m_pDevice->Release();
			m_pEnumerator->Release();
			CoUninitialize();
		}

		IMMDeviceEnumerator* m_pEnumerator = nullptr;
		IMMDevice* m_pDevice;
		IAudioEndpointVolume* m_endpointVolume = NULL;
		IAudioSessionManager2* m_audioSessionManager = nullptr;
		IAudioSessionEnumerator* m_sessionEnumerator;
	};
}

#endif